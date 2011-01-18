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


#include "KconfigWhitelist.h"
#include "ModelContainer.h"

#include <fstream>
#include <boost/regex.hpp>

bool KconfigWhitelist::isWhitelisted(const char *item) const {
    KconfigWhitelist::const_iterator it;

    for (it = begin(); it != end(); it++)
        if((*it).compare(item) == 0)
            return true;
    return false;
}

void KconfigWhitelist::addToWhitelist(const std::string item) {
    if(!isWhitelisted(item.c_str()))
        push_back(item);
}

KconfigWhitelist *KconfigWhitelist::getInstance() {
    static KconfigWhitelist *instance;
    if (!instance) {
        instance = new KconfigWhitelist();
    }
    return instance;
}

int KconfigWhitelist::loadWhitelist(const char *file) {
    ModelContainer *f = ModelContainer::getInstance();
    if (f->empty())
        return 0;

    std::ifstream whitelist(file);
    std::string line;
    const boost::regex r("^#.*", boost::regex::perl);

    int n = 0;

    while (std::getline(whitelist, line)) {
        boost::match_results<const char*> what;

        if (boost::regex_search(line.c_str(), what, r))
            continue;

        n++;
        this->addToWhitelist(line.c_str());
    }
    return n;
}

