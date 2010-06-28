// -*- mode: c++ -*-
#include <cstdlib>
#include <sstream>
#include <fstream>

#include "Ziz.h"
#include "CloudContainer.h"

int main (int argc, char **argv) {
    if (argc != 2)
	exit(1);

    CloudContainer s(argv[1]);

    try {
	std::cout << s.getConstraints() << std::endl;
	return EXIT_SUCCESS;
    } catch (std::runtime_error &e) {
	std::cerr << "FAILED: " << e.what() << std::endl;
	return EXIT_FAILURE;
    }

}
