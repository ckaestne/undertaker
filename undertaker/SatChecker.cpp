/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Ralf Hackner <sirahack@informatik.uni-erlangen.de>
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

#include "SatChecker.h"
#include "ModelContainer.h"
#include "ConditionalBlock.h"
#include "PumaConditionalBlock.h"
#include "ConfigurationModel.h"
#include "CnfConfigurationModel.h"
#include "Logging.h"
#include "CNFBuilder.h"
#include "exceptions/CNFBuilderError.h"
#include "cpp14.h"

#include <Puma/TokenStream.h>
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <pstreams/pstream.h>

#include <iostream>
#include <map>
#include <vector>
#include <sstream>

using kconfig::PicosatCNF;
using kconfig::CNFBuilder;


/************************************************************************/
/* SatChecker                                                           */
/************************************************************************/

bool SatChecker::check(const std::string &sat) {
    SatChecker c(sat);

    try {
        return c();
    } catch (CNFBuilderError &e) {
        logger << error << "Syntax Error:" << std::endl;
        logger << error << sat << std::endl;
        logger << error << "End of Syntax Error" << std::endl;
        throw e;
    }
}

SatChecker::SatChecker(std::string sat, bool mus, int debug)
        : debug_flags(debug), _do_mus_analysis(mus), _sat(std::move(sat)) {}

static std::unique_ptr<PicosatCNF> getCnfWithModelInit(const std::string &formula,
        Picosat::SATMode mode, std::string *result) {
    static const boost::regex modelvar_regexp("\\._\\.(.+)\\._\\.");
    boost::match_results<std::string::const_iterator> what;
    if (boost::regex_search(formula, what, modelvar_regexp)) {
        // CNF model
        std::string modelname = what[1];
        CnfConfigurationModel *cm = dynamic_cast<CnfConfigurationModel *>(
            ModelContainer::getInstance()->lookupModel(modelname));
        if (!cm) {
            logger << error << "Could not add model \"" << modelname  <<"\" to cnf" << std::endl;
            return nullptr;
        }
        const PicosatCNF *modelcnf = cm->getCNF();
        std::unique_ptr<PicosatCNF> cnf{ new PicosatCNF(*modelcnf) };
        cnf->setDefaultPhase(mode);
        *result = boost::regex_replace(formula, modelvar_regexp , "1");
        return cnf;
    } else {
        // RSF model
        *result = formula;
        return make_unique<PicosatCNF>(mode);
    }
}

bool SatChecker::operator()(Picosat::SATMode mode) {
    std::string sat;
    _cnf = getCnfWithModelInit(_sat, mode, &sat);
    _cnf->setMusAnalysis(_do_mus_analysis);

    CNFBuilder builder(_cnf.get(), sat, true, CNFBuilder::ConstantPolicy::FREE);
    int res = _cnf->checkSatisfiable();
    if (res) {
        /* Let's get the assigment out of picosat, because we have to
            reset the sat solver afterwards */
        for (const auto &entry : _cnf->getSymbolMap()) {  // pair<string, int>
            bool selected = this->_cnf->deref(entry.second);
            assignmentTable.emplace(entry.first, selected);
        }
    }
    return res;
}

std::string SatChecker::pprint() {
    if (debug_parser.size() == 0) {
        int old_debug_flags = debug_flags;
        debug_flags |= DEBUG_PARSER;
        (*this)();
        debug_flags = old_debug_flags;
    }
    if(debug_parser.size() > 0)
        return _sat + "\n\n" + debug_parser + "\n";
    else
        return _sat + "\n";
}

/************************************************************************/
/* Satchecker::AssignmentMap                                            */
/************************************************************************/

void SatChecker::AssignmentMap::setEnabledBlocks(std::vector<bool> &blocks) {
    static const boost::regex block_regexp("^B(\\d+)$", boost::regex::perl);
    for (const auto &entry : *this) {  // pair<string, bool>
        const std::string &name = entry.first;
        const bool &valid = entry.second;
        boost::smatch what;

        if (!valid || !boost::regex_match(name, what, block_regexp))
            continue;

        // Follow hack from commit e1e7f90addb15257520937c7782710caf56d4101
        if (what[1] == "00") {
            // B00 is first and means the whole block
            blocks[0] = true;
            continue;
        }

        // B0 starts at index 1
        int blockno = 1 + std::stoi(what[1]);
        blocks[blockno] = true;
    }
}

