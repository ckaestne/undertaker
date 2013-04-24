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

#include "ExpressionTranslator.h"
#include "UnsupportedFeatureException.h"

#include <sstream>
#include <iostream>
#include <stdio.h>

#include <string.h>
using namespace kconfig;

ExpressionTranslator::ExpressionTranslator() : symbolSet(0) {}

TristateRepr ExpressionTranslator::visit_symbol(struct symbol *sym) {
    if (sym == &symbol_no) {
        struct TristateRepr res = { B_CONST(false), B_CONST(false), false, S_TRISTATE };
        return res;
    } else if (sym == &symbol_yes) {
        struct TristateRepr res = { B_CONST(true), B_CONST(false), false, S_TRISTATE };
        return res;
    } else if (sym == &symbol_mod) {
        struct TristateRepr res = { B_CONST(false), B_CONST(true), false, S_TRISTATE };
        return res;
    } else if (sym->type == S_STRING || sym->type == S_HEX || sym->type == S_INT) {
        struct TristateRepr res = { B_CONST(false), B_CONST(false), true, sym->type };
        return res;
    } else if (sym == modules_sym) {
        struct TristateRepr res = { B_VAR(sym, rel_yes), B_CONST(false), true, S_BOOLEAN };
        return res;
    } else if (this->symbolSet && this->symbolSet->find(sym) ==  this->symbolSet->end()) {
        struct TristateRepr res = { B_CONST(false), B_CONST(false), false, S_BOOLEAN };
        return res;
    } else {
        struct TristateRepr res;
        res.yes = B_VAR(sym, rel_yes);
        if (sym->type == S_TRISTATE) {
            res.mod = B_VAR(sym, rel_mod);
        } else {
            res.mod = B_CONST(false);
        }
        res.invalid = !(sym->type == S_BOOLEAN || sym->type == S_TRISTATE);
        res.type = sym->type;
        return res;
    }
}

TristateRepr ExpressionTranslator::visit_and(expr *, TristateRepr left, TristateRepr right) {
    struct TristateRepr res;
    res.yes =  &(*left.yes && *right.yes);
    res.mod = &((*left.mod || *right.mod) && (*left.yes || *left.mod) && (*right.yes || *right.mod));
    return res;
}

TristateRepr ExpressionTranslator::visit_or(expr *, TristateRepr left, TristateRepr right) {
    struct TristateRepr res;
    res.yes = &(*left.yes || *right.yes);
    res.mod = &(!(*left.yes || *right.yes) && (*left.mod || *right.mod));
    return res;
}

TristateRepr ExpressionTranslator::visit_not(expr *, TristateRepr left) {
    struct TristateRepr res;
    res.yes =  &(!*left.yes && !*left.mod);
    res.mod = left.mod;
    return res;
}

TristateRepr ExpressionTranslator::visit_equal(struct expr *e, TristateRepr left, TristateRepr right) {
    struct TristateRepr res;
    if (! (e->left.sym->type == S_BOOLEAN  || e->right.sym->type == S_BOOLEAN ||
          e->left.sym->type == S_TRISTATE || e->right.sym->type == S_TRISTATE )) {
        static int counter;
        std::stringstream n;
        n << "__FREE__EQ"<< counter;
        counter++;
        res.yes=B_VAR(n.str());
        res.mod=B_CONST (false);
        return res;
    }
    res.yes =  &((*left.yes && *right.yes) || (*left.mod && *right.mod) ||
                (!*left.yes && !*right.yes && !*left.mod && !*right.mod ) );
    res.mod = B_CONST(false);
    return res;
}

TristateRepr ExpressionTranslator::visit_unequal(struct expr *e, TristateRepr left, TristateRepr right) {
    struct TristateRepr res;

    if (! (e->left.sym->type == S_BOOLEAN  || e->right.sym->type == S_BOOLEAN ||
          e->left.sym->type == S_TRISTATE || e->right.sym->type == S_TRISTATE )) {
        static int counter;
        std::stringstream n;
        n << "__FREE__NE"<< counter;
        counter++;
        res.yes=B_VAR(n.str());
        res.mod=B_CONST (false);
        return res;
    }
    res.yes = &((!*left.yes || !*right.yes) && (!*left.mod || !*right.mod) &&
                ( *left.yes || *right.yes || *left.mod || *right.mod ) );
    res.mod = B_CONST(false);

    return res;
}

TristateRepr ExpressionTranslator::visit_list(struct expr *e) {
    struct TristateRepr res;

    BoolExp *all = 0;
    BoolExp *mod = 0;
    for (struct expr * itnode = e; itnode != 0; itnode = itnode->left.expr) {
        struct symbol * yessym = itnode->right.sym;
        BoolExp *xorExp = 0;
        for (struct expr * itnode1 = e; itnode1 != 0; itnode1 = itnode1->left.expr) {
            struct symbol * cursym = itnode1->right.sym;
            BoolExp *lit = (cursym == yessym) ? B_VAR(cursym, rel_yes) : &( ! *B_VAR(cursym, rel_yes));
            xorExp = xorExp ? &( *xorExp && *lit): lit;
        }
        all = all ? &( *all || *xorExp): xorExp;
        mod = mod ? &( *mod || *B_VAR(yessym, rel_mod)) : B_VAR(yessym, rel_mod) ;
    }
    res.yes = all;
    res.mod = mod;
    return res;
}

TristateRepr ExpressionTranslator::visit_range(struct expr *) {
    struct TristateRepr res = { 0, 0, true, S_OTHER };
    throw new UnsupportedFeatureException("cannot handle ranges yet");
    return res;
}

TristateRepr ExpressionTranslator::visit_others(struct expr *)  {
    struct TristateRepr res = { 0, 0, true, S_OTHER };
    throw new InvalidNodeException("kconfig expression with unknown type");
    return res;
}
