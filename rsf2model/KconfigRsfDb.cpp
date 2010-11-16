#include "KconfigRsfDb.h"
#include "StringJoiner.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>

static std::list<std::string> itemsOfString(std::string str);

KconfigRsfDb::Item::Item(std::string name, unsigned type, bool required)
    : name_(name), type_(type), required_(required) { }

KconfigRsfDb::Item KconfigRsfDb::ItemDb::getItem(std::string key) const {
    ItemMap::const_iterator it = this->find(key);

    /* We only request config items, which are defined in the rsf.
       Be aware that no slicing is done here */
    assert(it != this->end());

    return (*it).second;
}

KconfigRsfDb::KconfigRsfDb(std::ifstream &in, std::ostream &log)
    : _in(in),
      /* This are RsfBlocks! */
      choice_(in, "Choice", log),
      choice_item_(in, "ChoiceItem", log),
      depends_(in, "Depends", log),
      item_(in, "Item", log)
{}

void KconfigRsfDb::initializeItems() {

    for(RsfBlocks::iterator i = this->item_.begin(); i != this->item_.end(); i++) {
        std::stringstream ss;
        const std::string &name = (*i).first;
        const std::string &type = (*i).second.front();

        // skip non boolean/tristate items
        if ( ! ( type.compare("boolean") ||
                 type.compare("tristate") ))
            continue;

        const std::string itemName("CONFIG_" + name);
        Item i(itemName, ITEM);

        // tristate constraints
        if (!type.compare("tristate")) {
            const std::string moduleName("CONFIG_" + name + "_MODULE");
            Item m(moduleName, ITEM);
            i.dependencies().push_front(" !" + moduleName);
            m.dependencies().push_front(" !" + itemName);
            allItems.insert(std::pair<std::string,Item>(moduleName, m));
        }

        allItems.insert(std::pair<std::string,Item>(itemName, i));
    }

    for(RsfBlocks::iterator i = this->choice_.begin(); i != this->choice_.end(); i++) {
        const std::string &itemName = (*i).first;
        const std::string &required = (*i).second.front();
        const std::string choiceName("CONFIG_" + itemName);
        Item ci(choiceName, CHOICE, required.compare("required") == 0);

        allItems.insert(std::pair<std::string,Item>(ci.name(), ci));
    }

    for(RsfBlocks::iterator i = this->depends_.begin(); i != this->depends_.end(); i++) {
        std::stringstream ss;
        const std::string &itemName = (*i).first;
        std::string &exp = (*i).second.front();

        Item item = allItems.getItem("CONFIG_" + itemName);

        ItemDb::iterator i = allItems.find("CONFIG_"+itemName);
        assert(i != allItems.end());
        (*i).second.dependencies().push_front(rewriteExpressionPrefix(exp));

    }

    for(RsfBlocks::iterator i = this->choice_item_.begin(); i != this->choice_item_.end(); i++) {
        const std::string &itemName = (*i).first;
        const std::string &choiceName = (*i).second.front();
        Item choiceItem = allItems.getItem("CONFIG_" + choiceName);
        Item item = allItems.getItem("CONFIG_" + itemName);

        ItemDb::iterator i = allItems.find("CONFIG_" + choiceName);
        assert(i != allItems.end());
        (*i).second.choiceAlternatives().push_back(item);
    }
}

std::string KconfigRsfDb::rewriteExpressionPrefix(std::string exp) {
    const std::string prefix = "CONFIG_";
    std::string separators[9] = { " ", "!", "(", ")", "=", "<", ">", "&", "|" };
    std::list<std::string> itemsExp = itemsOfString(exp);
    for(std::list<std::string>::iterator i = itemsExp.begin(); i != itemsExp.end(); i++) {
        size_t pos = 0;

        while ( (pos = exp.find(*i,pos)) != std::string::npos) {

            bool insert = false;
            for(int j = 0; j<9; j++) {
                if (pos == 0) {
                    insert = true;
                    break;
                }

                if (exp.compare(pos-1,1,separators[j]) == 0) {
                    insert = true;
                    break;
                }
            }

            if (insert)
                exp.insert(pos,prefix);

            pos += prefix.size() + (*i).size();
        }
    }
    return exp;
}

std::string KconfigRsfDb::Item::dumpChoiceAlternative() const {
    std::stringstream ret("");
    if (!isChoice() || choiceAlternatives_.size() == 0)
    return ret.str();

    ret << "(";
    for(std::deque<Item>::const_iterator i=choiceAlternatives_.begin();  i != choiceAlternatives_.end(); i++) {
    if (i != choiceAlternatives_.begin())
        ret << " || ";

    ret << (*i).name_;
    }
    ret << ")" << std::endl;
    return ret.str();
}

void KconfigRsfDb::dumpAllItems(std::ostream &out) const {
    ItemMap::const_iterator it;

    out << "I: Items-Count: "  << allItems.size()  << std::endl;
    out << "I: Format: <variable> [preconditional expression]" << std::endl;

    for(it = allItems.begin(); it != allItems.end(); it++) {
        Item item = (*it).second;
        out << item.name();
        if (item.dependencies().size() > 0) {
            out << " " << item.dependencies().join(" && ");
            if (item.isChoice()) {
                std::string ca = item.dumpChoiceAlternative();
                if (!ca.empty()) {
                    out << " && " << ca;
                }
            }
        } else {
            if (item.isChoice() && item.choiceAlternatives().size() > 0) {
                out << " " << item.dumpChoiceAlternative();
            }
        }
        out << std::endl;
    }
}

/* Returns all items (config tokens) from a string */
static std::list<std::string>
itemsOfString(std::string str) {
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
        case 'm':
        case 'n':
        case ' ':
            if (!tmp.empty()) {
                mylist.push_back(tmp);
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
