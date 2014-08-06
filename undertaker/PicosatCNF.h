// -*- mode: c++ -*-
/*
 *   boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2012-2013 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KCONFIG_PICOSATCNF_H
#define KCONFIG_PICOSATCNF_H

#include "Kconfig.h"

#include <vector>
#include <map>
#include <string>
#include <deque>

namespace Picosat {
    // Modes taken from picosat.h
    enum SATMode {
        SAT_MIN     = 0,
        SAT_MAX     = 1,
        SAT_DEFAULT = 2,
        SAT_RANDOM  = 3,
    };
};


namespace kconfig {
    class PicosatCNF {
        //! this map contains the the type of each Kconfig symbol
        std::map<std::string, kconfig_symbol_type> symboltypes;

        /**
        * \brief mapping between boolean variable names and their cnf-id
        *  Keep in sync with "booleanvars"
        */
        std::map<std::string, int> cnfvars;
        /** mapping between the names of boolean variables and symbols
            Some boolean variable represent model symbols. if so, the have
            to be stored in this map.
            Example:
            { "CONFIG_FOO"  -> "FOO", "CONFIG_FOO_MODULE" -> "FOO" }
        **/
        std::map<std::string, std::string> associatedSymbols;
        /** contains the variable name for cnf-id.
            Not all cnf-id will have a name. Must kept in sync with "symboltypes"
        **/
        std::map<int, std::string> boolvars;
        std::vector<int> clauses;
        std::vector<int> assumptions;
        std::map<std::string, std::deque<std::string>> meta_information;
        Picosat::SATMode defaultPhase;
        int varcount = 0;
        int clausecount = 0;
    public:
        PicosatCNF(Picosat::SATMode = Picosat::SAT_MIN);
        PicosatCNF(const PicosatCNF &, Picosat::SATMode);
        ~PicosatCNF();
        void readFromFile(const std::string &filename);
        void readFromStream(std::istream &i);
        void toFile(const std::string &filename) const;
        void toStream(std::ostream &out) const;
        kconfig_symbol_type getSymbolType(const std::string &name) const;
        void setSymbolType(const std::string &sym, kconfig_symbol_type type);
        int getCNFVar(const std::string &var) const;
        void setCNFVar(const std::string &var, int CNFVar);
        std::string &getSymbolName(int CNFVar);
        void pushVar(int v);
        void pushVar(std::string  &v, bool val);
        void pushClause(void);
        void pushClause(int *c);
        void pushAssumption(int v);
        void pushAssumption(const std::string &v,bool val);
        void pushAssumptions(std::map<std::string, bool> &a);
        bool checkSatisfiable(void);
        /** returns cnf-id of assumtions, that cause unresolvable conflicts.
            If checkSatisfiable returns false, this returns an array of assumptions
            that derived unsatisfiability (= failed assumptions).
            It does not contain all unsatisfiable assumptions.
            @returns array if of failed cnf-ids
        **/
        const int *failedAssumptions(void) const;
        bool deref(int s) const;
        bool deref(const std::string &s) const;
        bool deref(const char *s) const;
        int getVarCount(void) const { return varcount; }
        int getClauseCount(void) const { return clausecount; }
        const std::vector<int> &getClauses(void) const { return clauses; }
        int newVar(void);
        const std::string *getAssociatedSymbol(const std::string &var) const;
        const std::map<std::string, int> &getSymbolMap() const { return cnfvars; }
        const std::deque<std::string> *getMetaValue(const std::string &key) const;
        void addMetaValue(const std::string &key, const std::string &value);
    };
}
#endif
