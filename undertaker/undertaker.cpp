/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2013 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012-2014 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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
#include <vector>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <glob.h>

#include <boost/regex.hpp>
#include <boost/thread.hpp>

#include "KconfigWhitelist.h"
#include "ModelContainer.h"
#include "RsfConfigurationModel.h"
#include "PumaConditionalBlock.h"
#include "ConditionalBlock.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "CoverageAnalyzer.h"
#include <errno.h>
#include "Logging.h"
#include "../version.h"

typedef void (* process_file_cb_t) (const char *filename);

enum WorkMode {
    MODE_DEAD,      // Detect dead and undead files
    MODE_COVERAGE,  // Calculate the coverage
    MODE_CPPPC,     // CPP Preconditions for whole file
    MODE_BLOCKPC,   // Block precondition
};

enum CoverageOutputMode {
    COVERAGE_MODE_KCONFIG,      // kconfig specific mode
    COVERAGE_MODE_STDOUT,       // prints out on stdout generated configurations
    COVERAGE_MODE_MODEL,        // prints all configuration space itms, no blocks
    COVERAGE_MODE_CPP,          // prints all cpp items as cpp cmd-line arguments
    COVERAGE_MODE_EXEC,         // pipe file for every configuration to cmd
    COVERAGE_MODE_ALL,          // prints all items and blocks
    COVERAGE_MODE_COMBINED,     // create files for configuration, kconfig and modified source file
    COVERAGE_MODE_COMMENTED,    // creates file contining modified source
} coverageOutputMode;

enum CoverageOperationMode {
    COVERAGE_OP_SIMPLE,             // simple, fast
    COVERAGE_OP_MINIMIZE,           // hopefully minimal configuration set
} coverageOperationMode = COVERAGE_OP_SIMPLE;

static const char *coverage_exec_cmd = "cat";
static int RETVALUE = EXIT_SUCCESS;
static bool skip_non_configuration_based_defects = false;
static bool decision_coverage = false;
static bool do_mus_analysis = false;

void usage(std::ostream &out, const char *error) {
    if (error)
        out << error << std::endl << std::endl;
    out << "`undertaker' analyzes conditional C code with #ifdefs.\n";
    out << version << "\n\n";
    out << "Usage: undertaker [OPTIONS] <file..>\n";
    out << "\nOptions:\n";
    out << "  -V  print version information\n";
    out << "  -v  increase the log level (more verbose)\n";
    out << "  -q  decrease the log level (less verbose)\n";
    out << "  -m  specify the model(s) (directory or file)\n";
    out << "  -M  specify the main model\n";
    out << "  -i  specify a ignorelist\n";
    out << "  -W  specify a whitelist\n";
    out << "  -B  specify a blacklist\n";
    out << "  -b  specify a worklist (batch mode)\n";
    out << "  -t  specify count of parallel processes\n";
    out << "  -I  add an include path for #include directives\n";
    out << "  -s  skip non-configuration based defect reports\n";
    out << "  -u  report a 'minimal unsatisfiable subset' of the defect-formula\n";
    out << "  -j  specify the jobs which should be done\n";
    out << "      - dead: dead/undead file analysis (default)\n";
    out << "      - coverage: coverage file analysis\n";
    out << "      - cpppc: CPP Preconditions for whole file\n";
    out << "      - cpppc_decision: CPP Preconditions for whole file with decision mode preprocessing\n";
    out << "      - cppsym: statistics over CPP symbols mentioned in source files\n";
    out << "      - blockrange: List all blocks with corresponding ranges (format: <file>:<blockID>:<start>:<end>\n";
    out << "      - blockpc: Block precondition (format: <file>:<line>:<column>)\n";
    out << "      - symbolpc: Symbol precondition (format <symbol>)\n";
    out << "      - checkexpr: Find a configuration that satisfies expression\n";
    out << "      - interesting: Find related items (negated items are not in the model)\n";
    out << "      - blockconf: Find configuration enabling specified block (format: <file>:<line>)\n";
    out << "      - mergeblockconf: Find configuration enabling specified blocks in the given file\n";
    out << "\nCoverage Options:\n";
    out << "  -O: specify the output mode of generated configurations\n";
    out << "      - kconfig: generated partial kconfig configuration (default)\n";
    out << "      - stdout: print on stdout the found configurations\n";
    out << "      - cpp: print on stdout cpp -D command line arguments\n";
    out << "      - exec:cmd: pipe file for every configuration to cmd\n";
    out << "      - model:    print all options which are in the configuration space\n";
    out << "      - all:      dump every assigned symbol (both items and code blocks)\n";
    out << "      - combined: create files for both configuration and pre-commended sources\n";
    out << "  -C: specify coverage algorithm\n";
    out << "      simple           - relative simple and fast algorithm (default)\n";
    out << "      min              - slow but generates less configuration sets\n";
    out << "      simple_decision  - simple and decision coverage instead statement coverage\n";
    out << "      min_decision     - min and decision coverage instead statement coverage\n";
    out << "\nSpecifying Files:\n";
    out << "  You can specify one or many files (the format is according to the\n";
    out << "  job (-j) which should be done. If you specify - as file, undertaker\n";
    out << "  will load models and whitelist and read files from stdin (interactive).\n";
}

