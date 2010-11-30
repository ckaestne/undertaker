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
    out << "Usage: undertaker [-b worklist] [-w whitelist] [-a arch] [-m modeldir] [-c] [-r] -s" << " <file>\n";
    out << "  -b: specify a worklist (batch mode)\n";
    out << "  -t: specify count of parallel processes (only in batch mode)\n";
    out << "  -w: specify a whitelist\n";
    out << "  -a: specify a unique arch\n";
    out << "  -m: specify the models directory (default: ./models)\n";
    out << "  -c: coverage analysis mode\n";
    out << "  -s: skip loading variability models\n";
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
        std::string arch = "x86";

        ConfigurationModel *model = 0;
        if (loadModels) {
            ModelContainer *f = ModelContainer::getInstance();
            if (f->size() == 1)
                arch = f->begin()->first;
            model = f->lookupModel(arch.c_str());
        }

        CodeSatStream analyzer(codesat, filename, arch.c_str(),
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
    }

    std::istringstream cs(s.getConstraints());
    clock_t start, end;
    double t = -1;

    start = clock();
    for (CloudList::iterator c = s.begin(); c != s.end(); c++) {
      std::istringstream codesat(c->getConstraints());
      std::string primary_arch = "x86";
      ModelContainer *f = ModelContainer::getInstance();
      if (f->size() == 1)
        primary_arch = f->begin()->first;
      CodeSatStream analyzer(codesat, filename, primary_arch.c_str(),
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
    bool loadModels = true;
    bool dumpRuntimes = false;
    char *worklist = NULL;
    char *whitelist = NULL;
    char *arch = NULL;
    char *modeldir = NULL;
    bool arch_specific = false;
    bool coverage = false;

    int threads = 1;

    while ((opt = getopt(argc, argv, "sb:a:m:t:w:c")) != -1) {
	switch (opt) {
	case 's':
	    loadModels = false;
	    break;
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
	case 'a':
	    arch_specific = true;
            arch = strdup(optarg);
	    break;
        case 'r':
            dumpRuntimes = true;
            break;
        case 'c':
            coverage = true;
            break;
	case 'm':
        modeldir = strdup(optarg);
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

    if (loadModels) {
        if (arch_specific) {
            f->loadModels(modeldir ? modeldir : "models", arch);
        } else {
            f->loadModels(modeldir ? modeldir : "models");
        }
    }

    if (whitelist) {
        KconfigWhitelist *wl = KconfigWhitelist::getInstance();
        int n = wl->loadWhitelist(whitelist);
        std::cout << "I: loaded " << n << " items to whitelist" << std::endl;
        free(whitelist);
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
