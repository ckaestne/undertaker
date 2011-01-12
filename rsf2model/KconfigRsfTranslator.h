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
