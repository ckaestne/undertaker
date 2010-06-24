#include <sstream>

#include "SatContainer.h"
#include "Ziz.h"


struct StringJoiner : public std::deque<std::string> {
    std::string join(const char *j) {
	std::stringstream ss;
	if (size() == 0)
	    return "";
	
	ss << front();
	pop_front();
	
	while (size() > 0) {
	    ss << j << front();
	    pop_front();
	}
	return ss.str();
    }

    void push_back(const value_type &x) {
	if (x.compare("") == 0)
	    return;
	std::deque<value_type>::push_back(x);
    }
};

const char *SatBlockInfo::expression() {

    if (_expression)
        return _expression;

    std::string expression = _cb->ExpressionStr();
    std::string::size_type pos = std::string::npos;

    // normalize conjunctions and disjunctions for sat
    while((pos = expression.find("||")) != std::string::npos)
        expression.replace(pos, 2, 1, '|');

    while((pos = expression.find("&&")) != std::string::npos)
        expression.replace(pos, 2, 1, '&');

    return _expression = strdup(expression.c_str());
}

SatBlockInfo::~SatBlockInfo() {
    if (_expression)
        free(_expression);
}

SatContainer::SatContainer(const char *filename) {
    Ziz::Parser parser;

    try {
        _zfile = parser.Parse(filename);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
    } catch(...) {
        std::cerr << "caught exception" << std::endl;
    }
}

SatContainer::~SatContainer() {
    delete _zfile;
}


int SatContainer::scanBlocks(Ziz::BlockContainer *bc) {
    Ziz::ConditionalBlock::iterator i;
    int count = 0;

    if (!bc && bc->size() == 0)
	return 0;

    for (i = bc->begin(); i != bc->end(); i++) {
	Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
	if (cb) {
	    count++;
	    this->push_back(SatBlockInfo(cb));
	    count += this->scanBlocks(cb);
	}
    }

    return count;
}

void SatContainer::parseExpressions() {
    if(size() > 0)
	return;

    this->scanBlocks(_zfile);
};


int SatContainer::bId(index n) {
    return item(n).getId();
}


SatContainer::index SatContainer::search(int id) {

    std::stringstream ss;
    for (index i = 0; i < size(); i++)
	if (id == bId(i))
	    return i;
    ss << "id not found: " << id;
    throw std::runtime_error(ss.str());
}


SatBlockInfo &SatContainer::item(index n) {
    return (*this)[n];
}


std::string SatContainer::getBlockName(index n) {
    std::stringstream ss;
    ss << "B" << bId(n);
    return ss.str();
}

std::string SatContainer::parent(index n) {
    std::stringstream ss;

    const Ziz::ConditionalBlock *b = item(n).Block();
    if((b = b->ParentCondBlock())) {
	ss << " ( " << getBlockName(search(b->Id())) << " ) ";
    }
    return ss.str();
}

std::string SatContainer::expression(index n) {
    return item(n).expression();
}

std::string SatContainer::noPredecessor(index n) {
    std::stringstream ss;

    const Ziz::ConditionalBlock *b = item(n).Block();
    StringJoiner sj;

    while(b->PrevSibling()) {
	sj.push_back(getBlockName(search(b->PrevSibling()->Id())));
	b = b->PrevSibling();
    };

    if (sj.size() > 0)
	return  "( ! (" + sj.join(" | ") + ") ) ";
    else
	return "";
}

std::string SatContainer::runSat() {
    StringJoiner sj;
    for (index i = 0; i < size(); i++) {
	StringJoiner pc;
	pc.push_back(parent(i));
	pc.push_back(expression(i));
	pc.push_back(noPredecessor(i));
	
        sj.push_back("( " + getBlockName(i) + " <-> " + pc.join(" & ") + " )");
    }
    return sj.join("\n& ");
}
