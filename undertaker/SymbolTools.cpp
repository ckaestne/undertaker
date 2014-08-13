/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#include "SymbolTools.h"
#include "Logging.h"

#include <stack>
#include <string.h>

using namespace kconfig;


expr * kconfig::visibilityExpression(struct symbol *sym) {
    // symbol is visible (ie modifiable) if prompt1 or prompt2 or ... is visible.

    // dependencies for symbol are included...
    struct property *prop;
    expr *visible = nullptr;
    int promptc = 0;
    expr *cur;

    for_all_properties(sym, prop, P_PROMPT) {
        promptc++;
        cur = prop->visible.expr ? expr_copy(prop->visible.expr) : expr_alloc_symbol(&symbol_yes);
        visible = expr_alloc_or(visible, cur);
    }
    for_all_properties(sym, prop, P_MENU) {
        promptc++;
        cur = prop->visible.expr ? expr_copy(prop->visible.expr) : expr_alloc_symbol(&symbol_yes);
        visible = expr_alloc_or(visible, cur);
    }
    if (promptc == 1) {
        struct gstr s = str_new();
        struct gstr t = str_new();
        expr_gstr_print(cur, &s);
        for_all_properties(sym, prop, P_SYMBOL) {
            expr_gstr_print(prop->visible.expr, &t);
            if (!strcmp(s.s, t.s)) {
                expr_free(visible);
                return expr_alloc_symbol(&symbol_yes);
            }
            break;
        }
        str_free(&s);
        str_free(&t);
    }
    if (sym && sym->name) {
        Logging::debug(sym->name, " ", visible, " pc: ", promptc);
    }
    return visible ? visible : expr_alloc_symbol(&symbol_no);
}

bool kconfig::hasPrompt(struct symbol *sym) {
    struct property *prop;

    for_all_properties(sym, prop, P_PROMPT) {
        return true;
    }
    return false;
}

expr * kconfig::dependsExpression(struct symbol *sym) {
    // symbol is visible (ie modifiable) if prompt1 or prompt2 or ... is visible.

    // dependencies for symbol are included...
    struct property *prop;
    expr *depends = nullptr;
    for_all_properties(sym, prop, P_SYMBOL) {
        expr *current = prop->visible.expr ? expr_copy(prop->visible.expr)
                                           : expr_alloc_symbol(&symbol_yes);
        depends = expr_alloc_or(depends, current);
    }
    return depends ? depends : expr_alloc_symbol(&symbol_yes);
}

/* this function is ONLY valid for bool/tristate defaults! */
expr * kconfig::defaultExpression_bool_tristate(struct symbol *sym) {
    /* defaults to the first visible default:
     * d1.vis ? d1.val : (d2.vis ? d2.val : ... )
     */

    // dependencies for symbol are included...
    struct property *prop;
    std::stack<struct property *> propStack;
    expr *def = expr_alloc_symbol(&symbol_no);

    for_all_properties(sym, prop, P_DEFAULT) {
        propStack.push(prop);
    }
    while (!propStack.empty()) {
        struct property *cur = propStack.top();
        propStack.pop();
        /* default(n+1) = cur.vis ? cur.expr : default(n)
         * default(n+1) = (cur.vis && cur.expr)  || (!cur.vis && default(n))
         */
        //cur->visible.expr == nullptr <=> cur->visible.expr == yes
        expr *vis = cur->visible.expr ? expr_copy(cur->visible.expr)
                                      : expr_alloc_symbol(&symbol_yes);
        def = expr_alloc_or(
            expr_alloc_and(expr_copy(cur->visible.expr), expr_copy(cur->expr)),
            expr_alloc_and(expr_alloc_one(E_NOT, vis), def) );
    }
    return expr_eliminate_yn(def);
}

expr * kconfig::reverseDepExpression(struct symbol *sym) {
    if (sym_is_choice_value(sym) ) {
        return expr_alloc_symbol(&symbol_no);
    }
    return sym->rev_dep.expr ? expr_copy(sym->rev_dep.expr) : expr_alloc_symbol(&symbol_no);
}

expr * kconfig::choiceExpression(struct symbol *sym) {
    struct property *prop;

    for_all_properties(sym, prop, P_CHOICE) {
        return prop->expr;
    }
    return nullptr;
}

expr * kconfig::selectExpression(struct symbol *sym) {
    struct property *prop;
    expr *selects = nullptr;

    for_all_properties(sym, prop, P_SYMBOL) {
        selects = expr_alloc_and(selects, expr_copy(prop->visible.expr));
    }
    return selects ? selects : expr_alloc_symbol(&symbol_no);
}

void kconfig::nameSymbol(struct symbol *sym) {
    static int choiceCount=0;

    if (sym->name) {
        return;
    }
    if (sym_is_choice(sym)) {
        auto n = new char[19];
        snprintf(n, 19, "CHOICE_%d", choiceCount++);
        sym->name = n;
    }
    if (sym == modules_sym) {
        sym->name = strdup("___MODULES_MAGIC_INTERNAL_VAR___");
    }
}
