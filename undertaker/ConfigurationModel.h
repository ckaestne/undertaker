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
#ifndef configuration_model_h__
#define configuration_model_h__

#include <string>
#include <map>
#include <deque>
#include <set>
#include <list>

#include "RsfReader.h"

std::list<std::string> itemsOfString(const std::string &str);

class ConfigurationModel : public RsfReader {
public:

    struct Checker {
        //! checks if the item is a candidate for addition in the 'missing' set
        virtual bool operator()(const std::string &item) const = 0;
    };

    ConfigurationModel(std::string name, std::ifstream &in, std::ostream &log);

    int doIntersect(const std::set<std::string> myset, std::ostream &out,
                    std::set<std::string> &missing, const Checker *c=NULL) const;
    void findSetOfInterestingItems(std::set<std::string> &working) const;
    static std::string getMissingItemsConstraints(std::set<std::string> &missing);
    std::string getName() const { return _name; }
private:
    std::string _name;
};

#endif
