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

#include "PicosatCNF.h"
#include "exceptions/IOException.h"
#include "Logging.h"

#include <fstream>
#include <algorithm>

namespace Picosat {
// include picosat header as C
    extern "C" {
        #include "picosat.h"
    }
}

using namespace kconfig;


static bool picosatIsInitalized = false;
static PicosatCNF *currentContext = nullptr;

PicosatCNF::PicosatCNF(Picosat::SATMode defaultPhase) : defaultPhase (defaultPhase) {
    resetContext();
}

PicosatCNF::~PicosatCNF() {
    if (this == currentContext) {
        currentContext = nullptr;
    }
}

void PicosatCNF::setDefaultPhase(Picosat::SATMode phase) {
    this->defaultPhase = phase;
    //force a reload of the context to avoid random mixed phases
    if (this == currentContext) {
        currentContext = nullptr;
    }
}

void PicosatCNF::loadContext(void) {
    resetContext();
    currentContext = this;
    Picosat::picosat_set_global_default_phase(defaultPhase);

    for (const int &clause : clauses)
        Picosat::picosat_add(clause);

    if (do_mus_analysis) {
        // create a unique temporary directory
        std::string pref("/tmp/undertakermusXXXXXX");
        char *t =  mkdtemp(&pref[0]);
        if(!t) {
            logger << error << "Couldn't create tmpdir" << std::endl;
            exit(1);
        }
        musTmpDirName = t;
        std::string tmpf = t;
        tmpf += "/infile";

        std::ofstream out(tmpf);
        out << "p cnf " << varcount << " " << this->clausecount << std::endl;

        for (int &clause : clauses) {
            char sep = (clause == 0) ? '\n' : ' ';
            out << clause << sep;
        }
        out.close();
    }
}

void PicosatCNF::resetContext(void) {
    if (picosatIsInitalized) {
        Picosat::picosat_reset();
    }
    Picosat::picosat_init();
    picosatIsInitalized = true;
    currentContext = nullptr;
}

void PicosatCNF::readFromFile(const std::string &filename) {
    std::ifstream i(filename);
    if (!i.good()) {
        throw IOException("Could not open CNF-File");
    }
    readFromStream(i);
}

void PicosatCNF::readFromStream(std::istream &i) {
    std::string tmp;
    while (i >> tmp) {
        if (tmp == "c") {
            i >> tmp;
            if (tmp == "var") {
                // beginning of this line matches: ("^c var (.+) (\\d+)$")
                std::string varname;
                int cnfnumber;
                i >> varname; i >> cnfnumber;
                this->setCNFVar(varname, cnfnumber);
            } else if (tmp == "sym") {
                // beginning of this line matches: ("^c sym (.+) (\\d)$")
                std::string varname;
                int typeId;
                i >> varname; i >> typeId;
                this->setSymbolType(varname, (kconfig_symbol_type) typeId);
            } else if (tmp == "meta_value") {
                // beginning of this line matches: ("^c meta_value ([^\\s]+)\\s+(.+)$")
                std::string metavalue, item;
                i >> metavalue;
                while (i.peek() != '\n' && i >> item)
                    this->addMetaValue(metavalue, item);
            } else {
                // beginning of this line matches: ("^c ") and will be ignored
                std::getline(i, tmp);
            }
        } else if (tmp == "p") {
            // beginning of this line matches: ("^p cnf (\\d+) (\\d+)$")
            // Which is the CNF dimension descriptor line.
            // After this, only lines in DIMACs CNF Format remain
            i >> tmp;
            if (tmp != "cnf") {
                logger << error << "Invalid DIMACs CNF dimension descriptor." << std::endl;
                throw IOException("parse error while reading CNF file");
            }
            i >> varcount; i >> clausecount;
            // minimize reallocation of the clauses vector, each clause has 3-4 variables
            clauses.reserve(4*clausecount);
            int val;
            while(i >> val)
                clauses.push_back(val);
        } else {
            logger << error << "Line not starting with c or p." << std::endl;
            throw IOException("parse error while reading CNF file");
        }
    }
}

void PicosatCNF::toFile(const std::string &filename) const {
    std::ofstream out(filename);
    if (!out.good()) {
        logger << error << "Couldn't write to " << filename << std::endl;
        return;
    }
    toStream(out);
}

// XXX do not modify the output format without adjusting: readFromStream
void PicosatCNF::toStream(std::ostream &out) const {
    out << "c File Format Version: 2.0" << std::endl;
    out << "c Generated by satyr" << std::endl;
    out << "c Type info:" << std::endl;
    out << "c c sym <symbolname> <typeid>" << std::endl;
    out << "c with <typeid> being an integer out of:"  << std::endl;
    out << "c enum {S_BOOLEAN=1, S_TRISTATE=2, S_INT=3, S_HEX=4, S_STRING=5, S_OTHER=6}"
        << std::endl;
    out << "c variable names:" << std::endl;
    out << "c c var <variablename> <cnfvar>" << std::endl;

    for (const auto &entry : meta_information) {  // pair<string, deque<string>>
        std::stringstream sj;

        sj << "c meta_value " << entry.first;
        for (const std::string &str : entry.second)
            sj << " " << str;

        out << sj.str() << std::endl;
    }
    for (const auto &entry : this->symboltypes) {  // pair<string, kconfig_symbol_type>
        const std::string &sym = entry.first;
        int type = entry.second;
        out << "c sym " << sym << " " << type << std::endl;
    }
    for (const auto &entry : this->cnfvars) {  // pair<string, int>
        const std::string &sym = entry.first;
        int var = entry.second;
        out << "c var " << sym << " " << var << std::endl;
    }
    out << "p cnf " << varcount << " " << this->clausecount << std::endl;

    for (const int &clause : clauses) {
        char sep = (clause == 0) ? '\n' : ' ';
        out <<  clause << sep;
    }
}

