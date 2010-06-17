#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cassert>
#include <climits>

#include <ext/stdio_filebuf.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <sys/stat.h>

/* compile cache only on x86 machines for now, because we currently
 * don't compile on 64bit machines at all and in cross-compile mode, we
 * have no openssl. And actually, I'd like to get rid of libssl
 * completely...  
 */
#ifdef i386
#define WITH_CACHE 1
#endif


#include "BddContainer.h"
#include "VariableToBddMap.h"
#include "ExpressionParser.h"
#include "bdd.h"


static void gbc_handler(int pre, bddGbcStat *s) {
    VariableToBddMap *m = VariableToBddMap::getInstance();
    if (!pre)
    {
	printf("Garbage collection #%d: %d nodes / %d free, %d variables, %d entries ",
	       s->num, s->nodes, s->freenodes, bdd_varnum(), m->allocatedBdds());
	printf(" / %.1fs / %.1fs total\n",
	       (float)s->time/(float)(CLOCKS_PER_SEC),
	       (float)s->sumtime/(float)CLOCKS_PER_SEC);
    }
}

BddContainer::BddContainer(std::ifstream &in, char *argv0, std::ostream &log, bool force_run)
    : std::deque<value_type> (),
      CompilationUnitInfo(in, log),
      pcount_(0),
      parsedExpressions_(false),
      argv0(argv0),
      log(log),
      _force_run(force_run) {

    bdd_init(8000000, 100000);
    bdd_setvarnum(150);
    bdd_gbc_hook(gbc_handler);
    bdd_setmaxincrease(20000000); // default is 50000 (1MB)
//    bdd_setmaxnodenum(0); // no limit
    bdd_setminfreenodes(80);
    bdd_setcacheratio(64);
}

BddContainer::~BddContainer() {
    bdd_done();
}

BlockBddInfo &BddContainer::item(index n) {
    assert(n < size());
    return (*this)[n];
}

int BddContainer::bId(index n) {
    assert(n < size());
    return item(n).getId();
}

int BddContainer::bId(std::string n){
    std::stringstream ss;
    int i;
    ss << n;
    ss >> i;
    return bId(i);
}

bdd BddContainer::bBlock(index n) {
    return item(n).blockBdd();
}

bdd BddContainer::bBlock(std::string n) {
    std::stringstream ss;
    int i;
    ss << n;
    ss >> i;
    return bBlock(i);
}

bdd BddContainer::bExpression(index n) {
    assert(n < size());
    return item(n).expressionBdd();
}

bdd BddContainer::bExpression(std::string n) {
    std::stringstream ss;
    int i;
    ss << n;
    ss >> i;
    return bExpression(i);
}

bdd BddContainer::bParent(index n) {
    int blockno = item(n).getId();
    std::string *pParent = nested_in_.getValue(blockno);

    if (!pParent) {
	return bddtrue;
    }
    pcount_++;

    return bBlock(search(*pParent));
}


bdd BddContainer::do_calc(index n, RsfBlocks &blocks) {
    std::stringstream ss;
    ss << bId(n);
    std::string blockno = ss.str();

    bdd X;

    X = bddfalse;
    for (RsfBlocks::iterator i = blocks.begin(); i != blocks.end(); i++) {
	std::string key = (*i).first;
	std::string value = (*i).second.front();
	if (blockno == key) {
	    X |= bBlock(search(value));
	}
    }
    return X;
}

bdd BddContainer::Pred(index n) {
    return do_calc(n, OlderSibl_);
}

bdd BddContainer::Succ(index n) {
    return do_calc(n, YoungerSibl_);
}


void bddstat(std::ostream &out, bdd X) {
    out << bddset << X << "," << bdd_satcount(X) << "," << bdd_pathcount(X)
	<< std::endl;
}

