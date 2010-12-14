#include "ConfigurationModel.h"
#include "KconfigWhitelist.h"
#include "StringJoiner.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>


ConfigurationModel::ConfigurationModel(std::ifstream &in, std::ostream &log)
    : RsfReader(in, log) {
}

std::list<std::string> itemsOfString(std::string str) {
    std::list<std::string> mylist;
    std::string::iterator it = str.begin();
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

int ConfigurationModel::doIntersect(std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing) const {
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
            if (it->size() > 1)
                missing.insert(*it);
        }
    }
    out << sj.join("\n&&\n");

    return valid_items;
}
