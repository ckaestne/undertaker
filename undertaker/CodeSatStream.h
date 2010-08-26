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

typedef std::pair <const char *, clock_t> RuntimeEntry; //< filename, runtime
typedef std::list<RuntimeEntry> RuntimeTable;


class CodeSatStream : public std::stringstream {
public:
    CodeSatStream (std::istream &ifs, std::string filename, const char *primary_arch, std::map<std::string, std::string> parents, bool batch_mode=false, bool loadModels=false);
    const std::set<std::string> &Items()  const { return _items;  }
    const std::set<std::string> &FreeItems()  const { return _free_items;  }
    const std::set<std::string> &Blocks() const { return _blocks; }
    std::string buildTermMissingItems(std::set<std::string> missing) const;
    void composeConstraints(std::string block, const KconfigRsfDb *model);
    virtual void analyzeBlock(const char *block);
    void analyzeBlocks();

    static unsigned int getProcessedUnits()  { return processed_units; }
    static unsigned int getProcessedItems()  { return processed_items; }
    static unsigned int getProcessedBlocks() { return processed_blocks; }
    static unsigned int getFailedBocks()     { return failed_blocks; }

    std::string getCodeConstraints(const char *block);
    std::string getKconfigConstraints(const char * block, const KconfigRsfDb *model, std::set<std::string> &missing);
    std::string getMissingItemsConstraints(const char * block, const KconfigRsfDb *model,  std::set<std::string> &missing);

    static const RuntimeTable &getRuntimes();

protected:
    bool writePrettyPrinted(const char *filename, const char *contents) const;
    std::istream &_istream;
    std::set<std::string> _items; //kconfig items
    std::set<std::string> _free_items; //non-kconfig items
    std::set<std::string> _blocks;
    std::map<std::string, std::string> parents;
    std::string _filename;
    const char *_primary_arch;
    bool _doCrossCheck;
    const bool _batch_mode;

    std::stringstream codeConstraints; 
    std::stringstream kconfigConstraints; 
    std::stringstream missingItemsConstraints; 

    static unsigned int processed_units;
    static unsigned int processed_blocks;
    static unsigned int failed_blocks;
    static unsigned int processed_items;
};


#endif
