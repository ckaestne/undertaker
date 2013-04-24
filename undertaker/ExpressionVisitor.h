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

#include "ExpressionVisitor.t"
#endif
