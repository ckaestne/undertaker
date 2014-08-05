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
#include "exceptions/UnsupportedFeatureException.h"
#include "bool.h"

using namespace kconfig;


TristateRepr ExpressionTranslator::visit_symbol(struct symbol *sym) {
    if (sym == &symbol_no) {
        return {B_CONST(false), B_CONST(false), false, S_TRISTATE};
    } else if (sym == &symbol_yes) {
        return {B_CONST(true), B_CONST(false), false, S_TRISTATE};
    } else if (sym == &symbol_mod) {
        return {B_CONST(false), B_CONST(true), false, S_TRISTATE};
    } else if (sym->type == S_STRING || sym->type == S_HEX || sym->type == S_INT) {
        return {B_CONST(false), B_CONST(false), true, sym->type};
    } else if (sym == modules_sym) {
        return {B_VAR(sym, rel_yes), B_CONST(false), true, S_BOOLEAN};
    } else if (this->symbolSet && this->symbolSet->find(sym) == this->symbolSet->end()) {
        return {B_CONST(false), B_CONST(false), false, S_BOOLEAN};
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
    res.yes = &(*left.yes && *right.yes);
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
    res.yes = &(!*left.yes && !*left.mod);
    res.mod = left.mod;
    return res;
}

TristateRepr ExpressionTranslator::visit_equal(struct expr *e, TristateRepr left,
                                               TristateRepr right) {
    struct TristateRepr res;
    if (!(e->left.sym->type == S_BOOLEAN || e->right.sym->type == S_BOOLEAN
          || e->left.sym->type == S_TRISTATE || e->right.sym->type == S_TRISTATE)) {
        static int counter;
        res.yes = B_VAR("__FREE__EQ" + std::to_string(counter++));
        res.mod = B_CONST(false);
        this->_processedValComp++;
        return res;
    }
    res.yes = &((*left.yes && *right.yes) || (*left.mod && *right.mod)
                || (!*left.yes && !*right.yes && !*left.mod && !*right.mod));
    res.mod = B_CONST(false);
    return res;
}

TristateRepr ExpressionTranslator::visit_unequal(struct expr *e, TristateRepr left,
                                                 TristateRepr right) {
    struct TristateRepr res;
    if (!(e->left.sym->type == S_BOOLEAN || e->right.sym->type == S_BOOLEAN
          || e->left.sym->type == S_TRISTATE || e->right.sym->type == S_TRISTATE)) {
        static int counter;
        res.yes = B_VAR("__FREE__NE" + std::to_string(counter++));
        res.mod = B_CONST(false);
        this->_processedValComp++;
        return res;
    }
    res.yes = &((!*left.yes || !*right.yes) && (!*left.mod || !*right.mod)
                && (*left.yes || *right.yes || *left.mod || *right.mod));
    res.mod = B_CONST(false);

    return res;
}

TristateRepr ExpressionTranslator::visit_list(struct expr *e) {
    struct TristateRepr res;

    BoolExp *all = nullptr;
    BoolExp *mod = nullptr;
    for (struct expr *itnode = e; itnode != nullptr; itnode = itnode->left.expr) {
        struct symbol *yessym = itnode->right.sym;
        BoolExp *xorExp = nullptr;
        for (struct expr *itnode1 = e; itnode1 != nullptr; itnode1 = itnode1->left.expr) {
            struct symbol *cursym = itnode1->right.sym;
            BoolExp *lit = (cursym == yessym) ? B_VAR(cursym, rel_yes)
                                              : &(!*B_VAR(cursym, rel_yes));
            xorExp = xorExp ? &(*xorExp && *lit) : lit;
        }
        all = all ? &(*all || *xorExp) : xorExp;
        mod = mod ? &(*mod || *B_VAR(yessym, rel_mod)) : B_VAR(yessym, rel_mod);
    }
    res.yes = all;
    res.mod = mod;
    return res;
}

TristateRepr ExpressionTranslator::visit_range(struct expr *, TristateRepr, TristateRepr) {
    throw UnsupportedFeatureException("cannot handle ranges yet");
    return {nullptr, nullptr, true, S_OTHER};
}

TristateRepr ExpressionTranslator::visit_others(struct expr *) {
    throw InvalidNodeException("kconfig expression with unknown type");
    return {nullptr, nullptr, true, S_OTHER};
}
