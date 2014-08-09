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
#include "StringJoiner.h"
#include "KconfigWhitelist.h"
#include "ModelContainer.h"
#include "RsfConfigurationModel.h"
#include "PumaConditionalBlock.h"
#include "ConditionalBlock.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "CoverageAnalyzer.h"
#include "Logging.h"
#include "Tools.h"
#include "../version.h"

#include <errno.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <glob.h>

#include <boost/regex.hpp>
#include <boost/thread.hpp>


using process_file_cb_t = void (*)(const std::string &filename);

enum class CoverageOutput {
    KCONFIG,    // kconfig specific mode
    STDOUT,     // prints out on stdout generated configurations
    MODEL,      // prints all configuration space itms, no blocks
    CPP,        // prints all cpp items as cpp cmd-line arguments
    EXEC,       // pipe file for every configuration to cmd
    ALL,        // prints all items and blocks
    COMBINED,   // create files for configuration, kconfig and modified source file
    COMMENTED,  // creates file contining modified source
} coverageOutputMode;

enum class CoverageMode {
    SIMPLE,    // simple, fast
    MINIMIZE,  // hopefully minimal configuration set
} coverageMode;

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

    glob(pattern, 0, nullptr, &globbuf);
    int nr = globbuf.gl_pathc;
    for (int i = 0; i < nr; i++)
        if (0 != unlink(globbuf.gl_pathv[i]))
            fprintf(stderr, "E: Couldn't unlink %s: %m", globbuf.gl_pathv[i]);

    globfree(&globbuf);
    return nr;
}

