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

    struct Item {	
	enum ITEMTYPE { BOOLEAN=1, TRISTATE=2, ITEM=4, CHOICE=8, INVALID=16, WHITELIST=32 };
	std::string name_;
	unsigned int type_;
	bool required_;
	std::deque<std::string> dependencies_;
	std::deque<Item> choiceAlternatives_;

	bool printItemSat(std::ostream &out);
	std::string printItemSat();
	std::string printChoiceAlternative();

	std::string getDependencies() {
	    if (dependencies_.size() > 0)
		return dependencies_.front();
	    else
		return "";
	}

	bool isChoice() {
	    return ( (type_ & CHOICE) == CHOICE);
	}

	bool valid() {
	    return ( (type_ & INVALID) != INVALID);
	}
    
	void invalidate() {
	    type_ |= INVALID;
	}
    };


    struct ItemDb : public std::map<std::string, Item> {
        static std::map<std::string, Item> whitelist;
        std::map<std::string, Item> missing;
	Item getItem(std::string key) const {
	    Item ret;
	    std::map<std::string, Item>::const_iterator it = this->find(key);
	    if (it == this->end()) { 
	      if (key.compare(0,5,"COMP_") == 0) {
	        ret.name_ = key;
		return ret;
	      } else {
	        std::map<std::string, Item>::const_iterator it = this->whitelist.find(key);
	        if (it != this->whitelist.end()) {
		  return (*it).second;
		}
                ret.invalidate();
		ret.name_ = key;
		return ret;
              }
	    } else {
                ret = (*it).second;
		return ret;
	    }
	}
      
        void addToWhitelist(std::string name) {
	  Item item;
	  item.name_ = name;
	  item.type_ =  Item::WHITELIST;
          this->whitelist.insert(std::make_pair(item.name_,item));
        }
    };

    ItemDb allItems;

    KconfigRsfDb(std::ifstream &in, std::ostream &log);

    void dumpAllItems(std::ostream &out);
    void dumpMissing(std::ostream &out);
    void initializeItems();
    void findSetOfInterestingItems(std::set<std::string> &working) const;
    int doIntersect(const std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing) const;
    std::string rewriteExpressionPrefix(std::string exp);

    const RsfBlocks &choice() { return choice_; }
    const RsfBlocks &choice_item() { return choice_item_; }
    const RsfBlocks &depends() { return depends_; }
    const RsfBlocks &item() { return item_; }
	
    std::ifstream &_in;
    RsfBlocks choice_;
    RsfBlocks choice_item_;
    RsfBlocks depends_;
    RsfBlocks item_;
};

#endif

