/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
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
#include "SatChecker-grammar.t"
#include "PumaConditionalBlock.h"
#include "Logging.h"

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

/* Got the impression from normal lisp implementations */
int SatChecker::stringToSymbol(const std::string &key) {
    KconfigWhitelist *wl = KconfigWhitelist::getInstance();

    /* If whitelisted do always return a new symbol, so these items
       are free variables */

    if (wl->isWhitelisted(key.c_str())) {
        return newSymbol();
    }

    std::map<std::string, int>::iterator it;
    if ((it = symbolTable.find(key)) != symbolTable.end()) {
        return it->second;
    }
    int n = newSymbol();
    symbolTable[key] = n;
    return n;
}

int SatChecker::newSymbol(void) {
    return Picosat::picosat_inc_max_var();
}

void SatChecker::addClause(int *clause) {
    for (int *x = clause; *x; x++)
        Picosat::picosat_add(*x);
    Picosat::picosat_add(0);
    _clauses ++;
}

int SatChecker::notClause(int inner_clause) {
    int this_clause  = newSymbol();

    int clause1[3] = { this_clause,  inner_clause, 0};
    int clause2[3] = {-this_clause, -inner_clause, 0};

    addClause(clause1);
    addClause(clause2);

    return this_clause;
}

int SatChecker::andClause(int A_clause, int B_clause) {
    // This function just does binary ands in contradiction to the
    // andID transform_rec
    // A & B & ..:
    // !3 A 0
    // !3 B 0
    // 3 !A !B 0

    int this_clause = newSymbol();

    int A[] = {-this_clause, A_clause, 0};
    int B[] = {-this_clause, B_clause, 0};
    int last[] = {this_clause, -A_clause, -B_clause, 0};

    addClause(A);
    addClause(B);
    addClause(last);

    return this_clause;
}

int SatChecker::orClause(int A_clause, int B_clause) {
    // This function just does binary ands in contradiction to the
    // andID transform_rec
    // A & B & ..:
    // 3 !A 0
    // 3 !B 0
    // !3 A B 0

    int this_clause = newSymbol();

    int A[] = {this_clause, -A_clause, 0};
    int B[] = {this_clause, -B_clause, 0};
    int last[] = {-this_clause, A_clause, B_clause, 0};

    addClause(A);
    addClause(B);
    addClause(last);

    return this_clause;
}