int rm_pattern(const char *pattern) {
    glob_t globbuf;
    int i, nr;

    glob(pattern, 0, NULL, &globbuf);
    nr = globbuf.gl_pathc;
    for (i = 0; i < nr; i++)
        if (0 != unlink(globbuf.gl_pathv[i]))
            fprintf(stderr, "E: Couldn't unlink %s: %m", globbuf.gl_pathv[i]);

    globfree(&globbuf);
    return nr;
}

bool process_blockconf_helper(StringJoiner &sj,
                              std::map<std::string,bool> &filesolvable,
                              const char *locationname) {
    // used by process_blockconf and process_mergedblockconf
    ConfigurationModel *model = ModelContainer::lookupMainModel();

    std::string location = std::string(locationname);
    boost::regex regex("(.*):[0-9]+");
    boost::smatch results;
    if (!boost::regex_match(location, results, regex)) {
        logger << error << "invalid format for block precondition" << std::endl;
        return false;
    }

    std::string file = results[1];

    CppFile cpp(file.c_str());
    if (!cpp.good()) {
        logger << error << "failed to open file: `" << file << "'" << std::endl;
        return false;
    }

    int offset = strncmp("./", file.c_str(), 2)?0:2;
    std::stringstream fileCondition;
    fileCondition << "FILE_";
    fileCondition <<  ConditionalBlock::normalize_filename(file.substr(offset).c_str());
    std::string fileVar(fileCondition.str());

    // if file precondition has already been tested...
    if (filesolvable.find(fileVar) != filesolvable.end()) {
        // and conflicts with user defined lists, don't add it to formula
        if (!filesolvable[fileVar]) {
            logger << warn << "File " << file << " not included - " <<
                "conflict with white-/blacklist" << std::endl;
            return false;
        }
    } else {
    // ...otherwise test it and remember the result
        if (model) {
            std::set<std::string> missing;
            std::string intersected;

            model->doIntersect("(" + fileCondition.str() + ")", NULL, missing, intersected);
            if (!intersected.empty()) {
                fileCondition << " && " << intersected;
            }

            SatChecker fileChecker(fileCondition.str());
            if (!fileChecker()) {
                filesolvable[fileVar] = false;
                logger << warn << "File condition for location " << locationname
                    << " conflicting with black-/whitelist - not added"
                    << std::endl;
                return false;
            } else {
                filesolvable[fileVar] = true;
                sj.push_back(intersected);
            }
        }
        sj.push_back(fileVar);
    }

    ConditionalBlock * block = cpp.getBlockAtPosition(locationname);
    if (block == 0) {
        logger << info << "No block found at " << locationname << std::endl;
        return false;
    }

    // Get the precondition for current block
    DeadBlockDefect dead(block);
    std::string precondition = dead.getBlockPrecondition(model);

    logger << info << "Processing block " << block->getName() << std::endl;

    // check for satisfiability of block precondition before joining it
    try {
        SatChecker constraintChecker(precondition);
        if (!constraintChecker()) {
            logger << warn << "Code constraints for " << block->getName() <<
                " not satisfiable - override by black-/whitelist" << std::endl;
            return false;
        } else {
            sj.push_back(precondition);
        }
    } catch (std::runtime_error &e) {
        logger << error << "failed: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void process_mergeblockconf(const char *filepath) {
    /* Read files from worklist */
    std::ifstream workfile(filepath);
    std::string line;
    if (!workfile.good()) {
        usage(std::cout, "worklist was not found");
        exit(EXIT_FAILURE);
    }

    /* set extended Blockname */
    ConditionalBlock::setBlocknameWithFilename(true);

    StringJoiner sj;
    std::map<std::string,bool> filesolvable;
    while(std::getline(workfile, line)) {
        process_blockconf_helper(sj, filesolvable, line.c_str());
    }

    KconfigWhitelist *whitelist = KconfigWhitelist::getWhitelist();
    for(std::list<std::string>::const_iterator iterator = (*whitelist).begin(),
            end = (*whitelist).end(); iterator != end; ++iterator) {
        sj.push_back(*iterator);
    }

    KconfigWhitelist *blacklist = KconfigWhitelist::getBlacklist();
    for(std::list<std::string>::const_iterator iterator = (*blacklist).begin(),
            end = (*blacklist).end(); iterator != end; ++iterator) {
        sj.push_back("!"+(*iterator));
    }

    SatChecker sc(sj.join("\n&&\n"));
    MissingSet missingSet;

    // We want minimal configs, so we try to get many 'n's from the sat checker
    if (sc(Picosat::SAT_MIN)) {
        logger << info << "Solution found, result:" << std::endl;
        SatChecker::AssignmentMap current_solution = sc.getAssignment();
        stringstream configstream;
        current_solution.formatKconfig(configstream, missingSet);
        std::cout << configstream.str();
    } else {
        logger << error << "Wasn't able to generate a valid configuration" << std::endl;
    }
}

void process_blockconf(const char *locationname) {
    StringJoiner sj;
    std::map<std::string,bool> filesolvable;

    process_blockconf_helper(sj, filesolvable, locationname);

    SatChecker sc(sj.join("\n&&\n"));

    MissingSet missingSet;
    if (sc(Picosat::SAT_MIN)) {
        SatChecker::AssignmentMap current_solution = sc.getAssignment();
        stringstream configstream;
        current_solution.formatKconfig(configstream, missingSet);
        std::cout << configstream.str();
    }
}

void process_file_coverage_helper(const char *filename) {
    CppFile file(filename);

    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    } else if (decision_coverage) {
        file.decisionCoverage();
    }

    std::list<SatChecker::AssignmentMap> solution;
    std::map<std::string,std::string> parents;

    ModelContainer *f = ModelContainer::getInstance();
    ConfigurationModel *model = f->lookupMainModel();

    if (!model)
        logger << debug << "Running without a model!" << std::endl;

    // HACK: make B00 a 'regular' block
    file.push_front(file.topBlock());

    SimpleCoverageAnalyzer simple_analyzer(&file);
    MinimizeCoverageAnalyzer minimize_analyzer(&file);
    CoverageAnalyzer *analyzer = 0;
    if (coverageOperationMode == COVERAGE_OP_MINIMIZE) {
        logger << debug << "Calculating configurations using the 'greedy"
               << (decision_coverage ? " and decision coverage'" : "'") << " approach" << std::endl;
        analyzer = &minimize_analyzer;
    } else if (coverageOperationMode == COVERAGE_OP_SIMPLE) {
        logger << debug << "Calculating configurations using the 'simple"
               << (decision_coverage ? " and decision coverage'" : "'") << " approach" << std::endl;
        analyzer = &simple_analyzer;
    } else
        assert(false);

    solution = analyzer->blockCoverage(model);
    MissingSet missingSet = analyzer->getMissingSet();

    if (coverageOutputMode == COVERAGE_MODE_STDOUT) {
        SatChecker::pprintAssignments(std::cout, solution, model, missingSet);
        file.pop_front();
        return;
    }

    int cruft = 0;
    std::string pattern(filename);
    pattern.append(".cppflags*");
    cruft += rm_pattern(pattern.c_str());

    pattern = filename;
    pattern.append(".source*");
    cruft += rm_pattern(pattern.c_str());

    pattern = filename;
    pattern.append(".config*");
    cruft += rm_pattern(pattern.c_str());

    logger << info << "Removed " << cruft << " leftovers for "
           << filename << std::endl;

    int config_count = 1;
    std::vector<bool> blocks(file.size(), false); // bitvector

    std::list<SatChecker::AssignmentMap>::iterator it;
    unsigned current(0);
    for (it = solution.begin(); it != solution.end(); it++, current++) {
        static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
        SatChecker::AssignmentMap::iterator j;
        std::stringstream outfstream;
        outfstream << filename << ".config" << config_count++;
        std::ofstream outf;

        (*it).setEnabledBlocks(blocks);

        if (coverageOutputMode == COVERAGE_MODE_KCONFIG
            || coverageOutputMode == COVERAGE_MODE_MODEL
            || coverageOutputMode == COVERAGE_MODE_ALL) {
            outf.open(outfstream.str().c_str(), std::ios_base::trunc);
            if (!outf.good()) {
                logger << error <<" failed to write config in "
                       << outfstream.str().c_str()
                       << std::endl;
                outf.close();
                continue;
            }
        }

        switch (coverageOutputMode) {
        case COVERAGE_MODE_KCONFIG:
            (*it).formatKconfig(outf, missingSet);
            break;
        case COVERAGE_MODE_MODEL:
            if (!model) {
                logger << error << "no model loaded but, model output mode specified" << std::endl;
                exit(1);
            }
            (*it).formatModel(outf, model);
            break;
        case COVERAGE_MODE_ALL:
            (*it).formatAll(outf);
            break;
        case COVERAGE_MODE_CPP:
            (*it).formatCPP(std::cout, model);
            break;
        case COVERAGE_MODE_EXEC:
            (*it).formatExec(file, coverage_exec_cmd);
            break;
        case COVERAGE_MODE_COMBINED:
            (*it).formatCombined(file, model, missingSet, current);
            break;
        case COVERAGE_MODE_COMMENTED:
            // TODO creates preprocessed source in *.config$n
            (*it).formatCommented(outf, file);
            break;
        default:
            assert(false);
        }
        outf.close();
    }

    // statistics
    int enabled_blocks = 0;
    for (std::vector<bool>::iterator b = blocks.begin(); b != blocks.end(); b++) {
        if (*b)
            enabled_blocks++;
    }

    float ratio = 100.0 * enabled_blocks / file.size();

    logger << info << filename << ", "
           << "Found Solutions: " << solution.size() << ", " // #found solutions
           << "Coverage: " << enabled_blocks << "/" << file.size() << " blocks enabled "
           << "(" << ratio <<  "%)" << std::endl;

    // undo hack to avoid de-allocation failures
    file.pop_front();
}

