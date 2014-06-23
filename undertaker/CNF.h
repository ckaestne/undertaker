// -*- mode: c++ -*-
/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#include "Kconfig.h"

#include <map>
#include <string>
#include <deque>


namespace kconfig {
    class CNF {
    public:
        virtual ~CNF(){};
        virtual void setMusAnalysis(bool mus_analysis)               = 0;
        virtual std::string getMusDirName()                    const = 0;
        virtual void readFromFile(const std::string &filename)       = 0;
        virtual void readFromStream(std::istream &i)                 = 0;
        virtual void toFile(const std::string &filename)       const = 0;
        virtual void toStream(std::ostream &out)               const = 0;
        virtual kconfig_symbol_type getSymbolType(const std::string &name)          const = 0;
        virtual void setSymbolType(const std::string &sym, kconfig_symbol_type type)      = 0;
        virtual int getCNFVar(const std::string &var)          const = 0;
        virtual void setCNFVar(const std::string &var, int CNFVar)   = 0;
        virtual std::string &getSymbolName(int CNFVar)               = 0;
        virtual void pushVar(int v)                                  = 0;
        virtual void pushVar(std::string &v, bool val)               = 0;
        virtual void pushClause(void)                                = 0;
        virtual void pushClause(int *c)                              = 0;
        virtual void pushAssumption(int v)                           = 0;
        virtual void pushAssumption(const std::string &v, bool val)  = 0;
        virtual void pushAssumption(const char *v, bool val)         = 0;
        virtual void pushAssumptions(std::map<std::string, bool> &a) = 0;
        virtual bool checkSatisfiable(void)                          = 0;
        virtual const int *failedAssumptions(void)             const = 0;
        virtual bool deref(int s)                              const = 0;
        virtual bool deref(const std::string &s)               const = 0;
        virtual bool deref(const char *c)                      const = 0;
        virtual int getVarCount(void)                          const = 0;
        virtual int newVar(void)                                     = 0;
        virtual const std::string *getAssociatedSymbol(const std::string &var)      const = 0;
        virtual const std::map<std::string, int>& getSymbolMap()                    const = 0;
        virtual const std::deque<std::string> *getMetaValue(const std::string &key) const = 0;
        virtual void addMetaValue(const std::string &key, const std::string &value)       = 0;
    };
}
#endif
