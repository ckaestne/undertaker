#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/regex.hpp>

#include "KconfigRsfTranslator.h"

void usage(std::ostream &out, const char *error) {
    if (error)
	out << error << std::endl;
    out << std::endl;
    out << "Usage: rsf2model <rsf-file>\n";
}

int main (int argc, char ** argv) {
    if (argc < 2) {
        usage(std::cerr, "E: Please specify a architecture to dump");
        exit(EXIT_FAILURE);
    }

    std::ifstream rsf_file(argv[1]);
    static std::ofstream devnull("/dev/null");

    if (!rsf_file.good()) {
        std::cerr << "E: could not open file for reading: "
                  << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    /* Load the rsf model */
    KconfigRsfTranslator model(rsf_file, devnull);
    model.initializeItems();

    /* Dump all Items to stdout */
    model.dumpAllItems(std::cout);

    return EXIT_SUCCESS;
}
