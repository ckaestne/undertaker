#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "SatChecker.h"


///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  boolean expression parser
//
///////////////////////////////////////////////////////////////////////////////

struct bool_grammar : public grammar<bool_grammar>
{
    enum {symbolID = 2, not_symbolID, andID, orID, impliesID, iffID};

    template <typename ScannerT>
    struct definition
    {

        rule<ScannerT, parser_context<>, parser_tag<symbolID> > symbol;
        rule<ScannerT, parser_context<>, parser_tag<not_symbolID> > not_symbol;
        rule<ScannerT, parser_context<>, parser_tag<andID> > and_term;
        rule<ScannerT, parser_context<>, parser_tag<orID> > or_term;
        rule<ScannerT, parser_context<>, parser_tag<impliesID> > implies_term;
        rule<ScannerT, parser_context<>, parser_tag<iffID> > iff_term;
        rule<ScannerT> start_rule, group, term, expression;

        definition(bool_grammar const& self)  { 
            /* 
               Operators (from weak to strong): <->, ->, |, &, !(), 
             */
            (void) self;
            symbol       = lexeme_d[ leaf_node_d[ alpha_p >> *(alnum_p | ch_p('_')) ]];
            group        = no_node_d[ ch_p('(') ] >> start_rule >> no_node_d[ ch_p(')') ];
            not_symbol   = no_node_d[ch_p('!')] >> term;

            term         = group | not_symbol | symbol;

            and_term     = term         >> *(no_node_d[ch_p("&")] >> term);
            or_term      = and_term     >> *(no_node_d[ch_p("|")] >> and_term);

            implies_term = or_term      >> *(no_node_d[str_p("->")] >> or_term);
            iff_term     = implies_term >> *(no_node_d[ str_p("<->") ] >> implies_term);

            start_rule = iff_term >> no_node_d[ *(space_p | ch_p("\n") | ch_p("\r"))];
        };
        
        rule<ScannerT> const& start() const { return start_rule; }
    };
};

    /* Got the impression from normal lisp implementations */
int 
SatChecker::stringToSymbol(const std::string &key) {
    map<std::string, int>::iterator it;
    if ((it = symbolTable.find(key)) != symbolTable.end()) {
        return it->second;
    }
    symbolTable[key] = counter;
    return counter++;
}

void
SatChecker::addClause(int *clause) {
    /*    for (int *x = clause; *x; x++)
        cout << *x << " ";
        cout << "0" << endl; */
    Limmat::add_Limmat(limmat, clause);
}

