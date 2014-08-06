// -*- mode: c++ -*-
/*
 *   boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KCONFIG_BOOLEXPSYMBOLSET_H
#define KCONFIG_BOOLEXPSYMBOLSET_H

#include "bool.h"
#include "BoolVisitor.h"

#include <string>
#include <set>

namespace kconfig {
    class BoolExpSymbolSet : public BoolVisitor {
    private:
        std::set<std::string> symbolset;
        bool ignoreFunctionSymbols;

    public:
        BoolExpSymbolSet(BoolExp *e, bool ignoreFunctionSymbols = true);
        std::set<std::string> getSymbolSet(void);

    protected:
        virtual void visit(BoolExp *)      final override {}
        virtual void visit(BoolExpAnd *)   final override {}
        virtual void visit(BoolExpOr *)    final override {}
        virtual void visit(BoolExpNot *)   final override {}
        virtual void visit(BoolExpConst *) final override {}
        virtual void visit(BoolExpVar *e)  final override;
        virtual void visit(BoolExpImpl *)  final override {}
        virtual void visit(BoolExpEq *)    final override {}
        virtual void visit(BoolExpCall *e) final override;
        virtual void visit(BoolExpAny *)   final override {}
    };
}
#endif
