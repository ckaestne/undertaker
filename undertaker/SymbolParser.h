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

#ifndef KCONFIG_SYMBOLPARSER_H
#define KCONFIG_SYMBOLPARSER_H

#include "SymbolTools.h"

namespace kconfig {
    class SymbolParser {
    public:
        int parse(const char *path);
        void traverse(void);

    protected:
        virtual void visit_bool_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_tristate_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_int_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_hex_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_string_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_choice_symbol(struct symbol *sym) {
            visit_symbol(sym);
        };
        virtual void visit_symbol(struct symbol *sym) = 0;
    };
}
#endif
