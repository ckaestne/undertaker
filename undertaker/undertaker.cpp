#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/regex.hpp>

#include "KconfigWhitelist.h"
#include "ModelContainer.h"
#include "CloudContainer.h"
#include "CodeSatStream.h"
#include "BlockDefectAnalyzer.h"

typedef std::deque<BlockCloud> CloudList;
typedef void (* process_file_cb_t) (const char *filename, bool batch_mode, bool loadModels);


enum WorkMode {
    MODE_DEAD, // Detect dead and undead files
    MODE_COVERAGE, // Calculate the coverage
    MODE_CPPPC, // CPP Preconditions for whole file
    MODE_BLOCKPC, // Block precondition
};

void usage(std::ostream &out, const char *error) {
    if (error)
        out << error << std::endl;
    out << "Usage: undertaker [-b worklist] [-w whitelist] [-m model] [-M main model] [-j <job>] <file> [more files]\n";
    out << "  -m: specify the model(s) (directory or file)\n";
    out << "  -M: specify the main model\n";
    out << "  -w: specify a whitelist\n";
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -t: specify count of parallel processes\n";

    out << "  -j: specify the jobs which should be done\n";
    out << "      dead: dead/undead file analysis (default)\n";
    out << "      coverage: coverage file analysis (format: <filename>,<#blocks>,<#solutions>)\n";
    out << "      cpppc: CPP Preconditions for whole file\n";
    out << "      blockpc: Block precondition (format: <file>:<line>:<column>)\n";
    out << "      symbolpc: Symbol precondition (format <symbol>)\n";
    out << "      interesting: Find all interesting items for a symbol (format <symbol>)\n";
    out << "\nFiles:\n";
    out << "  You can specify one or many files (the format is according to the\n";
    out << "  job (-j) which should be done. If you specify - as file, undertaker\n";
    out << "  will load models and whitelist and read files from stdin.\n";
    out << std::endl;
}

