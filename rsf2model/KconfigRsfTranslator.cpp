#include "KconfigRsfTranslator.h"
#include "StringJoiner.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>

static std::list<std::string> itemsOfString(std::string str);

KconfigRsfTranslator::Item KconfigRsfTranslator::ItemDb::invalid_item("VAMOS_INAVLID", INVALID);

KconfigRsfTranslator::Item::Item(std::string name, unsigned type, bool required)
    : name_(name), type_(type), required_(required) { }

KconfigRsfTranslator::Item KconfigRsfTranslator::ItemDb::getItem(const std::string &key) const {
    ItemMap::const_iterator it = this->find(key);

    /* We only request config items, which are defined in the rsf.
       Be aware that no slicing is done here */
    if(it == this->end())
        return Item(invalid_item);

    return (*it).second;
}

KconfigRsfTranslator::Item &KconfigRsfTranslator::ItemDb::getItemReference(const std::string &key) {
    ItemMap::iterator it = this->find(key);

    /* We only request config items, which are defined in the rsf.
       Be aware that no slicing is done here */
    if(it == this->end())
        return invalid_item;

    return (*it).second;
}

KconfigRsfTranslator::KconfigRsfTranslator(std::ifstream &in, std::ostream &log)
    : _in(in),
      /* This are RsfBlocks! */
      choice_(in, "Choice", log),
      choice_item_(in, "ChoiceItem", log),
      depends_(in, "Depends", log),
      item_(in, "Item", log),
      defaults_(in, "Default", log),
      has_prompts_(in, "HasPrompts", log)
{}

void KconfigRsfTranslator::initializeItems() {

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
            /* Every _MODULE depends on the magic flag MODULES within
               kconfig */
            m.dependencies().push_front("CONFIG_MODULES");
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
        std::deque<std::string>::iterator iter = i->second.begin();
        const std::string &required = *iter;
        iter++;
        const std::string &type = *iter;


        const std::string choiceName("CONFIG_" + itemName);
        bool tristate = type.compare("tristate") == 0;
        Item ci(choiceName, CHOICE | (tristate ? TRISTATE : 0),
                required.compare("required") == 0);
        allItems.insert(std::pair<std::string,Item>(ci.name(), ci));
    }

    for(RsfBlocks::iterator i = this->choice_item_.begin(); i != this->choice_item_.end(); i++) {
        const std::string &itemName = (*i).first;
        const std::string &choiceName = (*i).second.front();

        ItemDb::iterator i = allItems.find("CONFIG_" + choiceName);
        assert(i != allItems.end());


        Item item("CONFIG_" + itemName, ITEM);
        allItems.insert(std::pair<std::string,Item>(item.name(), item));
        (*i).second.choiceAlternatives().push_back(item);
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
        if (item.isTristate() && !item.isChoice()) {
            ItemDb::iterator i = allItems.find("CONFIG_" + itemName + "_MODULE");
            assert(i != allItems.end());
            (*i).second.dependencies().push_front(rewritten);
        }
    }
    for(RsfBlocks::iterator i = this->defaults_.begin(); i != this->defaults_.end(); i++) {
        const std::string &itemName = (*i).first;
        Item &item = allItems.getItemReference("CONFIG_" + itemName);

        /* Ignore tristates and choices for now */
        if (item.isTristate() || item.isChoice()) continue;

        /* Check if the symbol has an prompt. If it has, skip all
           default entries */
        std::string *has_prompts = this->has_prompts_.getValue(itemName);
        assert(has_prompts);
        if (*has_prompts != "0") {
            //            std::cerr << itemName << " has prompts" << std::endl;
            continue;
        }

        // Format in Kconfig: default expr if visible_expr
        std::deque<std::string>::iterator iter = i->second.begin();
        std::string &expr = *iter;
        iter++;
        std::string &visible_expr = *iter;

        //        std::cerr << itemName << " " << (expr == "y") << " " << (visible_expr =="y") << std::endl;

        if (expr == "y" && visible_expr == "y") {
            /* These options are always on */
            alwaysOnItems.push_back(item);
        } else if (expr == "y" || visible_expr == "y") {
            /* if there is only one side y, we can treat this as
               normal dependency */
            std::string &dependency = expr;
            if (expr == "y")
                dependency = visible_expr;

            std::string rewritten = "(" + rewriteExpressionPrefix(dependency) + ")";

            item.dependencies().push_front(rewritten);
        }
    }

}


