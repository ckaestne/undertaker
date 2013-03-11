// -*- mode: c++ -*-
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

#ifndef KCONFIG_EXPRESSIONVISITOR_H
#define KCONFIG_EXPRESSIONVISITOR_H

#ifndef LKC_DIRECT_LINK
#define LKC_DIRECT_LINK
#endif

#include "bool.h"
#include "InvalidNodeException.h"

#include <string>
#include <iostream>

namespace kconfig {
    template <typename rt>
    class ExpressionVisitor {
    public:
        rt process(struct expr *e);

    protected:
        virtual rt visit_or(struct expr *e, rt left, rt right) = 0;
        virtual rt visit_and(struct expr *e, rt left, rt right) = 0;
        virtual rt visit_not(struct expr *e, rt left) = 0;
        virtual rt visit_equal(struct expr *e, rt left, rt right) = 0;
        virtual rt visit_unequal(struct expr *e, rt left, rt right) = 0;
        virtual rt visit_list(struct expr *e) {
            return visit_others(e);
        }
        virtual rt visit_symbol(struct symbol *) = 0;
        virtual rt visit_range(struct expr *e, rt, rt) {
            return visit_others(e);
        }
        virtual rt visit_others(struct expr *e) = 0;
    };
}

using namespace kconfig;

template<typename rt>
rt ExpressionVisitor<rt>::process(struct expr *e)  {
    rt left;
    rt right;

    switch (e->type) {
    case E_NONE:
        throw new InvalidNodeException("kconfig expression with unknown type");
        break;
    case E_OR:
        left = process(e->left.expr);
        right = process(e->right.expr);
        return visit_or(e, left, right);
        break;
    case E_AND:
        left = process(e->left.expr);
        right = process(e->right.expr);
        return visit_and(e, left, right);
        break;
    case E_NOT:
        left = process(e->left.expr);
        return visit_not(e, left);
        break;
    case E_EQUAL:
        left = visit_symbol(e->left.sym);
        right = visit_symbol(e->right.sym);
        return visit_equal(e, left, right);
        break;
    case  E_UNEQUAL:
        left = visit_symbol(e->left.sym);
        right = visit_symbol(e->right.sym);
        return visit_unequal(e, left, right);
        break;
    case E_LIST:
        return visit_list(e);
        break;
    case E_SYMBOL:
        return visit_symbol(e->left.sym);
        break;
    case E_RANGE:
        left = process(e->left.expr);
        right = process(e->right.expr);
        return visit_range(e, left, right);
        break;
    default:
        return visit_others(e);
    }
}
#endif
