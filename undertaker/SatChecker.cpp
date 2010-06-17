#include <pstreams/pstream.h>
#include <iostream>
#include <time.h>

#include "SatChecker.h"

SatChecker::SatChecker(const std::string sat) : _sat(sat), _runtime(0) {}

bool SatChecker::operator()() throw (SatCheckerError) {
    clock_t start, end;

    start = clock();

    redi::pstream limboole("limboole -s 2>/dev/null");
    std::string str;

    if (!limboole.is_open())
	throw SatCheckerError("failed to initialize limboole");

    //  std::cout << "Checking: " << sat << std::endl;
    limboole << _sat << redi::peof;
    while (limboole >> str) {
	if (str.compare("SATISFIABLE") == 0) {
	    limboole.close();
	    end = clock();
	    _runtime = end-start;
	    return true;
	}
	if (str.compare("UNSATISFIABLE") == 0) {
	    end = clock();
	    _runtime = end-start;
	    limboole.close();
	    return false;
	}
    }
    throw SatCheckerError("syntax error");
}

std::string SatChecker::pprinter(const char *sat) {
    std::string s;
    std::stringstream ss;
    redi::pstream pprinter("limboole -p");
    pprinter << sat << redi::peof;
    while (getline(pprinter, s))
	ss << s << std::endl;
    return std::string(ss.str());
}


#ifdef _TEST
void test(const char *s) {
    SatChecker checker(s);
    const char *t = "satisfiable";
    const char *f = "unsatisfiable";
    const char *a = checker() ? t:f;
    std::cout << s << " is " << a
	      << std::endl;
}
int main() {
    test("A & B");
    test("A & !A");
    return 0;
};
#endif
