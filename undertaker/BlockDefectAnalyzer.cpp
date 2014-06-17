/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#ifdef DEBUG
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif

#include <iostream>
#include <fstream>
#include <string>

#include "StringJoiner.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "PicosatCNF.h"
#include "ModelContainer.h"
#include "Logging.h"
#include "boost/filesystem.hpp"


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
    DeadBlockDefect *defect = new DeadBlockDefect(block);

    // If this is neither an Implementation, Configuration nor Referential *dead*,
    // then destroy the analysis and retry with an Undead Analysis
    if(!defect->isDefect(p_model)) {
        delete defect;
        defect = new UndeadBlockDefect(block);

        // No defect found, block seems OK
        if(!defect->isDefect(p_model)) {
            delete defect;
            return nullptr;
        }
    }
    assert(defect->defectType() != BlockDefectAnalyzer::None);

    // Check NoKconfig defect after (un)dead analysis
    if (defect->isNoKconfigDefect(p_model))
        defect->_defectType = BlockDefectAnalyzer::NoKconfig;

    // Save defectType in block
    block->defectType = defect->defectType();

    // Implementation (i.e., Code) or NoKconfig defects do not require a crosscheck
    if (!p_model || !defect->needsCrosscheck()
                 || defect->defectType() == BlockDefectAnalyzer::NoKconfig)
        return defect;

    const char *oldarch = defect->getArch();
    int original_classification = defect->defectType();
    for (auto &entry : *ModelContainer::getInstance()) { // pair<string, ConfigurationModel *>
        const ConfigurationModel *model = entry.second;

        if (!defect->isDefect(model)) {
            if (original_classification == BlockDefectAnalyzer::Configuration)
                defect->setArch(oldarch);
            return defect;
        }
    }
    defect->defectIsGlobal();
    return defect;
}


DeadBlockDefect::DeadBlockDefect(ConditionalBlock *cb) {
    this->_suffix = "dead";
    this->_cb = cb;
}

void DeadBlockDefect::reportMUS() const {
    // MUS only works on {code, kconfig} dead blocks
    if (_suffix != "dead" || _defectType == BlockDefectAnalyzer::None)
        return;
    SatChecker code_constraints(_musFormula, true);
    code_constraints();
    kconfig::CNF *cnf = code_constraints.getCNF();

    std::string inf(cnf->getMusDirName());
    inf += "/infile";
    std::string outf(cnf->getMusDirName());
    outf += "/outfile";
    // call "picomus infile outfile"
    std::ostringstream cmd;
    cmd << "picomus " << inf << " " << outf << " > /dev/null";
    int ret = std::system(cmd.str().c_str());
    if(ret != 5120) { // i have no idea why this is 5120 and not 20
        logger << error << "PICOMUS ERROR " << ret << std::endl;
        boost::filesystem::remove_all(cnf->getMusDirName());
        exit(ret);
    }
    std::ifstream ifs(outf.c_str());
    if(!ifs.good()) {
        logger << error << "Couldn't open picomus output file " << outf << std::endl;
        boost::filesystem::remove_all(cnf->getMusDirName());
        exit(512);
    }
    // create a string from DIMACs CNF Format (=picomus result) to a more readable CNF Format
    // Note: The formula is nearly empty since a lot operators create new CNF-IDs without having a
    // destinct Symbolname
    std::ostringstream formula;
    int vars, lines; std::string p, cnfstr;
    ifs >> p; ifs >> cnfstr; ifs >> vars; ifs >> lines;

    for (int i = 0, tmp; i < lines; i++) {
        formula << "(";
        ifs >> tmp;
        while(tmp != 0) {
            if (tmp < 0)
                formula << "!";
            formula << cnf->getSymbolName(abs(tmp));
            ifs >> tmp;
            if(tmp != 0)
                formula << " v ";
        }
        formula << ")";
        if(i < lines-1)
            formula << " ^ ";
    }
    // create filename for mus-defect report and open the outputfilestream
    std::string filename = getDefectReportFilename();
    filename += ".mus";

    std::ofstream ofs(filename.c_str());
    if (!ofs.good()) {
        logger << error << "Failed to open " << filename << " for writing." << std::endl;
        return;
    }
    // some statistics about the picomus performance
    std::ifstream pic_infile(inf.c_str());
    if(!pic_infile.good()) {
        logger << error << "Couldn't open picomus input file " << inf << std::endl;
        boost::filesystem::remove_all(cnf->getMusDirName());
        exit(512);
    }
    logger << info << "creating " << filename << std::endl;
    std::string l;
    std::getline(pic_infile, l); // get first line of the picosat inputfile
    ofs << "Minimized Formula from:" << std::endl << l << std::endl << "to" << std::endl;
    ofs << p << " " << cnfstr << " " << vars << " " << lines << std::endl;
    ofs << formula.str() << std::endl;

    // remove tmp directory and the cnf object we kept
    boost::filesystem::remove_all(cnf->getMusDirName());
    delete cnf;
}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;

    if(!_arch)
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back(_cb->getName());
    formula.push_back(code_formula);
    _formula = formula.join("\n&&\n");

    SatChecker code_constraints(_formula);

    if (!code_constraints()) {
        _defectType = BlockDefectAnalyzer::Implementation;
        _isGlobal = true;
        _musFormula = _formula;
        return true;
    }
    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula = _cb->getCodeConstraints();
        model->doIntersect(code_formula,  _cb->getFile()->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        // logger << debug << "kconfig_constraints: " << formula_str << std::endl;

        if (!kconfig_constraints()) {
            if (_defectType != BlockDefectAnalyzer::Configuration) {
                // Wasn't already identified as Configuration defect
                _arch = ModelContainer::lookupArch(model);
            }
            _formula = formula_str;
            if (model == ModelContainer::lookupMainModel())
                _musFormula = formula_str;
            _defectType = BlockDefectAnalyzer::Configuration;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't
            // generate referential errors
            if (!model->isComplete()) return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != BlockDefectAnalyzer::Configuration) {
                    _defectType = BlockDefectAnalyzer::Referential;
                }
                if (model == ModelContainer::lookupMainModel())
                    _musFormula = formula_str;
                _formula = formula_str;
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
        return false;
    default:
        // skip crosschecking if we already know that it is global
        return !_isGlobal;
    }
}

