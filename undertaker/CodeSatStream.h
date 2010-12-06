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

struct BlockDefect;

class CodeSatStream : public std::stringstream {
public:
    CodeSatStream (std::istream &ifs, std::string filename,
                   const ParentMap parents, BlockCloud *cc=NULL,
                   bool batch_mode=false, bool loadModels=false);
    const std::set<std::string> &Items()  const { return _items;  }
    const std::set<std::string> &FreeItems()  const { return _free_items;  }
    const std::set<std::string> &Blocks() const { return _blocks; }
    virtual const BlockDefect *analyzeBlock(const char *block, ConfigurationModel *p_model);

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
    std::string getKconfigConstraints(const ConfigurationModel *model, std::set<std::string> &missing);
    std::string getMissingItemsConstraints(std::set<std::string> &missing);

    /**
     * \brief Check for configuration coverage
     *
     * This method tries finds a set of configurations so that each and
     * every block (excluding dead blocks) is selected at least once.
     *
     * \param model If not NULL, take the constraints of the goven model into account
     */
    std::list<SatChecker::AssignmentMap> blockCoverage(ConfigurationModel *model);

    bool dumpRuntimes();
    RuntimeTable runtimes;

protected:
    std::istream &_istream;
    std::set<std::string> _items; //kconfig items
    std::set<std::string> _free_items; //non-kconfig items
    std::set<std::string> _blocks;
    std::string _filename;
    bool _doCrossCheck;
    BlockCloud *_cc;
    const bool _batch_mode;
    const ParentMap parents;

    std::stringstream codeConstraints;
    std::stringstream kconfigConstraints;
    std::stringstream missingItemsConstraints;

    static unsigned int processed_units;
    static unsigned int processed_blocks;
    static unsigned int failed_blocks;
    static unsigned int processed_items;
};


#endif
