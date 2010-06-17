#include <iostream>
#include <pstreams/pstream.h>

#include "CodeSatStream.h"
#include "KconfigRsfDbFactory.h"


void statistics () {
    const RuntimeTable &runtimes = CodeSatStream::getRuntimes();
    std::ofstream rf("runtimes.txt");

    std::cout << "Runtimes:" << std::endl;

    for (RuntimeTable::const_iterator i = runtimes.begin(); i != runtimes.end(); i++) {
	    std::stringstream ss;
	    ss << (*i).first << ", " << ((double)(*i).second / CLOCKS_PER_SEC) << std::endl;
	    rf << ss.str();
	    std::cout << ss.str();
    }

    std::cout << "Units processed:  " << CodeSatStream::getProcessedUnits() << std::endl
	      << "Items processed:  " << CodeSatStream::getProcessedItems() << std::endl
	      << "Blocks Processed: " << CodeSatStream::getProcessedBlocks() << std::endl
	      << "Blocks Failed:    " << CodeSatStream::getFailedBocks() << std::endl;
}

int main (int argc, char ** argv) {
    bool skipCrosscheck = false;
    int opt;

    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    f->loadModels();
    std::string line;

    while ((opt = getopt(argc, argv, "sw:")) != -1) {
	switch (opt) {
	case 's':
	    skipCrosscheck = true;
	    break;
	case 'w':
	    f->loadWhitelist(optarg);
	    break;
	default:
	    break;
	}
    }

    for (ModelContainer::iterator i = f->begin(); i!=f->end(); i++) {
	std::string line;
	std::string arch = (*i).first;
	char cmd[80];
	snprintf(cmd, sizeof cmd, "find arch/%s/ -name '*.codesat' 2>/dev/null",
		 arch.c_str());

	redi::ipstream archs_rsfs(cmd);
	while (std::getline(archs_rsfs, line)) {
	    // no cross check
	    CodeSatStream analyzer(line.c_str(), arch.c_str(), false);
	    analyzer.analyzeBlocks();
	}
    }

    if (skipCrosscheck) {
	std::cout << "Skipping crosscheck" << std::endl;
    } else {
	std::string line;
	redi::ipstream other_rsfs("find . -name 'arch' -prune -o -name '*.codesat'");
	while (std::getline(other_rsfs, line)) {
	    std::cout << "processing " << line << std::endl;
	    CodeSatStream analyzer(line.c_str(), "x86", true);
	    analyzer.analyzeBlocks();
	}
    }

    statistics();
}
