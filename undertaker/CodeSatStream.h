// -*- mode: c++ -*-
#ifndef codesatstream_h__
#define codesatstream_h__

#include <istream>
#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <list>

#include "KconfigRsfDb.h"
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


class CodeSatStream : public std::stringstream {
public:
    CodeSatStream (std::istream &ifs, std::string filename, const char *primary_arch,
           ParentMap parents, BlockCloud *cc=NULL,
           bool batch_mode=false, bool loadModels=false);
    const std::set<std::string> &Items()  const { return _items;  }
    const std::set<std::string> &FreeItems()  const { return _free_items;  }
    const std::set<std::string> &Blocks() const { return _blocks; }
    void composeConstraints(std::string block, const KconfigRsfDb *model);
    virtual void analyzeBlock(const char *block);
    void analyzeBlocks();

    static unsigned int getProcessedUnits()  { return processed_units; }
    static unsigned int getProcessedItems()  { return processed_items; }
    static unsigned int getProcessedBlocks() { return processed_blocks; }
    static unsigned int getFailedBocks()     { return failed_blocks; }
    std::string getFilename() { return _filename; }

    std::string getCodeConstraints();
    std::string getKconfigConstraints(const KconfigRsfDb *model, std::set<std::string> &missing);
    std::string getMissingItemsConstraints(std::set<std::string> &missing);
    std::list<SatChecker::AssignmentMap> blockCoverage();

    bool dumpRuntimes();
//    static const RuntimeTable &getRuntimes();
    RuntimeTable runtimes;

protected:
    bool writePrettyPrinted(const char *filename, std::string block, const char *contents) const;
    std::istream &_istream;
    std::set<std::string> _items; //kconfig items
    std::set<std::string> _free_items; //non-kconfig items
    std::set<std::string> _blocks;
    std::string _filename;
    const char *_primary_arch;
    bool _doCrossCheck;
    BlockCloud *_cc;
    const bool _batch_mode;
    ParentMap parents;

    std::stringstream codeConstraints;
    std::stringstream kconfigConstraints;
    std::stringstream missingItemsConstraints;

    static unsigned int processed_units;
    static unsigned int processed_blocks;
    static unsigned int failed_blocks;
    static unsigned int processed_items;
    std::string getLine(std::string block) const;
};


#endif
