/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
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


#include "PresetList.h"
#include "ModelContainer.h"
#include "Logging.h"

#include <sstream>
#include <list>
#include <string>
#include <fstream>
#include <boost/regex.hpp>


bool PresetList::isListed(const char *item) const {
    PresetList::const_iterator it;

    for (it = begin(); it != end(); it++)
        if((*it).compare(item) == 0)
            return true;
    return false;
}

bool PresetList::isValid(const char *item) const {
    static const boost::regex r("^[A-Za-z0-9_.]+$", boost::regex::perl);
    if (boost::regex_match(item, r))
        return true;
    else {
        logger << warn << "PresetList item '" << item
                << "' is not valid; ignoring" << std::endl;
        return false;
    }
}


void PresetList::addToBlacklist(const std::string item) {
    if (isValid(item.c_str()) && !isBlacklisted(item.c_str()))
        push_back(getNegatedName(item.c_str()));
}

void PresetList::addToWhitelist(const std::string item) {
    if(isValid(item.c_str()) && !isWhitelisted(item.c_str()))
        push_back(item);
}

PresetList *PresetList::getInstance() {
    static PresetList *instance;
    if (!instance) {
        instance = new PresetList();
    }
    return instance;
}


int PresetList::loadList(std::istream &list, bool white) {

    if (!list.good())
        return 0;

    std::string line;
    const boost::regex comment("^[ \t]#.*", boost::regex::perl);

    int n = 0;

    while (std::getline(list, line)) {

        if (boost::regex_match(line.c_str(), comment))
            continue;

        n++;
        if (white)
            this->addToWhitelist(line.c_str());
        else
            this->addToBlacklist(line.c_str());
    }
    return n;
}

std::string PresetList::getNegatedName(const char * name) const {
    std::stringstream ss;
    ss << "!" << name;
    if (name[0] == '!')
        return ss.str().substr(2);
    else
        return ss.str();
}

std::string PresetList::toSatStr() {
    return join(" && ");
}
