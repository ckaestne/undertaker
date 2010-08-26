#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>

#include "KconfigRsfDbFactory.h"
#include "CloudContainer.h"
#include "CodeSatStream.h"

void usage(std::ostream &out, const char *error) {
    if (error)
	out << error << std::endl;
    out << "Usage: undertaker [-b worklist] [-w whitelist] -s" << " <file>\n";
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -w: specify a whitelist\n";
    out << "  -s: skip loading variability models\n";
    out << std::endl;
}

void process_file(const char *filename, bool batch_mode, bool loadModels) {
    CloudContainer s(filename);
    if (!s.good()) {
	usage(std::cout, "couldn't open file");
	return;
    }

    std::istringstream codesat(s.getConstraints());
    CodeSatStream analyzer(codesat, filename, "x86", s.getParents(), batch_mode, loadModels);
    analyzer.analyzeBlocks();
}

int main (int argc, char ** argv) {
    int opt;
    bool loadModels = true;
    char *worklist = NULL;
    char *whitelist = NULL;

    while ((opt = getopt(argc, argv, "sb:")) != -1) {
	switch (opt) {
	case 's':
	    loadModels = false;
	    break;
	case 'w':
	    whitelist = strdup(optarg);
	    break;
	case 'b':
	    worklist = strdup(optarg);
	    break;
	default:
	    break;
	}
    }

    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();

    if (loadModels)
	f->loadModels();

    if (!worklist) {
	process_file(argv[optind], false, loadModels);
    } else {
	std::ifstream workfile(worklist);
	std::string line;
	while(std::getline(workfile, line)) {
	    process_file(line.c_str(), true, loadModels);
	}
    }
    return EXIT_SUCCESS;
}
