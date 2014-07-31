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

#include "BlockDefectAnalyzer.h"
#include "ConditionalBlock.h"
#include "StringJoiner.h"
#include "SatChecker.h"
#include "PicosatCNF.h"
#include "ModelContainer.h"
#include "Logging.h"
#include "Tools.h"
#include "exceptions/CNFBuilderError.h"

#include <pstreams/pstream.h>
#include <iostream>
#include <fstream>
#include <string>


/************************************************************************/
/* BlockDefectAnalyzer                                                  */
/************************************************************************/

std::string BlockDefectAnalyzer::getBlockPrecondition(ConditionalBlock *cb,
                                                      const ConfigurationModel *model) {
    StringJoiner formula;

    /* Adding block and code constraints extraced from code sat stream */
    const std::string code_formula = cb->getCodeConstraints();
    formula.push_back(cb->getName());
    formula.push_back(code_formula);

    if (model) {
        /* Adding kconfig constraints and kconfig missing */
        std::set<std::string> missingSet;
        std::string kconfig_formula;
        model->doIntersect(code_formula, cb->getFile()->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        if (model->isComplete())
            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
    }
    return formula.join("\n&& ");
}

static const BlockDefect *analyzeBlock_helper(ConditionalBlock *block,
                                              ConfigurationModel *main_model) {
    BlockDefect *defect = new DeadBlockDefect(block);

    // If this is neither an Implementation, Configuration nor Referential *dead*,
    // then destroy the analysis and retry with an Undead Analysis
    if (!defect->isDefect(main_model, true)) {
        delete defect;
        defect = new UndeadBlockDefect(block);

        // No defect found, block seems OK
        if (!defect->isDefect(main_model, true)) {
            delete defect;
            return nullptr;
        }
    }
    assert(defect->defectType() != BlockDefect::DEFECTTYPE::None);

    // Check NoKconfig defect after (un)dead analysis
    if (defect->isNoKconfigDefect(main_model))
        defect->setDefectType(BlockDefect::DEFECTTYPE::NoKconfig);

    // Save defectType in block
    block->defectType = defect->defectType();

    // defects in arch specific files do not require a crosscheck and are global defects, since
    // they are not compileable for other architectures
    if (block->getFile()->getSpecificArch() != "") {
        defect->markAsGlobal();
        return defect;
    }

    // Implementation (i.e., Code) or NoKconfig defects do not require a crosscheck
    if (!main_model || !defect->needsCrosscheck())
        return defect;

    const std::string &oldarch = defect->getArch();
    BlockDefect::DEFECTTYPE original_classification = defect->defectType();
    for (const auto &entry : ModelContainer::getInstance()) { // pair<string, ConfigurationModel *>
        const ConfigurationModel *model = entry.second;
        // don't check the main model twice
        if (model == main_model)
            continue;

        if (!defect->isDefect(model)) {
            if (original_classification == BlockDefect::DEFECTTYPE::Configuration)
                defect->setArch(oldarch);
            return defect;
        }
    }
    defect->markAsGlobal();
    return defect;
}

const BlockDefect *BlockDefectAnalyzer::analyzeBlock(ConditionalBlock *block,
                                                     ConfigurationModel *main_model) {
    try {
        return analyzeBlock_helper(block, main_model);
    } catch (CNFBuilderError &e) {
        logger << error << "Couldn't process " << block->getFile()->getFilename() << ":"
               << block->getName() << ": " << e.what() << std::endl;
    } catch (std::bad_alloc &) {
        logger << error << "Couldn't process " << block->getFile()->getFilename() << ":"
               << block->getName() << ": Out of Memory." << std::endl;
    }
    return nullptr;
}

/************************************************************************/
/* BlockDefect                                                          */
/************************************************************************/

const std::string BlockDefect::defectTypeToString() const {
    switch (_defectType) {
    case DEFECTTYPE::None:
        return "";  // Nothing to write
    case DEFECTTYPE::Implementation:
        return "code";
    case DEFECTTYPE::Configuration:
        return "kconfig";
    case DEFECTTYPE::Referential:
        return "missing";
    case DEFECTTYPE::NoKconfig:
        return "no_kconfig";
    default:
        assert(false);
    }
    return "";
}

bool BlockDefect::needsCrosscheck() const {
    switch (_defectType) {
    case DEFECTTYPE::None:
    case DEFECTTYPE::Implementation:
    case DEFECTTYPE::NoKconfig:
        return false;
    default:
        // skip crosschecking if we already know that it is global
        return !_isGlobal;
    }
}

std::string BlockDefect::getDefectReportFilename() const {
    StringJoiner fname_joiner;
    fname_joiner.push_back(_cb->getFile()->getFilename());
    fname_joiner.push_back(_cb->getName());
    fname_joiner.push_back(defectTypeToString());

    if (_isGlobal || _defectType == DEFECTTYPE::NoKconfig)
        fname_joiner.push_back("globally");
    else
        fname_joiner.push_back(_arch);

    fname_joiner.push_back(_suffix);
    return fname_joiner.join(".");
}

bool BlockDefect::isNoKconfigDefect(const ConfigurationModel *model) const {
    if (!model)
        return true;

    std::string expr;
    if (_cb->isElseBlock()) {
        // Check prior blocks
        const ConditionalBlock *prev = _cb->getPrev();
        while (prev) {
            if (prev->defectType != DEFECTTYPE::NoKconfig)
                return false;
            prev = prev->getPrev();
        }
    } else if (_cb == _cb->getFile()->topBlock()) {
        // If current block is the file, take the entire formula.
        expr = _formula;
    } else {
        // Otherwise, take the expression.
        expr = _cb->ifdefExpression();
    }
    for (const std::string &str : undertaker::itemsOfString(expr))
        if (model->inConfigurationSpace(str))
            return false;

    return true;
}

bool BlockDefect::writeReportToFile(bool skip_no_kconfig) const {
    if ((skip_no_kconfig && _defectType == DEFECTTYPE::NoKconfig)
        || _defectType == DEFECTTYPE::None)
        return false;
    const std::string filename = getDefectReportFilename();

    std::ofstream out(filename);

    if (!out.good()) {
        logger << error << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        logger << info << "creating " << filename << std::endl;
        out << "#" << _cb->getName() << ":"
            << _cb->filename() << ":" << _cb->lineStart() << ":" << _cb->colStart() << ":"
            << _cb->filename() << ":" << _cb->lineEnd() << ":" << _cb->colEnd() << ":"
            << std::endl;
        out << SatChecker::pprinter(_formula);
        out.close();
    }
    return true;
}

/************************************************************************/
/* DeadBlockDefect                                                      */
/************************************************************************/

DeadBlockDefect::DeadBlockDefect(ConditionalBlock *cb) : BlockDefect(cb) {
    this->_suffix = "dead";
}

void DeadBlockDefect::reportMUS() const {
    // MUS only works on {code, kconfig} dead blocks
    if (_defectType == DEFECTTYPE::None)
        return;
    // call Satchecker and get the CNF-Object
    SatChecker code_constraints(_musFormula);
    code_constraints();
    kconfig::CNF *cnf = code_constraints.getCNF();
    // call picosat in quiet mode with stdin as input and stdout as output
    redi::pstream cmd_process("picomus - -");
    // write to stdin of the process
    cmd_process << "p cnf " << cnf->getVarCount() << " " << cnf->getClauseCount() << std::endl;
    for (const int &clause : cnf->getClauses()) {
        char sep = (clause == 0) ? '\n' : ' ';
        cmd_process << clause << sep;
    }
    // send eof and tell cmd_process we will start reading from stdout of cmd
    redi::peof(cmd_process);
    cmd_process.out();
    // read everything from cmd_process's stdout and close it
    std::stringstream ss;
    ss << cmd_process.rdbuf();
    cmd_process.close();
    // remove first line from ss (=UNSATISFIABLE)
    std::string garbage;
    std::getline(ss, garbage);

    // create a string from DIMACs CNF Format (=picomus result) to a more readable CNF Format
    // Note: The formula might be incomplete, since a lot operators create new CNF-IDs without
    // having a destinct Symbolname, which are ignored in this output
    int vars, lines; std::string p, cnfstr;
    ss >> p; ss >> cnfstr; ss >> vars; ss >> lines;
    if (p != "p" || cnfstr != "cnf") {
        logger << error << "Mismatched output format, skipping MUS analysis." << std::endl;
        return;
    }
    std::vector<std::string> vec;
    for (int i = 0, tmp; i < lines; i++) {
        bool valid = false;
        std::stringstream tmpstr;
        tmpstr << "(";
        ss >> tmp;
        // process a line
        while (tmp != 0) {
            std::string sym = cnf->getSymbolName(abs(tmp));
            if (sym == "") {
                ss >> tmp;
                continue;
            }
            valid = true;
            if (tmp < 0)
                tmpstr << "!";
            tmpstr << sym;
            // get next int in the current line
            ss >> tmp;
            if (tmp != 0)
                tmpstr << " v ";
        }
        tmpstr << ")";
        if (valid)  // collect valid clauses
            vec.emplace_back(tmpstr.str());
    }
    // build minimized CNF Formula
    std::stringstream formula;
    for (std::string &str : vec) {
        formula << str;
        if (str != vec.back())
            formula << " ^ ";
    }
    // create filename for mus-defect report and open the outputfilestream
    std::string filename = this->getDefectReportFilename() + ".mus";

    std::ofstream ofs(filename);
    if (!ofs.good()) {
        logger << error << "Failed to open " << filename << " for writing." << std::endl;
        return;
    }
    // prepend some statistics about the picomus performance
    logger << info << "creating " << filename << std::endl;
    ofs << "ATTENTION: This formula _might_ be incomplete or even inconclusive!" << std::endl;
    ofs << "Minimized Formula from:" << std::endl;
    ofs << "p cnf " << cnf->getVarCount() << " " << cnf->getClauseCount() << std::endl;
    ofs << "to" << std::endl;
    ofs << "p cnf " << vars               << " " << lines                 << std::endl;
    ofs << formula.str() << std::endl;
}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model, bool is_main_model) {
    StringJoiner formula;

    if (_arch == "")
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back(_cb->getName());
    formula.push_back(code_formula);
    _formula = formula.join("\n&&\n");

    SatChecker code_constraints(_formula);

    if (!code_constraints()) {
        _defectType = DEFECTTYPE::Implementation;
        _isGlobal = true;
        _musFormula = _formula;
        return true;
    }
    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula = _cb->getCodeConstraints();
        model->doIntersect(code_formula, _cb->getFile()->getChecker(), missingSet,
                           kconfig_formula);
        formula.push_back(kconfig_formula);
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        // logger << debug << "kconfig_constraints: " << formula_str << std::endl;

        if (!kconfig_constraints()) {
            if (_defectType != DEFECTTYPE::Configuration) {
                // Wasn't already identified as Configuration defect
                _arch = ModelContainer::lookupArch(model);
            }
            _formula = formula_str;
            // save formula for mus analysis when we are analysing the main_model
            if (is_main_model)
                _musFormula = formula_str;
            _defectType = DEFECTTYPE::Configuration;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't generate referential errors
            if (!model->isComplete())
                return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != DEFECTTYPE::Configuration) {
                    _defectType = DEFECTTYPE::Referential;
                }
                // save formula for mus analysis when we are analysing the main_model
                if (is_main_model)
                    _musFormula = formula_str;
                _formula = formula_str;
                return true;
            }
        }
    }
    return false;
}

