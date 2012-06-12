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

#include "PicosatCNF.h"
#include "IOException.h"

#include<stdlib.h>
#include<stdio.h>
#include<iostream>
#include<sstream>

#include "KconfigWhitelist.h"

using namespace kconfig;

static bool picosatIsInitalized = false;
static PicosatCNF *currentContext = 0;

PicosatCNF::PicosatCNF(int defaultPhase)
    : defaultPhase (defaultPhase), varcount (0), clausecount (0) {
    resetContext();
}

PicosatCNF::~PicosatCNF() {}

void PicosatCNF::loadContext(void) {
    vector<int>::iterator it;

    resetContext();
    currentContext = this;
    Picosat::picosat_set_global_default_phase(defaultPhase);

    for (it = clauses.begin(); it != clauses.end(); it++) {
        Picosat::picosat_add(*it);
    }
}

void PicosatCNF::resetContext(void) {
    if (picosatIsInitalized) {
        Picosat::picosat_reset();
    }

    Picosat::picosat_init();
    picosatIsInitalized = true;
    currentContext = 0;
}

void PicosatCNF::readFromFile(istream &) {
    //TODO
}

void PicosatCNF::toFile(ostream &out) {
    std::map<string, int>::iterator it, it1;
    std::vector<int>::iterator it2;

    out << "c File Format Version: 2.0" << std::endl;
    out << "c Generated by satyr" << std::endl;
    out << "c Type info:" << std::endl;
    out << "c c sym <symbolname> <typeid>" << std::endl;
    out << "c with <typeid> being an integer out of:"  << std::endl;
    out << "c enum {S_BOOLEAN=1, S_TRISTATE=2, S_INT=3, S_HEX=4, S_STRING=5, S_OTHER=5}"
        << std::endl;
    out << "c variable names:" << std::endl;
    out << "c c var <variablename> <cnfvar>" << std::endl;

    for (it = this->symboltypes.begin(); it != this->symboltypes.end(); it++) {
        const string &sym = it->first;

        int type = it->second;
        out << "c sym " << sym.c_str() << " " << type << endl;
    }

    for (it1 = this->cnfvars.begin(); it1 != this->cnfvars.end(); it1++) {
        const string &sym = it1->first;
        int var = it1->second;
        out << "c var " << sym.c_str() << " " << var << endl;
    }

    out << "p cnf " << varcount << " " << this->clausecount << std::endl;

    for (it2 = clauses.begin(); it2 != clauses.end(); it2++) {
        char sep = (*it2 == 0) ? '\n' : ' ';
        out <<  *it2 << sep;
    }
}

int PicosatCNF::getSymbolType(string &name)
{
    return this->symboltypes[name];
}

void PicosatCNF::setSymbolType(string &sym, int type)
{
    this->symboltypes[sym] = type;
}

int PicosatCNF::getCNFVar(string &var)
{
    //TODO: add new mapping for unknown variables
    //necessary since mapping is done by CNFBuilder, not by picosat itself
    return this->cnfvars[var];
}

void PicosatCNF::setCNFVar(string &var, int CNFVar)
{
    this->cnfvars[var] = CNFVar;
    this->boolvars[CNFVar] = var;
}

string &PicosatCNF::getSymbolName(int CNFVar)
{
    return this->boolvars[CNFVar];
}

void PicosatCNF::pushVar(int v)
{
    if (abs(v) > this->varcount) {
        this->varcount = abs(v);
    }

    if(v == 0) {
        this->clausecount++;
    }

    clauses.push_back(v);
}

void PicosatCNF::pushVar(string  &v, bool val)
{
    int sign = val ? 1 : -1;
    int cnfvar = this->getCNFVar(v);
    this->pushVar(sign * cnfvar);
}

void PicosatCNF::pushClause(void)
{
    this->clausecount++;

    clauses.push_back(0);
}

void PicosatCNF::pushClause(int *c)
{
    while (*c) {
        this->pushVar(*c);
        c++;
    }
    this->pushClause();
}

void PicosatCNF::pushAssumption(int v)
{
    assumptions.push_back(v);
}

void PicosatCNF::pushAssumption(string &v,bool val)
{
    int cnfvar = this->getCNFVar(v);
    if(val)
        this->pushAssumption(cnfvar);
    else
        this->pushAssumption(-cnfvar);
}

void PicosatCNF::pushAssumption(const char *v,bool val)
{
    std::string s(v);

    this->pushAssumption(s, val);
}

bool PicosatCNF::checkSatisfiable(void)
{
    vector<int>::iterator it;

    if (this != currentContext){
        this->loadContext();
    }

    for (it = assumptions.begin(); it != assumptions.end(); it++) {
        Picosat::picosat_assume(*it);
    }

    assumptions.clear();
    return Picosat::picosat_sat(-1) == PICOSAT_SATISFIABLE;
}

void PicosatCNF::readAssumptionsFromFile(istream &) {
    // FIXME
}

bool PicosatCNF::deref(int s)
{
    return Picosat::picosat_deref(s) == 1;
}

bool PicosatCNF::deref(string &s)
{
    int cnfvar = this->getCNFVar(s);
    return this->deref(cnfvar);
}

std::map<string, int>::const_iterator PicosatCNF::getSymbolsItBegin()
{
    return this->cnfvars.begin();
}

std::map<string, int>::const_iterator PicosatCNF::getSymbolsItEnd()
{
    return this->cnfvars.end();
}
