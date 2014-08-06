/*
 *   undertaker - common helper functions
 *
 * Copyright (C) 2014 Stefan Hengelein <stefan.hengelein@fau.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Tools.h"
#include "BoolExpSymbolSet.h"

std::set<std::string> undertaker::itemsOfString(const std::string &str) {
    kconfig::BoolExp *e = kconfig::BoolExp::parseString(str);
    kconfig::BoolExpSymbolSet symset(e);
    delete e;
    return symset.getSymbolSet();
}

std::string undertaker::normalize_filename(std::string normalized) {
    for (char &c : normalized)
        if (c == '/' || c == '-' || c == '+' || c == ':')
            c = '_';

    return normalized;
}

