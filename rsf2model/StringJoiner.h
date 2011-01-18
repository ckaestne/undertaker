/*
 *   rsf2model - convert dumpconf output to undertaker model format
 *
 * Copyright (C) 2010-2011 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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


// -*- mode: c++ -*-
#ifndef string_joiner_h__
#define string_joiner_h__

#include <deque>
#include <string>
#include <sstream>

struct StringJoiner : public std::deque<std::string> {
    std::string join(const char *j) {
        std::stringstream ss;
        if (size() == 0)
            return "";

        ss << front();

        std::deque<std::string>::const_iterator i = begin() + 1;

        while (i != end()) {
            ss << j << *i;
            i++;
        }
        return ss.str();
    }

    void push_back(const value_type &x) {
        if (x.compare("") == 0)
            return;
        std::deque<value_type>::push_back(x);
    }
};

#endif
