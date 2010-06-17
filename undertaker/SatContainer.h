// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <iostream>
#include <deque>
#include "CompilationUnitInfo.h"

struct SatBlockInfo {
    SatBlockInfo(int i, const char *expression)
	: id_(i), expr_(expression) {}
    int getId() {return id_; }
    const char *expression() { return expr_; }
private:
    int id_;
    const char *expr_;
};

class SatContainer : public std::deque<SatBlockInfo>, protected CompilationUnitInfo {
public:
    typedef unsigned int index;

    SatContainer(std::ifstream &in, std::ostream &log);
    void parseExpressions();
    void runSat();

private:
    std::string getBlockName(int index);
    std::string noPredecessor(index n, RsfBlocks &blocks);
    SatBlockInfo &item(index n);
    int bId(index i);
    index search(std::string idstring);
    index search(int id);
};

#endif
