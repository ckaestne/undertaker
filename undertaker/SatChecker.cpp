/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Ralf Hackner <sirahack@informatik.uni-erlangen.de>
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


#include <Puma/CParser.h>
#include <Puma/TokenStream.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <pstreams/pstream.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "SatChecker.h"
#include "KconfigWhitelist.h"

#include "ModelContainer.h"
#include "ConfigurationModel.h"
#include "PumaConditionalBlock.h"
#include "Logging.h"

#include "CNFBuilder.h"
#include "PicosatCNF.h"
#include "CNF.h"
#include "bool.h"
#include "Kconfig.h"

bool SatChecker::check(const std::string &sat) throw (SatCheckerError) {
    SatChecker c(sat.c_str());

    try {
        return c();
    } catch (SatCheckerError &e) {
        logger << error << "Syntax Error:" << std::endl;
        logger << error << sat << std::endl;
        logger << error << "End of Syntax Error" << std::endl;
        throw e;
    }
    return false;
}

SatChecker::SatChecker(const char *sat, int debug)
    : debug_flags(debug), _sat(std::string(sat)), _clauses(0) { }

SatChecker::SatChecker(const std::string sat, int debug)
    : debug_flags(debug), _sat(std::string(sat)), _clauses(0) { }

bool SatChecker::operator()(Picosat::SATMode mode) throw (SatCheckerError) {

    _cnf = new PicosatCNF(mode);
    try {

        BoolExp *exp = BoolExp::parseString(_sat);

        if (!exp) {
           throw SatCheckerError("SatChecker: Couldn't parse: " + _sat);
        }

        CNFBuilder builder(true, CNFBuilder::FREE);

        builder.cnf = _cnf;
        builder.pushClause(exp);

        int res = _cnf->checkSatisfiable();
        if (res) {
            /* Let's get the assigment out of picosat, because we have to
               reset the sat solver afterwards */
            std::map<std::string, int>::const_iterator it;
            for (it = _cnf->getSymbolsItBegin(); it != _cnf->getSymbolsItEnd(); ++it) {
                bool selected = this->_cnf->deref(it->second);
                assignmentTable.insert(std::make_pair(it->first, selected));
            }
        }
        delete _cnf;
        delete exp;
        return res;
    }
    catch (std::bad_alloc &exception) {
        throw SatCheckerError("SatChecker: out of memory");
    }
}

std::string SatChecker::pprint() {
    if (debug_parser.size() == 0) {
        int old_debug_flags = debug_flags;
        debug_flags |= DEBUG_PARSER;
        (*this)();
        debug_flags = old_debug_flags;
    }
    return _sat + "\n\n" + debug_parser + "\n";
}

void SatChecker::AssignmentMap::setEnabledBlocks(std::vector<bool> &blocks) {
    static const boost::regex block_regexp("^B(\\d+)$", boost::regex::perl);
    for (AssignmentMap::iterator it = begin(); it != end(); it++) {
        const std::string &name = (*it).first;
        const bool &valid = (*it).second;
        boost::match_results<std::string::const_iterator> what;

        if (!valid || !boost::regex_match(name, what, block_regexp))
            continue;

        // Follow hack from commit e1e7f90addb15257520937c7782710caf56d4101

        if (what[1] == "00") {
            // B00 is first and means the whole block
            blocks[0] = true;
            continue;
        }

        // B0 starts at index 1
        int blockno = 1 + boost::lexical_cast<int>(what[1]);
        blocks[blockno] = true;
    }
}

