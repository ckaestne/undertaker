/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KCONFIG_CNF_H
#define KCONFIG_CNF_H

#include<map>
#include<string>
#include<iostream>
#include<deque>

#include "Kconfig.h"

using namespace std;
namespace kconfig
{
    class CNF
    {
        public:
            virtual ~CNF(){};
            virtual void readFromFile(istream &i) = 0;
            virtual void toFile(std::ostream &out) const = 0;
            virtual kconfig_symbol_type getSymbolType(const std::string &name) = 0;
            virtual void setSymbolType(const std::string &sym, kconfig_symbol_type type) = 0;
            virtual int getCNFVar(const string &var) = 0;
            virtual void setCNFVar(const string &var, int CNFVar) = 0;
            virtual string &getSymbolName(int CNFVar) = 0;
            virtual void pushVar(int v) = 0;
            virtual void pushVar(string  &v, bool val) = 0;
            virtual void pushClause(void) = 0;
            virtual void pushClause(int *c) = 0;
            virtual void pushAssumption(int v)= 0;
            virtual void pushAssumption(string &v,bool val)= 0;
            virtual void pushAssumption(const char *v,bool val)= 0;
            virtual bool checkSatisfiable(void)= 0;
            virtual bool deref(int s) = 0;
            virtual bool deref(string &s) = 0;
            virtual int getVarCount(void) = 0;
            virtual int newVar(void) = 0;
            virtual std::map<string, int>::const_iterator getSymbolsItBegin() = 0;
            virtual std::map<string, int>::const_iterator getSymbolsItEnd() = 0;
            virtual const std::deque<std::string> *getMetaValue(const std::string &key) const = 0;
            virtual void addMetaValue(const std::string &key, const std::string &value) = 0;

    };
}
#endif
