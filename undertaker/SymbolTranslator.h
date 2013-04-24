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

#include <list>
#include <set>

#ifndef USE_ZCONF
#define USE_ZCONF
#endif

#include "bool.h"
#include "SymbolParser.h"
#include "CNFBuilder.h"

namespace kconfig {
    class SymbolTranslator : public SymbolParser {
    public:
        SymbolTranslator (CNF *cnf);
        std::set<struct symbol *> *symbolSet;

    protected:
        CNFBuilder cnfbuilder;

        virtual void visit_bool_symbol (struct symbol *sym);
        virtual void visit_tristate_symbol (struct symbol *sym);
        virtual void visit_int_symbol (struct symbol *sym);
        virtual void visit_hex_symbol (struct symbol *sym);
        virtual void visit_string_symbol (struct symbol *sym);
        virtual void visit_symbol (struct symbol *sym);
        virtual void visit_choice_symbol (struct symbol *sym);
        virtual void addClause (BoolExp *clause);

    private:
        std::list<std::string> clauses;
    };
}
#endif
