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

typedef std::deque<BlockCloud> CloudList;

enum WorkMode {
    MODE_DEAD, // Detect dead and undead files
    MODE_COVERAGE, // Calculate the coverage
    MODE_CPPPC, // CPP Preconditions for whole file
};

void usage(std::ostream &out, const char *error) {
    if (error)
        out << error << std::endl;
    out << "Usage: undertaker [-b worklist] [-w whitelist] [-m model] [-M main model] [-j <job>] [-r] <file> [more files]\n";
    out << "  -m: specify the model(s) (directory or file)\n";
    out << "  -M: specify the main model\n";
    out << "  -w: specify a whitelist\n";
    
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -t: specify count of parallel processes (only in batch mode)\n";

    out << "  -j: specify the jobs which should be done\n";
    out << "      dead: dead/undead file analysis (default)\n";
    out << "      coverage: coverage file analysis\n";
    out << "      cpppc: CPP Preconditions for whole file\n";
    out << "  -r: dump runtimes\n";
    out << std::endl;
}

void process_file(const char *filename, bool batch_mode, bool loadModels,
                  bool dumpRuntimes, WorkMode work_mode) {

    CloudContainer s(filename);
    if (!s.good()) {
        usage(std::cout, "couldn't open file");
        return;
    }

    if (work_mode == MODE_COVERAGE) {
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
        std::cout << filename << " contains " << analyzer.Blocks().size()
                  << " blocks" << std::endl;
        std::cout << "Size of solution: " << solution.size() << std::endl;
        std::list<SatChecker::AssignmentMap>::iterator it;
        for (it = solution.begin(); it != solution.end(); it++) {
            static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
            SatChecker::AssignmentMap::iterator j;
            std::cout << "Solution " << i++ << ": ";
            for (j = (*it).begin(); j != (*it).end(); j++) {
                if (boost::regex_match((*j).first, block_regexp))
                    continue;
                std::cout << "(" << (*j).first << "=" << (*j).second << ") ";
            }
            std::cout << std::endl;
        }
        return;
    } else if (work_mode == MODE_CPPPC) {
        CloudContainer s(filename);
        try {
            std::cout << s.getConstraints() << std::endl;
        } catch (std::runtime_error &e) {
            std::cerr << "FAILED: " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        return;
    }

    assert(work_mode == MODE_DEAD);

    std::istringstream cs(s.getConstraints());
    clock_t start, end;
    double t = -1;

    start = clock();
    for (CloudList::iterator c = s.begin(); c != s.end(); c++) {
        std::istringstream codesat(c->getConstraints());

        CodeSatStream analyzer(codesat, filename,
                               s.getParents(), &(*c), batch_mode, loadModels);

        // this is the total runtime per *cloud*
        analyzer.analyzeBlocks();
        if(dumpRuntimes)
            analyzer.dumpRuntimes();
    }
    end = clock();
    t = (double) (end - start);
    std::cout.precision(10);

    // this is the total runtime per *file*
    if(dumpRuntimes)
        std::cout << "RTF:" << filename << ":" << t << std::endl;
}

int main (int argc, char ** argv) {
    int opt;
    bool dumpRuntimes = false;
    char *worklist = NULL;
    char *whitelist = NULL;

    int threads = 1;
    std::list<std::string> models;
    std::string main_model = "x86";
    /* Default is dead/undead analysis */
    WorkMode work_mode = MODE_DEAD;

    /* Command line structure:
       - Model Options:
       * Per Default no models are loaded
       -- main model     -M: Specify main model
       -- model          -m: Load a specifc model (if a dir is given
       - Work Options:
       -- job            -j: do a specific job
       load all model files in directory)
    */

    while ((opt = getopt(argc, argv, "b:M:m:t:w:j:")) != -1) {
        switch (opt) {
        case 'w':
            whitelist = strdup(optarg);
            break;
        case 'b':
            worklist = strdup(optarg);
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
            if (strcmp(optarg, "dead") == 0)
                work_mode = MODE_DEAD;
            else if (strcmp(optarg, "coverage") == 0)
                work_mode = MODE_COVERAGE;
            else if (strcmp(optarg, "cpppc") == 0)
                work_mode = MODE_CPPPC;
            else {
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

    if (threads > 1) {
        std::cout << workfiles.size() << " files will be analyzed by " << threads << " processes." << std::endl;

        std::vector<int> forks;
        /* Starting the threads */
        for (int thread_number = 0; thread_number < threads; thread_number++) {
            pid_t pid = fork();
            if (pid == 0) { /* child */
                std::cout << "Fork " << thread_number << " started." << std::endl;
                int worked_on = 0;
                for (unsigned int i = thread_number; i < workfiles.size(); i+= threads) {
                    process_file(workfiles[i].c_str(), true, loadModels, dumpRuntimes, work_mode);
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
            process_file(workfiles[i].c_str(), false, loadModels, dumpRuntimes, work_mode);
        }
    }
    return EXIT_SUCCESS;
}
