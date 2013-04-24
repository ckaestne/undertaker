#include "bool.h"
#include "BoolExpSymbolSet.h"
#include <set>
#include <iostream>
#include <check.h>

using namespace kconfig;

START_TEST(symtable) {
    //simple tests
    BoolExp *e;

    e = BoolExp::parseString("X || Y && Z || (Z && ! X)");
    BoolExpSymbolSet t0(e);

    std::set<std::string> s0=t0.getSymbolSet();
    fail_if(!e || s0.find("X") == s0.end());
    fail_if(!e || s0.find("Y") == s0.end());
    fail_if(!e || s0.find("A") != s0.end());
    fail_if(!e || s0.size() != 3);

    e = BoolExp::parseString("! HAVE_SMP.");
    BoolExpSymbolSet t1(e);

    std::set<std::string> s1=t1.getSymbolSet();
    fail_if(!e || s1.find("HAVE_SMP.") == s1.end());
    fail_if(!e || s1.size() != 1);
} END_TEST;

START_TEST(symtableWithCall) {
    BoolExp *e;

    e = BoolExp::parseString("foo(x,y) || bar(x,z)");
    BoolExpSymbolSet t0(e);

    std::set<std::string> s0=t0.getSymbolSet();
    fail_if(!e || s0.find("x") == s0.end());
    fail_if(!e || s0.find("y") == s0.end());
    fail_if(!e || s0.find("z") == s0.end());
    fail_if(!e || s0.size() != 3);
} END_TEST;


Suite *cond_block_suite(void) {
    Suite *s  = suite_create("Suite");
    TCase *tc = tcase_create("Bool");
    tcase_add_test(tc, symtable);
    tcase_add_test(tc, symtableWithCall);
    suite_add_tcase(s, tc);
    return s;
}

int main() {
    Suite *s = cond_block_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
