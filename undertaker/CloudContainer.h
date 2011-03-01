/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// -*- mode: c++ -*-
#ifndef sat_container_h__
#define sat_container_h__

#include <deque>
#include <map>

#include "Ziz.h"

int getLineFromPos(position_type pos);

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

typedef std::map<std::string, std::string> ParentMap;

class CloudContainer : public std::deque<BlockCloud> {
public:
    typedef std::deque<BlockCloud> CloudList;

    CloudContainer(const char *filename);
    ~CloudContainer();
    const std::string& getConstraints();
    bool good() const { return !_fail; }
    ParentMap getParents() const;
    const char *getFilename() const { return _filename; };
    std::string rewriteExpression(ZizCondBlockPtr&);
    const Ziz::Defines &getDefinesMap() const { return _zfile->getDefinesMap(); }

protected:
    Ziz::File *_zfile;
    bool _fail;
    std::set<std::string> rw_constraints;
    void rewriteAllExpressions();
    int getLastDefinePos(std::string flag, int limit);
    int getLastDefinePosFromDepth(std::string flag, int limit, Ziz::ConditionalBlock* cb);
    bool isLastDefineAtLevel(Ziz::Define* def, int limit);

    std::string *_constraints;
    const char *_filename;
};

#endif