int SatChecker::transform_bool_rec(iter_t const& input) {
    iter_t root_node = input;

 beginning_of_function:
    iter_t iter = root_node->children.begin();

    if (root_node->value.id() == bool_grammar::symbolID) {
        iter_t inner_node = root_node->children.begin();
        std::string value (inner_node->value.begin(), inner_node->value.end());
        _debug_parser("- " + value, false);
        return stringToSymbol(value);
    } else if (root_node->value.id() == bool_grammar::not_symbolID) {
        iter_t inner_node = root_node->children.begin()->children.begin();
        _debug_parser("- not");
        int inner_clause = transform_bool_rec(inner_node);
        _debug_parser();

        return notClause(inner_clause);
    } else if (root_node->value.id() == bool_grammar::andID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }

        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause = newSymbol();
        _debug_parser("- and");
        // A & B & ..:
        // !3 A 0
        // !3 B 0
        // ..
        // 3 !A !B .. 0
        end_clause[i++] = this_clause;

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {-this_clause, child_clause, 0};
            end_clause[i++] = -child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);

        _debug_parser();
        return this_clause;
    } else if (root_node->value.id() == bool_grammar::orID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause  = newSymbol();
        end_clause[i++] = -this_clause;
        // A | B
        // 3 !A 0
        // 3 !B 0
        // !3 A B 0
        _debug_parser("- or");

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {this_clause, -child_clause, 0};
            end_clause[i++] = child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
        _debug_parser();

        return this_clause;
    } else if (root_node->value.id() == bool_grammar::impliesID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        // A -> B
        // 3 A 0
        // 3 !B 0
        // !3 !A B 0
        // A  ->  B  ->  C
        // (A -> B)  ->  C
        //    A      ->  C
        //          A
        _debug_parser("- ->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
            int c1[] = { this_clause, A_clause, 0};
            int c2[] = { this_clause, -B_clause, 0};
            int c3[] = { -this_clause, -A_clause, B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            A_clause = this_clause;
        }
        _debug_parser();
        return A_clause;
    } else if (root_node->value.id() == bool_grammar::iffID) {
        iter_t iter = root_node->children.begin();
        /* Skip or rule if there is only one child */
        if ((iter+1) == root_node->children.end()) {
            return transform_bool_rec(iter);
        }
        // A <-> B
        // 3 !A  !B 0
        // 3 A B 0
        // !3 !A B 0
        // !3 A !B 0
        // A  <->  B  <->  C
        // (A <-> B)  <->  C
        //    A      <->  C
        //          A
        _debug_parser("- <->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
            int c1[] = { this_clause,  -A_clause, -B_clause, 0};
            int c2[] = { this_clause,   A_clause,  B_clause, 0};
            int c3[] = { -this_clause, -A_clause,  B_clause, 0};
            int c4[] = { -this_clause,  A_clause, -B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            addClause(c4);
            A_clause = this_clause;
        }
        _debug_parser();
        return A_clause;
    } else if (root_node->value.id() == bool_grammar::comparatorID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int this_clause  = newSymbol();
        _debug_parser("- __COMP__N", false);
        return this_clause;

    } else {
        /* Just a Container node, we simply go inside and try again. */
        root_node = root_node->children.begin();
        goto beginning_of_function;
    }
}

int SatChecker::fillSatChecker(std::string expression) throw (SatCheckerError) {
    static bool_grammar e;
    tree_parse_info<> info = pt_parse(expression.c_str(), e,
                                      space_p | ch_p("\n") | ch_p("\r"));

    if (info.full) {
        return fillSatChecker(info);
    } else {
        /* Enable this line to get the position where the parser dies
        std::cout << std::string(expression.begin(), expression.begin()
                                 + info.length) << endl;
        */
        Picosat::picosat_reset();
        throw SatCheckerError("SatChecker: Couldn't parse: " + expression);
    }
}

int SatChecker::fillSatChecker(tree_parse_info<>& info) {
    iter_t expression = info.trees.begin()->children.begin();
    int top_clause = transform_bool_rec(expression);

    /* This adds the last clause */
    Picosat::picosat_assume(top_clause);
    return top_clause;
}

SatChecker::SatChecker(const char *sat, int debug)
    : debug_flags(debug), _sat(std::string(sat)), _clauses(0) { }

SatChecker::SatChecker(const std::string sat, int debug)
    : debug_flags(debug), _sat(std::string(sat)), _clauses(0) { }

bool SatChecker::operator()() throw (SatCheckerError) {
    /* Clear the debug parser, if we are called multiple times */
    debug_parser.clear();
    debug_parser_indent = 0;

    try {
        Picosat::picosat_init();
        // try to enable as many features as possible
        Picosat::picosat_set_global_default_phase(1);

        fillSatChecker(_sat);

        int res = Picosat::picosat_sat(-1);

        if (res == PICOSAT_SATISFIABLE) {
            /* Let's get the assigment out of picosat, because we have to
               reset the sat solver afterwards */
            std::map<std::string, int>::const_iterator it;
            for (it = symbolTable.begin(); it != symbolTable.end(); ++it) {
                bool selected = Picosat::picosat_deref(it->second) == 1;
                assignmentTable.insert(std::make_pair(it->first, selected));
            }
        }

        Picosat::picosat_reset();


    if (res == PICOSAT_UNSATISFIABLE)
        return false;
    return true;
    } catch (std::bad_alloc &exception) {
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

        int blockno = boost::lexical_cast<int>(what[1]);
        blocks[blockno] = true;
    }
}

int SatChecker::AssignmentMap::formatKconfig(std::ostream &out, const MissingSet &missingSet) {
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
            std::string fullname = "CONFIG_" + name;
            if (missingSet.find(fullname) != missingSet.end()) {
                logger << debug << "Ignoring 'missing' item " << fullname << std::endl;
                other_variables[fullname] = valid ? yes : no;
            } else
                selection[name] = module;
        } else if (boost::regex_match(name, what, choice_regexp)) {
            other_variables[what[0]] = valid ? yes : no;
            continue;
        } else if (boost::regex_match(name, what, item_regexp)) {
            ConfigurationModel *model = ModelContainer::lookupMainModel();
            const std::string &item_name = what[1];

            logger << debug << "considering " << item_name << std::endl;

            if (missingSet.find(what[0]) != missingSet.end()) {
                logger << debug << "Ignoring 'missing' item " << what[0] << std::endl;

                other_variables[what[1]] = valid ? yes : no;
                continue;
            }

            // ignore entries if already set (e.g., by the module variant).
            if (selection.find(item_name) == selection.end())
                selection[item_name] = valid ? yes : no;

            // skip item if it is neither a 'boolean' nor a tristate one
            if (!boost::regex_match(name, module_regexp) && model &&
                !model->isBoolean(item_name) && !model->isTristate(item_name)) {
                logger << debug << "Ignoring 'non-boolean' item " << what[0] << std::endl;

                other_variables[what[1]] = valid ? yes : no;
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

        if (other_variables.find(item) != other_variables.end())
            continue;

        out << "CONFIG_" << item << "=";
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
    Puma::Unit *unit = top->pumaStartToken()->unit();

	/* If the child process terminates before reading all of stdin
	 * undertaker gets a SIGPIPE which we don't want to handle
	 */
    oldaction = signal(SIGPIPE, SIG_IGN);

    flag_map[top->pumaStartToken()] = true;
    flag_map[top->pumaEndToken()] = false;

    for (CppFile::const_iterator it = file.begin(); it != file.end(); ++it) {
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
            do { next = unit->next(next); } while (next->text()[0] != '\n');
            flag_map[next] = true;
        } else {
            // Block is disabled in this assignment
            next = block->pumaStartToken();
            flag_map[next] = false;
            do {
                next = unit->next(next);
            } while (next->text()[0] != '\n');
            flag_map[next] = false;
            next = block->pumaEndToken();
            do {
                next = unit->next(next);
            } while (next->text()[0] != '\n');
            flag_map[next] = true;
        }
    }

    /* Initialize token stream with the Puma::Unit */
    stream.push(unit);

    bool print_flag = true;
    bool after_newline = true;

    while ((next = stream.next())) {
        if (flag_map.find(next) != flag_map.end())
            print_flag = flag_map[next];

        if (!print_flag && after_newline)
            out << "// ";

        out << next->text();

        after_newline = strchr(next->text(), '\n') != NULL;
    }

    signal(SIGPIPE, oldaction);

    return size();
}

int SatChecker::AssignmentMap::formatCombined(const CppFile &file, const ConfigurationModel *model, unsigned number) {
    std::stringstream s;
    s << file.getFilename() << ".config" << number;

    std::ofstream modelstream(s.str().c_str());
    formatCPP(modelstream, model);

    s << ".src";
    std::ofstream commented(s.str().c_str());
    formatCommented(commented, file);

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
        /* Assume the top and clause */
        Picosat::picosat_assume(base_clause);

        /* Assume additional all given symbols */
        for (std::set<std::string>::const_iterator it = assumeSymbols.begin();
             it != assumeSymbols.end(); it++) {
            Picosat::picosat_assume(stringToSymbol((*it).c_str()));
        }

        // try to enable as many features as possible
        Picosat::picosat_set_global_default_phase(1);

        int res = Picosat::picosat_sat(-1);

        if (res == PICOSAT_SATISFIABLE) {
            /* Let's get the assigment out of picosat, because we have to
               reset the sat solver afterwards */
            assignmentTable.clear();
            std::map<std::string, int>::const_iterator it;
            for (it = symbolTable.begin(); it != symbolTable.end(); ++it) {
                bool selected = Picosat::picosat_deref(it->second) == 1;
                assignmentTable.insert(std::make_pair(it->first, selected));
            }
        }

        if (res == PICOSAT_UNSATISFIABLE) {
            return false;
        }
        return true;
    } catch (std::bad_alloc &exception) {
        throw SatCheckerError("SatChecker: out of memory");
    }
    return false;
}

BaseExpressionSatChecker::BaseExpressionSatChecker(const char *base_expression, int debug)
    : SatChecker(base_expression, debug) {
    Picosat::picosat_init();
    base_clause = fillSatChecker(base_expression);
}
