#include <sstream>

#include "CloudContainer.h"
#include "Ziz.h"

// #define DEBUG

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

const char *ZizCondBlockPtr::expression() const {

    if (_expression)
        return _expression;

    std::string expression = _cb->ExpressionStr();
    std::string::size_type pos = std::string::npos;

    // normalize conjunctions and disjunctions for sat
    while((pos = expression.find("||")) != std::string::npos)
        expression.replace(pos, 2, 1, '|');

    while((pos = expression.find("&&")) != std::string::npos)
        expression.replace(pos, 2, 1, '&');

    if (_cb->CondBlockType() == Ziz::Ifndef)
	expression = " ! " + expression;

    return _expression = strdup(expression.c_str());
}

ZizCondBlockPtr::~ZizCondBlockPtr() {
    if (_expression != NULL)
        free(_expression);
}

BlockCloud::BlockCloud() : _constraints(NULL) {} 
BlockCloud::BlockCloud(Ziz::ConditionalBlock *cb) : _constraints(NULL) {
    try {
	this->scanBlocks(cb);
    } catch (std::runtime_error &e) {
	std::cerr << "failed to parse a Blockcontainer with "
		  << cb->size() << " Blocks" << std::endl
		  << e.what() << std::endl;
	exit(EXIT_FAILURE);
    }
}

BlockCloud::~BlockCloud() {
    if (_constraints != NULL)
	delete _constraints;
}

const std::string& BlockCloud::getConstraints() const {
    StringJoiner sj;

    if (_constraints)
	return *_constraints;

    for (index i = 0; i < size(); i++) {
	StringJoiner pc;
	pc.push_back(parent(i));
	pc.push_back(expression(i));
	pc.push_back(noPredecessor(i));
	
        sj.push_back("( " + getBlockName(i) + " <-> " + pc.join(" & ") + " )");
    }
    _constraints = new std::string(sj.join("\n& "));
				  
    return *_constraints;
}

int BlockCloud::scanBlocks(Ziz::BlockContainer *bc) {
    Ziz::ConditionalBlock::iterator i;
    int count = 0;

    if (!bc && bc->size() == 0)
	return 0;

    for (i = bc->begin(); i != bc->end(); i++) {
	Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
	if (cb) {
	    count++;
#ifdef DEBUG
	    std::cout << "Adding nested   block " << cb->Id() << " to " << this << " at: " << cb->Start() << std::endl;
#endif
	    this->push_back(ZizCondBlockPtr(cb));
	    count += this->scanBlocks(cb);
	}
    }

    return count;
}

int BlockCloud::bId(index n) const {
    return item(n).getId();
}


BlockCloud::index BlockCloud::search(int id) const {

    std::stringstream ss;
    for (index i = 0; i < size(); i++)
	if (id == bId(i))
	    return i;
    ss << "id not found: " << id;
    throw std::runtime_error(ss.str());
}


const ZizCondBlockPtr &BlockCloud::item(index n) const {
    return (*this)[n];
}


std::string BlockCloud::getBlockName(index n) const {
    std::stringstream ss;
    ss << "B" << bId(n);
    return ss.str();
}

std::string BlockCloud::parent(index n) const {
    std::stringstream ss;

    const Ziz::ConditionalBlock *b = item(n).Block();
    if((b = b->ParentCondBlock())) {
	ss << " ( " << getBlockName(search(b->Id())) << " ) ";
    }
    return ss.str();
}

std::string BlockCloud::expression(index n) const {
    return item(n).expression();
}

std::string BlockCloud::noPredecessor(index n) const {
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


CloudContainer::CloudContainer(const char *filename)
  : _zfile(NULL), _fail(false), _constraints(NULL) {
    Ziz::Parser parser;

    try {
        _zfile = parser.Parse(filename);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
	_fail = true;
    } catch(...) {
        std::cerr << "caught exception" << std::endl;
	_fail = true;
    }
}

CloudContainer::~CloudContainer() {
    if(_zfile)
	delete _zfile;
    if(_constraints != NULL)
	delete _constraints;
}

std::map<std::string, std::string> CloudContainer::getParents() {
    std::map<std::string, std::string> ret;
    for (CloudList::iterator c = this->begin(); c != this->end(); c++) {
        for (unsigned int i = 0; i < c->size(); i++) {
            std::string papa(c->parent(i));
            if (!papa.empty()) {
	        std::string me(c->getBlockName(i));  
                ret.insert(std::pair<std::string,std::string>(me, papa));
            }
        }
    }
    return ret;
}

const std::string& CloudContainer::getConstraints() {
    StringJoiner sj;

    if (!_zfile)
	return *_constraints;

    if (_constraints)
	return *_constraints;

    if (size() == 0) { // we didn't scan for conditional blocks yet
	Ziz::BlockContainer::iterator i;
	for (i = _zfile->begin(); i != _zfile->end(); i++) {
	    Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
	    if(cb) {

		// a cloud consists of top level conditional blocks but include all
		// sister blocks!

		if (size() == 0 || (cb->PrevSibling() == NULL)) // start new cloud if no prev sibling
		    this->push_back(BlockCloud());

#ifdef DEBUG		    
		std::cout << "Adding toplevel block " << cb->Id() << " to cloud " << &(this->back())
			  << " (#" << size() << ")"
			  << std::endl;
#endif

		this->back().push_back(ZizCondBlockPtr(cb));
		this->back().scanBlocks(cb);
	    }
	}
    }

    unsigned int cloudno = 0;

    for (CloudList::iterator c = this->begin(); c != this->end(); c++) {

	try {    
	    sj.push_back(c->getConstraints());
	    cloudno++;
	} catch (std::runtime_error &e) {

	    std::cerr << "failed to process cloud no: "  << cloudno << std::endl
		      << e.what() << std::endl;
	}
    }
    
    _constraints = new std::string(sj.join("\n& "));
    return *_constraints;
}
