// -*- mode: c++ -*-
#ifndef sat_checker_h__
#define sat_checker_h__ 

#include <stdexcept>
#include <sstream>

struct SatCheckerError : public std::runtime_error {
    SatCheckerError(const char *s)
	: runtime_error(s) {}
};

class SatChecker {
public:
    SatChecker(const char *sat);
    SatChecker(const std::string sat);

    /**
     * Checks the given string with an sat solver
     * @param sat the formula to be checked
     * @returns true, if satisfiable, false otherwise
     * @throws if syntax error of if the checker tool could not be found
     */
    bool operator()() throw (SatCheckerError);

    static bool check(const std::string &sat) throw (SatCheckerError) {
	SatChecker c(sat.c_str());
	return c();
    }

    const char *c_str() { return _sat.c_str(); }
    const std::string str() { return _sat; }

    /** pretty prints the given string */
    static std::string pprinter(const char *sat);

    /** returns the runtime of the last run */
    clock_t runtime() { return _runtime; }

private:
    const std::string _sat;
    clock_t _runtime;
};
#endif
