#include "KconfigRsfTranslator.h"

#include <cstdlib>
#include <list>
#include <stack>

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
