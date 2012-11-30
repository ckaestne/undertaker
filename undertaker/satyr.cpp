/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/filesystem.hpp>

#include "SymbolTranslator.h"
#include "KconfigSymbolSet.h"

#include "bool.h"
#include "PicosatCNF.h"
#include "KconfigAssumptionMap.h"
#include "Logging.h"
#include "../version.h"

#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <libgen.h>

using namespace kconfig;

void usage(std::ostream &out) {
    out << "usage: satyr [-V] [-a <assumtion.config> | -c <out.cnf>] <model>" << std::endl;
    out << "       model:          a Kconfig file / translated cnf file" << std::endl;
    out << "       -a <assumtion>  a .config file to be validated" << std::endl;
    out << "                       (may be incomplete)" << std::endl;
    out << "       -c <out.cnf>   translates model to cnf and saves it to out.cnf" << std::endl;
    out << "       -V  print version information\n";
    exit(EXIT_FAILURE);
}

int process_assumptions(CNF *cnf, std::vector<boost::filesystem::path> assumptions) {
    int errors = 0;
    for (auto &assumption : assumptions) {  // boost::filesystem::path
        logger << info << "processing assumption " << assumption << std::endl;

        if (boost::filesystem::exists(assumption)) {
            std::ifstream in(assumption.string().c_str());
            KconfigAssumptionMap a(cnf);
            a.readAssumptionsFromFile(in);
            logger << info
                   << "processed " << a.size() << " items" << std::endl;
            cnf->pushAssumptions(a);
        }
        bool sat = cnf->checkSatisfiable();
        const char* result = sat ? "satisfiable" : "not satisfiable";
        if (!sat) {
            const int *failed = cnf->failedAssumptions();
            for (int i=0; failed != 0 && failed[i] != 0; i++) {
                std::string vn = cnf->getSymbolName(abs(failed[i]));
                logger << debug << "failed Assumption: " << failed[i] << " " << vn <<std::endl;
            }
        }
        errors += (int) !sat;
        logger << info << assumption << " is " << result << std::endl;
    }
    return errors;
}

int main(int argc, char **argv) {
    static bool saveTranslatedModel = false;
    static std::vector<boost::filesystem::path> assumptions;
    boost::filesystem::path saveFile;
    int exitstatus = 0;
    int opt;

    static int loglevel = logger.getLogLevel();

    while ((opt = getopt(argc, argv, "Vvc:a:")) != -1) {
        switch (opt) {
        case 'c':
            saveTranslatedModel = true;
            saveFile = optarg;
            break;
        case 'a':
            assumptions.push_back(optarg);
            break;
        case 'v':
            loglevel = loglevel - 10;
            if (loglevel < 0)
                loglevel = Logging::LOG_EVERYTHING;
            break;
        case '?':
            usage(std::cerr);
            break;
        case 'V':
            logger.setLogLevel(Logging::LOG_EVERYTHING);
            logger << info << "satyr " << version << std::endl;
            exit(0);
        default:
            usage(std::cerr);
        }
    }

    logger.setLogLevel(loglevel);

    if (optind >= argc) {
        usage(std::cerr);
    }
    boost::filesystem::path filepath(argv[optind]);
    if (!boost::filesystem::exists(filepath)) {
        logger << error << "File '" << filepath << "' does not exist"
               << std::endl;
        exit(EXIT_FAILURE);
    }
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    const char *arch = getenv("ARCH");
    const char *srcarch = getenv("SRCARCH");
    if (!arch) {
        static const char *default_arch = "x86";
        arch = default_arch;
    }
    if (!srcarch) {
        srcarch = arch;
    }
    fprintf(stderr, "using arch %s\n", arch);

    setenv("ARCH", arch, 1);
    setenv("SRCARCH", srcarch, 0);
    setenv("KERNELVERSION", "2.6.30-vamos", 0);

    CNF *cnf = new PicosatCNF();

    if (filepath.extension() == ".cnf") {
        logger << info << "Loading CNF model " << filepath << std::endl;
        cnf->readFromFile(filepath.string().c_str());
    } else {
        logger << info << "Parsing Kconfig file " << filepath << std::endl;
        SymbolTranslator *translator = new SymbolTranslator(cnf);
        KconfigSymbolSet *symbolSet = new KconfigSymbolSet();

        logger << debug << "parsing" << std::endl;
        translator->parse(filepath.string().c_str());
        translator->symbolSet = symbolSet;
        logger << debug << "traversing symbolset" << std::endl;
        symbolSet->traverse();
        logger << debug << "translating" << std::endl;
        translator->traverse();

        if (translator->featuresWithStringDependencies()) {
            logger << info << "Features w string dep (" << arch << "): "
                   << translator->featuresWithStringDependencies()
                   << " with " << translator->totalStringComparisons()
                   << " comparisons." << std::endl;
        }
        logger << info << "features in model: " << symbolSet->size() << std::endl;
    }
    if (saveTranslatedModel) {
        cnf->toFile(saveFile.string().c_str());
        logger << info << cnf->getVarCount() << " variables written to "
               << saveFile << std::endl;
    }
    exitstatus += process_assumptions(cnf, assumptions);
    return exitstatus;
}