static size_t
replace_item(std::string &exp, std::string fmt, size_t start_pos,
             size_t consume, const std::string item1, const std::string item2 = "") {
    size_t pos = 0;
    while ( (pos = fmt.find("%1", pos)) != std::string::npos) {
        fmt.replace(pos, 2, item1);
        pos += item1.size() - 2;
    }
    if (item2.compare("") != 0) {
        pos = 0;
        while ( (pos = fmt.find("%2", pos)) != std::string::npos) {
            fmt.replace(pos, 2, item2);
            pos += item2.size() - 2;
        }
    }

    exp.replace(start_pos, consume, fmt);
    return fmt.size();
}


KconfigRsfTranslator::rewriteAction
KconfigRsfTranslator::rewriteExpressionIdentify(const std::string &exp, const std::string &item, const std::string &next,
                                        size_t item_pos, size_t &consume) {

    rewriteAction action = NORMAL;
    int p = item_pos + item.size();
    consume = item.size();

    if (exp.compare(p, 3, "!=n") == 0) {
        action = NEQUALS_N;
        consume += 3;
    } else if (exp.compare(p, 3, "!=y") == 0) {
        action = NEQUALS_Y;
        consume += 3;
    } else if (exp.compare(p, 3, "!=m") == 0) {
        action = NEQUALS_M;
        consume += 3;
        /* EQUALS */
    } else if (exp.compare(p, 2, "=n") == 0) {
        action = EQUALS_N;
        consume += 2;
    } else if (exp.compare(p, 2, "=y") == 0) {
        action = EQUALS_Y;
        consume += 2;
    } else if (exp.compare(p, 2, "=m") == 0) {
        action = EQUALS_M;
        consume += 2;
    } else if (exp.compare(p, 1, "=") == 0) {
        /*  CONFIG_A=CONFIG_B */
        /* We have to save the left and the right side of
           the equal sign */
        action = EQUALS_SYMBOL;
        if (next == "")
            action = NORMAL;
        else {
            consume = item.size() + 1 + next.size();
        }
    } else if (exp.compare(p, 2, "!=") == 0) {
        /* CONFIG_A!=CONFIG_B */
        /* We have to save the left and the right side of
           the unequal sign */
        action = NEQUALS_SYMBOL;
        if (next == "")
            action = NORMAL;
        else {
            consume = item.size() + 2 + next.size();
        }
    }

    return action;
}



std::string KconfigRsfTranslator::rewriteExpressionPrefix(std::string exp) {
    std::string separators[] = {"(", ")", " ", "!", "=", "<", ">", "&", "|" };
    std::list<std::string> itemsExp = itemsOfString(exp);

    size_t pos = 0;
    for(std::list<std::string>::iterator i = itemsExp.begin(); i != itemsExp.end(); ++i) {
        Item item = allItems.getItem("CONFIG_" + *i);
        const bool tristate = item.isValid() && item.isTristate() && !item.isChoice();
        pos = exp.find(*i, pos);
        /* Item not found, we are at the end of the string */
        if (pos == std::string::npos) break;

        std::string *sep_before = 0;
        std::string *sep_after = 0;

        if (pos == 0)
            sep_before = &separators[0]; // ")"
        if (pos + i->size() == exp.size())
            sep_after = &separators[1]; // ")"

        for(unsigned int j = 0;
            j < (sizeof(separators)/sizeof(*separators));
            j++) {
            if (pos != 0 && exp.compare(pos - 1,1, separators[j]) == 0) {
                sep_before = &separators[j];
            }
            if (exp.compare(pos + i->size(), 1, separators[j]) == 0) {
                sep_after = &separators[j];
            }
            if (sep_before && sep_after)
                break;
        }

        if (sep_before && sep_after) {
            size_t consume;
            std::string emptyString;
            std::string &next = emptyString, &item = *i;
            ++i;
            if (itemsExp.end() != i) {
                next = *i;
                --i;
            }

            rewriteAction action = rewriteExpressionIdentify(exp, item, next, pos, consume);

            switch (action) {
            case NEQUALS_N:
                pos += replace_item(exp, "(CONFIG_%1_MODULE || CONFIG_%1)", pos, consume, item);
                break;
            case NEQUALS_M:
                pos += replace_item(exp, "!CONFIG_%1_MODULE", pos, consume, item);
                break;
            case NEQUALS_Y:
                pos += replace_item(exp, "!CONFIG_%1", pos, consume, item);
                break;
            case EQUALS_N:
                pos += replace_item(exp, "(!CONFIG_%1_MODULE && !CONFIG_%1)", pos, consume, item);
                break;
            case EQUALS_M:
                pos += replace_item(exp, "CONFIG_%1_MODULE", pos, consume, item);
                break;
            case EQUALS_SYMBOL:
                /* it is intended to modify the loop iterator
                   here, to jump over the next item */
                ++i;
                pos += replace_item(exp, "((CONFIG_%1 && CONFIG_%2) || "
                                    "(CONFIG_%1_MODULE && CONFIG_%2_MODULE) || "
                                    "(!CONFIG_%1 && !CONFIG_%2 && "
                                    "!CONFIG_%1_MODULE && !CONFIG_%2_MODULE))",
                                    pos, consume, item, next);
                break;
            case NEQUALS_SYMBOL:
                /* it is intended to modify the loop iterator
                   here, to jump over the next item */
                ++i;
                pos += replace_item(exp, "((CONFIG_%1 && !CONFIG_%2) || "
                                    "(CONFIG_%1_MODULE && !CONFIG_%2_MODULE) || "
                                    "(!CONFIG_%1 && CONFIG_%2 && "
                                    "!CONFIG_%1_MODULE && CONFIG_%2_MODULE))",
                                    pos, consume, item, next);
                break;
            case EQUALS_Y:
                pos += replace_item(exp, "CONFIG_%1", pos, consume, item);
                break;
            case NORMAL:
                if (tristate) {
                    pos += replace_item(exp, "(CONFIG_%1_MODULE || CONFIG_%1)", pos, consume, item);
                } else {
                    pos += replace_item(exp, "CONFIG_%1", pos, consume, item);
                }
                break;
            }
        }
    }

    return exp;
}