void BddContainer::parseExpressions() {
    VariableToBddMap *m = VariableToBddMap::getInstance();

    if(parsedExpressions_)
	return;

    for(RsfBlocks::iterator i = expressions_.begin();
	i != expressions_.end(); i++) {
	std::stringstream ss;
	int blockno;
	const std::string &blocknostr = (*i).first;
	const std::string &expression = (*i).second.front();

	ss << blocknostr;
	ss >> blockno;

	log << "I: parsing expression " << expression.c_str()
	    << "(bdd count: " << m->allocatedBdds() << ")"
	    << std::endl;

	ExpressionParser p(expression);
	// don't initialize the block bdds yet.
	// this ensures that the index in the variable map
	// corresponds to variable number
	this->push_back(BlockBddInfo(blockno, bddtrue, p.parse(), expression));
    }

    // late calculation of block bdd
    for (unsigned b = 0; b < size(); b++) {
    	int blockno = (*this)[b].getId();
	item(b).blockBdd() = m->make_block_bdd(blockno);
    }
    parsedExpressions_ = true;
}

bdd BddContainer::run() {
    bdd X;
    int iter_count = 20;
    clock_t start, end;

    if (expressions_.size() == 0)
	throw std::runtime_error("no expressions scanned");

    if (size() > 40 && !_force_run) {
	    log << "E: bdd is too large to process, skipping" << std::endl;
	    return bddfalse;
    }

    X = bddtrue;

    start = clock();
 
    for (unsigned b = 0; b < size(); b++) {
	end = clock();

	log << "I: Progress: (" << b << "/" << size() << ") runtime: "
	    << ((double) (end-start))/ CLOCKS_PER_SEC << "sec" << std::endl;

	X &= (bParent(b) & bExpression(b) & !Succ(b)) >> bBlock(b);
	X &= bBlock(b) >> bParent(b);
	X &= bBlock(b) >> bExpression(b);
	X &= bBlock(b) >> !Succ(b);

	if (iter_count-- == 0) {
	    clock_t start, end;
	    start = clock();
	    log << "I: reodering bdd with " << bdd_varnum() << " variables"
		<< std::endl;
	    bdd_reorder(BDD_REORDER_SIFTITE);
	    iter_count = 8;
	    end = clock();
	    log << "I: reodering took"
		<< ((double) (end-start))/ CLOCKS_PER_SEC << "sec" << std::endl;
	}

	if (X == bddfalse) {
		int blockid = bId(b);
		log << "E: run " << b << " BlockId: " << blockid
		    << " ruined the bdd " << std::endl;
		return X;
	}
    }

//    bddstat (std::cout, X);
    return X;
}

#if WITH_CACHE
std::string getMd5String(int fd) {
    BIO *bio, *mdtmp;
    char buf[1024];
    unsigned char mdbuf[16];
    int rdlen;
    std::stringstream ss("");

    bio = BIO_new_fd(fd, BIO_NOCLOSE);
    if(!bio) {
	perror("BIO_new_file");
	return "";
    }
    
    mdtmp = BIO_new(BIO_f_md());
    BIO_set_md(mdtmp, EVP_md5());
    bio = BIO_push(mdtmp, bio);

    do {
	rdlen = BIO_read(mdtmp, buf, sizeof(buf));
    } while (rdlen > 0);

    int mdlen = BIO_gets(mdtmp, (char*)mdbuf, sizeof(mdbuf));
    for (int i = 0; i < mdlen; i++)
	ss << std::setw(2) << std::setfill('0')
	   << std::hex << (unsigned int) mdbuf[i];

    return (ss.str());
}

int BddContainer::try_load(const char *filename, bdd &b){
    int error;
    struct stat bddstat;

    if(!stat(filename, &bddstat)) {
	struct stat argv0stat;

	if (argv0 && !(error = stat(argv0, &argv0stat))) {
	    if (bddstat.st_mtime < argv0stat.st_mtime) {
		log << "I: binary " << argv0
		    << " is newer than bdd file, deleting "
		    << filename
		    << std::endl;
		unlink(filename);
		return -1;
	    }
	    std::cout << "I: binary " << argv0
		      << " is older than bdd file. OK!"
		      << std::endl;

	} else {
	    std::cout << "I: could not check timestamp of binary" << std::endl;
	}

	if (0 == (error = bdd_fnload((char*)filename, b))) {
	    if (b == bddtrue || b == bddfalse) {
		    log << "loaded trivial bdd, unlinking file: "
			<< filename << std::endl;
		    unlink(filename);
		    return -1;
	    }
	    log << "I: successfully loaded cached bdd from file: "
		<< filename << std::endl;
	    return 0;
	} else
	    log << "couldn't open bdd file, error code: "
		<< error << std::endl;
    }
    return -1;
}
#endif

