// -*- mode: c++ -*-
/*
 *    satyr - compiles KConfig files to boolean formulas
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

#ifndef KCONFIG_TRANSLATEEXPRESSION_H
#define KCONFIG_TRANSLATEEXPRESSION_H

#include <string>
#include <map>
#include <set>
#include <stdlib.h>

#ifndef USE_ZCONF
#define USE_ZCONF
#endif
#include "bool.h"
#include "ExpressionVisitor.h"
#include "InvalidNodeException.h"

namespace kconfig {

    struct TristateRepr {
        BoolExp *yes;
        BoolExp *mod;
        bool invalid ;
        symbol_type type;
    };

    class ExpressionTranslator : public ExpressionVisitor<TristateRepr> {

    public:
        ExpressionTranslator();
        std::set<struct symbol *> *symbolSet;

    protected:
        virtual TristateRepr visit_symbol(struct symbol *sym);
        virtual TristateRepr visit_and(expr *e, TristateRepr left, TristateRepr right);
        virtual TristateRepr visit_or(expr *e, TristateRepr left, TristateRepr right);
        virtual TristateRepr visit_not(expr *e, TristateRepr left);
        virtual TristateRepr visit_equal(struct expr *e, TristateRepr left, TristateRepr right);
        virtual TristateRepr visit_unequal(struct expr *e, TristateRepr left, TristateRepr right);
        virtual TristateRepr visit_list(struct expr *e);
        virtual TristateRepr visit_range(struct expr *e);
        virtual TristateRepr visit_others(expr *);
    };
}
#endif
