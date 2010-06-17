// -*- mode: c++ -*-
#ifndef rsfblock_h__
#define rsfblock_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>


class RsfBlocks : public std::multimap<std::string, std::deque<std::string> > {
public:

    RsfBlocks(std::ifstream &f, std::string relation, std::ostream &log=std::cout);

    std::string *getValue(std::string key);
    std::string *getValue(int n);
    void print_contents(std::ostream &out);

private:
    void read_blocks(std::ifstream &rsf_file, std::string relation);
    std::ostream &log;
    std::string relation;

};

#endif