bool DeadBlockDefect::isNoKconfigDefect(const ConfigurationModel *model) {
    if (model) {
        std::string expr;
        if (_cb->isElseBlock()) {
            // Check prior blocks
            ConditionalBlock *prev = (ConditionalBlock *) _cb->getPrev();
            while (prev) {
                if (prev->defectType != BlockDefectAnalyzer::NoKconfig)
                    return false;
                prev = (ConditionalBlock *) prev->getPrev();
            }
        } else if (_cb == _cb->getFile()->topBlock()) {
            // If current block is the file, take the entire formula.
            expr = _formula;
        } else {
            // Otherwise, take the expression.
            expr = _cb->ifdefExpression();
        }
        for (const std::string &str : ConditionalBlock::itemsOfString(expr))
            if (model->inConfigurationSpace(str))
                return false;
    }
    return true;
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
    case NoKconfig:
        return "no_kconfig";
    default:
        assert(false);
    }
    return "";
}

std::string DeadBlockDefect::getDefectReportFilename() const {
    StringJoiner fname_joiner;
    fname_joiner.push_back(_cb->getFile()->getFilename());
    fname_joiner.push_back(_cb->getName());
    fname_joiner.push_back(defectTypeToString());

    if (_isGlobal || _defectType == BlockDefectAnalyzer::NoKconfig)
        fname_joiner.push_back("globally");
    else
        fname_joiner.push_back(_arch);

    fname_joiner.push_back(_suffix);
    return fname_joiner.join(".");
}

bool DeadBlockDefect::writeReportToFile(bool skip_no_kconfig) const {
    if ((skip_no_kconfig && _defectType == BlockDefectAnalyzer::NoKconfig)
            || _defectType == BlockDefectAnalyzer::None)
        return false;
    const std::string filename = getDefectReportFilename();
    const std::string &contents = _formula;

    std::ofstream out(filename.c_str());

    if (!out.good()) {
        logger << error << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        logger << info << "creating " << filename << std::endl;
        out << "#" << _cb->getName() << ":"
            << _cb->filename() << ":" << _cb->lineStart() << ":" << _cb->colStart() << ":"
            << _cb->filename() << ":" << _cb->lineEnd() << ":" << _cb->colEnd() << ":"
            << std::endl;
        out << SatChecker::pprinter(contents.c_str());
        out.close();
    }
    return true;
}

UndeadBlockDefect::UndeadBlockDefect(ConditionalBlock * cb) : DeadBlockDefect(cb) {
    this->_suffix = "undead";
}

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;
    const ConditionalBlock *parent = _cb->getParent();

    // no parent -> it's B00 -> impossible to be undead
    if (!parent)
        return false;

    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back("( " + parent->getName() + " && ! " + _cb->getName() + " )");
    formula.push_back(code_formula);
    _formula = formula.join("\n&&\n");

    SatChecker code_constraints(_formula);

    if (!code_constraints()) {
        _defectType = BlockDefectAnalyzer::Implementation;
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
            if (_defectType != BlockDefectAnalyzer::Configuration) {
                // Wasn't already identified as Configuration defect
                _arch = ModelContainer::lookupArch(model);
            }
            _formula = formula_str;
            _defectType = BlockDefectAnalyzer::Configuration;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't
            // generate referential errors
            if (!model->isComplete()) return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != BlockDefectAnalyzer::Configuration) {
                    _defectType = BlockDefectAnalyzer::Referential;
                }
                _formula = formula_str;
                return true;
            }
        }
    }
    return false;
}
