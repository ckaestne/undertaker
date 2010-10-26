#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
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
            symbol       = lexeme_d[ leaf_node_d[ *(alnum_p | ch_p('_')) ]];
            group        = no_node_d[ ch_p('(') ]
                >> start_rule
                >> no_node_d[ ch_p(')') ];
            not_symbol   = no_node_d[ch_p('!')] >> term;

            term         = group | not_symbol | symbol;

            and_term     = term         >> *(no_node_d[ch_p("&")] >> term);
            or_term      = and_term     >> *(no_node_d[ch_p("|")] >> and_term);

            implies_term = or_term      >> *(no_node_d[str_p("->")] >> or_term);
            iff_term     = implies_term
                >> *(no_node_d[ str_p("<->") ] >> implies_term);

            start_rule = iff_term
                >> no_node_d[ *(space_p | ch_p("\n") | ch_p("\r"))];
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
    iter_t root_node = input;
 beginning_of_function:
    iter_t iter = root_node->children.begin();


    if (root_node->value.id() == bool_grammar::symbolID) {
        iter_t inner_node = root_node->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());
        _debug_parser(value + " ");
        return stringToSymbol(value);
    } else if (root_node->value.id() == bool_grammar::not_symbolID) {
        iter_t inner_node = root_node->children.begin()->children.begin();
        _debug_parser("(! ");
        int inner_clause = transform_bool_rec(inner_node);
        _debug_parser(") ");

        int this_clause  = counter++;
        int clause1[3] = { this_clause,  inner_clause, 0};
        int clause2[3] = {-this_clause, -inner_clause, 0};

        addClause(clause1);
        addClause(clause2);

        return this_clause;
    } else if (root_node->value.id() == bool_grammar::andID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }

        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause = counter++;
        _debug_parser("(and ");
        // A & B & ..:
        // !3 A 0
        // !3 B 0
        // ..
        // 3 !A !B .. 0
        end_clause[i++] = this_clause;

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {-this_clause, child_clause, 0};
            end_clause[i++] = -child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);

        _debug_parser(") ");
        return this_clause;
    } else if (root_node->value.id() == bool_grammar::orID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause  = counter++;
        end_clause[i++] = -this_clause;
        // A | B
        // 3 !A 0
        // 3 !B 0
        // !3 A B 0
        _debug_parser("(or ");

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {this_clause, -child_clause, 0};
            end_clause[i++] = child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
        _debug_parser(") ");


        return this_clause;
    } else if (root_node->value.id() == bool_grammar::impliesID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
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
        _debug_parser("(-> ");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
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
        _debug_parser(") ");
        return A_clause;
    } else if (root_node->value.id() == bool_grammar::iffID) {
        iter_t iter = root_node->children.begin();
        /* Skip or rule if there is only one child */
        if ((iter+1) == root_node->children.end()) {
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
        _debug_parser("(<-> ");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
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
        _debug_parser(") ");
        return A_clause;
    } else {
        /* Just a Container node, we simply go inside and try again. */
        root_node = root_node->children.begin();
        goto beginning_of_function;
    }
}

void
SatChecker::fillSatChecker(std::string expression) throw (SatCheckerError) {
    static bool_grammar e;
    tree_parse_info<> info = pt_parse(expression.c_str(), e,
                                      space_p | ch_p("\n") | ch_p("\r"));

    if (info.full) {
        fillSatChecker(info);
    } else {
        /* Enable this line to get the position where the parser dies
        std::cout << std::string(expression.begin(), expression.begin()
                                 + info.length) << endl;
        */
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

SatChecker::SatChecker(const char *sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) {
    /* Counter for limboole symbols starts at 1 */
    counter = 1;
    limmat = Limmat::new_Limmat (0);
}

SatChecker::SatChecker(const std::string sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) {
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
    return _sat;
}

#ifdef TEST_SatChecker
#include <assert.h>
#include <typeinfo>


bool test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);

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
    test("B9\n&\n(B1 <-> !_DRIVERS_MISC_SGIXP_XP_H)\n&\n(B3 <-> B1 & CONFIG_X86_UV | CONFIG_IA64_SGI_UV)\n&\n(B6 <-> B1 & !is_uv)\n&\n(B9 <-> B1 & CONFIG_IA64)\n&\n(B12 <-> B1 & !is_shub1)\n&\n(B15 <-> B1 & !is_shub2)\n&\n(B18 <-> B1 & !is_shub)\n&\n(B21 <-> B1 & USE_DBUG_ON)\n&\n(B23 <-> B1 & !B21)\n&\n(B26 <-> B1 & XPC_MAX_COMP_338)\n&\n(CONFIG_ACPI -> !CONFIG_IA64_HP_SIM & (CONFIG_IA64 | CONFIG_X86) & CONFIG_PCI & CONFIG_PM)\n&\n(CONFIG_CHOICE_6 -> !CONFIG_X86_ELAN)\n&\n(CONFIG_CHOICE_8 -> CONFIG_X86_32)\n&\n(CONFIG_HIGHMEM64G -> CONFIG_CHOICE_8 & !CONFIG_M386 & !CONFIG_M486)\n&\n(CONFIG_INTR_REMAP -> CONFIG_X86_64 & CONFIG_X86_IO_APIC & CONFIG_PCI_MSI & CONFIG_ACPI & CONFIG_EXPERIMENTAL)\n&\n(CONFIG_M386 -> CONFIG_CHOICE_6 & CONFIG_X86_32 & !CONFIG_UML)\n&\n(CONFIG_M486 -> CONFIG_CHOICE_6 & CONFIG_X86_32)\n&\n(CONFIG_NUMA -> CONFIG_SMP & (CONFIG_X86_64 | CONFIG_X86_32 & CONFIG_HIGHMEM64G & (CONFIG_X86_NUMAQ | CONFIG_X86_BIGSMP | CONFIG_X86_SUMMIT & CONFIG_ACPI) & CONFIG_EXPERIMENTAL))\n&\n(CONFIG_PCI_MSI -> CONFIG_PCI & CONFIG_ARCH_SUPPORTS_MSI)\n&\n(CONFIG_PM -> !CONFIG_IA64_HP_SIM)\n&\n(CONFIG_X86_32_NON_STANDARD -> CONFIG_X86_32 & CONFIG_SMP & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_BIGSMP -> CONFIG_X86_32 & CONFIG_SMP)\n&\n(CONFIG_X86_ELAN -> CONFIG_X86_32 & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_EXTENDED_PLATFORM -> CONFIG_X86_64)\n&\n(CONFIG_X86_IO_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_LOCAL_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_NUMAQ -> CONFIG_X86_32_NON_STANDARD & CONFIG_PCI)\n&\n(CONFIG_X86_SUMMIT -> CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UP_APIC -> CONFIG_X86_32 & !CONFIG_SMP & !CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UV -> CONFIG_X86_64 & CONFIG_X86_EXTENDED_PLATFORM & CONFIG_NUMA & CONFIG_X86_X2APIC)\n&\n(CONFIG_X86_X2APIC -> CONFIG_X86_LOCAL_APIC & CONFIG_X86_64 & CONFIG_INTR_REMAP)\n&\n!(CONFIG_IA64 | CONFIG_IA64_HP_SIM | CONFIG_IA64_SGI_UV | CONFIG_UML)\n", false);
    return 0;
};
#endif
