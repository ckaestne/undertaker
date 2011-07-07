/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <string>

#include "StringJoiner.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "ModelContainer.h"


void BlockDefectAnalyzer::markOk(const std::string &arch) {
    _OKList.push_back(arch);
}

std::string BlockDefectAnalyzer::getBlockPrecondition(const ConfigurationModel *model) const {
    StringJoiner formula;

    /* Adding block and code constraints extraced from code sat stream */
    const std::string code_formula  = _cb->getCodeConstraints();
    formula.push_back(_cb->getName());
    formula.push_back(code_formula);


    if (model) {
        /* Adding kconfig constraints and kconfig missing */
        std::set<std::string> missingSet;
        std::string kconfig_formula;
        model->doIntersect(code_formula, _cb->getFile()->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        if (model->isComplete())
            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
    }

    return formula.join("\n&& ");
}

const BlockDefectAnalyzer *
BlockDefectAnalyzer::analyzeBlock(ConditionalBlock *block, ConfigurationModel *p_model) {
    BlockDefectAnalyzer *defect = new DeadBlockDefect(block);

    // If this is neither an Implementation, Configuration nor Referential *dead*,
    // then destroy the analysis and retry with an Undead Analysis
    if(!defect->isDefect(p_model)) {
        delete defect;
        defect = new UndeadBlockDefect(block);

        // No defect found, block seems OK
        if(!defect->isDefect(p_model)) {
            delete defect;
            return NULL;
        }
    }

    assert(defect->defectType() != BlockDefectAnalyzer::None);

    // (ATM) Implementation and Configuration defects do not require a crosscheck
    if (!p_model || !defect->needsCrosscheck())
        return defect;

    ModelContainer *f = ModelContainer::getInstance();
    for (ModelContainer::iterator i = f->begin(); i != f->end(); i++) {
        const std::string &arch = (*i).first;
        const ConfigurationModel *model = (*i).second;

        if (!defect->isDefect(model)) {
            defect->markOk(arch);
            return defect;
        }
    }
    defect->defectIsGlobal();
    return defect;
}


DeadBlockDefect::DeadBlockDefect(ConditionalBlock *cb)
    :  BlockDefectAnalyzer(None), _needsCrosscheck(false),
       _arch(NULL) {
    this->_suffix = "dead";
    this->_cb = cb;
}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;

    if(!_arch)
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back(_cb->getName());
    formula.push_back(code_formula);
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula = _cb->getCodeConstraints();
        model->doIntersect(code_formula,  _cb->getFile()->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        if (!kconfig_constraints()) {
            if (_defectType != Configuration) {
                // Wasn't already identified as Configuration defect
                // crosscheck doesn't overwrite formula
                _formula = formula_str;
                _arch = ModelContainer::lookupArch(model);
            }
            _defectType = Configuration;
            _isGlobal = true;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't
            // generate referential errors
            if (!model->isComplete()) return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != Configuration) {
                    _formula = formula_str;
                    _defectType = Referential;
                }
                return true;
            }
        }
    }
    return false;
}

bool DeadBlockDefect::isGlobal() const { return _isGlobal; }
bool DeadBlockDefect::needsCrosscheck() const {
    switch(_defectType) {
    case None:
    case Implementation:
    case Configuration:
        return false;
    default:
        // skip crosschecking if we already know that it is global
        return !_isGlobal;
    }
}

void BlockDefectAnalyzer::defectIsGlobal() { _isGlobal = true; }

const std::string BlockDefectAnalyzer::defectTypeToString() const {
    switch(_defectType) {
    case None: return "";  // Nothing to write
    case Implementation:
        return "code";
    case Configuration:
        return "kconfig";
    case Referential:
        return  "missing";
    default:
        assert(false);
    }
    return "";
}

bool DeadBlockDefect::writeReportToFile() const {
    StringJoiner fname_joiner;

    fname_joiner.push_back(_cb->getFile()->getFilename());
    fname_joiner.push_back(_cb->getName());
    if(_arch && (!_isGlobal || _defectType == Configuration))
        fname_joiner.push_back(_arch);

    switch(_defectType) {
    case None: return false;  // Nothing to write
    default:
        fname_joiner.push_back(defectTypeToString());
    }

    if (_isGlobal)
        fname_joiner.push_back("globally");

    fname_joiner.push_back(_suffix);

    const std::string filename = fname_joiner.join(".");
    const std::string &contents = _formula;

    std::ofstream out(filename.c_str());

    if (!out.good()) {
        std::cerr << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        std::cout << "I: creating " << filename << std::endl;
        out << "#" << _cb->getName() << ":" 
            << _cb->filename() << ":" << _cb->lineStart() << ":" << _cb->colStart() << ":"
            << _cb->filename() << ":" << _cb->lineEnd() << ":" << _cb->colEnd() << ":"
            << std::endl;
        out << SatChecker::pprinter(contents.c_str());
        out.close();
    }

    return true;
}

UndeadBlockDefect::UndeadBlockDefect(ConditionalBlock * cb)
    : DeadBlockDefect(cb) { this->_suffix = "undead"; }

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;
    const ConditionalBlock *parent = _cb->getParent();

    // no parent (or parent is file) -> impossible to be undead
    if (!parent || parent == _cb->getFile()->topBlock())
        return false;

    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back("( " + parent->getName() + " && ! " + _cb->getName() + " )");
    formula.push_back(code_formula);
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula = _cb->getCodeConstraints();
        model->doIntersect(code_formula,  _cb->getFile()->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        if (!kconfig_constraints()) {
            if (_defectType != Configuration) {
                // Wasn't already identified as Configuration defect
                // crosscheck doesn't overwrite formula
                _formula = formula_str;
                _arch = ModelContainer::lookupArch(model);
            }
            _defectType = Configuration;
            _isGlobal = true;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't
            // generate referential errors
            if (!model->isComplete()) return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != Configuration) {
                    _formula = formula_str;
                    _defectType = Referential;
                }
                return true;
            }
        }
    }
    return false;
}
