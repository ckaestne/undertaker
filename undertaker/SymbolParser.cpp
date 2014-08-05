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

#include "SymbolParser.h"
#include "SymbolTools.h"


int kconfig::SymbolParser::parse(const std::string &path) {
    // the state is stored in a global array
    // "struct symbol *symbol_hash[SYMBOL_HASHSIZE];"
    // which is defined in zconf.tab.c_shipped, for_all_symbols traverses through that array
    conf_parse(path.c_str());
    return 0;
}

void kconfig::SymbolParser::traverse(void) {
    unsigned int i;
    struct symbol *sym;

    for_all_symbols(i, sym) {
        if (!sym->name && !sym_is_choice(sym) && !modules_sym) {
            continue;
        }
        if (sym_is_choice(sym)) {
            nameSymbol(sym);
            visit_choice_symbol(sym);
            continue;
        }
        switch (sym->type) {
        case S_BOOLEAN:
            visit_bool_symbol(sym);
            break;
        case S_TRISTATE:
            visit_tristate_symbol(sym);
            break;
        case S_INT:
            visit_int_symbol(sym);
            break;
        case S_HEX:
            visit_hex_symbol(sym);
            break;
        case S_STRING:
            visit_string_symbol(sym);
            break;
        default:
            visit_symbol(sym);
        }
    }
}
