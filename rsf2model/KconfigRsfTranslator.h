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


// -*- mode: c++ -*-
#ifndef kconfigrsftranlator_h__
#define kconfigrsftranslator_h__

#include <string>
#include <map>
#include <list>
#include <deque>
#include <set>

#include "RsfBlocks.h"
#include "StringJoiner.h"
 
class KconfigRsfTranslator {
public:
    enum ITEMTYPE { BOOLEAN=1, TRISTATE=2, ITEM=4, CHOICE=8, INVALID=16, WHITELIST=32 };

    KconfigRsfTranslator(std::ifstream &in, std::ostream &log);

    void dumpAllItems(std::ostream &out) const;
    void initializeItems();

    enum rewriteAction { NORMAL,
           NEQUALS_N, NEQUALS_Y, NEQUALS_M,
           EQUALS_N, EQUALS_Y, EQUALS_M,
           EQUALS_SYMBOL, NEQUALS_SYMBOL
    };


    /* These are defined in KconfigRsfTranslatorRewrite.cpp */
    rewriteAction rewriteExpressionIdentify(const std::string &exp,const std::string &item,
                                            const std::string &next,
                                            size_t item_pos, size_t &consume);

    std::string rewriteExpressionPrefix(std::string exp);

    struct Item {
        Item(std::string name, unsigned type=ITEM, bool required=false);

        bool printItemSat(std::ostream &out) const;
        std::string printItemSat() const;
        std::string dumpChoiceAlternative() const;
        std::string getDependencies() const;
        bool isChoice() const { return ( (type_ & CHOICE) == CHOICE); }
        bool isTristate() const { return ( (type_ & TRISTATE) == TRISTATE); }
        bool isValid() const { return ( (type_ & INVALID) != INVALID); }
        bool isRequired() const { return required_; }

        std::string const& name() const { return name_; }
        StringJoiner &dependencies() { return dependencies_; }
        std::deque<Item> &choiceAlternatives()  { return choiceAlternatives_; }

    private:
        std::string name_;
        unsigned  type_;
        bool required_;

        StringJoiner dependencies_;
        std::deque<Item> choiceAlternatives_;
    };

    typedef std::map<std::string, Item> ItemMap;

protected:
    struct ItemDb : public ItemMap {
        static Item invalid_item;
        ItemMap missing;
        Item getItem(const std::string &key) const;
        Item &getItemReference(const std::string &key);
    };

    ItemDb allItems;

    const RsfBlocks &choice() const { return choice_; }
    const RsfBlocks &choice_item() const { return choice_item_; }
    const RsfBlocks &depends() const { return depends_; }
    const RsfBlocks &item() const { return item_; }
    const RsfBlocks &defaults() const { return defaults_; }
    const RsfBlocks &has_prompts() const { return has_prompts_; }

    std::ifstream &_in;
    RsfBlocks choice_;
    RsfBlocks choice_item_;
    RsfBlocks depends_;
    RsfBlocks item_;
    RsfBlocks defaults_;
    RsfBlocks has_prompts_;

    std::list<Item> alwaysOnItems;

};

#endif
