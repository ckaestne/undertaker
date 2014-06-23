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
#include "IOException.h"
#include "Logging.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

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

    for (int &clause : clauses)
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

        std::ofstream out(tmpf.c_str());
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

void PicosatCNF::readFromFile(const char *filename) {
    std::ifstream i(filename);
    if (!i.good()) {
        throw IOException("Could not open CNF-File");
    }
    readFromStream(i);
}

void PicosatCNF::readFromStream(std::istream &i) {
    std::string line;
    while(std::getline (i, line)) {
        static const boost::regex var_regexp("^c var (.+) (\\d+)$");
        static const boost::regex sym_regexp("^c sym (.+) (\\d)$");
        static const boost::regex dim_regexp("^p cnf (\\d+) (\\d+)$");
        static const boost::regex meta_regexp("^c meta_value ([^\\s]+)\\s+(.+)$");
        static const boost::regex comment_regexp("^c ");
        boost::match_results<std::string::const_iterator> what;

        if (boost::regex_match(line, what, var_regexp)) {
            std::string varname = what[1];
            int cnfnumber = boost::lexical_cast<int>(what[2]);
            this->setCNFVar(varname, cnfnumber);
        } else if (boost::regex_match(line, what, sym_regexp)) {
            std::string varname = what[1];
            int typeId = boost::lexical_cast<int>(what[2]);
            this->setSymbolType(varname, (kconfig_symbol_type) typeId);
        } else if (boost::regex_match(line, what, meta_regexp)) {
            std::string metavalue = what[1];
            std::stringstream content(what[2]);
            std::string item;

            while (content >> item)
                this->addMetaValue(metavalue, item);
        } else if (boost::regex_search(line, comment_regexp)) {
            //ignore
        } else if (boost::regex_match(line, what, dim_regexp)) {
            // handle dimension descriptor line: after this, only lines in DIMACs CNF Format remain
            int val;
            while(i >> val) {
                switch(val) {
                case 0:
                    this->pushClause(); break;
                default:
                    this->pushVar(val);
                }
            }
        } else {
            logger << error << "Failed to parse line: '" << line << "'" << std::endl;
            throw IOException("parse error while reading CNF file");
        }
    }
}

void PicosatCNF::toFile(const char *filename) const {
    std::ofstream out(filename);
    if (!out.good()) {
        logger << error << "Couldn't write to " << filename << std::endl;
        return;
    }
    toStream(out);
}

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

    for (auto &entry : meta_information) {  // pair<string, deque<string>>
        std::stringstream sj;

        sj << "c meta_value " << entry.first;
        for (const std::string &str : entry.second)
            sj << " " << str;

        out << sj.str() << std::endl;
    }
    for (auto &entry : this->symboltypes) {  // pair<string, kconfig_symbol_type>
        const std::string &sym = entry.first;
        int type = entry.second;
        out << "c sym " << sym.c_str() << " " << type << std::endl;
    }
    for (auto &entry : this->cnfvars) {  // pair<string, int>
        const std::string &sym = entry.first;
        int var = entry.second;
        out << "c var " << sym.c_str() << " " << var << std::endl;
    }
    out << "p cnf " << varcount << " " << this->clausecount << std::endl;

    for (const int &clause : clauses) {
        char sep = (clause == 0) ? '\n' : ' ';
        out <<  clause << sep;
    }
}

kconfig_symbol_type PicosatCNF::getSymbolType(const std::string &name) {
    std::map<std::string, kconfig_symbol_type>::const_iterator it = this->symboltypes.find(name);
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

int PicosatCNF::getCNFVar(const std::string &var) {
    std::map<std::string, int>::const_iterator it = this->cnfvars.find(var);
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
    for (int &assumption : assumptions)
        Picosat::picosat_assume(assumption);

    assumptions.clear();
    return Picosat::picosat_sat(-1) == PICOSAT_SATISFIABLE;
}

void PicosatCNF::pushAssumptions(std::map<std::string, bool> &a) {
    for (auto &entry : a) {  // pair<string, bool>
        std::string symbol = entry.first;
        bool value = entry.second;
        this->pushAssumption(symbol, value);
    }
}

bool PicosatCNF::deref(int s) {
    return Picosat::picosat_deref(s) == 1;
}

bool PicosatCNF::deref(const std::string &s) {
    int cnfvar = this->getCNFVar(s);
    return this->deref(cnfvar);
}

bool PicosatCNF::deref(const char *c) {
    std::string s(c);
    int cnfvar = this->getCNFVar(s);
    return this->deref(cnfvar);
}

const std::string *PicosatCNF::getAssociatedSymbol(const std::string &var) const {
    std::map<std::string, std::string>::const_iterator it = this->associatedSymbols.find(var);
    return (it == this->associatedSymbols.end()) ? nullptr : &(it->second);
}

const int *PicosatCNF::failedAssumptions(void) const {
    return Picosat::picosat_failed_assumptions();
}

void PicosatCNF::addMetaValue(const std::string &key, const std::string &value) {
    std::map<std::string, std::deque<std::string> >::const_iterator i = this->meta_information.find(key);
    std::deque<std::string> values;
    std::deque<std::string>::const_iterator j;

    if (i != meta_information.end()) {
        values = (*i).second;
        meta_information.erase(key);
    }
    j = std::find(values.begin(), values.end(), value);

    if (j == values.end()) {
        values.push_back(value);
    }
    meta_information.emplace(key, values);
}

const std::deque<std::string> *PicosatCNF::getMetaValue(const std::string &key) const {
    std::map<std::string, std::deque<std::string>>::const_iterator i = meta_information.find(key);
    if (i == meta_information.end()) // not found
        return nullptr;
    return &((*i).second);
}

int PicosatCNF::getVarCount(void) {
    return varcount;
}

int PicosatCNF::newVar(void) {
    varcount++;
    return varcount;
}
