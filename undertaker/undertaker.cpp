#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>

#include "KconfigRsfDbFactory.h"
#include "CloudContainer.h"
#include "CodeSatStream.h"

void usage(std::ostream &out, const char *error, char **argv ) {
    if (error)
	out << error << std::endl;
    out << "Usage: " << argv[0] << "[-v] [-f] [-w]" << " <file>" << std::endl;
}

int main (int argc, char ** argv) {
    if (argc != 2) {
	usage (std::cout, "not enough parameters", argv);
	exit(1);
    }
    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    f->loadModels();

    CloudContainer s(argv[1]);
    std::istringstream codesat(s.getConstraints());
    CodeSatStream analyzer(codesat, argv[1], "x86");
    analyzer.analyzeBlocks();

}
