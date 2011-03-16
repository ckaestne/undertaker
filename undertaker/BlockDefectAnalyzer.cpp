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


#include "StringJoiner.h"
#include "BlockDefectAnalyzer.h"
#include "ModelContainer.h"


void BlockDefectAnalyzer::markOk(const std::string &arch) {
    _OKList.push_back(arch);
}

std::string BlockDefectAnalyzer::getBlockPrecondition(const ConfigurationModel *model) const {
    StringJoiner formula;

    /* Adding block and code constraints extraced from code sat stream */
    formula.push_back(_block);
    formula.push_back(_cs->getCodeConstraints());


    if (model) {
        /* Adding kconfig constraints and kconfig missing */
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));
        formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
    }

    return formula.join("\n&&\n");
}

DeadBlockDefect::DeadBlockDefect(CodeSatStream *cs, const char *block)
    :  BlockDefectAnalyzer(None), _needsCrosscheck(false),
       _arch(NULL) {
    this->_suffix = "dead";
    this->_block = block;
    this->_cs = cs;
}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;

    if(!_arch)
        _arch = ModelContainer::lookupArch(model);

    formula.push_back(_block);
    formula.push_back(_cs->getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));
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

    fname_joiner.push_back(_cs->getFilename());
    fname_joiner.push_back(_block);
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
        out << "#" << _block << ":" << _cs->getLine(_block) << std::endl;
        out << SatChecker::pprinter(contents.c_str());
        out.close();
    }

    return true;
}

UndeadBlockDefect::UndeadBlockDefect(CodeSatStream *cs, const char *block)
    : DeadBlockDefect(cs, block) { this->_suffix = "undead"; }

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;
    const char *parent = _cs->getParent(_block);

    // no parent -> impossible to be undead
    if (!parent)
        return false;

    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

    formula.push_back("( " + std::string(parent) + " && ! " + std::string(_block) + " )");
    formula.push_back(_cs->getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));
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
