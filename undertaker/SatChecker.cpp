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
    enum {symbolID = 2, not_symbolID, andID, orID, impliesID, iffID,
          equals_noID, equals_moduleID, equals_yesID};

    template <typename ScannerT>
    struct definition
    {

        rule<ScannerT, parser_context<>, parser_tag<symbolID> > symbol;
        rule<ScannerT, parser_context<>, parser_tag<not_symbolID> > not_symbol;
        rule<ScannerT, parser_context<>, parser_tag<equals_noID> > equals_no_term;
        rule<ScannerT, parser_context<>, parser_tag<equals_moduleID> > equals_module_term;
        rule<ScannerT, parser_context<>, parser_tag<equals_yesID> > equals_yes_term;


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
            symbol       = lexeme_d[ leaf_node_d[ +(alnum_p | ch_p('_')) ]];
            group        = no_node_d[ ch_p('(') ]
                >> start_rule
                >> no_node_d[ ch_p(')') ];
            not_symbol   = no_node_d[ch_p('!')] >> term;

            // =y, =m, =n
            equals_no_term = (symbol >> no_node_d[ ch_p("=") >> ch_p("n") ])
                | (no_node_d[ch_p("n") >> ch_p("=")] >> symbol);

            equals_module_term = (symbol >> no_node_d[ ch_p("=") >> ch_p("m") ])
                | (no_node_d[ch_p("m") >> ch_p("=")] >> symbol);

            equals_yes_term = (symbol >> no_node_d[ ch_p("=") >> ch_p("y") ])
                | (no_node_d[ch_p("y") >> ch_p("=")] >> symbol);


            term = group | not_symbol
                | equals_no_term | equals_module_term | equals_yes_term
                | symbol;

            /* nodes that aren't leaf nodes */
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

bool
SatChecker::check(const std::string &sat) throw (SatCheckerError) {
    SatChecker c(sat.c_str());
    try {
        return c();
    } catch (SatCheckerError &e) {
        std::cout << "Syntax Error:" << std::endl;
        std::cout << sat << std::endl;
        std::cout << "End of Syntax Error" << std::endl;
        throw e;
    }
    return false;
}


    /* Got the impression from normal lisp implementations */
int
SatChecker::stringToSymbol(const std::string &key) {
    map<std::string, int>::iterator it;
    if ((it = symbolTable.find(key)) != symbolTable.end()) {
        return it->second;
    }
    int n = newSymbol();
    symbolTable[key] = n;
    return n;
}

int
SatChecker::newSymbol(void) {
    return Picosat::picosat_inc_max_var();
}

void
SatChecker::addClause(int *clause) {
    for (int *x = clause; *x; x++)
        Picosat::picosat_add(*x);
    Picosat::picosat_add(0);
}

int
SatChecker::notClause(int inner_clause) {
    int this_clause  = newSymbol();

    int clause1[3] = { this_clause,  inner_clause, 0};
    int clause2[3] = {-this_clause, -inner_clause, 0};

    addClause(clause1);
    addClause(clause2);

    return this_clause;
}

int
SatChecker::andClause(int A_clause, int B_clause) {
    // This function just does binary ands in contradiction to the
    // andID transform_rec
    // A & B & ..:
    // !3 A 0
    // !3 B 0
    // 3 !A !B 0

    int this_clause = newSymbol();

    int A[] = {-this_clause, A_clause, 0};
    int B[] = {-this_clause, B_clause, 0};
    int last[] = {this_clause, -A_clause, -B_clause, 0};

    addClause(A);
    addClause(B);
    addClause(last);

    return this_clause;
}

int
SatChecker::transform_bool_rec(iter_t const& input) {
    iter_t root_node = input;
 beginning_of_function:
    iter_t iter = root_node->children.begin();


    if (root_node->value.id() == bool_grammar::symbolID) {
        iter_t inner_node = root_node->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());
        _debug_parser("- " + value, false);
        return stringToSymbol(value);
    } else if (root_node->value.id() == bool_grammar::not_symbolID) {
        iter_t inner_node = root_node->children.begin()->children.begin();
        _debug_parser("- not");
        int inner_clause = transform_bool_rec(inner_node);
        _debug_parser();

        return notClause(inner_clause);
    } else if (root_node->value.id() == bool_grammar::equals_noID) {
        /* =n -> !SYMBOL & !SYMBOL_MODULE */
        iter_t inner_node = root_node->children.begin()->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());

        _debug_parser("- and");
        _debug_parser("- not");
        _debug_parser("- " + value, false);
        _debug_parser();
        _debug_parser("- not");
        _debug_parser("- " + value + "_MODULE", false);
        _debug_parser();
        _debug_parser();

        return andClause(notClause(stringToSymbol(value)),
                         notClause(stringToSymbol(value + "_MODULE")));
    } else if (root_node->value.id() == bool_grammar::equals_moduleID) {
        /* =m -> !SYMBOL & SYMBOL_MODULE */
        iter_t inner_node = root_node->children.begin()->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());

        _debug_parser("- and");
        _debug_parser("- not");
        _debug_parser("- " + value, false);
        _debug_parser();
        _debug_parser("- " + value + "_MODULE", false);
        _debug_parser();



        return andClause(notClause(stringToSymbol(value)),
                         stringToSymbol(value + "_MODULE"));
    } else if (root_node->value.id() == bool_grammar::equals_yesID) {
        /* =y -> SYMBOL & !SYMBOL_MODULE */
        iter_t inner_node = root_node->children.begin()->children.begin();
        string value (inner_node->value.begin(), inner_node->value.end());

        _debug_parser("- and");
        _debug_parser("- " + value, false);
        _debug_parser("- not");
        _debug_parser("- " + value + "_MODULE", false);
        _debug_parser();
        _debug_parser();

        return andClause(stringToSymbol(value),
                         notClause(stringToSymbol(value + "_MODULE")));
    } else if (root_node->value.id() == bool_grammar::andID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }

        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause = newSymbol();
        _debug_parser("- and");
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

        _debug_parser();
        return this_clause;
    } else if (root_node->value.id() == bool_grammar::orID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause  = newSymbol();
        end_clause[i++] = -this_clause;
        // A | B
        // 3 !A 0
        // 3 !B 0
        // !3 A B 0
        _debug_parser("- or");

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {this_clause, -child_clause, 0};
            end_clause[i++] = child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
        _debug_parser();

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
        _debug_parser("- ->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
            int c1[] = { this_clause, A_clause, 0};
            int c2[] = { this_clause, -B_clause, 0};
            int c3[] = { -this_clause, -A_clause, B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            A_clause = this_clause;
        }
        _debug_parser();
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
        _debug_parser("- <->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
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
        _debug_parser();
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
        Picosat::picosat_reset();
        throw SatCheckerError("SatChecker: Couldn't parse: " + expression);
    }
}

void
SatChecker::fillSatChecker(tree_parse_info<>& info) {
    iter_t expression = info.trees.begin()->children.begin();
    int top_clause = transform_bool_rec(expression);
    /* This adds the last clause */
    Picosat::picosat_assume(top_clause);
}

SatChecker::SatChecker(const char *sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) { }

SatChecker::SatChecker(const std::string sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) { }

SatChecker::~SatChecker() { }

bool SatChecker::operator()() throw (SatCheckerError) {
    /* Clear the debug parser, if we are called multiple times */
    debug_parser.clear();
    debug_parser_indent = 0;

    Picosat::picosat_init();

    fillSatChecker(_sat);

    int res = Picosat::picosat_sat(-1);
    Picosat::picosat_reset();
    if (res == PICOSAT_UNSATISFIABLE)
        return false;
    return true;
}

std::string SatChecker::pprint() {
    if (debug_parser.size() == 0) {
        int old_debug_flags = debug_flags;
        debug_flags |= DEBUG_PARSER;
        (*this)();
        debug_flags = old_debug_flags;
    }
    return debug_parser + "\n";
}

#ifdef TEST_SatChecker
#include <assert.h>
#include <typeinfo>
#include <check.h>


bool cnf_test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);

    if (error == 0)  {
        fail_unless(checker() == result,
                    "%s should evaluate to %d",
                    s.c_str(), (int) result);
        return false;
    } else {
        try {
            fail_unless(checker() == result,
                        "%s should evaluate to %d",
                        s.c_str(), (int) result);
            return false;
        } catch (std::runtime_error &e) {
            fail_unless(typeid(*error) == typeid(e),
                        "%s didn't throw the right exception should be %s, but is %s.",
                        typeid(*error).name(), typeid(e).name());
            return false;
        }
    }
    return true;
}