bool process_blockconf_helper(StringJoiner &sj, std::map<std::string, bool> &filesolvable,
                              const std::string &locationname) {
    // used by process_blockconf and process_mergedblockconf

    boost::regex regex("(.*):[0-9]+");
    boost::smatch results;
    if (!boost::regex_match(locationname, results, regex)) {
        logger << error << "invalid format for block precondition" << std::endl;
        return false;
    }

    std::string file = results[1];

    CppFile cpp(file);
    if (!cpp.good()) {
        logger << error << "failed to open file: `" << file << "'" << std::endl;
        return false;
    }
    // if the current file is arch specific, use only the matching model for analyses
    ConfigurationModel *main_model;
    if (cpp.getSpecificArch() != "")
        main_model = ModelContainer::lookupModel(cpp.getSpecificArch());
    else
        main_model = ModelContainer::lookupMainModel();

    int offset = strncmp("./", file.c_str(), 2) ? 0 : 2;
    std::stringstream fileCondition;
    fileCondition << "FILE_";
    fileCondition << undertaker::normalize_filename(file.substr(offset));
    std::string fileVar(fileCondition.str());

    // if file precondition has already been tested...
    if (filesolvable.find(fileVar) != filesolvable.end()) {
        // and conflicts with user defined lists, don't add it to formula
        if (!filesolvable[fileVar]) {
            logger << warn << "File " << file << " not included - "
                   << "conflict with white-/blacklist" << std::endl;
            return false;
        }
    } else {
        // ...otherwise test it and remember the result
        if (main_model) {
            std::set<std::string> missing;
            std::string intersected;

            main_model->doIntersect("(" + fileCondition.str() + ")", nullptr, missing,
                                    intersected);
            if (!intersected.empty()) {
                fileCondition << " && " << intersected;
            }

            SatChecker fileChecker(fileCondition.str());
            if (!fileChecker()) {
                filesolvable[fileVar] = false;
                logger << warn << "File condition for location " << locationname
                       << " conflicting with black-/whitelist - not added" << std::endl;
                return false;
            } else {
                filesolvable[fileVar] = true;
                sj.push_back(intersected);
            }
        }
        sj.push_back(fileVar);
    }

    ConditionalBlock *block = cpp.getBlockAtPosition(locationname);
    if (block == nullptr) {
        logger << info << "No block found at " << locationname << std::endl;
        return false;
    }

    // Get the precondition for current block
    std::string precondition = BlockDefectAnalyzer::getBlockPrecondition(block, main_model);

    logger << info << "Processing block " << block->getName() << std::endl;

    // check for satisfiability of block precondition before joining it
    try {
        SatChecker constraintChecker(precondition);
        if (!constraintChecker()) {
            logger << warn << "Code constraints for " << block->getName()
                   << " not satisfiable - override by black-/whitelist" << std::endl;
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

void process_mergeblockconf(const std::string &filename) {
    /* Read files from worklist */
    std::ifstream workfile(filename);
    if (!workfile.good()) {
        usage(std::cout, "worklist was not found");
        exit(EXIT_FAILURE);
    }

    /* set extended Blockname */
    ConditionalBlock::setBlocknameWithFilename(true);

    std::string line;
    StringJoiner sj;
    std::map<std::string, bool> filesolvable;
    while (std::getline(workfile, line))
        process_blockconf_helper(sj, filesolvable, line);

    for (const std::string &str : KconfigWhitelist::getWhitelist())
        sj.push_back(str);

    for (const std::string &str : KconfigWhitelist::getBlacklist())
        sj.push_back("!" + str);

    SatChecker sc(sj.join("\n&&\n"));

    // We want minimal configs, so we try to get many 'n's from the sat checker
    if (sc(Picosat::SAT_MIN)) {
        logger << info << "Solution found, result:" << std::endl;
        sc.getAssignment().formatKconfig(std::cout, {});
    } else {
        logger << error << "Wasn't able to generate a valid configuration" << std::endl;
    }
}

void process_blockconf(const std::string &locationname) {
    StringJoiner sj;
    std::map<std::string, bool> filesolvable;
    process_blockconf_helper(sj, filesolvable, locationname);

    SatChecker sc(sj.join("\n&&\n"));

    if (sc(Picosat::SAT_MIN))
        sc.getAssignment().formatKconfig(std::cout, {});
}

void process_file_coverage_helper(const std::string &filename) {
    CppFile file(filename);

    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    } else if (decision_coverage) {
        file.decisionCoverage();
    }
    // HACK: make B00 a 'regular' block
    file.push_front(file.topBlock());

    SimpleCoverageAnalyzer simple_analyzer(&file);
    MinimizeCoverageAnalyzer minimize_analyzer(&file);
    CoverageAnalyzer *analyzer = nullptr;
    if (coverageMode == CoverageMode::MINIMIZE) {
        logger << debug << "Calculating configurations using the 'greedy"
               << (decision_coverage ? " and decision coverage'" : "'") << " approach"
               << std::endl;
        analyzer = &minimize_analyzer;
    } else if (coverageMode == CoverageMode::SIMPLE) {
        logger << debug << "Calculating configurations using the 'simple"
               << (decision_coverage ? " and decision coverage'" : "'") << " approach"
               << std::endl;
        analyzer = &simple_analyzer;
    } else {
        assert(false);
    }
    // if the current file is arch specific, use only the matching model for analyses
    ConfigurationModel *main_model;
    if (file.getSpecificArch() != "")
        main_model = ModelContainer::lookupModel(file.getSpecificArch());
    else
        main_model = ModelContainer::lookupMainModel();

    if (!main_model)
        logger << debug << "Running without a model!" << std::endl;

    std::list<SatChecker::AssignmentMap> solutions = analyzer->blockCoverage(main_model);
    MissingSet missingSet = analyzer->getMissingSet();

    if (coverageOutputMode == CoverageOutput::STDOUT) {
        SatChecker::pprintAssignments(std::cout, solutions, main_model, missingSet);
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

    logger << info << "Removed " << cruft << " leftovers for " << filename << std::endl;

    int config_count = 1;
    std::vector<bool> block_bitvector(file.size(), false);

    unsigned int current = 0;
    for (auto &solution : solutions) {  // Satchecker::AssignmentMap
        static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
        std::stringstream outfstream;
        outfstream << filename << ".config" << config_count++;
        std::ofstream outf;

        solution.setEnabledBlocks(block_bitvector);

        if (coverageOutputMode == CoverageOutput::KCONFIG
            || coverageOutputMode == CoverageOutput::MODEL
            || coverageOutputMode == CoverageOutput::ALL) {
            outf.open(outfstream.str(), std::ios_base::trunc);
            if (!outf.good()) {
                logger << error << " failed to write config in " << outfstream.str() << std::endl;
                outf.close();
                current++;
                continue;
            }
        }

        switch (coverageOutputMode) {
        case CoverageOutput::KCONFIG:
            solution.formatKconfig(outf, missingSet);
            break;
        case CoverageOutput::MODEL:
            if (!main_model) {
                logger << error << "no model loaded but, model output mode specified" << std::endl;
                exit(1);
            }
            solution.formatModel(outf, main_model);
            break;
        case CoverageOutput::ALL:
            solution.formatAll(outf);
            break;
        case CoverageOutput::CPP:
            solution.formatCPP(std::cout, main_model);
            break;
        case CoverageOutput::EXEC:
            solution.formatExec(file, coverage_exec_cmd);
            break;
        case CoverageOutput::COMBINED:
            solution.formatCombined(file, main_model, missingSet, current);
            break;
        case CoverageOutput::COMMENTED:
            // TODO creates preprocessed source in *.config$n
            solution.formatCommented(outf, file);
            break;
        default:
            assert(false);
        }
        outf.close();
        current++;
    }

    // statistics
    int enabled_blocks = 0;
    for (const bool b : block_bitvector)
        if (b)
            enabled_blocks++;

    float ratio = 100.0 * enabled_blocks / file.size();

    logger << info << filename << ", "
           << "Found Solutions: " << solutions.size() << ", "  // #found solutions
           << "Coverage: " << enabled_blocks << "/" << file.size() << " blocks enabled "
           << "(" << ratio << "%)" << std::endl;

    // undo hack to avoid de-allocation failures
    file.pop_front();
}

void process_file_coverage(const std::string &filename) {
    boost::thread t(process_file_coverage_helper, filename);

    if (!t.timed_join(boost::posix_time::seconds(120))) {
        logger << error << "timeout passed while processing " << filename << std::endl;
        RETVALUE = EXIT_FAILURE;
    }
}

void process_file_cpppc(const std::string &filename) {
    CppFile file(filename);

    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    } else if (decision_coverage) {
        file.decisionCoverage();
    }

    logger << info << "CPP Precondition for " << filename << std::endl;
    try {
        StringJoiner sj;
        std::string code_formula = file.topBlock()->getCodeConstraints();

        sj.push_back(code_formula);
        if (ModelContainer::getInstance().size() > 0) {
            // if the current file is arch specific, use only the matching model for analyses
            ConfigurationModel *main_model;
            if (file.getSpecificArch() != "")
                main_model = ModelContainer::lookupModel(file.getSpecificArch());
            else
                main_model = ModelContainer::lookupMainModel();

            std::set<std::string> missingSet;

            main_model->doIntersect(code_formula, nullptr, missingSet, code_formula);
            sj.push_back(code_formula);
        }
        std::cout << sj.join("\n&& ") << std::endl;
    } catch (std::runtime_error &e) {
        logger << error << "failed: " << e.what() << std::endl;
        return;
    }
}

void process_file_cppsym_helper(const std::string &filename) {
    // vector of length 2, first: #references, second: #rewrites
    typedef std::vector<size_t> ItemStats;
    // key: name of the item.
    typedef std::map<std::string, ItemStats> FoundItems;

    CppFile file(filename);

    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }
    // if the current file is arch specific, use only the matching model for analyses
    ConfigurationModel *main_model;
    if (file.getSpecificArch() != "")
        main_model = ModelContainer::lookupModel(file.getSpecificArch());
    else
        main_model = ModelContainer::lookupMainModel();

    FoundItems found_items;
    static const boost::regex valid_item("^([A-Za-z_][0-9A-Za-z_]*?)(\\.*)(_MODULE)?$");
    for (const auto &block : file) {  // ConditionalBlock *
        std::string expr = block->ifdefExpression();
        for (const std::string &item : undertaker::itemsOfString(expr)) {
            boost::smatch what;

            if (boost::regex_match(item, what, valid_item)) {
                size_t rewrites = what[2].length();
                const std::string item_name = what[1];
                auto it = found_items.find(item_name);

                if (it == found_items.end()) {  // not found in found_items
                    ItemStats stats(2);
                    stats[0]++;  // increase refcount
                    found_items[item_name] = stats;
                } else {
                    ItemStats &stats = (*it).second;
                    stats[0]++;  // increase reference cound
                    stats[1] = std::max(stats[1], rewrites);
                }
            } else {
                logger << info << "Ignoring non-symbol: " << item << std::endl;
            }
        }
    }

    for (const auto &item : found_items) {  // pair<string, ItemStats>
        StringJoiner sj;
        static const boost::regex kconfig_regexp("^CONFIG_.+");
        boost::match_results<std::string::const_iterator> what;
        const std::string &item_name = item.first;
        const ItemStats &stats = item.second;

        sj.push_back(item_name);
        sj.push_back(std::to_string(stats[0]));  // # references
        sj.push_back(std::to_string(stats[1]));  // # rewrites

        if (main_model) {
            if (main_model->containsSymbol(item_name)) {
                sj.push_back("PRESENT");
                sj.push_back(main_model->getType(item_name));
            } else {
                sj.push_back("MISSING");
                sj.push_back(boost::regex_match(item_name, kconfig_regexp) ? "CONFIG_LIKE"
                                                                           : "NOT_CONFIG_LIKE");
            }
        } else {
            sj.push_back("NO_MODEL");
            sj.push_back(boost::regex_match(item_name, kconfig_regexp) ? "CONFIG_LIKE"
                                                                       : "NOT_CONFIG_LIKE");
        }
        assert(sj.size() == 5);
        std::cout << sj.join(", ") << std::endl;
    }
}

void process_file_cppsym(const std::string &filename) {
    boost::thread t(process_file_cppsym_helper, filename);

    if (!t.timed_join(boost::posix_time::seconds(30))) {
        logger << error << "timeout passed while processing " << filename << std::endl;
        RETVALUE = EXIT_FAILURE;
    }
}

void process_file_blockrange(const std::string &filename) {
    CppFile cpp(filename);

    if (!cpp.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    std::cout << filename << ":" << cpp.topBlock()->getName() << ":";
    std::cout << cpp.topBlock()->lineStart() << ":" << cpp.topBlock()->lineEnd() << std::endl;
    /* Iterate over all Blocks */
    for (const auto &block : cpp) {  // ConditionalBlock *
        std::cout << filename << ":" << block->getName() << ":";
        std::cout << block->lineStart() << ":" << block->lineEnd() << std::endl;
    }
}

void process_file_blockpc(const std::string &filename) {
    std::string file, position;
    size_t colon_pos = filename.find_first_of(':');

    if (colon_pos == filename.npos) {
        logger << error << "invalid format for block precondition" << std::endl;
        return;
    }

    file = filename.substr(0, colon_pos);
    position = filename.substr(colon_pos + 1);

    CppFile cpp(file);
    if (!cpp.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ConditionalBlock *block = cpp.getBlockAtPosition(filename);

    if (block == nullptr) {
        logger << info << "No block at " << filename << " was found." << std::endl;
        return;
    }
    // if the current file is arch specific, use only the matching model for analyses
    ConfigurationModel *main_model;
    if (cpp.getSpecificArch() != "")
        main_model = ModelContainer::lookupModel(cpp.getSpecificArch());
    else
        main_model = ModelContainer::lookupMainModel();

    const BlockDefect *defect = BlockDefectAnalyzer::analyzeBlock(block, main_model);
    std::string defect_string = "no";
    if (defect) {
        defect_string = defect->getSuffix() + "/" + defect->defectTypeToString();
    }
    logger << info << "Block " << block->getName() << " | Defect: " << defect_string
           << " | Global: " << (defect != nullptr ? defect->isGlobal() : 0) << std::endl;

    /* Get and print the Precondition */
    std::cout << BlockDefectAnalyzer::getBlockPrecondition(block, main_model) << std::endl;
}

void process_file_dead_helper(const std::string &filename) {
    CppFile file(filename);
    if (!file.good()) {
        logger << error << "failed to open file: `" << filename << "'" << std::endl;
        return;
    }
    // delete potential leftovers from previous run
    std::string pattern(filename);
    pattern.append("*.*dead");
    rm_pattern(pattern.c_str());

    // if the current file is arch specific, use only the matching model for analyses
    ConfigurationModel *main_model;
    if (file.getSpecificArch() != "")
        main_model = ModelContainer::lookupModel(file.getSpecificArch());
    else
        main_model = ModelContainer::lookupMainModel();

    static auto processBlock = [](ConditionalBlock *block, ConfigurationModel *main_model) {
        const BlockDefect *defect = BlockDefectAnalyzer::analyzeBlock(block, main_model);
        if (defect) {
            defect->writeReportToFile(skip_non_configuration_based_defects);
            if (do_mus_analysis)
                defect->reportMUS();
            delete defect;
        }
    };

    /* process File (B00 Block) */
    processBlock(file.topBlock(), main_model);
    /* Iterate over all Blocks */
    for (const auto &block : file)  // ConditionalBlock *
        processBlock(block, main_model);
}

void process_file_dead(const std::string &filename) {
    boost::thread t(process_file_dead_helper, filename);
    static unsigned int timeout = 120;  // default timeout in seconds

    ConfigurationModel *main_model = ModelContainer::lookupMainModel();
    if (main_model && "cnf" == main_model->getModelVersionIdentifier()) {
        logger << debug << "Increasing timeout for dead analysis to 3600 seconds" << std::endl;
        timeout = 3600;
    }

    if (!t.timed_join(boost::posix_time::seconds(timeout))) {
        logger << error << "timeout passed while processing " << filename << std::endl;
        RETVALUE = EXIT_FAILURE;
    }
}

void process_file_interesting(const std::string &check_item) {
    RsfConfigurationModel *main_model
        = dynamic_cast<RsfConfigurationModel *>(ModelContainer::lookupMainModel());

    if (!main_model) {
        logger << error << "for finding interessting items a (rsf based) model must be loaded"
               << std::endl;
        return;
    }
    /* Find all items that are related to the given item */
    std::set<std::string> interesting = main_model->findSetOfInterestingItems({check_item});

    /* remove the given item again */
    interesting.erase(check_item);
    std::cout << check_item;

    for (const std::string &str : interesting) {
        if (main_model->find(str) != main_model->end()) {
            /* Item is present in model */
            std::cout << " " << str;
        } else {
            /* Item is missing in this model */
            std::cout << " !" << str;
        }
    }
    std::cout << std::endl;
}

void process_file_checkexpr(const std::string &expression) {
    ConfigurationModel *main_model = ModelContainer::lookupMainModel();
    logger << debug << "Checking expr " << expression << std::endl;

    if (!main_model) {
        logger << error << "for finding interesting items a model must be loaded" << std::endl;
        return;
    }
    StringJoiner sj;
    std::set<std::string> missing;
    std::string intersected;

    sj.push_back(expression);

    main_model->doIntersect("(" + expression + ")",
                            nullptr,  // No ConfigurationModel::Checker
                            missing, intersected);

    sj.push_back(intersected);
    sj.push_back(ConfigurationModel::getMissingItemsConstraints(missing));

    std::string formula = sj.join("\n&&\n");
    logger << debug << "formula: " << formula << std::endl;

    SatChecker sc(formula);
    if (sc()) {
        SatChecker::AssignmentMap current_solution = sc.getAssignment();
        current_solution.formatKconfig(std::cout, missing);
    } else {
        logger << info << "Expression is NOT satisfiable" << std::endl;
        RETVALUE = EXIT_FAILURE;
    }
}

void process_file_symbolpc(const std::string &symbol) {
    ConfigurationModel *main_model = ModelContainer::lookupMainModel();
    if (!main_model) {
        logger << error << "for symbolpc models must be loaded" << std::endl;
        return;
    }
    std::set<std::string> missingItems;
    logger << info << "Symbol Precondition for `" << symbol << "'" << std::endl;

    /* Find all items that are related to the given item */
    std::string result;
    int valid_items = main_model->doIntersect(symbol, nullptr, missingItems, result);
    std::cout << result << std::endl;

    if (missingItems.size() > 0) {
        /* given symbol is in the model */
        if (valid_items != 0)
            std::cout << "\n&&" << std::endl;
        std::cout << ConfigurationModel::getMissingItemsConstraints(missingItems);
    }
    std::cout << std::endl;
}

process_file_cb_t parse_job_argument(const std::string arg) {
    if (arg == "dead") {
        return process_file_dead;
    } else if (arg == "coverage") {
        return process_file_coverage;
    } else if (arg == "cpppc") {
        return process_file_cpppc;
    } else if (arg == "cpppc_decision") {
        decision_coverage = true;
        return process_file_cpppc;
    } else if (arg == "cppsym") {
        return process_file_cppsym;
    } else if (arg == "blockpc") {
        return process_file_blockpc;
    } else if (arg == "blockrange") {
        return process_file_blockrange;
    } else if (arg == "interesting") {
        return process_file_interesting;
    } else if (arg == "checkexpr") {
        return process_file_checkexpr;
    } else if (arg == "symbolpc") {
        return process_file_symbolpc;
    } else if (arg == "blockconf") {
        return process_blockconf;
    } else if (arg == "mergeblockconf") {
        return process_mergeblockconf;
    }
    return nullptr;
}

void wait_for_forked_child(pid_t new_pid, int threads = 1, const char *argument = nullptr,
                           bool print_stats = false) {
    static struct { int ok, failed, signaled; } process_stats;

    static std::map<pid_t, const char *> arguments;
    static int running_processes = 0;

    if (new_pid) {
        running_processes++;
        if (argument)
            arguments[new_pid] = argument;
    }

    while (running_processes >= threads) {
        int state = 0;
        pid_t pid = waitpid(-1, &state, 0);
        if (pid == -1)
            break;

        if (WIFEXITED(state)) {
            if (WEXITSTATUS(state) == 0) {
                process_stats.ok++;
                arguments.erase(pid);
            } else {
                process_stats.failed++;
                logger << error << "Process (pid: " << pid << ", args: " << arguments[pid]
                       << ") failed with exitcode " << WEXITSTATUS(state) << std::endl;
            }
        } else if (WIFSIGNALED(state)) {
            process_stats.signaled++;
            logger << error << "Process (pid: " << pid << ", args: " << arguments[pid]
                   << ") failed with signal " << WTERMSIG(state) << std::endl;
        } else {
            continue;
        }
        running_processes--;
    }

    if (process_stats.failed > 0)
        RETVALUE = EXIT_FAILURE;

    if (print_stats) {
        /* Shutdown phase */
        logger << info << "Sucessful processed:  " << process_stats.ok << std::endl;
        logger << info << "Failed with exitcode: " << process_stats.failed << std::endl;
        logger << info << "Failed with signal:   " << process_stats.signaled << std::endl;
        for (const auto &it : arguments)  // pair<pid_t, const char *>
            logger << info << "Failed file: " << it.second << std::endl;
    }
}

int main(int argc, char **argv) {
    int opt;
    std::string worklist;
    long threads = 1;
    std::vector<std::string> models_from_parameters;
    /* Default main model will be x86 or the first one in model container if x86 is not loaded */
    std::string main_model = "default";
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
    coverageOutputMode = CoverageOutput::KCONFIG;
    coverageMode = CoverageMode::SIMPLE;

    while ((opt = getopt(argc, argv, "ucb:M:m:t:i:B:W:sj:O:C:I:Vhvq")) != -1) {
        switch (opt) {
            int n;
        case 'i':
            n = KconfigWhitelist::getIgnorelist().loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to ignorelist" << std::endl;
            } else {
                logger << error << "couldn't load ignorelist" << std::endl;
                exit(-1);
            }
            break;
        case 'W':
            n = KconfigWhitelist::getWhitelist().loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to whitelist" << std::endl;
            } else {
                logger << error << "couldn't load whitelist" << std::endl;
                exit(-1);
            }
            break;
        case 'B':
            n = KconfigWhitelist::getBlacklist().loadWhitelist(optarg);
            if (n >= 0) {
                logger << info << "loaded " << n << " items to blacklist" << std::endl;
            } else {
                logger << error << "couldn't load blacklist" << std::endl;
                exit(-1);
            }
            break;
        case 'b':
            worklist = optarg;
            break;
        case 'u':
            if (std::system("which picomus > /dev/null") == 0)
                do_mus_analysis = true;
            else
                logger << error << "Cannot do MUS-Analysis: picomus not in PATH. Continuing "
                                   "without MUS-Analysis." << std::endl;
            break;
        case 'c':
            process_file = process_file_coverage;
            break;
        case 'O':
            if (0 == strcmp(optarg, "kconfig")) {
                coverageOutputMode = CoverageOutput::KCONFIG;
            } else if (0 == strcmp(optarg, "stdout")) {
                coverageOutputMode = CoverageOutput::STDOUT;
            } else if (0 == strcmp(optarg, "model")) {
                coverageOutputMode = CoverageOutput::MODEL;
            } else if (0 == strcmp(optarg, "cpp")) {
                coverageOutputMode = CoverageOutput::CPP;
            } else if (0 == strcmp(optarg, "all")) {
                coverageOutputMode = CoverageOutput::ALL;
            } else if (0 == strcmp(optarg, "commented")) {
                coverageOutputMode = CoverageOutput::COMMENTED;
            } else if (0 == strcmp(optarg, "combined")) {
                coverageOutputMode = CoverageOutput::COMBINED;
            } else if (0 == strncmp(optarg, "exec", 4)) {
                coverageOutputMode = CoverageOutput::EXEC;
                if (optarg[4] == ':')
                    coverage_exec_cmd = &optarg[5];
            } else {
                logger << warn << "mode " << optarg << " is unknown, using 'kconfig' instead"
                       << std::endl;
            }
            break;
        case 'C':
            if (0 == strcmp(optarg, "simple")) {
                coverageMode = CoverageMode::SIMPLE;
            } else if (0 == strcmp(optarg, "min")) {
                coverageMode = CoverageMode::MINIMIZE;
            } else if (0 == strcmp(optarg, "simple_decision")) {
                decision_coverage = true;
                coverageMode = CoverageMode::SIMPLE;
            } else if (0 == strcmp(optarg, "min_decision")) {
                decision_coverage = true;
                coverageMode = CoverageMode::MINIMIZE;
            } else {
                logger << warn << "mode " << optarg << " is unknown, using 'simple' instead"
                       << std::endl;
            }
            break;
        case 't':
            threads = strtol(optarg, nullptr, 10);
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
            main_model = optarg;
            break;
        case 'm':
            models_from_parameters.emplace_back(optarg);
            break;
        case 'j':
            /* assign a new function pointer according to the jobs
               which should be done */
            process_file = parse_job_argument(optarg);
            process_mode = optarg;
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
            usage(std::cout, nullptr);
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

    if (worklist == "" && optind >= argc) {
        usage(std::cout, "please specify a file to scan or a worklist");
        return EXIT_FAILURE;
    }

    ModelContainer &model_container = ModelContainer::getInstance();
    KconfigWhitelist &bl = KconfigWhitelist::getBlacklist();
    KconfigWhitelist &wl = KconfigWhitelist::getWhitelist();

    if (0 == models_from_parameters.size() && (!bl.empty() || !wl.empty())) {
        usage(std::cout, "please specify a model to use white- or blacklists");
        return EXIT_FAILURE;
    }

    /* Load all specified models */
    for (const std::string &str : models_from_parameters) {
        if (!model_container.loadModels(str))
            logger << error << "Failed to load model " << str << std::endl;
    }
    /* Add white- and blacklisted features to all models */
    for (const auto &entry : model_container) {  // pair<string, ConfigurationModel *>
        ConfigurationModel *model = entry.second;
        for (const std::string &str : bl)
            model->addFeatureToBlacklist(str);

        for (const std::string &str : wl)
            model->addFeatureToWhitelist(str);
    }

    std::vector<std::string> workfiles;
    if (worklist == "") {
        /* Use files from command line */
        do {
            workfiles.push_back(argv[optind++]);
        } while (optind < argc);
    } else {
        /* Read files from worklist */
        std::ifstream workfile(worklist);
        std::string line;
        if (!workfile.good()) {
            usage(std::cout, "worklist was not found");
            exit(EXIT_FAILURE);
        }
        /* Collect all files that should be worked on */
        while (std::getline(workfile, line))
            workfiles.push_back(line);
    }

    /* Specify main model, if models where loaded */
    if (model_container.size() == 1) {
        /* If there is only one model file loaded use this */
        model_container.setMainModel(model_container.begin()->first);
    } else if (model_container.size() > 1) {
        /* the main model is default */
        if (main_model == "default") {
            // if 'x86' is not present, load the first one in model_container
            if (nullptr == model_container.lookupModel("x86")) {
                const std::string &first = model_container.begin()->first;
                logger << error << "Default Main-Model 'x86' not found. Using '" << first
                       << "' instead." << std::endl;
                model_container.setMainModel(first);
            } else {
                model_container.setMainModel("x86");
            }
        } else {
            model_container.setMainModel(main_model);
        }
    }

    /* Read from stdin after loading all models and whitelist */
    if (workfiles.size() > 0 && workfiles.begin()->compare("-") == 0) {
        std::string line;
        /* Read from stdin and call process file for every line */
        while (1) {
            std::cout << process_mode << ">>> ";
            if (!std::getline(std::cin, line))
                break;
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
                if (new_mode == "load") {
                    model_container.loadModels(line);
                    continue;
                } else if (new_mode == "main-model") {
                    ConfigurationModel *db = model_container.loadModels(line);
                    if (db) {
                        model_container.setMainModel(db->getName());
                    }
                    continue;
                } else { /* Change working mode */
                    process_file_cb_t new_function = parse_job_argument(new_mode);
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
                process_file(line);
        }
    } else {
        if (workfiles.size() > 1) {
            for (const std::string &file : workfiles) {
                pid_t pid = fork();
                if (pid == 0) { /* child */
                    /* calling the function pointer */
                    process_file(file);
                    return RETVALUE;
                } else if (pid < 0) {
                    logger << error << "forking failed. Exiting." << std::endl;
                    return EXIT_FAILURE;
                } else { /* Father process */
                    wait_for_forked_child(pid, threads, file.c_str());
                }
            }
            /* Wait until fork count reaches zero */
            wait_for_forked_child(0, 0, nullptr, threads > 1);
        } else if (workfiles.size() == 1) {
            process_file(workfiles[0]);
        }
    }
    return RETVALUE;
}
