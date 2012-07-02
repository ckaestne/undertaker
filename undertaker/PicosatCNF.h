/*
 *   boolframwork - boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#include<map>
#include<vector>
#include<string>
#include<deque>
#include<stdio.h>
#include<stdlib.h>
#include "CNF.h"

namespace Picosat
{
    #include <ctype.h>
    #include <assert.h>

    using std::string;
    extern "C"
    {
        #include <picosat/picosat.h>
    }
};

namespace kconfig
{
    class PicosatCNF: public CNF
    {
        private:
            std::map<string, int> symboltypes;
            std::map<string, int> cnfvars;
            std::map<int, string> boolvars;
            std::vector<int> clauses;
            std::vector<int> assumptions;
            std::map<std::string, std::deque<std::string> > meta_information;
            int defaultPhase;
            int varcount;
            int clausecount;

            void loadContext(void);
            void resetContext(void);

        public:
            PicosatCNF(int defaultPhase = 0);
            virtual ~PicosatCNF();
            virtual void readFromFile(istream &i);
            virtual void toFile(std::ostream &out) const;
            virtual int getSymbolType(const string &name);
            virtual void setSymbolType(const string &sym, int type);
            virtual int getCNFVar(const string &var);
            virtual void setCNFVar(const string &var, int CNFVar);
            virtual string &getSymbolName(int CNFVar);
            virtual void pushVar(int v);
            virtual void pushVar(string  &v, bool val);
            virtual void pushClause(void);
            virtual void pushClause(int *c);
            virtual void pushAssumption(int v);
            virtual void pushAssumption(string &v,bool val);
            virtual void pushAssumption(const char *v,bool val);
            virtual bool checkSatisfiable(void);
            virtual void readAssumptionsFromFile(istream &i);
            virtual bool deref(int s);
            virtual bool deref(string &s);
            virtual std::map<string, int>::const_iterator getSymbolsItBegin();
            virtual std::map<string, int>::const_iterator getSymbolsItEnd();
            const std::deque<std::string> *getMetaValue(const std::string &key) const;
            void addMetaValue(const std::string &key, const std::string &value);
    };
}
#endif
