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


#ifndef _CPP14_H_
#define _CPP14_H_

#include <memory>

template <typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args && ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
auto cbegin(const T &t) -> decltype(t.cbegin()) {
    return t.cbegin();
}

template <class T>
auto cend(const T &t) -> decltype(t.cend()) {
    return t.cend();
}

#endif
