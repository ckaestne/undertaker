// -*- mode: c++ -*-
#ifndef kconfigrsfdb_h__
#define kconfigrsfdb_h__

#include <string>
#include <map>
#include <deque>
#include <set>

#include "RsfBlocks.h"

class KconfigRsfDb {
public:
    KconfigRsfDb(std::ifstream &in, std::ostream &log);

    void dumpAllItems(std::ostream &out) const;
    void dumpMissing(std::ostream &out) const;
    void initializeItems();
    void findSetOfInterestingItems(std::set<std::string> &working) const;
    int doIntersect(const std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing, int &slice) const;
    std::string rewriteExpressionPrefix(std::string exp);

    struct Item {
        enum ITEMTYPE { BOOLEAN=1, TRISTATE=2, ITEM=4, CHOICE=8, INVALID=16, WHITELIST=32 };
        std::string name_;
        unsigned int type_;
        bool required_;
        std::deque<std::string> dependencies_;
        std::deque<Item> choiceAlternatives_;

        bool printItemSat(std::ostream &out) const;
        std::string printItemSat() const;
        std::string printChoiceAlternative() const;

        std::string getDependencies() const {
            if (dependencies_.size() > 0)
                return dependencies_.front();
            else
                return "";
        }

        bool isChoice() const {
            return ( (type_ & CHOICE) == CHOICE);
        }

        bool isValid() const {
            return ( (type_ & INVALID) != INVALID);
        }

        void invalidate() {
            type_ |= INVALID;
        }
    };

    typedef std::map<std::string, Item> ItemMap;

protected:

    struct ItemDb : public ItemMap {
        ItemMap missing;

        Item getItem(std::string key) const {
            Item ret;
            ItemMap::const_iterator it = this->find(key);
            if (it == this->end()) {
                if (key.compare(0,5,"COMP_") == 0) {
                    ret.name_ = key;
                    return ret;
                } else {
                    ret.invalidate();
                    ret.name_ = key;
                    return ret;
                }
            } else {
                ret = (*it).second;
                return ret;
            }
        }
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
