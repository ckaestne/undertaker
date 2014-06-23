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
#include <set>
#include <string>
#include <sstream>
#include <iostream>

/**
 * \brief Helper subclass of std::deque<std::string> for convenient
 *  concatenating strings with a separator.
 *
 * The behaviour is similar to the python list().join(",") construct.
 */
struct StringJoiner : public std::deque<std::string> {
    /**
     * \brief join all strings in the container,
     *
     * Join all the collected strings to one string. The separator is
     * inserted between each element.
     */
    std::string join(const std::string &j) {
        if (size() == 0)
            return "";

        std::stringstream ss;
        ss << front();

        for (auto it = begin() + 1, e = end(); it != e; ++it)
            ss << j << *it;

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

    /**
     * \brief append strings to list.
     *
     * Appends the given value to the list of values if it isn't the
     * empty string. "" will be ignored.
     */
    void push_front(const value_type &x) {
        if (x.compare("") == 0)
            return;
        std::deque<value_type>::push_front(x);
    }
};

struct UniqueStringJoiner : public StringJoiner {
    UniqueStringJoiner() = default;

    bool count (const value_type &x) {
        return _unique_set.count(x);
    }
    void push_back(const value_type &x) {
        if (uniqueFlag) {
            /* Simulate an map when StringJoiner is unique */
            if (_unique_set.count(x) > 0) return;
            _unique_set.insert(x);
        }

        StringJoiner::push_back(x);
    }

    void push_front(const value_type &x) {
        if (uniqueFlag) {
            /* Simulate an map when StringJoiner is unique */
            if (_unique_set.count(x) > 0) return;
            _unique_set.insert(x);
        }

        StringJoiner::push_front(x);
    }

    void disableUniqueness() {
        uniqueFlag = false;
    }

 private:
    bool uniqueFlag = true;
    std::set<std::string> _unique_set;
};

#endif
