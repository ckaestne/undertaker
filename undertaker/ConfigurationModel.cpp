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


#include "ConfigurationModel.h"
#include "KconfigWhitelist.h"
#include "StringJoiner.h"

#include <boost/algorithm/string/predicate.hpp>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>


ConfigurationModel::ConfigurationModel(std::string name, std::ifstream &in, std::ostream &log)
    : RsfReader(in, log, "UNDERTAKER_SET"), _name(name) {
    const StringList *configuration_space_regex = getMetaValue("CONFIGURATION_SPACE_REGEX");

    if (configuration_space_regex != NULL && configuration_space_regex->size() > 0) {
        std::cout << "I: Set configuration space regex to '" << configuration_space_regex->front() << "'" << std::endl;
        _inConfigurationSpace_regexp = boost::regex(configuration_space_regex->front(), boost::regex::perl);
    } else
        _inConfigurationSpace_regexp = boost::regex("^CONFIG_[^ ]+$", boost::regex::perl);
}

std::list<std::string> itemsOfString(const std::string &str) {
    std::list<std::string> mylist;
    std::string::const_iterator it = str.begin();
    std::string tmp = "";
    while (it != str.end()) {
        switch (*it) {
        case '(':
        case ')':
        case '!':
        case '&':
        case '=':
        case '<':
        case '>':
        case '|':
        case '-':
        case 'y':
        case 'n':
        case ' ':
            if (!tmp.empty()) {
                mylist.push_back(tmp);
                //std::cout << "    (itemsOfString) inserting " << tmp << "\n";
                tmp = "";
            }
        it++;
        break;
        default:
            tmp += (*it);
            it++;
            break;
        }
    }
    if (!tmp.empty()) {
        mylist.push_back(tmp);
    }
    mylist.unique();
    return mylist;
}

void ConfigurationModel::findSetOfInterestingItems(std::set<std::string> &initialItems) const {
    std::list<std::string> listtmp;
    std::stack<std::string> workingStack;
    std::string tmp;
    /* Initialize the working stack with the given elements */
    for(std::set<std::string>::iterator sit = initialItems.begin(); sit != initialItems.end(); sit++) {
        workingStack.push(*sit);
    }

    while (!workingStack.empty()) {
        const std::string *item = getValue(workingStack.top());
        workingStack.pop();
        if (item != NULL) {
            if (item->compare("") != 0) {
                listtmp = itemsOfString(*item);
                for(std::list<std::string>::iterator sit = listtmp.begin(); sit != listtmp.end(); sit++) {
                    /* Item already seen? continue */
                    if (initialItems.find(*sit) == initialItems.end()) {
                        workingStack.push(*sit);
                        initialItems.insert(*sit);
                    }
                }
            }
        }
    }
}

std::string ConfigurationModel::getMissingItemsConstraints(std::set<std::string> &missing) {
    std::stringstream m;
    for(std::set<std::string>::iterator it = missing.begin(); it != missing.end(); it++) {

        if (it == missing.begin()) {
            m << "( ! ( " << (*it);
        } else {
            m << " || " << (*it) ;
        }
    }
    if (!m.str().empty()) {
        m << " ) )";
    }
    return m.str();
}

int ConfigurationModel::doIntersect(std::set<std::string> myset, std::ostream &out,
                                    std::set<std::string> &missing, const ConfigurationModel::Checker *c) const {
     int valid_items = 0;
     StringJoiner sj;

    findSetOfInterestingItems(myset);

    for(std::set<std::string>::const_iterator it = myset.begin(); it != myset.end(); it++) {
        std::stringstream ss;
        const std::string *item = getValue(*it);

        if (item != NULL) {
            valid_items++;
            if (item->compare("") != 0)
                sj.push_back("(" + *it + " -> (" + *item + "))");
        } else {
            // check if the symbol might be in the model space.
            // if not it can't be missing!
            if (!inConfigurationSpace(*it))
                continue;

            // iff we are given a checker for items, skip if it doesn't pass the test
            if(c && ! (*c)(*it)) {
                continue;
            }

            /* free variables are never missing */
            if (it->size() > 1 && !boost::starts_with(*it, "__FREE__"))
                missing.insert(*it);
        }
    }
    out << sj.join("\n&&\n");

    return valid_items;
}

bool ConfigurationModel::inConfigurationSpace(const std::string &symbol) const {
    if (boost::regex_match(symbol, _inConfigurationSpace_regexp))
        return true;
    return false;
}

bool ConfigurationModel::isComplete() const {
    const StringList *configuration_space_complete = getMetaValue("CONFIGURATION_SPACE_INCOMPLETE");
    // Reverse logic at this point to ensure Legacy models for kconfig
    // to work
    return !(configuration_space_complete != NULL);
}

