// -*- mode: c++ -*-
#include <cstdlib>
#include <sstream>
#include <fstream>

#include "Ziz.h"
#include "SatContainer.h"

static std::ofstream devnull("/dev/null");

int main (int argc, char **argv) {
    if (argc != 2)
	exit(1);

    SatContainer s(argv[1]);
    s.parseExpressions();

    if (s.size() > 0) {
        std::cout << s.runSat() << std::endl;
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
