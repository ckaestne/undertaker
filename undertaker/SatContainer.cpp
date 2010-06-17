#include "SatContainer.h"
#include <stdexcept>
#include <sstream>

#include <cassert>
#include <cstring>

SatContainer::SatContainer(std::ifstream &in, std::ostream &log)
  : CompilationUnitInfo(in, log) {}
    

void SatContainer::parseExpressions() {
    static bool parsed = false;
    size_type pos = std::string::npos;

    if(parsed)
	return;

    for(RsfBlocks::iterator i = expressions_.begin();
	i != expressions_.end(); i++) {
	std::stringstream ss;
	int blockno;
	const std::string &blocknostr = (*i).first;
	std::string &expression = (*i).second.front();

	ss << blocknostr;
	ss >> blockno;

	// normalize conjunctions and disjunctions for sat
	while((pos = expression.find("||")) != std::string::npos)
		expression.replace(pos, 2, 1, '|');

	while((pos = expression.find("&&")) != std::string::npos)
		expression.replace(pos, 2, 1, '&');

	this->push_back(SatBlockInfo(blockno, expression.c_str()));
    }

    parsed=true;
};

SatContainer::index SatContainer::search(std::string idstring) {
    std::stringstream ss;
    index id;
    ss << idstring;
    ss >> id;
    return search(id);
}

SatContainer::index SatContainer::search(int id) {
    std::stringstream ss;
    for (unsigned int i = 0; i < size(); i++)
	if (id == bId(i))
	    return i;

    ss << "id not found: " << id;

    throw std::runtime_error(ss.str());
}


SatBlockInfo &SatContainer::item(index n) {
    assert(n < size());
    return (*this)[n];
}

int SatContainer::bId(index n) {
    assert(n < size());
    return item(n).getId();
}


std::string SatContainer::getBlockName(int index) {
    if (index == -1)
	return "";
    std::string s = "B";
    std::stringstream out;
    out << bId(index);
    s += out.str();
    return s;
}


std::string SatContainer::noPredecessor(index n, RsfBlocks &blocks) {
    std::stringstream ss;
    std::string result = "";
    ss << item(n).getId();
    std::string blockno = ss.str();

    for (RsfBlocks::iterator i = blocks.begin(); i != blocks.end(); i++) {
        std::string key = (*i).first;
        std::string value = (*i).second.front();
        if (blockno == key) {
            if (result.compare("") != 0)
		result += " | ";
            result += getBlockName(search(value));
	}
    }
    return result;
}


void SatContainer::runSat() {
    std::string bf;
    for (unsigned b = 0; b < size(); b++) {
        std::string blockName = getBlockName(b);
        std::string *pParent = nested_in_.getValue(item(b).getId());
        std::string strParent = "";
        if (b > 0)
		bf += " &\n";

        bf += "( " +  blockName + " <-> ";
        bool conjunction = false;

        if (pParent) {
                strParent += getBlockName(search(*pParent));
                bf += " (" + strParent + ") ";
                conjunction = true;
        }

        const char *strExp = item(b).expression();

        if (strExp && strncmp(strExp+1,"else",4) != 0) {
                if (conjunction)
                        bf += " & ";
                bf += " ( "+ std::string(strExp) + " ) ";
                conjunction = true;
        }

        std::string noPred = noPredecessor(b,YoungerSibl_);
        if (noPred.compare("")) {
                if (conjunction)
                        bf += " & ";
                bf += "( ! (" + noPredecessor(b,YoungerSibl_) + ") ) ";
        }

        bf += " )";
    }
    std::cout << bf << std::endl;
}

