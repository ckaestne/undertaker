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
