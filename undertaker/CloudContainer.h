// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <deque>
#include <map>

#include "Ziz.h"

class ZizCondBlockPtr {
public:
    ZizCondBlockPtr(const Ziz::ConditionalBlock *cb) :  _cb(cb), _expression(NULL) {}
    ~ZizCondBlockPtr();
    int getId() const {return _cb->Id(); }
    const char *expression() const;
    const Ziz::ConditionalBlock *Block() const { return _cb; }
    const Ziz::ConditionalBlock *_cb;
    mutable char *_expression; // cache for expression normalization.
    int getLine() {
      std::stringstream ss;
      ss << _cb->Start();
      std::string raw = ss.str();
      size_t p = std::string::npos;
      while ( (p = raw.find(":")) != std::string::npos) {
        raw.replace(p,1,1,' ');
      }
      std::string filename, line;
      std::stringstream sss(raw);
      sss >> filename;
      sss >> line;
      return atoi(line.c_str());
    }

};

class BlockCloud : public std::deque<ZizCondBlockPtr> {
public:
    typedef unsigned int index;
    BlockCloud();
    BlockCloud(Ziz::ConditionalBlock *bc);
    const std::string& getConstraints() const;
    ~BlockCloud();
    int scanBlocks(Ziz::BlockContainer *b);
    std::string getBlockName(index n) const;
    std::string parent(index n) const;
    index search(std::string idstring) const;
    index search(int id) const;
    std::string getPosition(std::string block) const { return positions[block]; } //fixme error checking
protected:
    std::string expression(index n) const;
    std::string noPredecessor(index n) const; //< @return the ORed '|' expression of all preds
    const ZizCondBlockPtr &item(index n) const;
    int bId(index i) const; //< @return the block id the given index

private:
    mutable std::string *_constraints;
    mutable std::map<std::string,std::string> positions;
};

class CloudContainer : public std::deque<BlockCloud> {
public:
    typedef std::deque<BlockCloud> CloudList;

    CloudContainer(const char *filename);
    ~CloudContainer();
    const std::string& getConstraints();
    bool good() const { return !_fail; }
    std::map<std::string, std::string> getParents();

protected:
    Ziz::File *_zfile;
    bool _fail;

private:
    std::string *_constraints;
};

#endif