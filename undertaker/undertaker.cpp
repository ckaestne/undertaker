#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <glob.h>

#include <boost/regex.hpp>

#include "KconfigWhitelist.h"
#include "ModelContainer.h"
#include "ConditionalBlock.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "CoverageAnalyzer.h"

#define __cpp_str(x) #x
#define _cpp_str(x) __cpp_str(x)

typedef void (* process_file_cb_t) (const char *filename);

enum WorkMode {
    MODE_DEAD, // Detect dead and undead files
    MODE_COVERAGE, // Calculate the coverage
    MODE_CPPPC, // CPP Preconditions for whole file
    MODE_BLOCKPC, // Block precondition
};

enum CoverageOutputMode {
    COVERAGE_MODE_KCONFIG, // kconfig specific mode
    COVERAGE_MODE_STDOUT,  // prints out on stdout generated configurations
    COVERAGE_MODE_MODEL,   // prints all configuration space itms, no blocks
    COVERAGE_MODE_CPP,     // prints all cpp items as cpp cmd-line arguments
    COVERAGE_MODE_ALL,     // prints all items and blocks
} coverageOutputMode;

enum CoverageOperationMode {
    COVERAGE_OP_SIMPLE,   // simple, fast
} coverageOperationMode = COVERAGE_OP_SIMPLE;

