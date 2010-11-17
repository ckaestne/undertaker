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
    if(it == this->end())
        return Item("", INVALID);

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

        // tristate constraints
        if (!type.compare("tristate")) {
            const std::string moduleName("CONFIG_" + name + "_MODULE");
            Item i(itemName, ITEM | TRISTATE);
            Item m(moduleName, ITEM);

            i.dependencies().push_front("!" + moduleName);
            m.dependencies().push_front("!" + itemName);

            allItems.insert(std::pair<std::string,Item>(itemName, i));
            allItems.insert(std::pair<std::string,Item>(moduleName, m));
        } else {
            Item i(itemName, ITEM | BOOLEAN);
            allItems.insert(std::pair<std::string,Item>(itemName, i));
        }
    }

    for(RsfBlocks::iterator i = this->choice_.begin(); i != this->choice_.end(); i++) {
        const std::string &itemName = (*i).first;
        const std::string &required = (*i).second.front();
        (*i).second.pop_front();
        const std::string &type = (*i).second.front();

        if (!type.compare("boolean")) {
            const std::string choiceName("CONFIG_" + itemName);
            Item ci(choiceName, CHOICE, required.compare("required") == 0);
            allItems.insert(std::pair<std::string,Item>(ci.name(), ci));
        } else if (!type.compare("tristate")){
            const std::string choiceName("CONFIG_" + itemName);
            const std::string choiceModuleName("CONFIG_" + itemName + "_MODULE");

            Item ci(choiceName, CHOICE | TRISTATE, required.compare("required") == 0);
            Item cmi(choiceModuleName, CHOICE, required.compare("required") == 0);

            ci.dependencies().push_front("!" + choiceModuleName);
            cmi.dependencies().push_front("!" + choiceName);

            allItems.insert(std::pair<std::string,Item>(ci.name(), ci));
            allItems.insert(std::pair<std::string,Item>(cmi.name(), cmi));
        }
    }


    for(RsfBlocks::iterator i = this->choice_item_.begin(); i != this->choice_item_.end(); i++) {
        const std::string &itemName = (*i).first;
        const std::string &choiceName = (*i).second.front();

        ItemDb::iterator i = allItems.find("CONFIG_" + choiceName);
        assert(i != allItems.end());


        Item item("CONFIG_" + itemName, ITEM);
        allItems.insert(std::pair<std::string,Item>(item.name(), item));
        (*i).second.choiceAlternatives().push_back(item);

        if ((*i).second.isTristate()) {
            ItemDb::iterator i = allItems.find("CONFIG_" + choiceName + "_MODULE");
            assert(i != allItems.end());
            (*i).second.choiceAlternatives().push_front(item);
        }
    }

    for(RsfBlocks::iterator i = this->depends_.begin(); i != this->depends_.end(); i++) {
        std::stringstream ss;
        const std::string &itemName = (*i).first;
        std::string &exp = (*i).second.front();

        Item item = allItems.getItem("CONFIG_" + itemName);

        ItemDb::iterator i = allItems.find("CONFIG_"+itemName);
        assert(i != allItems.end());
        std::string rewritten = "(" + rewriteExpressionPrefix(exp) + ")";
        (*i).second.dependencies().push_front(rewritten);

        /* Add dependency also if item is tristate to
           CONFIG_..._MODULE */
        if (item.isTristate()) {
            ItemDb::iterator i = allItems.find("CONFIG_" + itemName + "_MODULE");
            assert(i != allItems.end());
            (*i).second.dependencies().push_front(rewritten);
        }


    }
}


static size_t
replace_item(std::string &exp, std::string fmt, const std::string item, size_t start_pos, size_t additional_consume = 0) {
    size_t pos = 0;
    while ( (pos = fmt.find("%%", pos)) != std::string::npos) {
        fmt.replace(pos, 2, item);
        pos += item.size() - 2;
    }

    exp.replace(start_pos, item.size() + additional_consume, fmt);
    return fmt.size();
}

