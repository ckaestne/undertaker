// -*- mode: c++
#ifndef bdd_container_h__
#define bdd_container_h__

#include <deque>
#include <istream>
#include <ostream>
#include <string>
#include "RsfBlocks.h"
#include "CompilationUnitInfo.h"
#include "bdd.h"

class BlockBddInfo {
public:
    BlockBddInfo(int i, bdd block, bdd expressionBdd,
		 const std::string &expressionString)
	: id_(i), block_(block), expression_(expressionBdd), 
          expr_(expressionString) {}
    int getId() { return id_; }
    bdd &blockBdd() { return block_; }
    bdd &expressionBdd() { return expression_; }
    const std::string &expressionString() { return expr_; }
private:
    int id_;
    bdd block_;
    bdd expression_;
    const std::string &expr_;
};

// contents of the tuple: bdd for block, bdd for expression of block
class BddContainer : public std::deque<BlockBddInfo>, protected CompilationUnitInfo {

public:
    typedef unsigned int index;

    BddContainer(std::ifstream &in, char *argv0 = NULL,
		 std::ostream &out = std::cout, bool force_run = false);
    ~BddContainer();
    bdd do_build();
    bdd run();

    void parseExpressions();
    void print_stats(std::ostream &out);
    size_t numExpr();
    size_t numVar();

    BlockBddInfo &item(index n);
    int bId(index n);
    int bId(std::string n);
    bdd bParent(index blockno);
    bdd bBlock(index n);
    bdd bBlock(std::string s);
    bdd bExpression(index n);
    bdd bExpression(std::string n);


protected:
    index search(std::string s);
    index search(int i);

private:
    bdd Pred(index n);
    bdd Succ(index n);
    bdd do_calc(index n, RsfBlocks &blocks);
    int try_load(const char *filename, bdd &b);

    int pcount_;
    bool parsedExpressions_;
    char *argv0;
    std::ostream &log;
    bool _force_run;
};

#endif