void process_file_coverage (const char *filename) {
    boost::thread t(process_file_coverage_helper, filename);

    if (!t.timed_join(boost::posix_time::seconds(120)))
        logger << error << "timeout passed while processing " << filename
               << std::endl;
}

void process_file_cpppc(const char *filename) {
    CppFile s(filename);

    if (!s.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    } else if (decision_coverage) {
        s.decisionCoverage();
    }

    logger << info << "CPP Precondition for " << filename << std::endl;
    try {
        StringJoiner sj;
        ModelContainer *f = ModelContainer::getInstance();
        std::string code_formula = s.topBlock()->getCodeConstraints();

        sj.push_back(code_formula);
        if (f && f->size() > 0) {
            ConfigurationModel *model = f->lookupMainModel();
            std::set<std::string> missingSet;

            model->doIntersect(code_formula, NULL, missingSet, code_formula);
            sj.push_back(code_formula);
        }
        std::cout << sj.join("\n&& ") << std::endl;
    } catch (std::runtime_error &e) {
        logger << error << "failed: " << e.what() << std::endl;
        return;
    }
}

void process_file_cppsym_helper(const char *filename) {
    // vector of length 2, first: #references, second: #rewrites
    typedef std::vector<size_t> ItemStats;
    // key: name of the item.
    typedef std::map<std::string, ItemStats> FoundItems;

    CppFile s(filename);

    if (!s.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ModelContainer *f = ModelContainer::getInstance();
    ConfigurationModel *model = f->lookupMainModel();

    FoundItems found_items;

    for (CppFile::const_iterator c = s.begin(); c != s.end(); ++c) {
        ConditionalBlock *block = *c;
        std::string expr = block->ifdefExpression();

        std::set<std::string> items = ConditionalBlock::itemsOfString(expr);
        for (std::set<std::string>::const_iterator i = items.begin(); i != items.end(); i++) {
            static const boost::regex valid_item("^([A-Za-z_][0-9A-Za-z_]*?)(\\.*)(_MODULE)?$");
            boost::match_results<std::string::const_iterator> what;

            if (boost::regex_match(*i, what, valid_item)) {
                size_t rewrites = what[2].length();
                const std::string item_name = what[1];
                FoundItems::iterator it = found_items.find(item_name);

                if (it == found_items.end()) {
                    ItemStats s(2);
                    s[0]++; // increase refcount
                    found_items[item_name] = s;
                } else {
                    ItemStats &s = (*it).second;
                    s[0]++; // increase reference cound
                    s[1] = std::max(s[1], rewrites);
                }
            } else {
                logger << info << "Ignoring non-symbol: " << *i << std::endl;
            }
        }
    }

    for (FoundItems::const_iterator i = found_items.begin(); i != found_items.end(); ++i) {
        StringJoiner sj;
        static const boost::regex kconfig_regexp("^CONFIG_.+");
        boost::match_results<std::string::const_iterator> what;
        const std::string &item_name = (*i).first;
        const ItemStats &stats = (*i).second;
        std::ostringstream references;
        std::ostringstream rewrites;

        sj.push_back(item_name);

        references << stats[0];
        sj.push_back(references.str());

        rewrites << stats[1];
        sj.push_back(rewrites.str());

        if (model) {
            if (model->containsSymbol(item_name)) {
                sj.push_back("PRESENT");
                sj.push_back(model->getType(item_name));
            } else {
                sj.push_back("MISSING");
                sj.push_back(boost::regex_match(item_name, kconfig_regexp) ?
                             "CONFIG_LIKE" : "NOT_CONFIG_LIKE");
            }
        } else {
            sj.push_back("NO_MODEL");
            sj.push_back(boost::regex_match(item_name, kconfig_regexp) ?
                         "CONFIG_LIKE" : "NOT_CONFIG_LIKE");
        }
        assert(sj.size() == 5);
        std::cout << sj.join(", ") << std::endl;
    }
}

void process_file_cppsym(const char *filename) {
    boost::thread t(process_file_cppsym_helper, filename);

    if (!t.timed_join(boost::posix_time::seconds(30)))
        logger << error << "timeout passed while processing " << filename
               << std::endl;
}

void process_file_blockrange(const char *filename) {
    CppFile cpp(filename);

    if (!cpp.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ConditionalBlock *block = cpp.topBlock();
    std::cout << filename << ":" << block->getName() << ":";
    std::cout << block->lineStart() << ":" << block->lineEnd() << std::endl;
    /* Iterate over all Blocks */
    for (CppFile::iterator c = cpp.begin(); c != cpp.end(); ++c) {
        block = *c;
        std::cout << filename << ":" << block->getName() << ":";
        std::cout << block->lineStart() << ":" << block->lineEnd() << std::endl;
    }
}

void process_file_blockpc(const char *filename) {
    std::string fname = std::string(filename);
    std::string file, position;
    size_t colon_pos = fname.find_first_of(':');

    if (colon_pos == fname.npos) {
        logger << error << "invalid format for block precondition" << std::endl;
        return;
    }

    file = fname.substr(0, colon_pos);
    position = fname.substr(colon_pos + 1);

    CppFile cpp(file.c_str());
    if (!cpp.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ConditionalBlock * block = cpp.getBlockAtPosition(filename);

    if (block == 0) {
        logger << info << "No block at " << filename << " was found." << std::endl;
        return;
    }

    ConfigurationModel *model = ModelContainer::lookupMainModel();
    const BlockDefectAnalyzer *defect = 0;
    try {
        defect = BlockDefectAnalyzer::analyzeBlock(block, model);
    } catch (SatCheckerError &e) {
        logger << error << "Couldn't process " << filename << ":" << block->getName() << ": "
               << e.what() << std::endl;
    }

    // /* Get the Precondition */
    DeadBlockDefect dead(block);
    std::string precondition = dead.getBlockPrecondition(model);

    std::string defect_string = "no";
    if (defect) {
         defect_string = defect->getSuffix() + "/" + defect->defectTypeToString();
    }

    logger << info << "Block " << block->getName()
           << " | Defect: " << defect_string
           << " | Global: " << (defect != 0 ? defect->isGlobal() : 0)<< std::endl;

    std::cout << precondition << std::endl;
}

void process_file_dead_helper(const char *filename) {
    CppFile file(filename);
    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    // delete potential leftovers from previous run
    std::string pattern(filename);
    pattern.append("*.*dead");
    rm_pattern(pattern.c_str());

    ConfigurationModel *mainModel = ModelContainer::lookupMainModel();

    /* process File (B00 Block) */
    try {
        const BlockDefectAnalyzer *defect = BlockDefectAnalyzer::analyzeBlock(file.topBlock(), mainModel);
        if (defect) {
            defect->writeReportToFile(skip_non_configuration_based_defects);
            if (do_mus_analysis) {
                defect->reportMUS();
            }
            delete defect;
        }
    } catch (SatCheckerError &e) {
        logger << error << "Couldn't process " << filename << ":B00: "
               << e.what() << std::endl;
    }

    /* Iterate over all Blocks */
    for (CppFile::iterator c = file.begin(); c != file.end(); c++) {
        ConditionalBlock *block = *c;

        try {
            const BlockDefectAnalyzer *defect = BlockDefectAnalyzer::analyzeBlock(block, mainModel);
            if (defect) {
                defect->writeReportToFile(skip_non_configuration_based_defects);
                if (do_mus_analysis) {
                    defect->reportMUS();
                }
                delete defect;
            }
        } catch (SatCheckerError &e) {
            logger << error << "Couldn't process " << filename << ":" << block->getName() << ": "
                   << e.what() << std::endl;
        }
    }
}

void process_file_dead(const char *filename) {
    boost::thread t(process_file_dead_helper, filename);
    static unsigned int timeout = 120; // default timeout in seconds

    ConfigurationModel *model = ModelContainer::lookupMainModel();
    if (model && !strcmp("cnf", model->getModelVersionIdentifier())) {
        logger << debug << "Increasing timeout for dead analysis to 3600 seconds" << std::endl;
        timeout = 3600;
    }

    if (!t.timed_join(boost::posix_time::seconds(timeout)))
        logger << error << "timeout passed while processing " << filename
               << std::endl;
}

void process_file_interesting(const char *filename) {
    RsfConfigurationModel *model = dynamic_cast<RsfConfigurationModel *> (ModelContainer::lookupMainModel());

    if (!model) {
        logger << error << "for finding interessting items a (rsf based) model must be loaded" << std::endl;
        return;
    }

    std::set<std::string> initialItems;
    initialItems.insert(filename);

    /* Find all items that are related to the given item */
    std::set<std::string> interesting = model->findSetOfInterestingItems(initialItems);

    /* remove the given item again */
    interesting.erase(filename);
    std::cout << filename;

    for(std::set<std::string>::const_iterator it = interesting.begin(); it != interesting.end(); ++it) {
        if (model->find(*it) != model->end()) {
            /* Item is present in model */
            std::cout << " " << *it;
        } else {
            /* Item is missing in this model */
            std::cout << " !" << *it;
        }
    }
    std::cout << std::endl;
}

void process_file_checkexpr(const char *expression) {
    ConfigurationModel *model = ModelContainer::lookupMainModel();
    logger << debug << "Checking expr " << expression << std::endl;

    if (!model) {
        logger << error << "for finding interessting items a model must be loaded" << std::endl;
        return;
    }

    assert(model != NULL); // some main model must be loaded

    StringJoiner sj;
    std::set<std::string> missing;
    std::string intersected;

    sj.push_back(expression);

    model->doIntersect("(" + std::string(expression) + ")",
                       0, // No ConfigurationModel::Checker
                       missing, intersected);

    sj.push_back(intersected);
    sj.push_back(ConfigurationModel::getMissingItemsConstraints(missing));

    std::string formula = sj.join("\n&&\n");
    logger << debug << "formula: " << formula << std::endl;

    SatChecker sc(formula);
    if (sc()) {
        SatChecker::AssignmentMap current_solution = sc.getAssignment();
        current_solution.formatKconfig(std::cout, missing);
    }
    else {
        logger << info << "Expression is NOT satisfiable" << std::endl;
        RETVALUE = EXIT_FAILURE;
    }
}

void process_file_symbolpc(const char *filename) {
    ConfigurationModel *model = ModelContainer::lookupMainModel();

    if (!model) {
        logger << error << "for symbolpc models must be loaded" << std::endl;
        return;
    }

    assert(model != NULL); // there should always be a main model loaded

    std::set<std::string> missingItems;

    logger << info << "Symbol Precondition for `" << filename << "'" << std::endl;


    /* Find all items that are related to the given item */
    std::string result;
    int valid_items = model->doIntersect(std::string(filename), 0, missingItems, result);
    std::cout << result << std::endl;

    if (missingItems.size() > 0) {
        /* given symbol is in the model */
        if (valid_items != 0)
            std::cout <<  "\n&&" << std::endl;
        std::cout << ConfigurationModel::getMissingItemsConstraints(missingItems);
    }

    std::cout << std::endl;;
}

process_file_cb_t parse_job_argument(const char *arg) {
    if (strcmp(arg, "dead") == 0) {
        return process_file_dead;
    } else if (strcmp(arg, "coverage") == 0) {
        return process_file_coverage;
    } else if (strcmp(arg, "cpppc") == 0) {
        return process_file_cpppc;
    } else if (strcmp(arg, "cpppc_decision") == 0) {
        decision_coverage = true;
        return process_file_cpppc;
    } else if (strcmp(arg, "cppsym") == 0) {
        return process_file_cppsym;
    } else if (strcmp(arg, "blockpc") == 0) {
        return process_file_blockpc;
    }  else if (strcmp(arg, "blockrange") == 0) {
        return process_file_blockrange;
    } else if (strcmp(arg, "interesting") == 0) {
        return process_file_interesting;
    } else if (strcmp(arg, "checkexpr") == 0) {
        return process_file_checkexpr;
    } else if (strcmp(arg, "symbolpc") == 0) {
        return process_file_symbolpc;
    } else if (strcmp(arg, "blockconf") == 0) {
        return process_blockconf;
    } else if (strcmp(arg, "mergeblockconf") == 0) {
        return process_mergeblockconf;
    }
    return NULL;
}

void wait_for_forked_child(pid_t new_pid, int threads = 1, const char *argument = 0, bool print_stats = false) {
    static struct {
        int ok, failed, signaled;
    } process_stats;

    static std::map<pid_t, const char *> arguments;
    static int running_processes = 0;

    if (new_pid) {
        running_processes ++;
        if (argument)
            arguments[new_pid] = argument;
    }

    while (running_processes >= threads) {
        int state = 0;
        pid_t pid = waitpid(-1, &state, 0);
        if (pid == -1) break;

        if (WIFEXITED(state)) {
            if (WEXITSTATUS(state) == 0) {
                process_stats.ok ++;
                arguments.erase(pid);
            } else {
                process_stats.failed ++;
                logger << error << "Process (pid: " << pid
                       << ", args: " << arguments[pid]
                       << ") failed with exitcode "
                       << WEXITSTATUS(state) << std::endl;
            }
        } else if (WIFSIGNALED(state)) {
            process_stats.signaled++;
            logger << error << "Process (pid: " << pid
                   << ", args: " << arguments[pid]
                   << ") failed with signal " << WTERMSIG(state) << std::endl;
        } else {
            continue;
        }
        running_processes --;
    }

    if (process_stats.failed > 0)
        RETVALUE = EXIT_FAILURE;

    if (print_stats) {
        /* Shutdown phase */
        logger << info << "Sucessful processed:  " << process_stats.ok << std::endl;
        logger << info << "Failed with exitcode: " << process_stats.failed << std::endl;
        logger << info << "Failed with signal:   " << process_stats.signaled << std::endl;
        for (std::map<pid_t, const char *>::const_iterator it = arguments.begin() ; it != arguments.end(); it++ )
            logger << info << "Failed file: " << (*it).second << std::endl;
    }
}

int main (int argc, char ** argv) {
    int opt;
    char *worklist = NULL;
    long threads = 1;
    std::list<std::string> models_from_parameters;
    std::string main_model = "x86";
    /* Default is dead/undead analysis */
    std::string process_mode = "dead";
    process_file_cb_t process_file = process_file_dead;

    int loglevel = logger.getLogLevel();

    /* Command line structure:
       - Model Options:
       * Per Default no models are loaded
       -- main model     -M: Specify main model
       -- model          -m: Load a specifc model (if a dir is given
       - Work Options:
       -- job            -j: do a specific job
       load all model files in directory)
    */
    coverageOutputMode = COVERAGE_MODE_KCONFIG;

    while ((opt = getopt(argc, argv, "ucb:M:m:t:i:B:W:sj:O:C:I:Vhvq")) != -1) {
        switch (opt) {
            int n;
            KconfigWhitelist *wl;
        case 'i':
            wl = KconfigWhitelist::getIgnorelist();
            n = wl->loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to ignorelist" << std::endl;
            } else {
                logger << error << "couldn't load ignorelist" << std::endl;
                exit(-1);
            }
            break;
        case 'W':
            wl = KconfigWhitelist::getWhitelist();
            n = wl->loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to whitelist" << std::endl;
            } else {
                logger << error << "couldn't load whitelist" << std::endl;
                exit(-1);
            }
            break;
        case 'B':
            wl = KconfigWhitelist::getBlacklist();
            n = wl->loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to blacklist" << std::endl;
            } else {
                logger << error << "couldn't load blacklist" << std::endl;
                exit(-1);
            }
            break;
        case 'b':
            worklist = strdup(optarg);
            break;
        case 'u':
            if (std::system("which picomus > /dev/null") == 0)
                do_mus_analysis = true;
            else
                logger << error << "Cannot do MUS-Analysis: picomus not in PATH. Continuing without MUS-Analysis." << std::endl;
            break;
        case 'c':
            process_file = process_file_coverage;
            break;
        case 'O':
            if (0 == strcmp(optarg, "kconfig"))
                coverageOutputMode = COVERAGE_MODE_KCONFIG;
            else if (0 == strcmp(optarg, "stdout"))
                coverageOutputMode = COVERAGE_MODE_STDOUT;
            else if (0 == strcmp(optarg, "model"))
                coverageOutputMode = COVERAGE_MODE_MODEL;
            else if (0 == strcmp(optarg, "cpp"))
                coverageOutputMode = COVERAGE_MODE_CPP;
            else if (0 == strcmp(optarg, "all"))
                coverageOutputMode = COVERAGE_MODE_ALL;
            else if (0 == strcmp(optarg, "commented"))
                coverageOutputMode = COVERAGE_MODE_COMMENTED;
            else if (0 == strcmp(optarg, "combined"))
                coverageOutputMode = COVERAGE_MODE_COMBINED;
            else if (0 == strncmp(optarg, "exec", 4)) {
                coverageOutputMode = COVERAGE_MODE_EXEC;
                if (optarg[4] == ':')
                    coverage_exec_cmd = &optarg[5];
            }
            else
                logger << warn << "mode " << optarg << " is unknown, using 'kconfig' instead"
                       << std::endl;
            break;
        case 'C':
            if (0 == strcmp(optarg, "simple")) {
                coverageOperationMode = COVERAGE_OP_SIMPLE;
            } else if (0 == strcmp(optarg, "min")) {
                coverageOperationMode = COVERAGE_OP_MINIMIZE;
            } else if (0 == strcmp(optarg, "simple_decision")) {
                decision_coverage = true;
                coverageOperationMode = COVERAGE_OP_SIMPLE;
            } else if (0 == strcmp(optarg, "min_decision")) {
                decision_coverage = true;
                coverageOperationMode = COVERAGE_OP_MINIMIZE;
            } else {
                logger << warn << "mode " << optarg << " is unknown, using 'simple' instead"
                       << std::endl;
            }
            break;
        case 't':
            threads = strtol(optarg, (char **)0, 10);
            if ((errno == ERANGE && (threads == LONG_MAX || threads == LONG_MIN))
                || (errno != 0 && threads == 0)) {
                perror("strtol");
                exit(EXIT_FAILURE);
            }
            if (threads < 1) {
                logger << warn << "Invalid numbers of threads, using 1 instead." << std::endl;
                threads = 1;
            }
            break;
        case 'M':
            /* Specify a new main arch */
            main_model = std::string(optarg);
            break;
        case 'm':
            models_from_parameters.push_back(std::string(optarg));
            break;
        case 'j':
            /* assign a new function pointer according to the jobs
               which should be done */
            process_file = parse_job_argument(optarg);
            process_mode = std::string(optarg);
            if (!process_file) {
                usage(std::cerr, "Invalid job specified");
                return EXIT_FAILURE;
            }
            break;
        case 'I':
            PumaConditionalBlockBuilder::addIncludePath(optarg);
            break;
        case 's':
            skip_non_configuration_based_defects = true;
            break;
        case 'h':
            usage(std::cout, NULL);
            exit(0);
            break;
        case 'V':
            std::cout << "undertaker " << version << std::endl;
            exit(0);
            break;
        case 'q':
            loglevel = loglevel + 10;
            logger.setLogLevel(loglevel);
            break;
        case 'v':
            loglevel = loglevel - 10;
            if (loglevel < 0)
                loglevel = Logging::LOG_EVERYTHING;
            logger.setLogLevel(loglevel);
            break;
        default:
            break;
        }
    }

    logger << debug << "undertaker " << version << std::endl;

    if (!worklist && optind >= argc) {
        usage(std::cout, "please specify a file to scan or a worklist");
        return EXIT_FAILURE;
    }

    ModelContainer *model_container = ModelContainer::getInstance();
    KconfigWhitelist *bl = KconfigWhitelist::getBlacklist();
    KconfigWhitelist *wl = KconfigWhitelist::getWhitelist();

    if (0 == models_from_parameters.size() && (!bl->empty() || !wl->empty())) {
        usage(std::cout, "please specify a model to use white- or blacklists");
        return EXIT_FAILURE;
    }

    /* Load all specified models */
    for (std::list<std::string>::const_iterator i = models_from_parameters.begin();
            i != models_from_parameters.end(); ++i) {
        if (!model_container->loadModels(*i))
            logger << error << "Failed to load model " << *i << std::endl;
    }
    /* Add white- and blacklisted features to all models */
    for (std::map<std::string, ConfigurationModel *>::const_iterator i = model_container->begin();
            i != model_container->end(); ++i) {
        ConfigurationModel *model = (*i).second;
        std::list<std::string>::iterator itl;
        for (itl = bl->begin(); !bl->empty() && itl != bl->end(); itl++)
            model->addFeatureToBlacklist(*itl);

        for (itl = wl->begin(); !wl->empty() && itl != wl->end(); itl++)
            model->addFeatureToWhitelist(*itl);
    }

    std::vector<std::string> workfiles;
    if (!worklist) {
        /* Use files from command line */
        do {
            workfiles.push_back(argv[optind++]);
        } while (optind < argc);
    } else {
        /* Read files from worklist */
        std::ifstream workfile(worklist);
        std::string line;
        if (! workfile.good()) {
            usage(std::cout, "worklist was not found");
            exit(EXIT_FAILURE);

        }
        /* Collect all files that should be worked on */
        while(std::getline(workfile, line)) {
            workfiles.push_back(line);
        }
    }

    /* Specify main model, if models where loaded */
    if (model_container->size() == 1) {
        /* If there is only one model file loaded use this */
        model_container->setMainModel(model_container->begin()->first);
    } else if (model_container->size() > 1) {
        model_container->setMainModel(main_model);
    }

    /* Read from stdin after loading all models and whitelist */
    if (workfiles.size() > 0 && workfiles.begin()->compare("-") == 0) {
        std::string line;
        /* Read from stdin and call process file for every line */
        while (1) {
            std::cout << process_mode << ">>> ";
            if (!std::getline(std::cin, line)) break;
            if (line.compare(0, 2, "::") == 0) {
                /* Change mode */
                std::string new_mode;
                size_t space;
                /* Possible valid inputs:
                   ::<mode>
                   ::<mode> <file>
                */
                if ((space = line.find(" ")) == line.npos) {
                    new_mode = line.substr(2);
                    line = "";
                } else {
                    new_mode = line.substr(2, space - 2);
                    line = line.substr(space + 1);
                }
                if (new_mode.compare("load") == 0) {
                    model_container->loadModels(line);
                    continue;
                } else if (new_mode.compare("main-model") == 0) {
                    ConfigurationModel *db = model_container->loadModels(line);
                    if (db) {
                        model_container->setMainModel(db->getName());
                    }
                    continue;
                } else { /* Change working mode */
                    process_file_cb_t new_function = parse_job_argument(new_mode.c_str());
                    if (!new_function) {
                        logger << error << "Invalid new working mode: " << new_mode << std::endl;
                        continue;
                    }
                    /* now change the pointer and the working mode */
                    process_file = new_function;
                    process_mode = new_mode;
                }
            }
            if (line.size() > 0)
                process_file(line.c_str());
        }
    } else {
        if (workfiles.size() > 1) {
            for (unsigned int i = 0; i < workfiles.size(); i++) {
                pid_t pid = fork();
                if (pid == 0) { /* child */
                    /* calling the function pointer */
                    process_file(workfiles[i].c_str());
                    return RETVALUE;
                } else if (pid < 0) {
                    logger << error << "forking failed. Exiting." << std::endl;
                    return EXIT_FAILURE;
                } else { /* Father process */
                    wait_for_forked_child(pid, threads, workfiles[i].c_str());
                }
            }
            /* Wait until fork count reaches zero */
            wait_for_forked_child(0, 0, 0, threads > 1);
        } else {
            if (workfiles.size() > 0)
                process_file(workfiles[0].c_str());
        }
    }
    return RETVALUE;
}