START_TEST(cnf_translator_test)
{
    SatCheckerError error("");

    cnf_test("A & B", true);
    cnf_test("A & !A", false);
    cnf_test("(A -> B) -> A -> A", true);
    cnf_test("(A <-> B) & (B <-> C) & (C <-> A)", true);
    cnf_test("B6 & \n(B5 <-> !S390) & (B6 <-> !B5) & !S390", false);
    cnf_test("a >,,asd/.sa,;.ljkxf;vnbmkzjghrarjkf.dvd,m.vjkb54y98g4tjoij", false, &error);
    cnf_test("B90 & \n( B90 <->  ! MODULE )\n& ( B92 <->  ( B90 ) \n & CONFIG_STMMAC_TIMER )\n", true);
    cnf_test("B9\n&\n(B1 <-> !_DRIVERS_MISC_SGIXP_XP_H)\n&\n(B3 <-> B1 & CONFIG_X86_UV | CONFIG_IA64_SGI_UV)\n&\n(B6 <-> B1 & !is_uv)\n&\n(B9 <-> B1 & CONFIG_IA64)\n&\n(B12 <-> B1 & !is_shub1)\n&\n(B15 <-> B1 & !is_shub2)\n&\n(B18 <-> B1 & !is_shub)\n&\n(B21 <-> B1 & USE_DBUG_ON)\n&\n(B23 <-> B1 & !B21)\n&\n(B26 <-> B1 & XPC_MAX_COMP_338)\n&\n(CONFIG_ACPI -> !CONFIG_IA64_HP_SIM & (CONFIG_IA64 | CONFIG_X86) & CONFIG_PCI & CONFIG_PM)\n&\n(CONFIG_CHOICE_6 -> !CONFIG_X86_ELAN)\n&\n(CONFIG_CHOICE_8 -> CONFIG_X86_32)\n&\n(CONFIG_HIGHMEM64G -> CONFIG_CHOICE_8 & !CONFIG_M386 & !CONFIG_M486)\n&\n(CONFIG_INTR_REMAP -> CONFIG_X86_64 & CONFIG_X86_IO_APIC & CONFIG_PCI_MSI & CONFIG_ACPI & CONFIG_EXPERIMENTAL)\n&\n(CONFIG_M386 -> CONFIG_CHOICE_6 & CONFIG_X86_32 & !CONFIG_UML)\n&\n(CONFIG_M486 -> CONFIG_CHOICE_6 & CONFIG_X86_32)\n&\n(CONFIG_NUMA -> CONFIG_SMP & (CONFIG_X86_64 | CONFIG_X86_32 & CONFIG_HIGHMEM64G & (CONFIG_X86_NUMAQ | CONFIG_X86_BIGSMP | CONFIG_X86_SUMMIT & CONFIG_ACPI) & CONFIG_EXPERIMENTAL))\n&\n(CONFIG_PCI_MSI -> CONFIG_PCI & CONFIG_ARCH_SUPPORTS_MSI)\n&\n(CONFIG_PM -> !CONFIG_IA64_HP_SIM)\n&\n(CONFIG_X86_32_NON_STANDARD -> CONFIG_X86_32 & CONFIG_SMP & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_BIGSMP -> CONFIG_X86_32 & CONFIG_SMP)\n&\n(CONFIG_X86_ELAN -> CONFIG_X86_32 & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_EXTENDED_PLATFORM -> CONFIG_X86_64)\n&\n(CONFIG_X86_IO_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_LOCAL_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_NUMAQ -> CONFIG_X86_32_NON_STANDARD & CONFIG_PCI)\n&\n(CONFIG_X86_SUMMIT -> CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UP_APIC -> CONFIG_X86_32 & !CONFIG_SMP & !CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UV -> CONFIG_X86_64 & CONFIG_X86_EXTENDED_PLATFORM & CONFIG_NUMA & CONFIG_X86_X2APIC)\n&\n(CONFIG_X86_X2APIC -> CONFIG_X86_LOCAL_APIC & CONFIG_X86_64 & CONFIG_INTR_REMAP)\n&\n!(CONFIG_IA64 | CONFIG_IA64_HP_SIM | CONFIG_IA64_SGI_UV | CONFIG_UML)\n", false);

}
END_TEST


