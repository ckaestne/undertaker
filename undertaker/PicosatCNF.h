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

#include "CNF.h"

#include <vector>

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
    class PicosatCNF: public CNF {
    private:
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
        bool do_mus_analysis = false;;
        std::string musTmpDirName;

        void loadContext(void);
        void resetContext(void);

    public:
        PicosatCNF(Picosat::SATMode = Picosat::SAT_MIN);
        virtual ~PicosatCNF();
        void setDefaultPhase(Picosat::SATMode phase);
        virtual void setMusAnalysis(bool mus_analysis)                 final override {
            this->do_mus_analysis = mus_analysis;
        }
        virtual std::string getMusDirName()                      const final override {
            return musTmpDirName;
        }
        virtual void readFromFile(const std::string &filename)         final override;
        virtual void readFromStream(std::istream &i)                   final override;
        virtual void toFile(const std::string &filename)         const final override;
        virtual void toStream(std::ostream &out)                 const final override;
        virtual kconfig_symbol_type getSymbolType(const std::string &name)     const final override;
        virtual void setSymbolType(const std::string &sym, kconfig_symbol_type type) final override;
        virtual int getCNFVar(const std::string &var)            const final override;
        virtual void setCNFVar(const std::string &var, int CNFVar)     final override;
        virtual std::string &getSymbolName(int CNFVar)                 final override;
        virtual void pushVar(int v)                                    final override;
        virtual void pushVar(std::string  &v, bool val)                final override;
        virtual void pushClause(void)                                  final override;
        virtual void pushClause(int *c)                                final override;
        virtual void pushAssumption(int v)                             final override;
        virtual void pushAssumption(const std::string &v,bool val)     final override;
        virtual void pushAssumption(const char *v,bool val)            final override;
        virtual void pushAssumptions(std::map<std::string, bool> &a)   final override;
        virtual bool checkSatisfiable(void)                            final override;
        /** returns cnf-id of assumtions, that cause unresolvable conflicts.
            If checkSatisfiable returns false, this returns an array of assumptions
            that derived unsatisfiability (= failed assumptions).
            It does not contain all unsatisfiable assumptions.
            @returns array if of failed cnf-ids
        **/
        virtual const int *failedAssumptions(void)               const final override;
        virtual bool deref(int s)                                const final override;
        virtual bool deref(const std::string &s)                 const final override;
        virtual bool deref(const char *s)                        const final override;
        virtual int getVarCount(void)                            const final override;
        virtual int newVar(void)                                       final override;
        virtual const std::string *getAssociatedSymbol(const std::string &var) const final override;

        virtual const std::map<std::string, int>& getSymbolMap() const final override {
            return this->cnfvars;
        }
        virtual const std::deque<std::string> *getMetaValue(const std::string &key)
                                                                 const final override;
        virtual void addMetaValue(const std::string &key, const std::string &value)
                                                                       final override;
    };
}
#endif