/************************************************************************/
/* UndeadBlockDefect                                                    */
/************************************************************************/

UndeadBlockDefect::UndeadBlockDefect(ConditionalBlock *cb) : BlockDefect(cb) {
    this->_suffix = "undead";
}

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model, bool) {
    StringJoiner formula;
    const ConditionalBlock *parent = _cb->getParent();

    // no parent -> it's B00 -> impossible to be undead
    if (!parent)
        return false;

    if (_arch == "")
        _arch = ModelContainer::lookupArch(model);

    std::string code_formula = _cb->getCodeConstraints();
    formula.push_back("( " + parent->getName() + " && ! " + _cb->getName() + " )");
    formula.push_back(code_formula);
    _formula = formula.join("\n&&\n");

    SatChecker code_constraints(_formula);

    if (!code_constraints()) {
        _defectType = DEFECTTYPE::Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula = _cb->getCodeConstraints();
        model->doIntersect(code_formula, _cb->getFile()->getChecker(), missingSet,
                           kconfig_formula);
        formula.push_back(kconfig_formula);
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        if (!kconfig_constraints()) {
            if (_defectType != DEFECTTYPE::Configuration) {
                // Wasn't already identified as Configuration defect
                _arch = ModelContainer::lookupArch(model);
            }
            _formula = formula_str;
            _defectType = DEFECTTYPE::Configuration;
            return true;
        } else {
            // An incomplete model (not all symbols mentioned) can't
            // generate referential errors
            if (!model->isComplete())
                return false;

            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != DEFECTTYPE::Configuration) {
                    _defectType = DEFECTTYPE::Referential;
                }
                _formula = formula_str;
                return true;
            }
        }
    }
    return false;
}