void
parse_test(string input, bool good) {
    static bool_grammar e;

    tree_parse_info<> info = pt_parse(input.c_str(), e, space_p);

    fail_unless((info.full ? true : false) == good,
                "%s: %d", input.c_str(), info.full);

    if (!info.full) {
        try {
            fail_if(SatChecker::check(input));
            fail();
        } catch (SatCheckerError &e) {/*IGNORE*/}
    }
}


START_TEST(bool_parser_test)
{
    parse_test("A", true);
    parse_test("! A", true);
    parse_test("--0--", false);
    parse_test("A & B", true);
    parse_test("A  |   B", true);
    parse_test("A &", false);
    parse_test("(A & B) | C", true);
    parse_test("A & B & C & D", true);
    parse_test("A | C & B", true);
    parse_test("C & B | A", true);
    parse_test("! ( ! (A))", true);
    parse_test("!!!!!A", true);

    parse_test("A -> B", true);
    parse_test(" -> B", false);
    parse_test("(A -> B) -> A -> A", true);
    parse_test("(A <-> ! B) | ( B <-> ! A)", true);
    parse_test("A -> B -> C -> (D -> C)", true);

    parse_test("A & !A | B & !B", true);
    parse_test("A -> B -> C", true);
    parse_test("A <-> B", true);
}
END_TEST

