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

namespace Picosat {
    #include <ctype.h>
    #include <assert.h>

    using std::string;

    /* Include the Limmat library header as C */
    extern "C" {
#include "picosat.h"
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

    SatChecker(const char *sat, int debug = 0);
    SatChecker(const std::string sat, int debug = 0);
    ~SatChecker();

    /**
     * Checks the given string with an sat solver
     * @param sat the formula to be checked
     * @returns true, if satisfiable, false otherwise
     * @throws if syntax error
     */
    bool operator()() throw (SatCheckerError);

    static bool check(const std::string &sat) throw (SatCheckerError);
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

    enum Debug { DEBUG_NONE = 0,
                 DEBUG_PARSER = 1,
                 DEBUG_CNF = 2 };

    /* After doing the check, you can get the assignments for the
       formular */
    std::map<std::string, int> getAssignment() {
        return assignmentTable;
    }

private:
    std::map<std::string, int> symbolTable;
    std::map<std::string, int> assignmentTable;
    int debug_flags;
    std::string debug_parser;
    int debug_parser_indent;
    std::string debug_cnf;

    const std::string _sat;
    clock_t _runtime;

    int stringToSymbol(const std::string &key);
    int newSymbol(void);
    void addClause(int *clause);

    int notClause(int inner_clause);
    int andClause(int A_clause, int B_clause);
    int orClause(int A_clause, int B_clause);

    int transform_bool_rec(iter_t const& input);
    void fillSatChecker(std::string expression) throw (SatCheckerError);
    void fillSatChecker(tree_parse_info<>& info);

    // Debugging stuff
    void _debug_parser(std::string d = "", bool newblock = true) {
        if (debug_flags & DEBUG_PARSER) {
            if (d.size()) {
                debug_parser += "\n";
                for (int i = 0; i < debug_parser_indent; i++)
                    debug_parser += " ";

                debug_parser += d;
                if (newblock)
                    debug_parser_indent += 2;
            } else {
                debug_parser_indent -= 2;
            }
        }
    }

};
#endif
