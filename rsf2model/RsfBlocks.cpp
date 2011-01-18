/*
 *   rsf2model - convert dumpconf output to undertaker model format
 *
 * Copyright (C) 2010-2011 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#include "RsfBlocks.h"

RsfBlocks::RsfBlocks(std::ifstream &f, std::string relation, std::ostream &log)
    : std::multimap<key_type, mapped_type>(), log(log), relation(relation) {
    this->read_blocks(f, relation);
}


void RsfBlocks::print_contents(std::ostream &out) {

    for (iterator i = begin(); i != end(); i++)
        out << (*i).first << " : " << (*i).second.front() << std::endl;
}

typedef std::deque<std::string> strings;

strings parse(const std::string& line) {
    strings result;
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

void dumpList(strings ss)
{
    for (int i=0; i<(signed)ss.size(); ++i) std::cout << ss.at(i) << ", ";
    std::cout << std::endl;

}

void RsfBlocks::read_blocks(std::ifstream &rsf_file, std::string relation) {

    // std::clog << "I: stream is at pos " << rsf_file.tellg() << std::endl;

    while (rsf_file.good()) {
        std::string s;
        getline (rsf_file, s);
        strings v = parse(s);
        if (v.size() == 0)
            continue;
        std::string k = v.front();
        if (k == relation) {
            v.pop_front();
            std::string blockno = v.front();
            v.pop_front();
            insert(make_pair(blockno, v));
        }
    }

    log << "I: " << relation << ": inserted " << this->size()
        << " items" << std::endl;

    rsf_file.clear();
    rsf_file.seekg(0);

}

std::string *RsfBlocks::getValue(std::string key) {
    iterator i = find(key);
    if (i == end())
        return NULL;
    return &((*i).second.front());
}

std::string *RsfBlocks::getValue(int n) {
    std::stringstream ss;
    ss << n;
    return getValue(ss.str());
}