void process_file_coverage(const char *filename, bool batch_mode, bool loadModels) {
    CloudContainer s(filename);
    if (!s.good()) {
        std::cerr << "Failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    std::list<SatChecker::AssignmentMap> solution;
    std::map<std::string,std::string> parents;
    std::istringstream codesat(s.getConstraints());

    ConfigurationModel *model = 0;
    if (loadModels) {
        ModelContainer *f = ModelContainer::getInstance();
        model = f->lookupMainModel();
    }

    CodeSatStream analyzer(codesat, filename,
                           s.getParents(), NULL, batch_mode, loadModels);
    int i = 1;

    solution = analyzer.blockCoverage(model);
    std::cout << filename << ","
              << analyzer.Blocks().size()
              << ","
              << solution.size()
              << std::endl;

    std::list<SatChecker::AssignmentMap>::iterator it;
    for (it = solution.begin(); it != solution.end(); it++) {
        static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
        SatChecker::AssignmentMap::iterator j;
        std::stringstream outfstream;
        outfstream << filename << ".config" << i++;
        std::ofstream outf;

        outf.open(outfstream.str().c_str(), std::ios_base::trunc);
        if (!outf.good()) {
            std::cerr << "failed to write config in " 
                      << outfstream.str().c_str()
                      << std::endl;
            outf.close();
            continue;
        }

        SatChecker::formatConfigItems(*it, outf);
        outf.close();
    }
}

void process_file_cpppc(const char *filename, bool batch_mode, bool loadModels) {
    (void) batch_mode;
    (void) loadModels;

    CloudContainer s(filename);
    if (!s.good()) {
        std::cerr << "Failed to open file: `" << filename << "'" << std::endl;
        return;
    }
    std::cout << "I: CPP Precondition for " << filename << std::endl;
    try {
        std::cout << s.getConstraints() << std::endl;
    } catch (std::runtime_error &e) {
        std::cerr << "FAILED: " << e.what() << std::endl;
        return;
    }
}

void process_file_blockpc(const char *filename, bool batch_mode, bool loadModels) {
    (void) batch_mode;

    std::string fname = std::string(filename);
    std::string file, position;
    size_t colon_pos = fname.find_first_of(':');
    if (colon_pos == fname.npos) {
        std::cerr << "FAILED: " << "Invalid format for block precondition" << std::endl;
        return;
    }
    file = fname.substr(0, colon_pos);
    position = fname.substr(colon_pos + 1);

    CloudContainer cloud(file.c_str());
    if (!cloud.good()) {
        std::cerr << "Failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    std::istringstream cs(cloud.getConstraints());
    std::string matched_block;
    CodeSatStream *sat_stream = 0;
    for (CloudList::iterator c = cloud.begin(); c != cloud.end(); c++) {
        std::istringstream codesat(c->getConstraints());
        CodeSatStream *analyzer = new CodeSatStream(codesat, file,
                                                    cloud.getParents(), &(*c), false, loadModels);

        std::string block = analyzer->positionToBlock(filename);
        if (block.size() != 0) {
            matched_block = block;
            sat_stream = analyzer;
            break;
        }
        delete analyzer;
    }

    if (matched_block.size() == 0) {
        std::cout << "I: No block at " << filename << " was found." << std::endl;
        return;
    }


    ConfigurationModel *model = ModelContainer::lookupMainModel();
    const BlockDefectAnalyzer *defect = sat_stream->analyzeBlock(matched_block.c_str(), model);

    /* Get the Precondition */
    DeadBlockDefect dead(sat_stream, matched_block.c_str());
    std::string precondition = dead.getBlockPrecondition(model);

    std::string defect_string = "no";
    if (defect) {
        defect_string = defect->getSuffix() + "/" + defect->defectTypeToString();
    }

    std::cout << "I: Block " << matched_block
              << " | Defect: " << defect_string
              << " | Global: " << (defect != 0 ? defect->isGlobal() : 0)<< std::endl;

    std::cout << precondition << std::endl;
}

void process_file_dead(const char *filename, bool batch_mode, bool loadModels) {
    CloudContainer s(filename);
    if (!s.good()) {
        std::cerr << "Failed to open file: `" << filename << "'" << std::endl;
        return;
    }

    std::istringstream cs(s.getConstraints());

    for (CloudList::iterator c = s.begin(); c != s.end(); c++) {
        std::istringstream codesat(c->getConstraints());

        CodeSatStream analyzer(codesat, filename,
                               s.getParents(), &(*c), batch_mode, loadModels);

        // this is the total runtime per *cloud*
        analyzer.analyzeBlocks();
    }
}

void process_file_interesting(const char *filename, bool batch_mode, bool loadModels) {
    (void) batch_mode;

    if (!loadModels) {
        usage(std::cerr, "To find interessing items for a given symbol you must load a model");
        return;
    }

    ConfigurationModel *model = ModelContainer::lookupMainModel();
    assert(model != NULL); // there should always be a main model loaded

    std::set<std::string> initialItems;
    initialItems.insert(filename);

    /* Find all items that are related to the given item */
    model->findSetOfInterestingItems(initialItems);

    /* remove the given item again */
    initialItems.erase(filename);
    std::cout << filename;

    for(std::set<std::string>::const_iterator it = initialItems.begin(); it != initialItems.end(); ++it) {
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

void process_file_symbolpc(const char *filename, bool batch_mode, bool loadModels) {
    (void) batch_mode;

    if (!loadModels) {
        usage(std::cerr, "To find interessing items for a given symbol you must load a model");
        return;
    }

    ConfigurationModel *model = ModelContainer::lookupMainModel();
    assert(model != NULL); // there should always be a main model loaded

    std::set<std::string> initialItems, missingItems;
    initialItems.insert(filename);

    std::cout << "I: Symbol Precondition for `" << filename << "'" << std::endl;


    /* Find all items that are related to the given item */
    int valid_items = model->doIntersect(initialItems, std::cout, missingItems);

    if (missingItems.size() > 0) {
        /* given symbol is in the model */
        if (valid_items != 0)
            std::cout <<  "\n&&" << std::endl;
        std::cout << ConfigurationModel::getMissingItemsConstraints(missingItems);
    }

    std::cout << std::endl;;
}

int main (int argc, char ** argv) {
    int opt;
    char *worklist = NULL;
    char *whitelist = NULL;

    int threads = 1;
    std::list<std::string> models;
    std::string main_model = "x86";
    /* Default is dead/undead analysis */
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

    while ((opt = getopt(argc, argv, "cb:M:m:t:w:j:")) != -1) {
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
            if (strcmp(optarg, "dead") == 0) {
                process_file = process_file_dead;
            } else if (strcmp(optarg, "coverage") == 0) {
                process_file = process_file_coverage;
            } else if (strcmp(optarg, "cpppc") == 0) {
                process_file = process_file_cpppc;
            } else if (strcmp(optarg, "blockpc") == 0) {
                process_file = process_file_blockpc;
            } else if (strcmp(optarg, "interesting") == 0) {
                process_file = process_file_interesting;
            } else if (strcmp(optarg, "symbolpc") == 0) {
                process_file = process_file_symbolpc;
            } else {
                usage(std::cerr, "Invalid job specified");
                return EXIT_FAILURE;
            }
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
        std::cout << "I: loaded " << n << " items to whitelist" << std::endl;
        free(whitelist);
    }

    /* Are there any models loaded? */
    bool loadModels = f->size() > 0;
    /* Specify main model, if models where loaded */
    if (f->size() == 1) {
        /* If there is only one model file loaded use this */
        f->setMainModel(f->begin()->first);
    } else if (f->size() > 1) {
        f->setMainModel(main_model);
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
        /* Collect all files that should be worked on */
        while(std::getline(workfile, line)) {
            workfiles.push_back(line);
        }
    }

    /* Read from stdin after loading all models and whitelist */
    if (workfiles.begin()->compare("-") == 0) {
        std::string line;
        /* Read from stdin and call process file for every line */
        while (1) {
            std::cout << ">>> ";
            if (!std::getline(std::cin, line)) break;
            process_file(line.c_str(), false, loadModels);
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
                    process_file(workfiles[i].c_str(), true, loadModels);
                    worked_on++;
                }
                std::cerr << "I: finished: " << worked_on << " files done (" << thread_number << ")" << std::endl;
                break;
            } else if (pid < 0) {
                std::cerr << "E: Forking failed. Exiting." << std::endl;
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
            process_file(workfiles[i].c_str(), false, loadModels);
        }
    }
    return EXIT_SUCCESS;
}