bdd BddContainer::do_build() {
    bdd b;

#if WITH_CACHE
    int error;
    struct stat statbuf;
    static const char *CACHE_DIR=".bddcache";

    if (0 != stat(CACHE_DIR, &statbuf)) {
	if(0 != mkdir(CACHE_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
	    perror("create cachedir");
	    return bddtrue;
	}
    }

    // ucking fugly, but *should* work...
    __gnu_cxx::stdio_filebuf<char> *filebuf;
    filebuf = reinterpret_cast<__gnu_cxx::stdio_filebuf<char> *>(_in.rdbuf());

    std::string cachedfile = CACHE_DIR + std::string("/") 
	+ getMd5String(filebuf->fd())
	+ std::string(".bdd");

    char *filename = (char*) cachedfile.c_str();
#endif

    parseExpressions();

    log << "I: parsing finished, now running algo" << std::endl;

#if WITH_CACHE
    if (!try_load(filename, b))
	return b;
#endif
    
    b = run();

#if WITH_CACHE
    if (b == bddtrue || b == bddfalse) {
	log << "E: refusing to save trivial bdd"
	    << std::endl;
	return b;
    }

    if (0 != (error = bdd_fnsave(filename, b)))
	log << "E: couldn't write bdd file, error code: "
	    << error << std::endl;
    else
	log << "I: saved bdd as: "
	    << filename << std::endl;
#endif    
    return b;
}

struct ExpressionPrinter {
    ExpressionPrinter(std::ostream &out) : out_(out) {}
    void operator()(RsfBlocks::value_type &i){
	static unsigned n = 1;
	out_ << "I: Expression " << n++ << ": " 
	     << i.second.front() << std::endl;
    }
private:
    std::ostream &out_;
};

struct VariablePrinter {
    VariablePrinter(std::ostream &out) : out_(out) {}
    void operator()(VariableToBddMap::value_type &i){
	static unsigned n = 1;
	out_ << "I: Variable " << n++ << ": " 
	     << i.getName() << std::endl;
    }
private:
    std::ostream &out_;
};

void BddContainer::print_stats(std::ostream &out) {
    VariableToBddMap &m = *VariableToBddMap::getInstance();

    out << "I: " << m.allocatedBdds() << " Bdds allocated " << std::endl;
    out << "I: " << numExpr() << " expressions loaded" << std::endl;
    std::for_each(expressions_.begin(), expressions_.end(), ExpressionPrinter(out));
    out << "I: " << numVar() << " configuration variables found" << std::endl;
    std::for_each(m.begin(), m.end(), VariablePrinter(out));
    out << "I: " << pcount_ << " parents found" << std::endl;

    out << "I: following bdds have been allocated" << std::endl;
    for (unsigned i = 0; i < m.size(); i++)
	out << "  " << m[i].getName() << ": " << m[i].getBdd() << std::endl;
}

size_t BddContainer::numExpr() {
    return size();
}

size_t BddContainer::numVar() {
    return VariableToBddMap::variables;
}

BddContainer::index BddContainer::search(std::string idstring) {
    std::stringstream ss;
    index id;
    ss << idstring;
    ss >> id;
    return search(id);
}

BddContainer::index BddContainer::search(int id) {
    std::stringstream ss;
    for (unsigned int i = 0; i < size(); i++)
	if (id == bId(i))
	    return i;

    ss << "id not found: " << id;

    throw std::runtime_error(ss.str());
}