std::string KconfigRsfDb::rewriteExpressionPrefix(std::string exp) {
    std::string separators[9] = { " ", "!", "(", ")", "=", "<", ">", "&", "|" };
    std::list<std::string> itemsExp = itemsOfString(exp);

    for(std::list<std::string>::iterator i = itemsExp.begin(); i != itemsExp.end(); i++) {
        Item item = allItems.getItem("CONFIG_" + *i);
        const bool tristate = item.isValid() && item.isTristate();
        size_t pos = 0;

        while ( (pos = exp.find(*i,pos)) != std::string::npos) {

            bool sep_before = false;
            bool sep_after = false;

            if (pos == 0)
                sep_before = true;
            if (pos + i->size() == exp.size())
                sep_after = true;

            /* Nine separators */
            for(int j = 0; j<9; j++) {
                if (pos != 0 && exp.compare(pos - 1,1, separators[j]) == 0) {
                    sep_before = true;
                }
                if (exp.compare(pos + i->size(), 1, separators[j]) == 0) {
                    sep_after = true;
                }
                if (sep_before && sep_after)
                    break;
            }

            if (sep_before && sep_after) {
                /* Check if character after token isn't a blank or a )
                   => do not rewrite it here */
                size_t additional_consume = 0;
                enum { NOP,
                       NEQUALS_N, NEQUALS_Y, NEQUALS_M,
                       EQUALS_N, EQUALS_Y, EQUALS_M,
                       EQUALS_SYMBOL,
                } state = NOP;

                int p = pos + (*i).size();
                if (exp.compare(p, 3, "!=n") == 0) {
                    state = NEQUALS_N;
                    additional_consume = 3;
                } else if (exp.compare(p, 3, "!=y") == 0) {
                    state = NEQUALS_Y;
                    additional_consume = 3;
                } else if (exp.compare(p, 3, "!=m") == 0) {
                    state = NEQUALS_M;
                    additional_consume = 3;
                    /* EQUALS */
                } else if (exp.compare(p, 2, "=n") == 0) {
                    state = EQUALS_N;
                    additional_consume = 2;
                } else if (exp.compare(p, 2, "=y") == 0) {
                    state = EQUALS_Y;
                    additional_consume = 2;
                } else if (exp.compare(p, 2, "=m") == 0) {
                    state = EQUALS_M;
                    additional_consume = 2;
                } else if (exp.compare(p, 1, "=") == 0 || (pos != 0 && exp.compare(pos - 1, 1, "=") == 0)) {
                    /* Something completly fucked up like
                       CONFIG_A=CONFIG_B */
                    state = EQUALS_SYMBOL;
                } else if (tristate) {
                    /* No Postfix, but a tristate */
                    state = NEQUALS_N;
                }

                switch (state) {
                case NEQUALS_N:
                    pos += replace_item(exp, "(CONFIG_%%_MODULE || CONFIG_%%)", *i, pos, additional_consume);
                    break;
                case NEQUALS_M:
                    pos += replace_item(exp, "!CONFIG_%%_MODULE", *i, pos, additional_consume);
                    break;
                case NEQUALS_Y:
                    pos += replace_item(exp, "!CONFIG_%%", *i, pos, additional_consume);
                    break;
                case EQUALS_N:
                    pos += replace_item(exp, "(!CONFIG_%%_MODULE && !CONFIG_%%)", *i, pos, additional_consume);
                    break;
                case EQUALS_M:
                    pos += replace_item(exp, "CONFIG_%%_MODULE", *i, pos, additional_consume);
                    break;
                case EQUALS_Y:
                case EQUALS_SYMBOL:
                case NOP:
                    pos += replace_item(exp, "CONFIG_%%", *i, pos, additional_consume);
                    break;
                }

                if (state == EQUALS_SYMBOL) {
                    pos += 4;
                }
            }
            pos += 1;
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
    ret << ")";
    return ret.str();
}

void KconfigRsfDb::dumpAllItems(std::ostream &out) const {
    ItemMap::const_iterator it;

    out << "I: Items-Count: "  << allItems.size()  << std::endl;
    out << "I: Format: <variable> [presence condition]" << std::endl;

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
