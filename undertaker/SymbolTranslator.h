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

#ifndef KCONFIG_SYMBOLTRANSLATOR_H
#define KCONFIG_SYMBOLTRANSLATOR_H

#include "SymbolParser.h"
#include "CNFBuilder.h"

#include <set>


namespace kconfig {
    class SymbolTranslator : public SymbolParser {
        // statistic data
        int _featuresWithStringDep = 0;
        int _totalStringComp = 0;
        CNFBuilder cnfbuilder;

        void addClause(BoolExp *clause);
        void pushSymbolInfo(struct symbol *sym);
    public:
        SymbolTranslator(PicosatCNF *cnf) : cnfbuilder(cnf) {}

        std::set<struct symbol *> *symbolSet = nullptr;

        int featuresWithStringDependencies() { return _featuresWithStringDep; }
        int totalStringComparisons() { return _totalStringComp; }
    protected:
        virtual void visit_bool_symbol (struct symbol *sym)     final override;
        virtual void visit_tristate_symbol (struct symbol *sym) final override;
        virtual void visit_int_symbol (struct symbol *sym)      final override;
        virtual void visit_hex_symbol (struct symbol *sym)      final override;
        virtual void visit_string_symbol (struct symbol *sym)   final override;
        virtual void visit_symbol (struct symbol *sym)          final override;
        virtual void visit_choice_symbol (struct symbol *sym)   final override;
    };
}
#endif