std::string KconfigRsfTranslator::Item::dumpChoiceAlternative() const {
    std::stringstream ret("");

    if (!isChoice() || choiceAlternatives_.size() == 0)
        return ret.str();

    /* For not tristate choices we will simply exclude the single
       choice items, only one can be true and exactly one is true:
       (A ^ B  ^ C) = (A & !B & !C) || (!A & B & !C) || (!A & !B & C)

       If we are tristate also none of the options can be selected. So
       we add || (CONFIG_MODULES & !A & !B & !C)
    */

    StringJoiner orClause;

    for (unsigned int isTrue = 0; isTrue < choiceAlternatives_.size(); isTrue++) {
        /* (A & !B & !C) -> isTrue == 0 */
        unsigned int count = 0;
        StringJoiner andClause;
        for(std::deque<Item>::const_iterator i = choiceAlternatives_.begin();
            i != choiceAlternatives_.end(); ++i, count++) {

            andClause.push_back(((count == isTrue) ? "" : "!") + i->name());
        }
        orClause.push_back("(" + andClause.join(" && ") + ")");
    }

    if (isTristate()) {
        StringJoiner lastClause;
        /* If the choice is optional, also without MODULES all options
           can be off */
        if (isRequired())
            lastClause.push_back("CONFIG_MODULES");
        for(std::deque<Item>::const_iterator i = choiceAlternatives_.begin();
            i != choiceAlternatives_.end(); ++i) {

            lastClause.push_back("!" + i->name());
        }
        orClause.push_back("(" + lastClause.join(" && ") + ")");
    }



    return "(" + orClause.join(" || ") + ")";
}

void KconfigRsfTranslator::dumpAllItems(std::ostream &out) const {
    ItemMap::const_iterator it;

    out << "I: Items-Count: "  << allItems.size()  << std::endl;
    out << "I: Format: <variable> [presence condition]" << std::endl;
    if (!alwaysOnItems.empty()) {
        /* Handle the always on options */
        out << "ALWAYS_ON";
        for (std::list<Item>::const_iterator it = alwaysOnItems.begin(); it != alwaysOnItems.end(); ++it) {
            Item item = *it;
            out << " \"" << item.name() << "\"";
        }
        out << std::endl;
    }

    for(it = allItems.begin(); it != allItems.end(); it++) {
        Item item = (*it).second;
        out << item.name();
        if (item.dependencies().size() > 0) {
            out << " \"" << item.dependencies().join(" && ");
            if (item.isChoice()) {
                std::string ca = item.dumpChoiceAlternative();
                if (!ca.empty()) {
                    out << " && " << ca;
                }
            }
            out << "\"";
        } else {
            if (item.isChoice() && item.choiceAlternatives().size() > 0) {
                out << " \"" << item.dumpChoiceAlternative() << "\"";
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

    return mylist;
}
