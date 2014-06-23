/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#include "RsfReader.h"
//#include "Logging.h"

#include <boost/regex.hpp>
#include <sstream>
#include <iostream>


RsfReader::RsfReader(std::istream &f, std::string metaflag) : metaflag(metaflag) {
    this->read_rsf(f);
}

void RsfReader::print_contents(std::ostream &out) {
    for (auto &entry : *this)  // pair<string, deque<string>>
        out << entry.first << " : " << entry.second.front() << std::endl;
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

size_t RsfReader::read_rsf(std::istream &rsf_file) {
    std::string line;

    /* Read all lines, and store it into the key value store */
    while (std::getline(rsf_file, line)) {
        StringList columns = parse(line);

        if (columns.size() == 0)
            continue;

        std::string key = columns.front();
        columns.pop_front();
        // Check if the current line is an metainformation line
        // if so, put it there
        // UNDERTAKER_SET ALWAYS_ON fooooo
        // self.meta_information("ALWAYS_ON") == ["foooo"]
        if (metaflag.size() > 0 && key.compare(metaflag) == 0) {
            if (columns.size() == 0)
                continue;
            std::string key = columns.front();
            columns.pop_front();
            meta_information.emplace(key, columns);
        } else {
            static const boost::regex file_identifier("^FILE_.*$");

            if (boost::regex_match(key, file_identifier)) {
//                logger << debug << "found a file presence condition for " << key << std::endl;
//                bool dirty=false;
                for (size_t pos = 0; pos < key.length(); pos++) {
                    if (key.at(pos) == '/' || key.at(pos) == '-') {
                        key[pos] = '_';
//                        dirty = true;
                    }
                }
//                if (dirty)
//                    logger << debug << "Normalized item: " << key << std::endl;
            }
            this->emplace(key, columns);
        }
    }
    return this->size();
}

const std::string *RsfReader::getValue(const std::string &key) const {
    static std::string null_string("");
    const_iterator i = find(key);

    if (i == end())
        return nullptr;
    if (i->second.size() == 0)
        return &null_string;

    return &(i->second.front());
}

const StringList *RsfReader::getMetaValue(const std::string &key) const {
    static std::string null_string("");

    std::map<std::string, StringList>::const_iterator i = meta_information.find(key);
    if (i == meta_information.end())
        return nullptr;
    return &((*i).second);
}

void RsfReader::addMetaValue(const std::string &key, const std::string &value) {
    StringList values;
    std::map<std::string, StringList>::const_iterator i = meta_information.find(key);
    if (i != meta_information.end()) {
        values = (*i).second;
        meta_information.erase(key);
    }
    values.push_back(value);
    meta_information.emplace(key, values);
}

ItemRsfReader::ItemRsfReader(std::istream &f) : RsfReader() {
    read_rsf(f);
}

size_t ItemRsfReader::read_rsf(std::istream &rsf_file) {
    std::string line;

    /* Read all lines, and store it into the key value store */
    while (std::getline(rsf_file, line)) {
        StringList columns = parse(line);

        if (columns.size() == 0)
            continue;

        std::string key = columns.front();
        columns.pop_front();

        // Skip lines that do not start with 'Item'
        if (0 != key.compare("Item"))
            continue;

        key = columns.front(); columns.pop_front();
        this->emplace(key, columns);
    }
    return this->size();
}
