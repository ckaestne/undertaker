#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include "VariableToBddMap.h"

size_t VariableToBddMap::count_;
int VariableToBddMap::variables;
int VariableToBddMap::blocks;
bool VariableToBddMap::debug;

void VariableToBddMap::do_insert(std::string s) {
    if ((size_t) bdd_varnum() == count_) {
	static int extra = 100;
	std::cerr << "inserting variable " << s
		  << ", current size is now: " << bdd_varnum()
		  << ", extending number of bdd variables by " << extra
		  << std::endl;
	bdd_extvarnum(extra++);
	extra += 100;
	bdd_reorder(BDD_REORDER_WIN3ITE);
    }

    VariableEntry e(s, bdd_ithvar(count_++));
    push_back(e);
    assert(count_ == this->size());
    if (debug)
	std::cout << "   do_insert-> " << s << std::endl;
}

bdd VariableToBddMap::make_block_bdd(unsigned blockno) {
    std::stringstream ss;

    ss << "Block_" << blockno;
    if ((size_t)bdd_varnum() == count_) {
	static int extra = 100;
	std::cerr << "inserting block no: " << blockno
		  << ", current size is now: " << bdd_varnum()
		  << ", extending number of bdd variables by " << extra
		  << std::endl;
	bdd_extvarnum(extra++);
	extra += 100;
	bdd_reorder(BDD_REORDER_SIFT);
    }

    blocks++;
    bdd ret= bdd_ithvar(count_++);
    VariableEntry e(ss.str(), ret);
    push_back(e);
    return ret;
}

bdd VariableToBddMap::add_new_variable(std::string s) {

    if (empty()) {
	do_insert(s);
	variables++;
	return back().getBdd();
    }

    for (iterator i = begin(); i != end(); i++)
	if ( (*i).getName() == s )
	    return (*i).getBdd();

    do_insert(s);
    variables++;
    return back().getBdd();
}

bdd VariableToBddMap::get_bdd(std::string s) throw(std::runtime_error) {
    for (iterator i = begin(); i != end(); i++)
	if ( (*i).getName() == s )
	    return (*i).getBdd();

    std::stringstream ss;
    ss << "Variable " << s << " not found";

    throw std::runtime_error(ss.str());
}

void VariableToBddMap::print_stats(std::ostream &out) {

    if (size() == 0) {
	out << "E: no variables found" << std::endl;
	return;
    }

    out << size() << " distinct variables found: ";
    for(VariableToBddMap::iterator v = this->begin(); v != this->end(); ++v) {
	out << (*v).getName() << " ";

    }
    out << std::endl;
}

std::string VariableToBddMap::getName(unsigned i) {
    assert(i < size());
    return (*this)[i].getName();
}

void VariableToBddMap::clear() {
    count_ = blocks = variables = 0;
    std::deque<VariableEntry>::clear();
}
