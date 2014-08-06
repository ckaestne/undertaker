/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#include <fstream>
#include <algorithm>

bool KconfigWhitelist::isWhitelisted(const std::string &item) const {
    return std::find(begin(), end(), item) != end();
}

KconfigWhitelist &KconfigWhitelist::getIgnorelist() {
    static KconfigWhitelist instance;
    return instance;
}

KconfigWhitelist &KconfigWhitelist::getWhitelist() {
    static KconfigWhitelist instance;
    return instance;
}

KconfigWhitelist &KconfigWhitelist::getBlacklist() {
    static KconfigWhitelist instance;
    return instance;
}

int KconfigWhitelist::loadWhitelist(const char *file) {
    std::ifstream whitelist(file);

    if (!whitelist.good())
        return -1;

    std::string line;
    int n = size();

    while (std::getline(whitelist, line)) {
        if (line[0] == '#')
            continue;

        if (!isWhitelisted(line))
            emplace_back(line);
    }
    return size() - n;
}