START_TEST(pprinter_test)
{
    fail_unless(SatChecker("B72 & ( B67 <->  ! CONFIG_MMU ) "
                           "& ( B69 <->  ( B67 )  "
                           "&  ! CONFIG_MMU ) "
                           "& ( B72 <->  ( B67 )  & CONFIG_MMU )").pprint()
                ==
                "\n"
                "- and\n"
                "  - B72\n"
                "  - <->\n"
                "    - B67\n"
                "    - not\n"
                "      - CONFIG_MMU\n"
                "  - <->\n"
                "    - B69\n"
                "    - and\n"
                "      - B67\n"
                "      - not\n"
                "        - CONFIG_MMU\n"
                "  - <->\n"
                "    - B72\n"
                "    - and\n"
                "      - B67\n"
                "      - CONFIG_MMU\n");
}
END_TEST

START_TEST(equals_no_test) {
    /* This test want to check if the parsing of =n and correct
       translation works
    */
    parse_test("n=MODULE_A", true);
    parse_test("MODULE_A=n", true);
    parse_test("MODULE_A = n", true);

    cnf_test("EQUAL_NO & EQUAL_NO=n", false);
    cnf_test("A & (B23 <-> A=n) & B23", false);
    cnf_test("A_MODULE & A=n", false);
    cnf_test("A & (B23 <-> A) & B23=n", false);
    cnf_test("!A & !A_MODULE & A=n", true);

} END_TEST

START_TEST(equals_module_test) {
    /* This test want to check if the parsing of =n and correct
       translation works
    */
    parse_test("m=MODULE_A", true);
    parse_test("MODULE_A=m", true);
    parse_test("MODULE_A = m", true);

    cnf_test("EQUAL_NO & EQUAL_NO=m", false);
    cnf_test("A & (B23 <-> A=m) & B23", false);
    cnf_test("A_MODULE & (B23 <-> A=m) & B23", true);
    cnf_test("A_MODULE & A=m", true);
    cnf_test("A & (B23 <-> A) & B23=m", false);
    cnf_test("!A & A_MODULE & A=m", true);

} END_TEST

START_TEST(equals_yes_test) {
    /* This test want to check if the parsing of =n and correct
       translation works
    */
    parse_test("y=MODULE_A", true);
    parse_test("MODULE_A=y", true);
    parse_test("MODULE_A = y", true);

    cnf_test("EQUAL_NO & EQUAL_NO=y", true);
    cnf_test("A_MODULE & (B23 <-> A=y) & B23", false);
    cnf_test("A & (B23 <-> A=y) & B23", true);
    cnf_test("A_MODULE & A=y", false);
    cnf_test("A & (B23_MODULE <-> A) & B23=y", false);
    cnf_test("A & !A_MODULE & A=y", true);

} END_TEST


Suite *
satchecker_suite(void) {
    Suite *s  = suite_create("SatChecker");
    TCase *tc = tcase_create("SatChecker");
    tcase_add_test(tc, cnf_translator_test);
    tcase_add_test(tc, pprinter_test);
    tcase_add_test(tc, bool_parser_test);
    tcase_add_test(tc, equals_no_test);
    tcase_add_test(tc, equals_module_test);
    tcase_add_test(tc, equals_yes_test);
    suite_add_tcase(s, tc);

    return s;
}

int main() {
    Suite *s = satchecker_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
