/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

/**
 * \brief Helper subclass of std::deque<std::string> for convenient
 *  concatenating strings with a separator.
 *
 * The behaviour is similar to the python list().join(",") construct.
 * 
 */
struct StringJoiner : public std::deque<std::string> {
    /**
     * \brief join all strings in the container,
     *
     * Join all the collected strings to one string. The separator is
     * inserted between each element.
     */
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

    /**
     * \brief append strings to list.
     *
     * Appends the given value to the list of values if it isn't the
     * empty string. "" will be ignored.
     */
    void push_back(const value_type &x) {
        if (x.compare("") == 0)
            return;
        std::deque<value_type>::push_back(x);
    }
};

#endif
