// -*- mode: c++ -*-
#include <cstdlib>
#include <sstream>
#include <fstream>

#include "Ziz.h"
#include "CloudContainer.h"
#include "CodeSatStream.h"

int main (int argc, char **argv) {
    if (argc != 2)
        exit(EXIT_FAILURE);

    CloudContainer s(argv[1]);
    std::istringstream codesat(s.getConstraints());
    std::map<std::string,std::string> parents;
    CodeSatStream analyzer(codesat, argv[1], "x86", parents);

    try {
        std::cout << s.getConstraints() << std::endl;
        return EXIT_SUCCESS;
    } catch (std::runtime_error &e) {
        std::cerr << "FAILED: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
