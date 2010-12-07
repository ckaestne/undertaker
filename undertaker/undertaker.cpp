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

void usage(std::ostream &out, const char *error) {
    if (error)
        out << error << std::endl;
    out << "Usage: undertaker [-b worklist] [-w whitelist] [-m modeldir] [-c] [-r] -s" << " <file>\n";
    out << "  -m: specify the model(s) (directory or file)\n";
    out << "  -M: specify the main model\n";
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -t: specify count of parallel processes (only in batch mode)\n";
    out << "  -w: specify a whitelist\n";
    out << "  -c: coverage analysis mode\n";
    out << "  -r: dump runtimes\n";
    out << std::endl;
}

void process_file(const char *filename, bool batch_mode, bool loadModels,
                  bool dumpRuntimes, bool coverage) {

    CloudContainer s(filename);
    if (!s.good()) {
        usage(std::cout, "couldn't open file");
        return;
    }

    if (coverage) {
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

            for (j = (*it).begin(); j != (*it).end(); j++) {
                if (boost::regex_match((*j).first, block_regexp))
                    continue;
                outf << "(" << (*j).first << "=" << (*j).second << ") ";
            }
            outf << std::endl;
            outf.close();
        }
        return;
    }

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

    bool coverage = false;


    int threads = 1;
    std::list<std::string> models;
    std::string main_model = "x86";

    /* Command line structure:
       - Model Options:
       * Per Default no models are loaded
       -- main model     -M: Specify main model
       -- model          -m: Load a specifc model (if a dir is given,
       load all model files in directory)
    */

    while ((opt = getopt(argc, argv, "b:M:m:t:w:c")) != -1) {
        switch (opt) {
        case 'w':
            whitelist = strdup(optarg);
            break;
        case 'b':
            worklist = strdup(optarg);
            break;
        case 'c':
            coverage = true;
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
    if (!worklist) {
        /* If not in batch mode, don't do any parallel things */

        process_file(argv[optind], false, loadModels, dumpRuntimes, coverage);
    } else {
        std::ifstream workfile(worklist);
        std::string line;
        /* Collect all files that should be worked on */
        std::vector<std::string> workfiles;

        while(std::getline(workfile, line)) {
            workfiles.push_back(line);
        }
        std::cout << workfiles.size() << " files will be analyzed by " << threads << " processes." << std::endl;

        std::vector<int> forks;
        /* Starting the threads */
        for (int thread_number = 0; thread_number < threads; thread_number++) {
            pid_t pid = fork();
            if (pid == 0) { /* child */
                std::cout << "Fork " << thread_number << " started." << std::endl;
                int worked_on = 0;
                for (unsigned int i = thread_number; i < workfiles.size(); i+= threads) {
                    process_file(workfiles[i].c_str(), true, loadModels, dumpRuntimes, coverage);
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

    }
    return EXIT_SUCCESS;
}
