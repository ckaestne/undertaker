#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <boost/regex.hpp>

#include "KconfigRsfDb.h"
#include "CodeSatStream.h"

static std::ofstream devnull("/dev/null");

void usage(const char *msg) {
    std::cout << msg << std::endl;
    std::cout << "usage: KconfigIntersect <kconfig.rsf> <worklist>" << std::endl;
}

int main (int argc, char **argv) {
    boost::match_results<const char*> what;
    const boost::regex kconfigfile_regexp("kconfig-([[:alnum:]]+)\\.rsf$",
                      boost::regex::perl);
    unsigned processed = 0;
    unsigned totalextracted = 0;
    unsigned totalblocks = 0;

    std::string arch("");

    if (argc < 2) {
    char msg[] = "too few arguments";
    usage(msg);
        exit(EXIT_FAILURE);
    }

    static std::ifstream kconfigRsfFile(argv[1]);
    if (!kconfigRsfFile.good()) {
        std::stringstream ss;
        ss << "failed to open: " << argv[1];
    usage(ss.str().c_str());
        exit(EXIT_FAILURE);
    }

    if (boost::regex_search(argv[1], what, kconfigfile_regexp)) {
    arch = what[1];
    std::cout << "working on arch: " << arch << std::endl;
    }

    // open worklist
    static std::ifstream worklist(argv[2]);
    if (!worklist.good()) {
        std::stringstream ss;
        ss << "failed to open: " << argv[2];
    usage(ss.str().c_str());
    exit(EXIT_FAILURE);
    }

    KconfigRsfDb s(kconfigRsfFile, devnull);
    s.initializeItems(); // EXPENSIVE!

    while(worklist.good()) {
    std::string codesatfile;

    worklist >> codesatfile;
    if (codesatfile.size() == 0) // ignore empty lines
        break;

    CodeSatStream codesat(codesatfile.c_str(), "x86", false);
    int extracted = codesat.Items().size();
    if (extracted == 0)
        continue;

    const std::set<std::string> &blocks = codesat.Blocks();
    totalblocks += blocks.size();
    totalextracted += extracted;

    codesat.analyzeBlocks();

    }

    std::cout << "Extracted "
          << totalextracted  << " items from "
          << totalblocks << " conditional blocks while processing "
          << processed << " codesat files" << std::endl;
}
