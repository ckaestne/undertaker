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

#ifdef DEBUG
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif

#include "SymbolTranslator.h"
#include "KconfigSymbolSet.h"
#include "PicosatCNF.h"
#include "KconfigAssumptionMap.h"
#include "Logging.h"
#include "../version.h"

#include <boost/filesystem.hpp>
#include <locale.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>

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

int process_assumptions(PicosatCNF &cnf, const std::vector<boost::filesystem::path> assumptions) {
    int errors = 0;
    for (const auto &assumption : assumptions) {  // boost::filesystem::path
        Logging::info("processing assumption ", assumption);

        if (boost::filesystem::exists(assumption)) {
            std::ifstream in(assumption.string());
            KconfigAssumptionMap a(&cnf);
            a.readAssumptionsFromFile(in);
            Logging::info("processed ", a.size(), " items");
            cnf.pushAssumptions(a);
        }
        bool sat = cnf.checkSatisfiable();
        const std::string result = sat ? "satisfiable" : "not satisfiable";
        if (!sat) {
            const int *failed = cnf.failedAssumptions();
            for (int i = 0; failed != nullptr && failed[i] != 0; i++) {
                std::string vn = cnf.getSymbolName(abs(failed[i]));
                Logging::debug("failed Assumption: ", failed[i], " ", vn);
            }
        }
        errors += (int)!sat;
        Logging::info(assumption, " is ", result);
    }
    return errors;
}

int main(int argc, char **argv) {
    bool saveTranslatedModel = false;
    std::vector<boost::filesystem::path> assumptions;
    boost::filesystem::path saveFile;
    int exitstatus = 0;
    int opt;

    int loglevel = Logging::getLogLevel();

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
            Logging::setLogLevel(Logging::LOG_EVERYTHING);
            Logging::info("satyr ", version);
            exit(0);
        default:
            usage(std::cerr);
        }
    }

    Logging::setLogLevel(loglevel);

    if (optind >= argc) {
        usage(std::cerr);
    }
    boost::filesystem::path filepath(argv[optind]);
    if (!boost::filesystem::exists(filepath)) {
        Logging::error("File '", filepath, "' does not exist");
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
    std::cerr << "using arch " << arch << std::endl;

    setenv("ARCH", arch, 1);
    setenv("SRCARCH", srcarch, 0);
    setenv("KERNELVERSION", "2.6.30-vamos", 0);

    PicosatCNF cnf;

    if (filepath.extension() == ".cnf") {
        Logging::info("Loading CNF model ", filepath);
        cnf.readFromFile(filepath.string());
    } else {
        Logging::info("Parsing Kconfig file ", filepath);
        SymbolTranslator translator(&cnf);
        KconfigSymbolSet symbolSet;

        Logging::debug("parsing");
        translator.parse(filepath.string());
        Logging::debug("traversing symbolset");
        symbolSet.traverse();
        Logging::debug("translating");
        translator.symbolSet = &symbolSet;
        translator.traverse();

        if (translator.featuresWithStringDependencies()) {
            Logging::info("Features w string dep (", arch, "): ",
                          translator.featuresWithStringDependencies(), " with ",
                          translator.totalStringComparisons(), " comparisons.");
        }
        Logging::info("features in model: ", symbolSet.size());
    }
    if (saveTranslatedModel) {
        cnf.toFile(saveFile.string());
        Logging::info(cnf.getVarCount(), " variables written to ", saveFile);
    }
    exitstatus += process_assumptions(cnf, assumptions);
    return exitstatus;
}
