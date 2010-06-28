// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <deque>

#include "Ziz.h"

class ZizCondBlockPtr {
public:
    ZizCondBlockPtr(const Ziz::ConditionalBlock *cb) :  _cb(cb), _expression(NULL) {}
    ~ZizCondBlockPtr();
    int getId() {return _cb->Id(); }
    const char *expression();
    const Ziz::ConditionalBlock *Block() { return _cb; }
private:
    const Ziz::ConditionalBlock *_cb;
    char *_expression; // cache for expression normalization.
};

class BlockCloud : public std::deque<ZizCondBlockPtr> {
public:
    typedef unsigned int index;
    BlockCloud();
    BlockCloud(Ziz::ConditionalBlock *bc);
    const std::string& getConstraints();
    ~BlockCloud();
    int scanBlocks(Ziz::BlockContainer *b);
protected:
    std::string getBlockName(index n);
    std::string parent(index n);
    std::string expression(index n);
    std::string noPredecessor(index n); //< @return the ORed '|' expression of all preds
    ZizCondBlockPtr &item(index n);
    int bId(index i); 		//< @return the block id the given index
    index search(std::string idstring);
    index search(int id);

private:
    std::string *_constraints;
};

class CloudContainer : public std::deque<BlockCloud> {
public:
    typedef std::deque<BlockCloud> CloudList;

    CloudContainer(const char *filename);
    ~CloudContainer();
    const std::string& getConstraints();

protected:
    Ziz::File *_zfile;

private:
    std::string *_constraints;
};

#endif
