// -*- mode: c++ -*-
/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#include "ExpressionVisitor.h"
#include "InvalidNodeException.h"

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
