#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <malloc.h>
#include <time.h>

#include "BddContainer.h"
#include "VariableToBddMap.h"
#include "ExpressionParser.h"

// first argument, contains the filename to rsf data of main compilation unit
static std::string code_rsf_file;
static bdd V;
static std::ofstream devnull("/dev/null");
static clock_t reordering_time;


void usage(std::ostream &out, const char *error, char **argv ) {
    if (error)
	out << error << std::endl;
    out << "Usage: " << argv[0] << "[-v] [-f] [-w]" << " <rsf-file>" << std::endl;
}

static void bdd_error_handler(int e) {
	fprintf(stderr, "BDD error: %s\n", bdd_errstring(e));
	bdd_fprintstat(stderr);
	abort();
};

bdd analyze(BddContainer &c, std::ifstream &config_stream, bool verbose) {

    bdd_error_hook(bdd_error_handler);

    // main work: process the compilation unit
    V = c.do_build();
    if (V == bddtrue || V == bddfalse)
	return V;


    if (verbose) {
	    clock_t start, end;

	    start = clock();
	    std::cout << "rsf for compilation unit built successfully, "
		      << "reodering table now" << std::endl;

	    bdd_reorder(BDD_REORDER_SIFT);

	    end = clock();
	    reordering_time = end-start;
	    std::cout << "finished in " << ((double)reordering_time)/ CLOCKS_PER_SEC
		      << "sec" << std::endl;
    } else {
	    bdd_reorder(BDD_REORDER_SIFT);
    }

    // secondary work: process the kconfig file
    VariableToBddMap &map = (*VariableToBddMap::getInstance());
    RsfBlocks items(config_stream, "Item", devnull);
    RsfBlocks depends(config_stream, "Depends", devnull);

    // ensure that the items are in the map
    for (RsfBlocks::iterator i = items.begin(); i != items.end(); i++) {
	map.add_new_variable("CONFIG_" + (*i).first);
    }

    for (RsfBlocks::iterator i = depends.begin(); i != depends.end(); i++) {
	std::string item = (*i).first;
	std::string expr = (*i).second.front();

	std::cout << item << " -> " << expr << std::endl;

	bdd bdd_item = map.get_bdd("CONFIG_" + item);
	ExpressionParser p(expr, true, true);
	bdd expression = p.expression();
	V &= bdd_item >> expression;
    }

    return V;
}

int count_zombies(BddContainer &c, bdd V) {
    int dead = 0;
    int alive = 0;

    VariableToBddMap &m = (*VariableToBddMap::getInstance());

    int base = m.size() - c.size();
    for (unsigned b = 0; b < c.size(); b++) {
	bdd bdddead = V & m[b+base].getBdd();
	bdd bddalive = V & (!m[b+base].getBdd());
	int rdead = bdd_pathcount(bdddead);
	int ralive = bdd_pathcount(bddalive);
	if (!rdead) {
	  dead++;
	  std::cerr << "Block: " << c.bId(b) << " is DEAD: " << rdead
		  // << ", " << m[b+base].getName()
		    << std::endl;
	}
	if (!ralive) {
	  alive++;
	  std::cerr << "Block: " << c.bId(b) << " is Zombie: " << ralive
		  // << ", " << m[b+base].getName()
		    << std::endl;
	}
    }

    if (dead + alive > 0) {
	    std::cerr << "found zombies/deads in file: " << code_rsf_file
		      << std::endl;
    }

    return (dead+alive);
}

int main(int argc, char ** argv) {
    bool verbose = false;
    bool force_run = false;
    int zombies = 0;
    std::string *statfile = 0;
    clock_t start, end;
    int opt;

    while ((opt = getopt(argc, argv, "fvw:")) != -1) {
	switch (opt) {
	case 'v':
	    verbose = true;
	    break;
	case 'f':
	    force_run = true;
	    break;
	case 'w':
	    statfile = new std::string(optarg);
	    break;
	default: /* ? */
	    usage(std::cerr, "failed to parse arguments", argv);
	    exit(EXIT_FAILURE);
	}
    }

    if (argc < optind)
	usage(std::cerr, "too few arguments", argv);

    static std::ifstream code_stream(argv[optind]);
    if (!code_stream.good()) {
	std::stringstream ss;
	ss << "failed to open: " << argv[optind];
	usage(std::cerr, ss.str().c_str() , argv);
	exit(EXIT_FAILURE);
    }

    std::clog << "Processing file: " << code_rsf_file.assign(argv[optind])
	      << std::endl;

    BddContainer *code_bdd;
    start = clock();

    if (verbose) {
	code_bdd = new BddContainer(code_stream, argv[optind], std::cout, force_run);
    } else {
	code_bdd = new BddContainer(code_stream, argv[optind], devnull, force_run);
    }

    if (argc-1 > optind) {
	const char *kconfig_rsf = argv[optind+1];
	static std::ifstream config_stream(kconfig_rsf);
	if(!config_stream.good()) {
	    std::stringstream ss;
	    ss << "failed to open: " << kconfig_rsf << "continuing without!"
	       << std::endl;
	    zombies = 0;
	    goto out;
	}

	std::clog << "Processing config file: " << argv[2]
		  << std::endl;

	bdd V = analyze(*code_bdd, config_stream, verbose);
	if (V == bddtrue || V == bddfalse) {
	    zombies = -1;
	} else {
	    zombies = count_zombies(*code_bdd, V);
	}
    } else {
	std::ifstream config_stream("/dev/null");
	bdd V = analyze(*code_bdd, config_stream, verbose);

	if (V == bddtrue || V == bddfalse) {
	    zombies = -1;
	} else {
	    zombies = count_zombies(*code_bdd, V);
	}
    }
    out:
    delete code_bdd;

    struct mallinfo info = mallinfo();
    end = clock();

    if (verbose) {
	    std::clog << "undertaker used " << info.arena << " bytes of memory"
		      << " and " << ((double)(end - start)) / CLOCKS_PER_SEC << "sec" 
		      << std::endl;
    }
    if (statfile) {
	VariableToBddMap &m = (*VariableToBddMap::getInstance());

	/* csv format: memory, cputime, reodering time, dead+alive features, varnum */
	std::ofstream out(statfile->c_str(), std::ios_base::out| std::ios_base::app);
	if (!out.good()) {
		std::cerr << "couldn't open " << statfile << "for writing" << std::endl;
		return zombies;
	}

	out << argv[optind] 
	    << "," << info.arena 
	    << "," << ((double)(end - start)) / CLOCKS_PER_SEC
	    << "," << ((double)reordering_time) / CLOCKS_PER_SEC
	    << "," << zombies
	    << "," << m.allocatedBdds()
	    << std::endl;
	delete statfile;
    }
    return zombies;
}
