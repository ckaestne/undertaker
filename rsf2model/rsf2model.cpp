/*
 *   rsf2model - convert dumpconf output to undertaker model format
 *
 * Copyright (C) 2010-2011 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


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
