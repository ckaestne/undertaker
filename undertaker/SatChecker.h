/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

// -*- mode: c++ -*-
#ifndef sat_checker_h__
#define sat_checker_h__

#include <stdexcept>
#include <map>
#include <sstream>
#include <set>
#include <list>
#include <vector>

#include "PicosatCNF.h"
// Forward declaration for configuration model, becasue the boost
// namespace seems to break, when we include ConfigurationModel
class ConfigurationModel;
class CppFile;
typedef std::set<std::string> MissingSet;

using namespace kconfig;

namespace Picosat
{
    #include <ctype.h>
    #include <assert.h>

    using std::string;

    /* Include the Limmat library header as C */
    extern "C"
    {
        #include <picosat/picosat.h>
    }
};

struct SatCheckerError : public std::runtime_error
{
    SatCheckerError(const char *s)
        : runtime_error(s) {}
    SatCheckerError(std::string s)
        : runtime_error(s.c_str()) {}
};

class SatChecker
{
    protected:
        CNF *_cnf;
    public:
        typedef char const*                              iterator_t;

        SatChecker(const char *sat, int debug = 0);
        SatChecker(const std::string sat, int debug = 0);
        virtual ~SatChecker() {};

        /**
         * Checks the given string with an sat solver
         * @param sat the formula to be checked
         * @param picosat_mode set picosat's initial phase. Defaults to 1
         * @returns true, if satisfiable, false otherwise
         * @throws if syntax error
         */
        virtual bool operator()(Picosat::SATMode mode=Picosat::SAT_MAX)
            throw (SatCheckerError);

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

        enum Debug
        {
            DEBUG_NONE = 0,
            DEBUG_PARSER = 1,
            DEBUG_CNF = 2
        };

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
        struct AssignmentMap : public std::map<std::string, bool>
        {

            /**
             * \brief order independent content comparison
             *
             * This method compares if two assignments are equivalent.
             */
            bool operator==(const AssignmentMap &other) const
            {
                for (const_iterator it = begin(); it != end(); it++) {
                    const_iterator ot = other.find((*it).first);
                    if (ot == other.end() || (*ot).second != (*it).second)
                        return false;
                }
                return true;
            }

            /**
             * \brief collect enabled blocks
             *
             * The idea of this method is to set all blocks that are enabled
             * in a bitvector. Hereby, the position of each bit in the
             * vector represents the block number. 1 represents a selected
             * block. Bits are only set and never unset.
             */
            void setEnabledBlocks(std::vector<bool> &blocks);

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

            /**
             * \brief format solutions as CPP compatible arguments
             *
             *  Print out all cpp symbols
             *
             * \param out an output stream on which the solution shall be
             *     printed
             */
            int formatCPP(std::ostream &out, const ConfigurationModel *model);

            /**
             * \brief pipe all activated blocks into command
             *
             * \param CPP file which is basis for the analysis
             * \param cmd the command that is spawned
             */
            int formatExec(const CppFile &file, const char *cmd);

            /**
             * \brief comments out all unselected blocks
             *
             * \param out the output stream to work on
             * \param file the corresponding CppFile for this AssignmentMap
             */
            int formatCommented(std::ostream &out, const CppFile &file);

            /**
             * \brief combination of formatCPP, formatCommented and formatKconfig.
             *
             * This output format uses all of formatCPP,
             * formatCommented and formatKconfig. Unlike the other methods, this mode
             * produces three additional files with the content.
             *
             * \param file passed to formatCommented
             * \param model passed to formatCPP
             * \param missingSet passwd to formatKconfig
             * \param number a running numer that is encoded in the filename
             */
            int formatCombined(const CppFile &file, const ConfigurationModel *model,
                const MissingSet& missingSet, unsigned number);
        };

        /**
         * After doing the check, you can get the assignments for the
         * formula
         */
        const AssignmentMap& getAssignment() {
            return assignmentTable;
        }

        /**
         * Prints the assignments in an human readable way on stdout
         */
        static void pprintAssignments(std::ostream& out,
            const std::list< AssignmentMap > solution,
            const ConfigurationModel *model,
            const MissingSet &missingSet);

        int countClauses() { return _clauses; };

    protected:
        std::map<std::string, int> symbolTable;
        AssignmentMap assignmentTable;
        int debug_flags;
        std::string debug_parser;
        int debug_parser_indent;
        std::string debug_cnf;

        Picosat::SATMode mode;

        const std::string _sat;
        int _clauses;

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
                }
                else {
                    debug_parser_indent -= 2;
                }
            }
        }
        enum state {no, yes, module};
};

class BaseExpressionSatChecker : public SatChecker
{
    public:
        BaseExpressionSatChecker(const char *base_expression, int debug = 0);

        virtual ~BaseExpressionSatChecker();
        virtual bool operator()(const std::set<std::string> &assumeSymbols) throw (SatCheckerError);

    protected:
        int base_clause;
};
#endif
