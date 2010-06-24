// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <deque>

#include "Ziz.h"

struct SatBlockInfo {
    SatBlockInfo(const Ziz::ConditionalBlock *cb) :  _cb(cb), _expression(NULL) {}
    ~SatBlockInfo();
    int getId() {return _cb->Id(); }
    const char *expression();
    const Ziz::ConditionalBlock *Block() { return _cb; }
private:
    const Ziz::ConditionalBlock *_cb;
    char *_expression;
};

class SatContainer : public std::deque<SatBlockInfo> {
public:
    typedef unsigned int index;

    SatContainer(const char *filename);
    ~SatContainer();
    void parseExpressions();
    void runSat();

protected:
    std::string getBlockName(index n);
    std::string parent(index n);
    std::string expression(index n);
    std::string noPredecessor(index n); //< @return the ORed '|' expression of all preds
    SatBlockInfo &item(index n);
    int bId(index i); 		//< @return the block id the given index
    index search(std::string idstring);
    index search(int id);

private:
    int scanBlocks(Ziz::BlockContainer *b);
    Ziz::File *_zfile;
};

#endif