void usage(std::ostream &out, const char *error) {
    if (error)
        out << error << std::endl << std::endl;
    out << "`undertaker' does analysis on conditinal C code.\n\n";
    out << "Usage: undertaker [OPTIONS] <file..>\n";
    out << "\nOptions:\n";
    out << "  -m  specify the model(s) (directory or file)\n";
    out << "  -M  specify the main model\n";
    out << "  -w  specify a whitelist\n";
    out << "  -b  specify a worklist (batch mode)\n";
    out << "  -t  specify count of parallel processes\n";
    out << "  -j  specify the jobs which should be done\n";
    out << "      - dead: dead/undead file analysis (default)\n";
    out << "      - coverage: coverage file analysis\n";
    out << "      - cpppc: CPP Preconditions for whole file\n";
    out << "      - blockpc: Block precondition (format: <file>:<line>:<column>)\n";
    out << "      - symbolpc: Symbol precondition (format <symbol>)\n";
    out << "      - interesting: Find all interesting items for a symbol (format <symbol>)\n";
    out << "\nCoverage Options:\n";
    out << "  -O: specify the output mode of generated configurations\n";
    out << "      - kconfig: generated partial kconfig configuration (default)\n";
    out << "      - stdout: print on stdout the found configurations\n";
    out << "      - cpp: print on stdout cpp -D command line arguments\n";
    out << "      - model:   print all options which are in the configuration space\n";
    out << "      - all:     dump every assigned symbol (both items and code blocks)\n";
    out << "  -C: specify coverage algorithm\n";
    out << "      simple  - relative simple and fast algorithm (default)\n";
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

void process_file_coverage(const char *filename) {
    CppFile file(filename);
    if (!file.good()) {
        std::cerr << "E: failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    std::list<SatChecker::AssignmentMap> solution;
    std::map<std::string,std::string> parents;
    MissingSet missingSet;


    ModelContainer *f = ModelContainer::getInstance();
    ConfigurationModel *model = f->lookupMainModel();
    CoverageAnalyzer analyzer(&file);
    solution = analyzer.blockCoverage(model);

    if (coverageOutputMode == COVERAGE_MODE_STDOUT) {
        SatChecker::pprintAssignments(std::cout, solution, model, missingSet);
        return;
    }

    std::cout << "I: "
              << filename << ","
              << file.size()
              << ","
              << solution.size()
              << std::endl;

    std::cout << "I: Entries in missingSet: " << missingSet.size() << std::endl;

    std::string pattern(filename);
    pattern.append(".config*");
    int r = rm_pattern(pattern.c_str());

    std::cout << "I: Removed " << r << " old configurations matching " << pattern
              << std::endl;

    int config_count = 1;
    std::list<SatChecker::AssignmentMap>::iterator it;
    for (it = solution.begin(); it != solution.end(); it++) {
        static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
        SatChecker::AssignmentMap::iterator j;
        std::stringstream outfstream;
        outfstream << filename << ".config" << config_count++;
        std::ofstream outf;

        if (coverageOutputMode == COVERAGE_MODE_KCONFIG
            || coverageOutputMode == COVERAGE_MODE_MODEL
            || coverageOutputMode == COVERAGE_MODE_ALL) {
            outf.open(outfstream.str().c_str(), std::ios_base::trunc);
            if (!outf.good()) {
                std::cerr << "E: failed to write config in "
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
                std::cerr << "E: no model loaded but, model output mode specified" << std::endl;
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
        default:
            assert(false);
        }
        outf.close();
    }
}

void process_file_cpppc(const char *filename) {
    CppFile s(filename);
    if (!s.good()) {
        std::cerr << "E: failed to open file: `" << filename << "'" << std::endl;
        return;
    }
    std::cout << "I: CPP Precondition for " << filename << std::endl;
    try {
        std::cout << s.topBlock()->getCodeConstraints() << std::endl;
    } catch (std::runtime_error &e) {
        std::cerr << "E: failed: " << e.what() << std::endl;
        return;
    }
}

void process_file_blockpc(const char *filename) {
    std::string fname = std::string(filename);
    std::string file, position;
    size_t colon_pos = fname.find_first_of(':');
    if (colon_pos == fname.npos) {
        std::cerr << "E: invalid format for block precondition" << std::endl;
        return;
    }
    file = fname.substr(0, colon_pos);
    position = fname.substr(colon_pos + 1);

    CppFile cpp(file.c_str());
    if (!cpp.good()) {
        std::cerr << "E: failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ConditionalBlock * block = cpp.getBlockAtPosition(filename);

    if (block == 0) {
         std::cout << "I: No block at " << filename << " was found." << std::endl;
         return;
    }


    ConfigurationModel *model = ModelContainer::lookupMainModel();
    const BlockDefectAnalyzer *defect = 0;
    try {
        defect = BlockDefectAnalyzer::analyzeBlock(block, model);
    } catch (SatCheckerError &e) {
        std::cerr << "Couldn't process " << filename << ":" << block->getName() << ": "
                  << e.what() << std::endl;
    }

    // /* Get the Precondition */
    DeadBlockDefect dead(block);
    std::string precondition = dead.getBlockPrecondition(model);

    std::string defect_string = "no";
    if (defect) {
         defect_string = defect->getSuffix() + "/" + defect->defectTypeToString();
    }

    std::cout << "I: Block " << block->getName()
              << " | Defect: " << defect_string
              << " | Global: " << (defect != 0 ? defect->isGlobal() : 0)<< std::endl;

    std::cout << precondition << std::endl;
}

void process_file_dead(const char *filename) {
    CppFile file(filename);
    if (!file.good()) {
        std::cerr << "E: failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    ConfigurationModel *model = ModelContainer::lookupMainModel();


    /* Iterate over all Blocks */
    for (CppFile::iterator c = file.begin(); c != file.end(); c++) {
        ConditionalBlock * block = *c;

        try {
            const BlockDefectAnalyzer *defect = BlockDefectAnalyzer::analyzeBlock(block, model);
            if (defect) {
                defect->writeReportToFile();
                delete defect;
            }
        } catch (SatCheckerError &e) {
            std::cerr << "Couldn't process " << filename << ":" << block->getName() << ": "
                      << e.what() << std::endl;
        }

    }
}

void process_file_interesting(const char *filename) {
    ConfigurationModel *model = ModelContainer::lookupMainModel();

    if (!model) {
        std::cerr << "E: for finding interessting items a model must be loaded" << std::endl;
        return;
    }


    assert(model != NULL); // there should always be a main model loaded

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

void process_file_symbolpc(const char *filename) {
    ConfigurationModel *model = ModelContainer::lookupMainModel();

    if (!model) {
        std::cerr << "E: for symbolpc models must be loaded" << std::endl;
        return;
    }

    assert(model != NULL); // there should always be a main model loaded

    std::set<std::string> missingItems;

    std::cout << "I: Symbol Precondition for `" << filename << "'" << std::endl;


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
    } else if (strcmp(arg, "blockpc") == 0) {
        return process_file_blockpc;
    } else if (strcmp(arg, "interesting") == 0) {
        return process_file_interesting;
    } else if (strcmp(arg, "symbolpc") == 0) {
        return process_file_symbolpc;
    }
    return NULL;
}



int main (int argc, char ** argv) {
    int opt;
    char *worklist = NULL;
    char *whitelist = NULL;

    int threads = 1;
    std::list<std::string> models;
    std::string main_model = "x86";
    /* Default is dead/undead analysis */
    std::string process_mode = "dead";
    process_file_cb_t process_file = process_file_dead;

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

    while ((opt = getopt(argc, argv, "cb:M:m:t:w:j:O:C:vh")) != -1) {
        switch (opt) {
        case 'w':
            whitelist = strdup(optarg);
            break;
        case 'b':
            worklist = strdup(optarg);
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
            else
                std::cerr << "WARNING, mode " << optarg << " is unknown, using 'kconfig' instead"
                          << std::endl;
            break;
        case 'C':
            // currently only the simple strategy is implemented
            coverageOperationMode = COVERAGE_OP_SIMPLE;
            break;
        case 't':
            threads = strtol(optarg, (char **)0, 10);
            if ((errno == ERANGE && (threads == LONG_MAX || threads == LONG_MIN))
                || (errno != 0 && threads == 0)) {
                perror("strtol");
                exit(EXIT_FAILURE);
            }
            if (threads < 1) {
                std::cerr << "WARNING: Invalid numbers of threads, using 1 instead." << std::endl;
                threads = 1;
            }
            break;
        case 'M':
            /* Specify a new main arch */
            main_model = std::string(optarg);
            break;
        case 'm':
            models.push_back(std::string(optarg));
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
        case 'h':
            usage(std::cout, NULL);
            exit(0);
            break;
        case 'v':
            std::cout << "undertaker " << _cpp_str(VERSION) << std::endl;
            exit(0);
            break;
        default:
            break;
        }
    }

    if (!worklist && optind >= argc) {
        usage(std::cout, "please specify a file to scan or a worklist");
        return EXIT_FAILURE;
    }

    ModelContainer *f = ModelContainer::getInstance();

    /* Load all specified models */
    for (std::list<std::string>::const_iterator i = models.begin(); i != models.end(); ++i) {
        f->loadModels(*i);
    }

    if (whitelist) {
        KconfigWhitelist *wl = KconfigWhitelist::getInstance();
        int n = wl->loadWhitelist(whitelist);
        if (n >= 0) {
            std::cout << "I: loaded " << n << " items to whitelist" << std::endl;
        } else {
            std::cout << "E: couldn't load whitelist" << std::endl;
            exit(-1);
        }
        free(whitelist);
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
    if (f->size() == 1) {
        /* If there is only one model file loaded use this */
        f->setMainModel(f->begin()->first);
    } else if (f->size() > 1) {
        f->setMainModel(main_model);
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
                    f->loadModels(line);
                    continue;
                } else if (new_mode.compare("main-model") == 0) {
                    ConfigurationModel *db = f->loadModels(line);
                    if (db) {
                        f->setMainModel(db->getName());
                    }
                    continue;
                } else { /* Change working mode */
                    process_file_cb_t new_function = parse_job_argument(new_mode.c_str());
                    if (!new_function) {
                        std::cerr << "Invalid new working mode: " << new_mode << std::endl;
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
    } else if (threads > 1) {
        std::cout << workfiles.size() << " files will be analyzed by " << threads << " processes." << std::endl;

        std::vector<int> forks;
        /* Starting the threads */
        for (int thread_number = 0; thread_number < threads; thread_number++) {
            pid_t pid = fork();
            if (pid == 0) { /* child */
                std::cout << "Fork " << thread_number << " started." << std::endl;
                int worked_on = 0;
                for (unsigned int i = thread_number; i < workfiles.size(); i+= threads) {
                    /* calling the function pointer */
                    process_file(workfiles[i].c_str());
                    worked_on++;
                }
                std::cerr << "I: finished: " << worked_on << " files done (" << thread_number << ")" << std::endl;
                break;
            } else if (pid < 0) {
                std::cerr << "E: forking failed. Exiting." << std::endl;
                exit(EXIT_FAILURE);
            } else { /* Father process */
                forks.push_back(pid);
            }
        }

        /* Waiting for the childs to terminate */
        std::vector<int>::iterator iter;
        for (iter = forks.begin(); iter != forks.end(); iter++) {
            int state;
            waitpid(*iter, &state, 0);
        }
    } else {
        /* Now forks do anything sequential */
        for (unsigned int i = 0; i < workfiles.size(); i++) {
            /* call process_file function pointer */
            process_file(workfiles[i].c_str());
        }
    }
    return EXIT_SUCCESS;
}
