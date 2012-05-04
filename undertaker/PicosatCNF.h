/*
 *   boolframwork - boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
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
#include<string>
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

using namespace std;
namespace kconfig
{
    class PicosatCNF: public CNF
    {
        private:
            map<string, int> symboltypes;
            map<string, int> cnfvars;
            map<int, string> boolvars;
            int varcount;
            int clausecount;

        public:
            PicosatCNF(int defaultPhase = 0);
            ~PicosatCNF();
            virtual void readFromFile(istream &i);
            virtual void toFile(string &path);
            virtual int getSymbolType(string &name);
            virtual void setSymbolType(string &sym, int type);
            virtual int getCNFVar(string &var);
            virtual void setCNFVar(string &var, int CNFVar);
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

    };
}
#endif
