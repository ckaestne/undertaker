// -*- mode: c++ -*-
#ifndef bddvars_h__
#define bddvars_h__


#include <deque>
#include <string>
#include <bdd.h>
#include <utility>
#include <stdexcept>


struct VariableEntry {
   VariableEntry(std::string s, bdd b) 
     : name(s), b(b) {}
   std::string getName() { return name; }
   bdd getBdd() { return b; }

private:
   std::string name;
   bdd b;
};

// Holds bdd for a specific variable
class VariableToBddMap : public std::deque<VariableEntry> {
public:

    static VariableToBddMap *getInstance() {
	static VariableToBddMap *__instance;
	if (!__instance)
	    __instance = new VariableToBddMap();
	return __instance;
    }

    void clear();
    bdd add_new_variable(std::string s);
    bdd make_block_bdd(unsigned blockno);
    bdd get_bdd(std::string s) throw(std::runtime_error);
    bdd ith_bdd(int i) {
	VariableToBddMap::iterator j = this->begin() + i;
	return (*j).getBdd();
    }
    void print_stats(std::ostream &out);
    int allocatedBdds() { return count_; }
    std::string getName(unsigned i);

    static bool debug;
    static int variables;
    static int blocks;

private:

    VariableToBddMap()
	: std::deque<value_type>() {}

    VariableToBddMap *instance();
    static size_t count_;
    void do_insert(std::string s);
};

#endif