int SatChecker::AssignmentMap::formatKconfig(std::ostream &out,
                                             const MissingSet &missingSet) {
    std::map<std::string, state> selection, other_variables;

    logger << debug << "---- Dumping new assignment map" << std::endl;

    for (const auto &entry : *this) {  // pair<string, bool>
        static const boost::regex item_regexp("^CONFIG_(.*[^.])$", boost::regex::perl);
        static const boost::regex module_regexp("^CONFIG_(.*)_MODULE$", boost::regex::perl);
        static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
        static const boost::regex choice_regexp("^CONFIG_CHOICE_.*$", boost::regex::perl);
        const std::string &name = entry.first;
        const bool &valid = entry.second;
        boost::match_results<std::string::const_iterator> what;

        if (valid && boost::regex_match(name, what, module_regexp)) {
            const std::string &tmpName = what[1];
            std::string basename = "CONFIG_" + tmpName;
            if (missingSet.find(basename) != missingSet.end() ||
                missingSet.find(what[0])  != missingSet.end()) {
                logger << debug << "Ignoring 'missing' module item "
                       << what[0] << std::endl;
                other_variables[basename] = valid ? state::yes : state::no;
            } else {
                selection[basename] = state::module;
            }
            continue;
        } else if (boost::regex_match(name, what, choice_regexp)) {
            // choices are anonymous in kconfig and only used for
            // cardinality constraints, ignore
            other_variables[what[0]] = valid ? state::yes : state::no;
            continue;
        } else if (boost::regex_match(name, what, item_regexp)) {
            ConfigurationModel *model = ModelContainer::lookupMainModel();
            const std::string &item_name = what[1];

            logger << debug << "considering " << what[0] << std::endl;

            if (missingSet.find(what[0]) != missingSet.end()) {
                logger << debug << "Ignoring 'missing' item "
                       << what[0] << std::endl;
                other_variables[what[0]] = valid ? state::yes : state::no;
                continue;
            }

            // ignore entries if already set (e.g., by the module variant).
            if (selection.find(what[0]) == selection.end()) {
                selection[what[0]] = valid ? state::yes : state::no;
                logger << debug << "Setting " << what[0] << " to " << valid << std::endl;
            }

            // skip item if it is neither a 'boolean' nor a tristate one
            if (!boost::regex_match(name, module_regexp) && model &&
                !model->isBoolean(item_name) && !model->isTristate(item_name)) {
                logger << debug << "Ignoring 'non-boolean' item " << what[0] << std::endl;

                other_variables[what[0]] = valid ? state::yes : state::no;
                continue;
            }

        } else if (boost::regex_match(name, block_regexp)) {
            // ignore block variables
            continue;
        } else {
            other_variables[name] = valid ? state::yes : state::no;
        }
    }

    for (const auto &entry : selection) {  // pair<string, state>
        const std::string &item = entry.first;
        const state &stat = entry.second;

        out << item << "=";
        if (stat == state::no)
            out << "n";
        else if (stat == state::module)
            out << "m";
        else if (stat == state::yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }

    for (const auto &entry : other_variables) {  // pair<string, state>
        const std::string &item = entry.first;
        const state &stat = entry.second;

        if (boost::starts_with(item, "CONFIG_CHOICE_") ||
            boost::starts_with(item, "__FREE__") ||
            item == "CONFIG_n" ||
            item == "CONFIG_y")
            continue;

        if (selection.find(item) != selection.end())
            continue;

        out << "# " << item << "=";
        if (stat == state::no)
            out << "n";
        else if (stat == state::yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }
    return selection.size();
}

int SatChecker::AssignmentMap::formatModel(std::ostream &out, const ConfigurationModel *model) {
    int items = 0;

    for (const auto &entry : *this) {  // pair<string, bool>
        const std::string &name = entry.first;
        const bool &valid = entry.second;
        if (model && !model->inConfigurationSpace(name))
            continue;
        out << name << "=" << (valid ? 1 : 0) << std::endl;;
        items++;
    }
    return items;
}

int SatChecker::AssignmentMap::formatAll(std::ostream &out) {
    for (const auto &entry : *this) {  // pair<string, bool>
        const std::string &name = entry.first;
        const bool &valid = entry.second;
        out << name << "=" << (valid ? 1 : 0) << std::endl;;
    }
    return size();
}

int SatChecker::AssignmentMap::formatCPP(std::ostream &out, const ConfigurationModel *model) {
    static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
    static const boost::regex valid_regexp("^[_a-zA-Z].*$", boost::regex::perl);

    for (const auto &entry : *this) {  // pair<string, bool>
        const std::string &name = entry.first;
        // ignoring block variables
        if (boost::regex_match(name, block_regexp))
            continue;

        // ignoring symbols that can be defined
        if (boost::algorithm::ends_with(name, "."))
            continue;

        // ignoring invalid cpp flags
        if (!boost::regex_match(name, valid_regexp))
            continue;

        // only in model space
        if (model && !model->inConfigurationSpace(name))
            continue;

        const bool &on = entry.second;

        // only true symbols
        if (!on)
            continue;

        out << " -D" << name << "=1";
    }
    out << std::endl;
    return size();
}

int SatChecker::AssignmentMap::formatCommented(std::ostream &out, const CppFile &file) {
    std::map<Puma::Token *, bool> flag_map;
    Puma::Token *next;
    Puma::TokenStream stream;
    sighandler_t oldaction;

    PumaConditionalBlock *topBlock = (PumaConditionalBlock *) file.topBlock();
    Puma::Unit *unit = topBlock->unit();
    if (!unit) {
        // in this case we have lost. this can happen e.g. on an empty
        // file, such as /dev/null
        return 0;
    }

    /* If the child process terminates before reading all of stdin
     * undertaker gets a SIGPIPE which we don't want to handle
     */
    oldaction = signal(SIGPIPE, SIG_IGN);

    flag_map[topBlock->pumaStartToken()] = true;
    flag_map[topBlock->pumaEndToken()] = false;

    // iterate over all ConditionalBlocks but skip first block (B00)
    for (auto it = ++(file.begin()); it != file.end(); ++it) {
        PumaConditionalBlock *block = (PumaConditionalBlock *)(*it);
        if (block->isDummyBlock()) {
            continue;
        } else if ((*this)[block->getName()] == true) {
            // Block is enabled in this assignment
            next = block->pumaStartToken();
            flag_map[next] = false;
            do {
                next = unit->next(next);
            } while (next->text()[0] != '\n');
            flag_map[next] = true;

            next = block->pumaEndToken();
            flag_map[next] = false;
            do { next = unit->next(next); } while ( next && next->text()[0] != '\n');
            if (next)
                flag_map[next] = true;
        } else {
            // Block is disabled in this assignment
            next = block->pumaStartToken();
            flag_map[next] = false;
            do {
                next = unit->next(next);
            } while (next && next->text()[0] != '\n');
            if (next)
                flag_map[next] = false;
            next = block->pumaEndToken();
            do {
                next = unit->next(next);
            } while (next && next->text()[0] != '\n');
            if (next)
                flag_map[next] = true;
        }
    }

    /* Initialize token stream with the Puma::Unit */
    stream.push(unit);

    bool print_flag = true;
    bool after_newline = true;

    int printed_newlines = 1;

    while ((next = stream.next())) {
        if (flag_map.find(next) != flag_map.end())
            print_flag = flag_map[next];

        if (!print_flag && after_newline)
            out << "// ";

        for (const char *p = next->text(); *p; p++) {
            if (*p == '\n') {
                printed_newlines++;
                out << "\n";
                // if (!print_flag)
                //    out << "// ";
            } else {
                out << *p;
            }
        }
        while (after_newline && printed_newlines < next->location().line()) {
            out << "\n";
            printed_newlines++;
        }
        after_newline = strchr(next->text(), '\n') != nullptr;
    }
    signal(SIGPIPE, oldaction);

    return size();
}

int SatChecker::AssignmentMap::formatCombined(const CppFile &file, const ConfigurationModel *model,
                                              const MissingSet& missingSet, unsigned number) {
    std::stringstream s;
    s << file.getFilename() << ".cppflags" << number;

    std::ofstream modelstream(s.str());
    formatCPP(modelstream, model);

    s.str("");
    s.clear();
    s << file.getFilename() << ".source" << number;
    std::ofstream commented(s.str());
    formatCommented(commented, file);

    s.str("");
    s.clear();
    s << file.getFilename() << ".config" << number;
    std::ofstream kconfig(s.str());
    formatKconfig(kconfig, missingSet);

    return size();
}

int SatChecker::AssignmentMap::formatExec(const CppFile &file, const char *cmd) {
    redi::opstream cmd_process(cmd);

    logger << info << "Calling: " << cmd << std::endl;
    formatCommented(cmd_process, file);
    cmd_process.close();

    return size();
}

void SatChecker::pprintAssignments(std::ostream& out,
                                   const std::list<SatChecker::AssignmentMap> solutions,
                                   const ConfigurationModel *model,
                                   const MissingSet &missingSet) {
    out << "I: Found " << solutions.size() << " assignments" << std::endl;
    out << "I: Entries in missingSet: " << missingSet.size() << std::endl;

    std::map<std::string, bool> common_subset;

    for (const auto &conf : solutions) {  // AssignmentMap
        for (const auto &entry : conf) {  // pair<string, bool>
            const std::string &name = entry.first;
            const bool &valid = entry.second;

            if (model && !model->inConfigurationSpace(name))
                continue;

            if (conf == solutions.front()) {
                // Copy first assignment to common subset map
                common_subset[name] = valid;
            } else {
                if (common_subset.find(name) != common_subset.end()
                    && common_subset[name] != valid) {
                    // In this assignment the symbol has a different
                    // value, remove it from the common subset map
                    common_subset.erase(name);
                }
            }
        }
    }

    // Remove all items from common subset, that are not in all assignments
    for (const auto &entry : common_subset) {  // pair<string, bool>
        const std::string &name = entry.first;
        for (auto &conf : solutions) {  // AssignmentMap
            if (conf.find(name) == conf.end())
                common_subset.erase(name);
        }
    }

    out << "I: In all assignments the following symbols are equally set" << std::endl;
    for (const auto &entry : common_subset) {  // pair<string, bool>
            const std::string &name = entry.first;
            const bool &valid = entry.second;
            out << name << "=" << (valid ? 1 : 0) << std::endl;
    }

    out << "I: All differences in the assignments" << std::endl;
    int i = 0;
    for (const auto &conf : solutions) {  // AssignmentMap
        out << "I: Config " << i++ << std::endl;
        for (const auto &entry : conf) {  // pair<string, bool>
            const std::string &name = entry.first;
            const bool &valid = entry.second;

            if (model && !model->inConfigurationSpace(name))
                continue;

            if (common_subset.find(name) != common_subset.end())
                continue;

            out << name << "=" << (valid ? 1 : 0) << std::endl;
        }
    }
}


/************************************************************************/
/* BaseExpressionSatChecker                                             */
/************************************************************************/

bool BaseExpressionSatChecker::operator()(const std::set<std::string> &assumeSymbols) {
    /* Assume additional all given symbols */
    for (const std::string &str : assumeSymbols)
        _cnf->pushAssumption(str, true);

    int res = _cnf->checkSatisfiable();
    if (res) {
        /* Let's get the assigment out of picosat, because we have to
            reset the sat solver afterwards */
        assignmentTable.clear();
        for (const auto &entry : _cnf->getSymbolMap()) {  // pair<string, int>
            bool selected = this->_cnf->deref(entry.second);
            assignmentTable.emplace(entry.first, selected);
        }
    }
    return res;
}

BaseExpressionSatChecker::BaseExpressionSatChecker(std::string base_expression, int debug)
        : SatChecker(base_expression, false, debug) {
    _cnf = getCnfWithModelInit(base_expression, Picosat::SAT_MAX, &base_expression);
    CNFBuilder builder(_cnf.get(), base_expression, true, CNFBuilder::ConstantPolicy::BOUND);
}
