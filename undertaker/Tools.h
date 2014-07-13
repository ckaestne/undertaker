/*
 *   undertaker - temporary header for features which will be introduced in c++14
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


#ifndef _UNDERTAKER_TOOLS_H_
#define _UNDERTAKER_TOOLS_H_

#include <string>
#include <set>


namespace undertaker {

    //! returns all (configuration) items of the given string
    std::set<std::string> itemsOfString(const std::string &);

    //!< replaces invalid characters with '_'
    std::string normalize_filename(std::string);

}

#endif
