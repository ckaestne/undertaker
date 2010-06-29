#include "KconfigRsfDb.h"

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>

std::map<std::string, KconfigRsfDb::Item> KconfigRsfDb::ItemDb::whitelist;

KconfigRsfDb::KconfigRsfDb(std::ifstream &in, std::ostream &log)
    : _in(in),
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
	Item i;
	i.name_ = itemName;
	i.type_ = i.ITEM;
	allItems.insert(std::pair<std::string,Item>(itemName, i));

	// tristate constraints
	if (!type.compare("tristate")) {
		const std::string moduleName("CONFIG_" + name + "_MODULE");
		Item i;
		i.name_ = moduleName;
		i.type_ = i.ITEM;
		allItems.insert(std::pair<std::string,Item>(moduleName, i));

		allItems[itemName].dependencies_.push_front(" ! " + moduleName);
		allItems[moduleName].dependencies_.push_front(" ! " + itemName);
	}
    }

    for(RsfBlocks::iterator i = this->choice_.begin(); i != this->choice_.end(); i++) {
    	const std::string &itemName = (*i).first;
	const std::string &required = (*i).second.front();
	const std::string choiceName("CONFIG_" + itemName);
	Item ci;
	ci.name_ = choiceName;
	ci.type_ = ci.CHOICE;
	ci.required_ = required.compare("required") ? true : false;
	allItems.insert(std::pair<std::string,Item>(ci.name_, ci));
    }

    for(RsfBlocks::iterator i = this->depends_.begin(); i != this->depends_.end(); i++) {
	std::stringstream ss;
	const std::string &itemName = (*i).first;
	std::string &exp = (*i).second.front();
	std::string::size_type pos = std::string::npos;

	while ((pos = exp.find("&&")) != std::string::npos)
	    exp.replace(pos, 2, 1, '&');

	while ((pos = exp.find("||")) != std::string::npos)
	    exp.replace(pos, 2, 1, '|');
	
	Item item = allItems.getItem("CONFIG_"+itemName);

	if (item.valid()) {
	    allItems["CONFIG_"+itemName].dependencies_.push_front(rewriteExpressionPrefix(exp));
	} 
    }

    for(RsfBlocks::iterator i = this->choice_item_.begin(); i != this->choice_item_.end(); i++) {
    	const std::string &itemName = (*i).first;
	const std::string &choiceName = (*i).second.front();
        Item choiceItem = allItems.getItem(choiceName);
        Item item = allItems.getItem(itemName);
	if (item.valid() && choiceItem.valid())
	    allItems[choiceName].choiceAlternatives_.push_back(item);
    }
}

std::list<std::string> itemsOfString(std::string str);
std::string KconfigRsfDb::rewriteExpressionPrefix(std::string exp) {
    const std::string prefix = "CONFIG_";
    std::string separators[9] = { " ", "!", "(", ")", "=", "<", ">", "&", "|" };
    std::list<std::string> itemsExp = itemsOfString(exp);
    for(std::list<std::string>::iterator i = itemsExp.begin(); i != itemsExp.end(); i++) {
        unsigned int pos = 0;
	  
        while ( (pos = exp.find(*i,pos)) != std::string::npos) {

            bool insert = false;
	    for(int j=0; j<9; j++) {
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

std::string KconfigRsfDb::Item::printChoiceAlternative() {
    std::stringstream ret("");
    if (!isChoice() || choiceAlternatives_.size() == 0)
	return ret.str();

    ret << "( " << name_ << " -> (";
    for(std::deque<Item>::iterator i=choiceAlternatives_.begin();  i != choiceAlternatives_.end(); i++) {
	if (i != choiceAlternatives_.begin())
	    ret << " | ";

	ret << (*i).name_;
    }
    ret << ") )" << std::endl;
    return ret.str();
}

bool KconfigRsfDb::Item::printItemSat(std::ostream &out) {
    if (dependencies_.size() > 0) {
	out << "( " << name_ << " -> (" << dependencies_.front() << ") )" << std::endl;
	if (isChoice()) { 
	    out << "&" << std::endl;
	    out << printChoiceAlternative();
	}
	return true;
    } else {
	if (isChoice() && choiceAlternatives_.size() > 0) {
	    out << printChoiceAlternative();
	    return true;
	}
	return false;
    }
}


std::string KconfigRsfDb::Item::printItemSat() {
    std::stringstream ss("");
    printItemSat(ss);
    return ss.str();
}

void KconfigRsfDb::dumpAllItems(std::ostream &out) {
    std::map<std::string,Item>::iterator it;
    for(it = allItems.begin(); it != allItems.end(); it++) {
	Item item = (*it).second;
	if(item.printItemSat(out))
	    out << std::endl;
	else
	    out << "No dependency for item: " << (*it).first << "\n";
    }
}

void KconfigRsfDb::dumpMissing(std::ostream &out) {
    std::map<std::string,Item>::iterator it;
    out << "missing items size: " << this->allItems.missing.size() << std::endl;
    for(it = this->allItems.missing.begin(); it != this->allItems.missing.end(); it++) {
      out << "Missing item: " << (*it).first << "\n";
    }
}


std::list<std::string> itemsOfString(std::string str) {
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
	case 'n':
	case ' ':
	    if (!tmp.empty()) {
		mylist.push_back(tmp);
		//std::cout << "    (itemsOfString) inserting " << tmp << "\n";
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

void KconfigRsfDb::findSetOfInterestingItems(std::set<std::string> &initialItems) const {
    std::list<std::string> listtmp;
    std::stack<std::string> workingStack;
    std::string tmp;
    for(std::set<std::string>::iterator sit = initialItems.begin(); sit != initialItems.end(); sit++) {
        if ((*sit).compare(0,6,"CONFIG_") == 0) //consider only flags that should come from the kconfig model
	  workingStack.push(*sit);
    }

    while (!workingStack.empty()) {
	Item item = allItems.getItem(workingStack.top()); 
	workingStack.pop();
	if (item.valid()) {
	    std::string exp = item.printItemSat();//allItems.items_[*it].getDependencies(); 
	    if (!exp.empty()) {
		listtmp = itemsOfString(exp);
		for(std::list<std::string>::iterator sit = listtmp.begin(); sit != listtmp.end(); sit++) {
		    if (initialItems.find(*sit) == initialItems.end()) {
			workingStack.push(*sit); 
			initialItems.insert(*sit);
		    }
		}
	    }
	}
    }
}


int KconfigRsfDb::doIntersect(std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing) const {
    int valid_items = 0;
    bool conj = false;

    findSetOfInterestingItems(myset);

    for(std::set<std::string>::iterator it = myset.begin(); it != myset.end(); it++) {
	std::stringstream ss;
	KconfigRsfDb::Item item = allItems.getItem(*it); 
	if (item.valid()) {
	    bool go = item.printItemSat(ss);
	    if (go) {
		valid_items++;
		if (conj) {
		    out << "& ";
		}
		out << ss.str() ;
		conj = true;
	    }
	} else {
	  if (item.name_.size() > 1)
	    missing.insert(item.name_);
        }
    }
    return valid_items;
}
