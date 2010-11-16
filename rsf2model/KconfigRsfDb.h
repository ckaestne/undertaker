// -*- mode: c++ -*-
#ifndef kconfigrsfdb_h__
#define kconfigrsfdb_h__

#include <string>
#include <map>
#include <deque>
#include <set>

#include "RsfBlocks.h"
#include "StringJoiner.h"

class KconfigRsfDb {
public:
    enum ITEMTYPE { BOOLEAN=1, TRISTATE=2, ITEM=4, CHOICE=8, INVALID=16, WHITELIST=32 };

    KconfigRsfDb(std::ifstream &in, std::ostream &log);

    void dumpAllItems(std::ostream &out) const;
    void initializeItems();
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
        ItemMap missing;
        Item getItem(std::string key) const;        
    };

    ItemDb allItems;

    const RsfBlocks &choice() const { return choice_; }
    const RsfBlocks &choice_item() const { return choice_item_; }
    const RsfBlocks &depends() const { return depends_; }
    const RsfBlocks &item() const { return item_; }

    std::ifstream &_in;
    RsfBlocks choice_;
    RsfBlocks choice_item_;
    RsfBlocks depends_;
    RsfBlocks item_;
};

#endif
