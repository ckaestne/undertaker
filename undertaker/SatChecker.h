// -*- mode: c++ -*-
#ifndef sat_checker_h__
#define sat_checker_h__ 
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <stdexcept>
#include <map>
#include <sstream>

using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace Limmat {
    #include <ctype.h>
    #include <assert.h>

    using std::string;

    /* Include the Limmat library header as C */
    extern "C" {
#include "limmat.h"
    }
};

struct SatCheckerError : public std::runtime_error {
    SatCheckerError(const char *s)
        : runtime_error(s) {}
    SatCheckerError(std::string s)
        : runtime_error(s.c_str()) {}
};

class SatChecker {
public:
    typedef char const*                              iterator_t;
    typedef tree_match<iterator_t>                   parse_tree_match_t;
    typedef parse_tree_match_t::const_tree_iterator  iter_t;

    SatChecker(const char *sat);
    SatChecker(const std::string sat);
    ~SatChecker();

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
    static std::string pprinter(const char *sat) {
        SatChecker c(const_cast<const char*>(sat));
        return c.pprint();
    }

    /** pretty prints the saved expression */
    std::string pprint();

    /** returns the runtime of the last run */
    clock_t runtime() { return _runtime; }

private:
    int counter;
    std::map<std::string, int> symbolTable;
    std::string debug;
    
    const std::string _sat;
    Limmat::Limmat *limmat;
    clock_t _runtime;

    int stringToSymbol(const std::string &key);
    void addClause(int *clause);
    int transform_bool_rec(iter_t const& input);
    void fillSatChecker(std::string expression) throw (SatCheckerError);
    void fillSatChecker(tree_parse_info<>& info);

};
#endif