int SatChecker::AssignmentMap::formatKconfig(std::ostream &out,
                                             const MissingSet &missingSet) {
    typedef std::map<std::string, state> SelectionType;
    SelectionType selection, other_variables;

    logger << debug << "---- Dumping new assignment map" << std::endl;

    for (AssignmentMap::iterator it = begin(); it != end(); it++) {
        static const boost::regex item_regexp("^CONFIG_(.*[^.])$", boost::regex::perl);
        static const boost::regex module_regexp("^CONFIG_(.*)_MODULE$", boost::regex::perl);
        static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
        static const boost::regex choice_regexp("^CONFIG_CHOICE_.*$", boost::regex::perl);
        const std::string &name = (*it).first;
        const bool &valid = (*it).second;
        boost::match_results<std::string::const_iterator> what;

        if (valid && boost::regex_match(name, what, module_regexp)) {
            const std::string &name = what[1];
            std::string basename = "CONFIG_" + name;
            if (missingSet.find(basename) != missingSet.end() ||
                missingSet.find(what[0])  != missingSet.end()) {
                logger << debug << "Ignoring 'missing' module item "
                       << what[0] << std::endl;
                other_variables[basename] = valid ? yes : no;
            } else {
                selection[basename] = module;
            }
            continue;
        } else if (boost::regex_match(name, what, choice_regexp)) {
            // choices are anonymous in kconfig and only used for
            // cardinality constraints, ignore
            other_variables[what[0]] = valid ? yes : no;
            continue;
        } else if (boost::regex_match(name, what, item_regexp)) {
            ConfigurationModel *model = ModelContainer::lookupMainModel();
            const std::string &item_name = what[1];

            logger << debug << "considering " << what[0] << std::endl;

            if (missingSet.find(what[0]) != missingSet.end()) {
                logger << debug << "Ignoring 'missing' item "
                       << what[0] << std::endl;
                other_variables[what[0]] = valid ? yes : no;
                continue;
            }

            // ignore entries if already set (e.g., by the module variant).
            if (selection.find(what[0]) == selection.end()) {
                selection[what[0]] = valid ? yes : no;
                logger << debug << "Setting " << what[0] << " to " << valid << std::endl;
            }

            // skip item if it is neither a 'boolean' nor a tristate one
            if (!boost::regex_match(name, module_regexp) && model &&
                !model->isBoolean(item_name) && !model->isTristate(item_name)) {
                logger << debug << "Ignoring 'non-boolean' item " << what[0] << std::endl;

                other_variables[what[0]] = valid ? yes : no;
                continue;
            }

        } else if (boost::regex_match(name, block_regexp))
            // ignore block variables
            continue;
        else
            other_variables[name] = valid ? yes : no;
    }

    for (SelectionType::iterator s = selection.begin(); s != selection.end(); s++) {
        const std::string &item = (*s).first;
        const int &state = (*s).second;

        out << item << "=";
        if (state == no)
            out << "n";
        else if (state == module)
            out << "m";
        else if (state == yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }
    for (SelectionType::iterator s = other_variables.begin(); s != other_variables.end(); s++) {
        const std::string &item = (*s).first;
        const int &state = (*s).second;

        if (boost::starts_with(item, "CONFIG_CHOICE_") ||
            boost::starts_with(item, "__FREE__") ||
            item == "CONFIG_n" ||
            item == "CONFIG_y")
            continue;

        if (selection.find(item) != selection.end())
            continue;

        out << "# " << item << "=";
        if (state == no)
            out << "n";
        else if (state == yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }
    return selection.size();
}

int SatChecker::AssignmentMap::formatModel(std::ostream &out, const ConfigurationModel *model) {
    int items = 0;

    for (AssignmentMap::iterator it = begin(); it != end(); it++) {
        const std::string &name = (*it).first;
        const bool &valid = (*it).second;
        if (model && !model->inConfigurationSpace(name))
            continue;
        out << name << "=" << (valid ? 1 : 0) << std::endl;;
        items ++;
    }
    return items;
}

int SatChecker::AssignmentMap::formatAll(std::ostream &out) {
    for (AssignmentMap::iterator it = begin(); it != end(); it++) {
        const std::string &name = (*it).first;
        const bool &valid = (*it).second;
        out << name << "=" << (valid ? 1 : 0) << std::endl;;
    }
    return size();
}

int SatChecker::AssignmentMap::formatCPP(std::ostream &out, const ConfigurationModel *model) {
    static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
    static const boost::regex valid_regexp("^[_a-zA-Z].*$", boost::regex::perl);

    for (AssignmentMap::iterator it = begin(); it != end(); it++) {
        const std::string &name = (*it).first;
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

        const bool &on = (*it).second;

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

    PumaConditionalBlock *top = (PumaConditionalBlock *) file.topBlock();
    Puma::Unit *unit = top->unit();
    if (!unit) {
        // in this case we have lost. this can happen e.g. on an empty
        // file, such as /dev/null
        return 0;
    }

        /* If the child process terminates before reading all of stdin
         * undertaker gets a SIGPIPE which we don't want to handle
         */
    oldaction = signal(SIGPIPE, SIG_IGN);

    flag_map[top->pumaStartToken()] = true;
    flag_map[top->pumaEndToken()] = false;

    for (CppFile::const_iterator it = ++file.begin(); it != file.end(); ++it) {
        PumaConditionalBlock *block = (PumaConditionalBlock *) (*it);
        if ((*this)[block->getName()] == true) {
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
            } else
                out << *p;
        }

        while (after_newline && printed_newlines < next->location().line()) {
            out << "\n";
            printed_newlines++;
        }

        after_newline = strchr(next->text(), '\n') != NULL;
    }

    signal(SIGPIPE, oldaction);

    return size();
}

int SatChecker::AssignmentMap::formatCombined(const CppFile &file, const ConfigurationModel *model,
                                              const MissingSet& missingSet, unsigned number) {
    std::stringstream s;
    s << file.getFilename() << ".cppflags" << number;

    std::ofstream modelstream(s.str().c_str());
    formatCPP(modelstream, model);

    s.str("");
    s.clear();
    s << file.getFilename() << ".source" << number;
    std::ofstream commented(s.str().c_str());
    formatCommented(commented, file);

    s.str("");
    s.clear();
    s << file.getFilename() << ".config" << number;
    std::ofstream kconfig(s.str().c_str());
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
                                   const std::list<SatChecker::AssignmentMap> solution,
                                   const ConfigurationModel *model,
                                   const MissingSet &missingSet) {
    out << "I: Found " << solution.size() << " assignments" << std::endl;
    out << "I: Entries in missingSet: " << missingSet.size() << std::endl;

    std::map<std::string, bool> common_subset;

    for (std::list<AssignmentMap>::const_iterator conf = solution.begin(); conf != solution.end(); ++conf) {
        for (AssignmentMap::const_iterator  it = (*conf).begin(); it != (*conf).end(); it++) {
            const std::string &name = (*it).first;
            const bool &valid = (*it).second;

            if (model && !model->inConfigurationSpace(name))
                continue;

            if (conf == solution.begin()) {
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

    // Remove all items from common subset, that are not in all
    // assignments
    for (std::map<std::string, bool>::const_iterator it = common_subset.begin();
         it != common_subset.end(); ++it) {

        const std::string &name = (*it).first;

        for (std::list<AssignmentMap>::const_iterator conf = solution.begin(); conf != solution.end(); ++conf) {
            if ((*conf).find(name) == (*conf).end()) {
                common_subset.erase(name);
            }
        }
    }

    out << "I: In all assignments the following symbols are equally set" << std::endl;
    for (std::map<std::string, bool>::const_iterator it = common_subset.begin();
         it != common_subset.end(); ++it) {
            const std::string &name = (*it).first;
            const bool &valid = (*it).second;

            out << name << "=" << (valid ? 1 : 0) << std::endl;
    }

    out << "I: All differences in the assignments" << std::endl;

    int i = 0;
    for (std::list<AssignmentMap>::const_iterator conf = solution.begin(); conf != solution.end(); ++conf) {
        out << "I: Config " << i++ << std::endl;
        for (AssignmentMap::const_iterator  it = (*conf).begin(); it != (*conf).end(); it++) {
            const std::string &name = (*it).first;
            const bool &valid = (*it).second;

            if (model && !model->inConfigurationSpace(name))
                continue;

            if (common_subset.find(name) != common_subset.end())
                continue;

            out << name << "=" << (valid ? 1 : 0) << std::endl;
        }
    }
}

// -----------------
// BaseExpressionSatChecker
// ----------------

bool BaseExpressionSatChecker::operator()(const std::set<std::string> &assumeSymbols) throw (SatCheckerError) {
    try {
        /* Assume additional all given symbols */
        for (std::set<std::string>::const_iterator it = assumeSymbols.begin();
                it != assumeSymbols.end(); it++) {
            std::string a = *it;
            _cnf->pushAssumption(a, true);
        }

        int res = _cnf->checkSatisfiable();
        if (res) {
            /* Let's get the assigment out of picosat, because we have to
               reset the sat solver afterwards */
            assignmentTable.clear();
            std::map<std::string, int>::const_iterator it;
            for (it = _cnf->getSymbolsItBegin(); it != _cnf->getSymbolsItEnd(); ++it) {
                bool selected = this->_cnf->deref(it->second);
                assignmentTable.insert(std::make_pair(it->first, selected));
            }
        }

        return res;
    }
    catch (std::bad_alloc &exception) {
        throw SatCheckerError("SatChecker: out of memory");
    }
    return false;
}

BaseExpressionSatChecker::BaseExpressionSatChecker(const char *base_expression, int debug)
: SatChecker(base_expression, debug) {

    _cnf = new PicosatCNF(Picosat::SAT_MAX);
    std::string base_expression_s(base_expression);
    BoolExp *exp = BoolExp::parseString(base_expression_s);
    if (!exp){
        throw SatCheckerError("SatChecker: Couldn't parse: " + base_expression_s);
    }

    CNFBuilder builder(true, CNFBuilder::BOUND);

    builder.cnf = _cnf;
    builder.pushClause(exp);

}

BaseExpressionSatChecker::~BaseExpressionSatChecker() {
    delete _cnf;
}
