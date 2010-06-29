// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <deque>

#include "Ziz.h"

class ZizCondBlockPtr {
public:
    ZizCondBlockPtr(const Ziz::ConditionalBlock *cb) :  _cb(cb), _expression(NULL) {}
    ~ZizCondBlockPtr();
    int getId() const {return _cb->Id(); }
    const char *expression() const;
    const Ziz::ConditionalBlock *Block() const { return _cb; }
private:
    const Ziz::ConditionalBlock *_cb;
    mutable char *_expression; // cache for expression normalization.
};

class BlockCloud : public std::deque<ZizCondBlockPtr> {
public:
    typedef unsigned int index;
    BlockCloud();
    BlockCloud(Ziz::ConditionalBlock *bc);
    const std::string& getConstraints() const;
    ~BlockCloud();
    int scanBlocks(Ziz::BlockContainer *b);
protected:
    std::string getBlockName(index n) const;
    std::string parent(index n) const;
    std::string expression(index n) const;
    std::string noPredecessor(index n) const; //< @return the ORed '|' expression of all preds
    const ZizCondBlockPtr &item(index n) const;
    int bId(index i) const; //< @return the block id the given index
    index search(std::string idstring) const;
    index search(int id) const;

private:
    mutable std::string *_constraints;
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
