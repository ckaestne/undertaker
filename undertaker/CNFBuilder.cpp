/*
 * boolean framework for undertaker and satyr
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

#include "ConditionalBlock.h"
#include "CNFBuilder.h"

using namespace kconfig;

CNFBuilder::CNFBuilder(bool useKconfigWhitelist, enum ConstantPolicy constPolicy)
    : boolvar(0), wl(useKconfigWhitelist ? KconfigWhitelist::getIgnorelist() : 0),
    constPolicy(constPolicy) {}

void CNFBuilder::pushClause(BoolExp *e) {
    visited.clear();
    BoolExpConst *constant = dynamic_cast<BoolExpConst*>(e);

    if (constant) {
        bool v = constant->value;
        if (v) { // ... && 1 can be filtered out
            return;
        }
    }
    // If we push a variable directly to the stack of clauses, we can
    // assume that it is always set. Therefore, also add it to the
    // ALWAYS_ON meta variable, to preserve this observation so that
    // it can be exploited in a later stage.
    BoolExpVar *variable = dynamic_cast<BoolExpVar*>(e);
    if (variable) {
        const std::string always_on("ALWAYS_ON");
        cnf->addMetaValue(always_on, variable->str());
    }
    e->accept(this);
    cnf->pushVar(e->CNFVar);
    cnf->pushClause();
}

int CNFBuilder::addVar(std::string symname) {
    std::map<std::string, int>::iterator it;
    symname = ConditionalBlock::normalize_filename(symname.c_str());
    int cv = cnf->getCNFVar(symname);

    if (cv == 0) {
        // Var not known yet: add new var
        int newVar = this->cnf->newVar();

        // add var to mapping
        cnf->setCNFVar(symname, newVar);
        return newVar;
    }
    // var known:
    return cv;
}

void CNFBuilder::visit(BoolExp *) {
    throw "CNF ERROR";
}

void CNFBuilder::visit(BoolExpAnd *e) {
    if (e->CNFVar)
        return;

    e->CNFVar = this->cnf->newVar();

    // add clauses
    int h = e->CNFVar;
    int a = e->left->CNFVar;
    int b = e->right->CNFVar;
    // H <-> (A && B)
    // (!H || A) && ( !H || B) && ( H || !A || !B)
    cnf->pushVar(-h);
    cnf->pushVar(a);
    cnf->pushClause();

    cnf->pushVar(-h);
    cnf->pushVar(b);
    cnf->pushClause();

    cnf->pushVar(h);
    cnf->pushVar(-a);
    cnf->pushVar(-b);
    cnf->pushClause();
}

void CNFBuilder::visit(BoolExpOr *e) {
    if (e->CNFVar)
        return;

    e->CNFVar = this->cnf->newVar();

    // add clauses
    int h = e->CNFVar;
    int a = e->left->CNFVar;
    int b = e->right->CNFVar;

    // H <-> (A || B)
    // (H || !A) && ( H || !B) && ( !H || A || B)
    cnf->pushVar(h);
    cnf->pushVar(-a);
    cnf->pushClause();

    cnf->pushVar(h);
    cnf->pushVar(-b);
    cnf->pushClause();

    cnf->pushVar(-h);
    cnf->pushVar(a);
    cnf->pushVar(b);
    cnf->pushClause();
}

void CNFBuilder::visit(BoolExpImpl *e) {
    if (e->CNFVar)
        return;

    e->CNFVar = this->cnf->newVar();

    // add clauses
    int h = e->CNFVar;
    int a = e->left->CNFVar;
    int b = e->right->CNFVar;
    // H <-> (A -> B)
    // (H ||  A) && (H || !B ) && (!H || !A || B)
    cnf->pushVar(h);
    cnf->pushVar(a);
    cnf->pushClause();

    cnf->pushVar(h);
    cnf->pushVar(-b);
    cnf->pushClause();

    cnf->pushVar(-h);
    cnf->pushVar(-a);
    cnf->pushVar(b);
    cnf->pushClause();
}

void CNFBuilder::visit(BoolExpEq *e) {
    if (e->CNFVar)
        return;

    e->CNFVar = this->cnf->newVar();

    // add clauses
    int h = e->CNFVar;
    int a = e->left->CNFVar;
    int b = e->right->CNFVar;
    // H <-> (A <-> B)
    cnf->pushVar(h);
    cnf->pushVar(-a);
    cnf->pushVar(-b);
    cnf->pushClause();

    cnf->pushVar(h);
    cnf->pushVar(a);
    cnf->pushVar(b);
    cnf->pushClause();

    cnf->pushVar(-h);
    cnf->pushVar(a);
    cnf->pushVar(-b);
    cnf->pushClause();

    cnf->pushVar(-h);
    cnf->pushVar(-a);
    cnf->pushVar(b);
    cnf->pushClause();
}

void CNFBuilder::visit(BoolExpAny *e) {
    // add free variable for arith. Operators (ie, handle them later..)
    e->CNFVar = this->cnf->newVar();
}

void CNFBuilder::visit(BoolExpCall *e) {
    // add free variable for function calls (i.e., handle them later..)
    e->CNFVar = this->cnf->newVar();
}

void CNFBuilder::visit(BoolExpNot *e) {
    if (e->CNFVar)
        return;

    e->CNFVar = -(e->right->CNFVar);
}

void CNFBuilder::visit(BoolExpConst *e) {
    if (e->CNFVar)
        return;

    if (constPolicy == CNFBuilder::FREE){
        //handle consts as free var
        e->CNFVar = this->cnf->newVar();
        return;
    }
    if (!this->boolvar) {
        this->boolvar = this->cnf->newVar();
        cnf->pushVar(boolvar);
        cnf->pushClause();
    }
    e->CNFVar = e->value ? boolvar : -boolvar;
}

void CNFBuilder::visit(BoolExpVar *e) {
    std::string symname = e->str();
    std::map<std::string, int>::iterator it;

    if (e->CNFVar) {
        return;
    }
    if (this->wl && wl->isWhitelisted(symname.c_str())) {
        // use free variables for symbols in wl
        e->CNFVar = this->cnf->newVar();
    } else {
        e->CNFVar = this->addVar(symname);
    }
}
