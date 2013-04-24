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

#include "BoolExpSymbolSet.h"

using namespace kconfig;

BoolExpSymbolSet::BoolExpSymbolSet(BoolExp *e, bool ignoreFunctionSymbols) {
    this->ignoreFunctionSymbols = ignoreFunctionSymbols;
    if(e)
        e->accept(this);
}

std::set<std::string> BoolExpSymbolSet::getSymbolSet(void) {
     return symbolset;
}

void BoolExpSymbolSet::visit(BoolExpCall *e) {
    if(ignoreFunctionSymbols)
        return;

    symbolset.insert(e->getName());
}

void BoolExpSymbolSet::visit(BoolExpVar *e) {
    symbolset.insert(e->str());
}
