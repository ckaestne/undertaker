/*
 * satyr - compiles KConfig files to boolean formulas
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

#include "Logging.h"
#include "SymbolTranslator.h"
#include "SymbolTools.h"

#include "PicosatCNF.h"
#include "KconfigAssumptionMap.h"
#include "ExpressionTranslator.h"

#include <iostream>
#include <stack>
#include <iomanip>
#include <string.h>
#include <fstream>

kconfig::SymbolTranslator::SymbolTranslator(CNF *cnf) : symbolSet(0) {
    this-> cnfbuilder.cnf = cnf;
}


void kconfig::SymbolTranslator::visit_bool_symbol(struct symbol *sym) {
    if (!sym)
        return;

    if (sym->name)
        logger << debug << "CONFIG " << sym->name << " (bool)" << std::endl;
    else
        logger << debug << "CONFIG <unnamed> (?)" << std::endl;

    ExpressionTranslator expTranslator;
    expTranslator.symbolSet = this->symbolSet;

    expr *rev = reverseDepExpression(sym);
    expr *vis = visibilityExpression(sym);
    expr *dep = dependsExpression(sym);
    expr *def = defaultExpression_bool_tristate(sym);

    BoolExp &f1yes = *B_VAR(sym, rel_yes);

    TristateRepr transVis = expTranslator.process(vis);
    TristateRepr transDep = expTranslator.process(dep);
    TristateRepr transDef = expTranslator.process(def);
    TristateRepr transRev = expTranslator.process(rev);

    //invisible  symbols
    // F1.yes->(rev.yes || rev.mod || def.yes ||def.mod)
    BoolExp &FDef =  !f1yes || *transDef.yes || *transDef.mod || *transRev.yes  || *transRev.mod;
    // ((def.yes||def.mod) && (dep.yes ||dep.mod)) -> f1
    BoolExp &FDefRev = !((*transDef.yes || *transDef.mod ) && (*transDep.yes || *transDep.mod)) || f1yes;
    //!vis -> (FDef && FDefRev)
    BoolExp &completeInv = *transVis.yes || *transVis.mod || (FDef && FDefRev);

    //visible symbols:
    // F1.yes-> dep.yes || dep.mod || rev.yes || rev.mod
    BoolExp &FYes = !f1yes || *transDep.yes || *transDep.mod || *transRev.yes  || *transRev.mod;
    // rev.yes -> F1.yes
    BoolExp &FRevYes = !*transRev.yes || f1yes;
    // rev.mod -> F1.yes
    BoolExp &FRevMod = !*transRev.mod || f1yes;
    this->cnfbuilder.pushSymbolInfo(sym);
    this->addClause(FYes.simplify());
    this->addClause(FRevYes.simplify());
    this->addClause(FRevMod.simplify());
    this->addClause(completeInv.simplify());

    expr_free(def);
    expr_free(dep);
    expr_free(vis);
    expr_free(rev);
};

void kconfig::SymbolTranslator::visit_tristate_symbol(struct symbol *sym) {
    ExpressionTranslator expTranslator;
    expTranslator.symbolSet = this->symbolSet;
    expr *rev = reverseDepExpression(sym);
    expr *vis = visibilityExpression(sym);
    expr *dep = dependsExpression(sym);
    expr *def = defaultExpression_bool_tristate(sym);

    logger <<  debug << "CONFIG " << sym->name << " (tristate)" << std::endl;
    BoolExp &f1yes = *B_VAR(sym, rel_yes);
    BoolExp &f1mod = *B_VAR(sym, rel_mod);

    TristateRepr transVis = expTranslator.process(vis);
    TristateRepr transDep = expTranslator.process(dep);
    TristateRepr transDef = expTranslator.process(def);
    TristateRepr transRev = expTranslator.process(rev);

    //invisible  symbols
    //f1->(s1|r1)
    BoolExp &Inv0 = !f1yes||(*transDef.yes||*transRev.yes);
    //f0 -> (s1|s0|r0)
    BoolExp &Inv1 = !f1mod || (*transDef.yes||*transDef.mod||*transRev.mod);
    //(s0|s1) -> (~(d1|d0) |f1 |f0)
    BoolExp &Inv2 = !(*transDef.mod||*transDef.yes) || (!(*transDep.yes||*transDep.mod) ||f1yes ||f1mod);
    //(s1 & d1)-> f1
    BoolExp &Inv3 = !(*transDef.yes && *transDep.yes)||f1yes;

    BoolExp &completeInv = (*transVis.yes||*transVis.mod)|| (Inv0 && Inv1 && Inv2 &&Inv3);

    //visible symbols:
    // F1.yes-> dep.yes ||rev.yes
    BoolExp &FYes = *B_IMPL(&f1yes ,&( *transDep.yes || *transRev.yes));
    // F1.mod-> rev.mod || dep.yes|| dep.mod ;
    BoolExp &FMod = !f1mod || *transRev.mod || *transDep.yes || *transDep.mod;
    // rev.yes -> F1.yes
    BoolExp &FRevYes = !*transRev.yes || f1yes;
    // rev.mod -> F1.yes || F1.mod
    BoolExp &FRevMod = !*transRev.mod || f1yes || f1mod;

    //mustn't become '11'
    BoolExp &guard = sym->type == S_BOOLEAN ? (!f1mod ) : (!(f1yes && f1mod));

    this->cnfbuilder.pushSymbolInfo(sym);
    //this->addComment(sym->name ? sym->name : "Unnamed Menu Or Choice");
    this->addClause(FYes.simplify());

    this->addClause(FMod.simplify());
    this->addClause(FRevYes.simplify());
    this->addClause(FRevMod.simplify());
    this->addClause(completeInv.simplify());
    this->addClause(&guard);

    expr_free(def);
    expr_free(dep);
    expr_free(vis);
    expr_free(rev);
}

/* the presence of a string/int/hex-symbol is only affeced by its
    (reverse) dependencies. We assume, that all symbols are set to their
    default and bool (anystring) <-> y.
    ie:
    F1 <-> rev.yes | rev.mod | dep.yes |dep.mod
*/

