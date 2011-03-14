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
#ifndef codesatstream_h__
#define codesatstream_h__

#include <istream>
#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <list>

#include "ConfigurationModel.h"
#include "CloudContainer.h"
#include "SatChecker.h"

//typedef std::pair <const char *, clock_t> RuntimeEntry; //< filename, runtime
class RuntimeEntry {
  public:
  std::string filename;
  std::string cloud;
  std::string block;
  clock_t rt_full_analysis;
  int i_items;
  int slice;
  RuntimeEntry() {}
  RuntimeEntry (const char* fl, const char* c, const char* bl, clock_t rt, int i, int s) :
  filename(fl), cloud(c), block(bl), rt_full_analysis(rt), i_items(i), slice(s) {}
  std::string getString() {
    std::stringstream ss;
    ss << "RT:" << filename
       << ":" << cloud
       << ":"<< block
       << ":" << rt_full_analysis
       << ":" << i_items
       << ":" << slice
       << std::endl;
    return ss.str();
  }
};

typedef std::list<RuntimeEntry> RuntimeTable;
typedef std::set<std::string> MissingSet;

struct BlockDefectAnalyzer;

class CodeSatStream : public std::stringstream {
public:
    CodeSatStream (std::istream &ifs,
                   const CloudContainer &cloudContainer,
                   BlockCloud *blockCloud,
                   bool batch_mode=false, bool loadModels=false);
    const std::set<std::string> &Items()  const { return _items;  }
    const std::set<std::string> &Blocks() const { return _blocks; }
    virtual const BlockDefectAnalyzer *analyzeBlock(const char *block, ConfigurationModel *p_model);

    /**
     * \brief Look up the enclosing block, if any
     *
     * \return enclosing block, or NULL, if the block is already top-level
     */
    const char *getParent(const char *block);

    void analyzeBlocks();

    static unsigned int getProcessedUnits()  { return processed_units; }
    static unsigned int getProcessedItems()  { return processed_items; }
    static unsigned int getProcessedBlocks() { return processed_blocks; }
    static unsigned int getFailedBocks()     { return failed_blocks; }
    const std::string& getFilename() const   { return _filename; }
    std::string getLine(const char *block) const;

    std::string getCodeConstraints();
    std::string getKconfigConstraints(const ConfigurationModel*, MissingSet&);

    /**
     * \brief Check for configuration coverage
     *
     * This method tries finds a set of configurations so that each and
     * every block (excluding dead blocks) is selected at least once.
     *
     * \param model If not NULL, take the constraints of the goven model into account
     */
    std::list<SatChecker::AssignmentMap> blockCoverage(ConfigurationModel*, MissingSet&);

    /**
     * Find innermost block for a given position
     *
     * \param string of the form <line>:<column>
     *
     * \return innermost block or "" if no block matches
     */
    std::string positionToBlock(std::string);


    bool dumpRuntimes();
    RuntimeTable runtimes;

protected:

    struct ItemChecker : public ConfigurationModel::Checker {
        ItemChecker(const CloudContainer &cc) : _cloudContainer(cc) {}
        bool operator()(const std::string &item) const;
    private:
        const CloudContainer &_cloudContainer;
    };

    std::istream &_istream;
    std::set<std::string> _items; // All these options might be in an model
    std::set<std::string> _blocks;
    std::string _filename;
    bool _doCrossCheck;
    const CloudContainer &_cloudContainer;
    BlockCloud *_blockCloud;
    const bool _batch_mode;
    const ParentMap parents;

    std::stringstream codeConstraints;
    std::stringstream kconfigConstraints;
    std::stringstream missingItemsConstraints;

    static unsigned int processed_units;
    static unsigned int processed_blocks;
    static unsigned int failed_blocks;
    static unsigned int processed_items;

    const ItemChecker _defineChecker;
};


#endif