int
SatChecker::transform_bool_rec(iter_t const& input) {
    iter_t iter = input->children.begin();
    
    if (input->value.id() == bool_grammar::symbolID) {
        iter_t inner_node = input->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());
        debug += "'" + value + "' ";
        return stringToSymbol(value);
    } else if (input->value.id() == bool_grammar::not_symbolID) {
        iter_t inner_node = input->children.begin()->children.begin();
        debug += "(! ";
        int inner_clause = transform_bool_rec(inner_node);
        debug += ")";
        int this_clause  = counter++;
        int clause1[3] = { this_clause,  inner_clause, 0};
        int clause2[3] = {-this_clause, -inner_clause, 0};
            
        addClause(clause1);
        addClause(clause2);

        return this_clause;
    } else if (input->value.id() == bool_grammar::andID) {
        /* Skip and rule if there is only one child */
        if (input->children.size() == 1) {
            return transform_bool_rec(iter);
        }

        int i = 0, end_clause[input->children.size() + 2];
            
        int this_clause = counter++;
        debug += "(and(" + boost::lexical_cast<std::string>(this_clause) + ") ";
        // A & B & ..:
        // !3 A 0
        // !3 B 0
        // ..
        // 3 !A !B .. 0
        end_clause[i++] = this_clause;

        for (; iter != input->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {-this_clause, child_clause, 0};
            end_clause[i++] = -child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
            
        debug += ") ";
        return this_clause;
    } else if (input->value.id() == bool_grammar::orID) {
        /* Skip and rule if there is only one child */
        if (input->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int i = 0, end_clause[input->children.size() + 2];
            
        int this_clause  = counter++;
        end_clause[i++] = -this_clause;
        // A | B
        // 3 !A 0
        // 3 !B 0
        // !3 A B 0
        debug += "(or(" + boost::lexical_cast<std::string>(this_clause) + ") ";

        for (; iter != input->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {this_clause, -child_clause, 0};
            end_clause[i++] = child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
        debug += ") ";

            
        return this_clause;
    } else if (input->value.id() == bool_grammar::impliesID) {
        /* Skip and rule if there is only one child */
        if (input->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        // A -> B
        // 3 A 0
        // 3 !B 0
        // !3 !A B 0
        // A  ->  B  ->  C
        // (A -> B)  ->  C
        //    A      ->  C
        //          A
        debug += "(-> ";
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != input->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = counter++;
            int c1[] = { this_clause, A_clause, 0};
            int c2[] = { this_clause, -B_clause, 0};
            int c3[] = { -this_clause, -A_clause, B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            A_clause = this_clause;
        }
        debug += ") ";
        return A_clause;
    } else if (input->value.id() == bool_grammar::iffID) {
        iter_t iter = input->children.begin();
        /* Skip or rule if there is only one child */
        if ((iter+1) == input->children.end()) {
            return transform_bool_rec(iter);
        }
        // A <-> B
        // 3 !A  !B 0
        // 3 A B 0
        // !3 !A B 0
        // !3 A !B 0
        // A  <->  B  <->  C
        // (A <-> B)  <->  C
        //    A      <->  C
        //          A
        debug += "(<-> ";
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != input->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = counter++;
            int c1[] = { this_clause,  -A_clause, -B_clause, 0};
            int c2[] = { this_clause,   A_clause,  B_clause, 0};
            int c3[] = { -this_clause, -A_clause,  B_clause, 0};
            int c4[] = { -this_clause,  A_clause, -B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            addClause(c4);
            A_clause = this_clause;
        }
        debug += ") ";
        return A_clause;
    } else {
        /* Just a Container node */
        return transform_bool_rec(input->children.begin());
    }
}

void
SatChecker::fillSatChecker(std::string expression) throw (SatCheckerError) {
    static bool_grammar e;
    tree_parse_info<> info = pt_parse(expression.c_str(), e, space_p | ch_p("\n") | ch_p("\r"));

    if (info.full) {
        fillSatChecker(info);
    } else {
        std::cout << std::string(expression.begin(), expression.begin() + info.length) << endl;
        throw SatCheckerError("SatChecker: Couldn't parse: " + expression);
    }
}

void
SatChecker::fillSatChecker(tree_parse_info<>& info) {
    iter_t expression = info.trees.begin()->children.begin();
    int top_clause = transform_bool_rec(expression);
    /* This adds the last clause */
    int clause[2] = {top_clause, 0};
    addClause(clause);
}

SatChecker::SatChecker(const char *sat) : _sat(std::string(sat)) {
    /* Counter for limboole symbols starts at 1 */
    counter = 1;
    limmat = Limmat::new_Limmat (0);
}

SatChecker::SatChecker(const std::string sat) : _sat(sat) {
    /* Counter for limboole symbols starts at 1 */
    counter = 1;
    limmat = Limmat::new_Limmat (0);
}

SatChecker::~SatChecker() {
    if (limmat != 0)
        Limmat::delete_Limmat(limmat);
}

bool SatChecker::operator()() throw (SatCheckerError) {
    fillSatChecker(_sat);
    int res = Limmat::sat_Limmat(limmat, -1);
    
    if (res <= 0) 
        return false;
    return true;
}

std::string SatChecker::pprint() {
    fillSatChecker(_sat);
    return debug;
}

#ifdef TEST_SatChecker
#include <assert.h>
#include <typeinfo>


bool test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);

    std::cout << checker.pprint() << endl;

    if (error == 0)  {
        if (checker() != result) {
            std::cerr << "FAILED: " << s << " should be " << result << std::endl;
            return false;
        }
    }
    else {
        try {
            if (checker() != result) {
                std::cerr << "FAILED: " << s << " should be " << result << std::endl; 
                return false;
            }
        } catch (std::runtime_error &e) {
            if (typeid(*error) != typeid(e)) {
                std::cerr << "FAILED: " << s << " didn't throw the right exception (Should " << typeid(*error).name() 
                          << ", is " << typeid(e).name() << ")" << std::endl;
                return false;
            }
        }
    }
    std::cout << "PASSED: " << s << std::endl;
    return true;
}

int main() {

    test("A & B", true);
    test("A & !A", false);
    test("(A -> B) -> A -> A", true);
    test("(A <-> B) & (B <-> C) & (C <-> A)", true);
    test("B6 & \n(B5 <-> !S390) & (B6 <-> !B5) & !S390", false);
    test("a >,,asd/.sa,;.ljkxf;vnbmkzjghrarjkf.dvd,m.vjkb54y98g4tjoij", false, &SatCheckerError(""));
    test("B90 & \n( B90 <->  ! MODULE )\n& ( B92 <->  ( B90 ) \n & CONFIG_STMMAC_TIMER )\n", true);
    
    return 0;
};
#endif
