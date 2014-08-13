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

#ifndef KCONFIG_TRANSLATEEXPRESSION_H
#define KCONFIG_TRANSLATEEXPRESSION_H

#include "ExpressionVisitor.h"

#include <set>


namespace kconfig {
    class BoolExp;

    struct TristateRepr {
        BoolExp *yes;
        BoolExp *mod;
        bool invalid ;
        symbol_type type;
    };

    class ExpressionTranslator : public ExpressionVisitor<TristateRepr> {
        // some statistical data
        int _processedValComp = 0;
        std::set<struct symbol *> *symbolSet = nullptr;
    public:
        ExpressionTranslator(std::set<struct symbol *> *s = nullptr) : symbolSet(s) {}

        int getValueComparisonCounter() { return _processedValComp; }
    protected:
        virtual TristateRepr visit_symbol(struct symbol *)                     final override;
        virtual TristateRepr visit_and(expr *, TristateRepr, TristateRepr)     final override;
        virtual TristateRepr visit_or(expr *, TristateRepr, TristateRepr)      final override;
        virtual TristateRepr visit_not(expr *, TristateRepr)                   final override;
        virtual TristateRepr visit_equal(expr *, TristateRepr, TristateRepr)   final override;
        virtual TristateRepr visit_unequal(expr *, TristateRepr, TristateRepr) final override;
        virtual TristateRepr visit_list(expr *e)                               final override;
        virtual TristateRepr visit_range(expr *, TristateRepr, TristateRepr)   final override;
        virtual TristateRepr visit_others(expr *)                              final override;
    };
}
#endif
