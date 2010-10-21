#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>

#include "KconfigRsfDbFactory.h"
#include "CloudContainer.h"
#include "CodeSatStream.h"

typedef std::deque<BlockCloud> CloudList;

void usage(std::ostream &out, const char *error) {
    if (error)
	out << error << std::endl;
    out << "Usage: undertaker [-b worklist] [-w whitelist] [-a arch] -s" << " <file>\n";
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -w: specify a whitelist\n";
    out << "  -s: skip loading variability models\n";
    out << "  -a: specify a unique arch \n";
    out << std::endl;
}

void process_file(const char *filename, bool batch_mode, bool loadModels) {
    CloudContainer s(filename);
    if (!s.good()) {
	usage(std::cout, "couldn't open file");
	return;
    }

    std::istringstream cs(s.getConstraints());
    clock_t start, end;
    double t = -1;
    start = clock();
    for (CloudList::iterator c = s.begin(); c != s.end(); c++) {
      std::map<std::string,std::string> parents;
      std::istringstream codesat(c->getConstraints());
      std::string primary_arch = "x86";
      KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
      if (f->size() == 1)
        primary_arch = f->begin()->first;
      CodeSatStream analyzer(codesat, filename, primary_arch.c_str(), s.getParents(), &(*c), batch_mode, loadModels);
      analyzer.analyzeBlocks();
      analyzer.dumpRuntimes();
    }
    end = clock();
    t = (double) (end - start);
    std::cout.precision(10);
    std::cout << "RTF:" << filename << ":" << t << std::endl;

}

int main (int argc, char ** argv) {
    int opt;
    bool loadModels = true;
    char *worklist = NULL;
    char *whitelist = NULL;
    char *arch = NULL;
    bool arch_specific = false;

    while ((opt = getopt(argc, argv, "sb:a:")) != -1) {
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
	case 'a':
	    arch_specific = true;
            arch = strdup(optarg);	    
	    std::cout << arch;
	    break;
	default:
	    break;
	}
    }

    if (worklist || optind >= argc) {
	usage(std::cout, "please specify a file to scan or a worklist");
	return EXIT_FAILURE;
    }

    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();

    if (loadModels) {
        if (arch_specific) {
	  f->loadModels(arch);
	} else {
	  f->loadModels();
	}
    }


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
