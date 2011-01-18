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


#include <sstream>
#include <deque>
#include <fstream>
#include <iostream>

#include "RsfReader.h"

RsfReader::RsfReader(std::ifstream &f, std::ostream &log)
    : std::map<key_type, mapped_type>(), log(log) {
    this->read_rsf(f);
}


void RsfReader::print_contents(std::ostream &out) {
    for (iterator i = begin(); i != end(); i++)
        out << (*i).first << " : " << (*i).second.front() << std::endl;
}

StringList RsfReader::parse(const std::string& line) {
    StringList result;
    std::string item;
    std::stringstream ss(line);

    while(ss >> item){
        if (item[0]=='"') {
            if (item[item.length() - 1]=='"') {
                // special case: single word was needlessly quoted
                result.push_back(item.substr(1, item.length()-2));
            } else {
                // use the free-standing std::getline to read a "line"
                // into another string
                // starting after the current word, delimited by double-quote
                std::string restOfItem;
                getline(ss, restOfItem, '"');
                // That trailing quote is removed from the stream
                // and not copied to restOfItem.
                result.push_back(item.substr(1) + restOfItem);
            }
        } else {
            result.push_back(item);
        }
    }
    return result;
}

void RsfReader::read_rsf(std::ifstream &rsf_file) {

    /* Read all lines, and store it into the key value store */
    while (rsf_file.good()) {
        std::string line;
        getline (rsf_file, line);
        StringList columns = parse(line);
        if (columns.size() == 0)
            continue;
        std::string key = columns.front();
        columns.pop_front();
        insert(make_pair(key, columns));
    }

    log << "I: " << " inserted " << this->size()
        << " items" << std::endl;

    rsf_file.close();
}

const std::string *RsfReader::getValue(const std::string &key) const {
    static std::string null_string("");
    const_iterator i = find(key);
    if (i == end())
        return NULL;
    if (i->second.size() == 0)
        return &null_string;

    return &(i->second.front());
}
