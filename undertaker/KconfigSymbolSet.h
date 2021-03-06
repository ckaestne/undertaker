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

#ifndef KCONFIG_KCONFIGSYMBOLSET_H
#define KCONFIG_KCONFIGSYMBOLSET_H

#include "SymbolParser.h"
#include "bool.h"

#include <set>


namespace kconfig {
    class KconfigSymbolSet : public SymbolParser, public std::set<struct symbol *> {
    public:
        KconfigSymbolSet(void) {}

    protected:
        virtual void visit_bool_symbol(struct symbol *sym)     final override {
            this->insert(sym);
        }
        virtual void visit_tristate_symbol(struct symbol *sym) final override {
            this->insert(sym);
        }
        virtual void visit_int_symbol(struct symbol *sym)      final override {
            this->insert(sym);
        }
        virtual void visit_hex_symbol(struct symbol *sym)      final override {
            this->insert(sym);
        }
        virtual void visit_string_symbol(struct symbol *sym)   final override {
            this->insert(sym);
        }
        virtual void visit_symbol(struct symbol *)             final override {
            //do nothing
        }
        virtual void visit_choice_symbol(struct symbol *sym)   final override {
            this->insert(sym);
        }
    };
}
#endif
