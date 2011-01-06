#include "KconfigRsfDb.h"

#include <assert.h>
#include <typeinfo>
#include <check.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>

static std::ofstream devnullo("/dev/null");
static std::ifstream devnulli("/dev/null");

typedef std::list<std::pair<std::string, std::string> > StringPairList;

START_TEST(rewrite_expression)
{
    KconfigRsfDb db(devnulli, devnullo);

    StringPairList teststrings;
    teststrings.push_back(std::make_pair("FOO=y", "CONFIG_FOO"));
    teststrings.push_back(std::make_pair("FOO=n", "(!CONFIG_FOO_MODULE && !CONFIG_FOO)"));
    teststrings.push_back(std::make_pair("FOO=m", "CONFIG_FOO_MODULE"));
    teststrings.push_back(std::make_pair("FOO!=y", "!CONFIG_FOO"));
    teststrings.push_back(std::make_pair("FOO!=n", "(CONFIG_FOO_MODULE || CONFIG_FOO)"));
    teststrings.push_back(std::make_pair("FOO!=m", "!CONFIG_FOO_MODULE"));
    teststrings.push_back(std::make_pair("FOO=BAR",
       "((CONFIG_FOO && CONFIG_BAR) || (CONFIG_FOO_MODULE && CONFIG_BAR_MODULE) || "
       "(!CONFIG_FOO && !CONFIG_BAR && !CONFIG_FOO_MODULE && !CONFIG_BAR_MODULE))"));
    teststrings.push_back(std::make_pair("FOO!=BAR",
       "((CONFIG_FOO && !CONFIG_BAR) || (CONFIG_FOO_MODULE && !CONFIG_BAR_MODULE) || "
       "(!CONFIG_FOO && CONFIG_BAR && !CONFIG_FOO_MODULE && CONFIG_BAR_MODULE))"));

    for (StringPairList::iterator i = teststrings.begin();
         i != teststrings.end(); ++i) {
        const char *input     = (*i).first.c_str();
        const char *reference = (*i).second.c_str();
        char *output          = strdup(db.rewriteExpressionPrefix(input).c_str());
        
        ck_assert_str_eq(output, reference);
        free(output);
    }
}
END_TEST

Suite * satchecker_suite(void) {
    Suite *s  = suite_create("SatChecker");
    TCase *tc = tcase_create("SatChecker");
    tcase_add_test(tc, rewrite_expression);

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
