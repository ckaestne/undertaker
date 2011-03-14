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


#include "KconfigRsfTranslator.h"
#include "StringJoiner.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>

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
        out << "UNDERTAKER_SET ALWAYS_ON";
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

