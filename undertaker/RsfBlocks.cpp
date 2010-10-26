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