void kconfig::SymbolTranslator::visit_int_symbol(struct symbol *sym) {
    this->visit_string_symbol(sym);
}

void kconfig::SymbolTranslator::visit_hex_symbol(struct symbol *sym) {
    this->visit_string_symbol(sym);
}

void kconfig::SymbolTranslator::visit_string_symbol(struct symbol *sym) {
    logger << debug << "CONFIG " << sym->name << " (string-like)" << std::endl;
    ExpressionTranslator expTranslator;
    expTranslator.symbolSet = this->symbolSet;

    expr *rev = reverseDepExpression(sym);
    expr *dep = dependsExpression(sym);

    //BoolExp &f1yes = *B_VAR(sym, rel_yes);

    //TristateRepr transDep = expTranslator.process(dep);
    //TristateRepr transRev = expTranslator.process(rev);


    // (rev.yes || rev.mod || dep.yes ||dep.mod) -> f1
    //BoolExp &F2 = !(*transDep.yes || *transDep.mod || *transRev.yes  || *transRev.mod) || f1yes;
    this->cnfbuilder.pushSymbolInfo(sym);

    expr_free(dep);
    expr_free(rev);
}

void kconfig::SymbolTranslator::visit_choice_symbol(struct symbol *sym) {
    ExpressionTranslator expTranslator;
    expTranslator.symbolSet = this->symbolSet;
    TristateRepr transChoice = expTranslator.process(choiceExpression(sym));
    BoolExp &f1yes = *B_VAR(sym, rel_yes);

    if (sym->type == S_BOOLEAN) {
        logger << debug << "CONFIG " << sym->name << " (choice bool)" <<std::endl;
        visit_bool_symbol(sym);
        // F1.yes-> choice.yes
        BoolExp &CYes = !f1yes || *transChoice.yes || *transChoice.mod;

        this->addClause(CYes.simplify());
    } else if (sym->type == S_TRISTATE) {
        logger << debug << "CONFIG " << sym->name << " (choice tri)" <<std::endl;
        visit_tristate_symbol(sym);
        // F1.yes-> choice.yes
        BoolExp &CYes = !f1yes || *transChoice.yes;
        // F1.yes-> !choice.mod ;
        BoolExp &CMod = !f1yes || !*transChoice.mod;

        this->addClause(CYes.simplify());
        this->addClause(CMod.simplify());
        visit_tristate_symbol(sym);
    } else {
        throw "Invalid choice type";
    }
}

void kconfig::SymbolTranslator::visit_symbol(struct symbol *) {
    // various meta symbols: ignore
}

void kconfig::SymbolTranslator::addClause(BoolExp *clause) {
    BoolExpConst *cs = dynamic_cast<BoolExpConst *>(clause);

    if (cs && cs->value) {
        return;
    }
    this->cnfbuilder.pushClause(clause);
}