kconfig_symbol_type PicosatCNF::getSymbolType(const std::string &name) const {
    const auto &it = this->symboltypes.find(name); // pair<string, kconfig_symbol_type>
    return (it == this->symboltypes.end()) ? K_S_UNKNOWN : it->second;
}

void PicosatCNF::setSymbolType(const std::string &sym, kconfig_symbol_type type) {
    std::string config_sym = "CONFIG_" + sym;
    this->associatedSymbols[config_sym] = sym;

    if (type == 2) {
        std::string config_sym_mod = "CONFIG_" + sym + "_MODULE";
        this->associatedSymbols[config_sym_mod] = sym;
    }
    this->symboltypes[sym] = type;
}

int PicosatCNF::getCNFVar(const std::string &var) const {
    const auto &it = this->cnfvars.find(var); // pair<string, int>
    return (it == this->cnfvars.end()) ? 0 : it->second;
}

void PicosatCNF::setCNFVar(const std::string &var, int CNFVar) {
    if (abs(CNFVar) > this->varcount) {
        this->varcount = abs(CNFVar);
    }
    this->cnfvars[var] = CNFVar;
    this->boolvars[CNFVar] = var;
}

std::string &PicosatCNF::getSymbolName(int CNFVar) {
    return this->boolvars[CNFVar];
}

void PicosatCNF::pushVar(int v) {
    if (abs(v) > this->varcount) {
        this->varcount = abs(v);
    }
    if (v == 0) {
        this->clausecount++;
    }
    clauses.push_back(v);
}

void PicosatCNF::pushVar(std::string &v, bool val) {
    int sign = val ? 1 : -1;
    int cnfvar = this->getCNFVar(v);
    this->pushVar(sign * cnfvar);
}

void PicosatCNF::pushClause(void) {
    this->clausecount++;
    clauses.push_back(0);
}

void PicosatCNF::pushClause(int *c) {
    while (*c) {
        this->pushVar(*c);
        c++;
    }
    this->pushClause();
}

void PicosatCNF::pushAssumption(int v) {
    assumptions.push_back(v);
}

void PicosatCNF::pushAssumption(const std::string &v, bool val) {
    int cnfvar = this->getCNFVar(v);

    if (cnfvar == 0) {
        logger << error << "Picosat: ignoring variable " << v
               << "as it has not been registered yet!" << std::endl;
        return;
    }
    if (val)
        this->pushAssumption(cnfvar);
    else
        this->pushAssumption(-cnfvar);
}

void PicosatCNF::pushAssumption(const char *v, bool val) {
    std::string s(v);
    this->pushAssumption(s, val);
}

bool PicosatCNF::checkSatisfiable(void) {
    if (this != currentContext){
        this->loadContext();
    }
    for (const int &assumption : assumptions)
        Picosat::picosat_assume(assumption);

    assumptions.clear();
    return Picosat::picosat_sat(-1) == PICOSAT_SATISFIABLE;
}

void PicosatCNF::pushAssumptions(std::map<std::string, bool> &a) {
    for (const auto &entry : a) {  // pair<string, bool>
        std::string symbol = entry.first;
        bool value = entry.second;
        this->pushAssumption(symbol, value);
    }
}

bool PicosatCNF::deref(int s) const {
    return Picosat::picosat_deref(s) == 1;
}

bool PicosatCNF::deref(const std::string &s) const {
    int cnfvar = this->getCNFVar(s);
    return this->deref(cnfvar);
}

bool PicosatCNF::deref(const char *c) const {
    int cnfvar = this->getCNFVar(c);
    return this->deref(cnfvar);
}

const std::string *PicosatCNF::getAssociatedSymbol(const std::string &var) const {
    const auto &it = this->associatedSymbols.find(var); // pair<string, string>
    return (it == this->associatedSymbols.end()) ? nullptr : &(it->second);
}

const int *PicosatCNF::failedAssumptions(void) const {
    return Picosat::picosat_failed_assumptions();
}

void PicosatCNF::addMetaValue(const std::string &key, const std::string &value) {
    std::deque<std::string> &values = meta_information[key];
    if (std::find(values.begin(), values.end(), value) == values.end())
        // value wasn't found within values, add it
        values.push_back(value);
}

const std::deque<std::string> *PicosatCNF::getMetaValue(const std::string &key) const {
    const auto &i = meta_information.find(key); // pair<string, deque<string>>
    if (i == meta_information.end()) // key not found
        return nullptr;
    return &((*i).second);
}

int PicosatCNF::getVarCount(void) const {
    return varcount;
}

int PicosatCNF::newVar(void) {
    varcount++;
    return varcount;
}
