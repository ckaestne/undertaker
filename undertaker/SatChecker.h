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


// -*- mode: c++ -*-
#ifndef sat_checker_h__
#define sat_checker_h__
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <stdexcept>
#include <map>
#include <sstream>
#include <set>

// Forward declaration for configuration model, becasue the boost
// namespace seems to break, when we include ConfigurationModel
class ConfigurationModel;
typedef std::set<std::string> MissingSet;

using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace Picosat {
    #include <ctype.h>
    #include <assert.h>

    using std::string;

    /* Include the Limmat library header as C */
    extern "C" {
#include <picosat/picosat.h>
    }
};

struct SatCheckerError : public std::runtime_error {
    SatCheckerError(const char *s)
        : runtime_error(s) {}
    SatCheckerError(std::string s)
        : runtime_error(s.c_str()) {}
};

class SatChecker {
public:
    typedef char const*                              iterator_t;
    typedef tree_match<iterator_t>                   parse_tree_match_t;
    typedef parse_tree_match_t::const_tree_iterator  iter_t;

    SatChecker(const char *sat, int debug = 0);
    SatChecker(const std::string sat, int debug = 0);
    ~SatChecker();

    /**
     * Checks the given string with an sat solver
     * @param sat the formula to be checked
     * @returns true, if satisfiable, false otherwise
     * @throws if syntax error
     */
    bool operator()() throw (SatCheckerError);

    static bool check(const std::string &sat) throw (SatCheckerError);
    const char *c_str() { return _sat.c_str(); }
    const std::string str() { return _sat; }

    /** pretty prints the given string */
    static std::string pprinter(const char *sat) {
        SatChecker c(const_cast<const char*>(sat));
        return c.pprint();
    }

    /** pretty prints the saved expression */
    std::string pprint();

    /** returns the runtime of the last run */
    clock_t runtime() { return _runtime; }

    enum Debug { DEBUG_NONE = 0,
                 DEBUG_PARSER = 1,
                 DEBUG_CNF = 2 };

    /**
     * \brief Representation of a variable selection
     *
     * The solutions in this map contain all kinds of SAT variables,
     * including block variables (e.g., B42), comparator (fake)
     * variables (e.g., COMP_42) and item variables (e.g.,
     * CONFIG_ACPI_MODULES).
     *
     *   - key:   something like 'B42'
     *   - value: true if set, false if unset or unknown
     */
    struct AssignmentMap : public std::map<std::string, bool> {

        /**
         * \brief order independent content comparison
         *
         * This method compares if two assignments are equivalent.
         */
        bool operator==(const AssignmentMap &other) const {
            for (const_iterator it = begin(); it != end(); it++) {
                const_iterator ot = other.find((*it).first);
                if (ot == other.end() || (*ot).second != (*it).second)
                    return false;
            }
            return true;
        }

        /**
         * \brief format solutions (kconfig specific)
         *
         * This method filters out comparators and block variables from the
         * given AssignmentMap solution.  Additionally,
         * CONFIG_ACPI_MODULE and the like lead to the CONFIG_ACPI variable
         * being set to '=m'. This output resembles a partial KConfig
         * selection.
         *
         * \param out an output stream on which the solution shall be printed
         * \param missingSet set of items that are not available in the model
         */
        int formatKconfig(std::ostream &out,
                          const MissingSet &missingSet);

        /**
         * \brief format solutions (model)
         *
         *  Print out all assignments that are in the configuration space
         *
         * \param out an output stream on which the solution shall be
         *     printed
         * \param configuration model, that specifies the
         *     configuration space
         */
        int formatModel(std::ostream &out, const ConfigurationModel *model);

        /**
         * \brief format solutions (all)
         *
         *  Print out all assignments!
         *
         * \param out an output stream on which the solution shall be
         *     printed
         */
        int formatAll(std::ostream &out);

    };

    /**
     * After doing the check, you can get the assignments for the
     * formula
     */
    const AssignmentMap& getAssignment() {
        return assignmentTable;
    }


private:
    std::map<std::string, int> symbolTable;
    AssignmentMap assignmentTable;
    int debug_flags;
    std::string debug_parser;
    int debug_parser_indent;
    std::string debug_cnf;

    const std::string _sat;
    clock_t _runtime;

    int stringToSymbol(const std::string &key);
    int newSymbol(void);
    void addClause(int *clause);

    int notClause(int inner_clause);
    int andClause(int A_clause, int B_clause);
    int orClause(int A_clause, int B_clause);

    int transform_bool_rec(iter_t const& input);
    void fillSatChecker(std::string expression) throw (SatCheckerError);
    void fillSatChecker(tree_parse_info<>& info);

    // Debugging stuff
    void _debug_parser(std::string d = "", bool newblock = true) {
        if (debug_flags & DEBUG_PARSER) {
            if (d.size()) {
                debug_parser += "\n";
                for (int i = 0; i < debug_parser_indent; i++)
                    debug_parser += " ";

                debug_parser += d;
                if (newblock)
                    debug_parser_indent += 2;
            } else {
                debug_parser_indent -= 2;
            }
        }
    }
    enum state {no, yes, module};
};
#endif
