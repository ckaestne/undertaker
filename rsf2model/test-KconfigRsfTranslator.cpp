/*
 *   rsf2model - convert dumpconf output to undertaker model format
 *
 * Copyright (C) 2010-2011 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "KconfigRsfTranslator.h"

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
    std::ifstream rsf("validation/equals-module.fm");
    fail_if(!rsf.good());
    KconfigRsfTranslator db(rsf, devnullo);
    db.initializeItems();

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
    teststrings.push_back(std::make_pair(
       "FOO && BAR",
       "CONFIG_FOO && CONFIG_BAR"));
    teststrings.push_back(std::make_pair(
       "FOO && BAR=HURZ",
       "CONFIG_FOO && "
         "((CONFIG_BAR && CONFIG_HURZ) || (CONFIG_BAR_MODULE && CONFIG_HURZ_MODULE) || "
         "(!CONFIG_BAR && !CONFIG_HURZ && !CONFIG_BAR_MODULE && !CONFIG_HURZ_MODULE))"));
    teststrings.push_back(std::make_pair(
       "FOO && BAR=FOO",
       "CONFIG_FOO && "
         "((CONFIG_BAR && CONFIG_FOO) || (CONFIG_BAR_MODULE && CONFIG_FOO_MODULE) || "
         "(!CONFIG_BAR && !CONFIG_FOO && !CONFIG_BAR_MODULE && !CONFIG_FOO_MODULE))"));
    teststrings.push_back(std::make_pair(
       "B43 && (HW_RANDOM || HW_RANDOM=B43)",
       "CONFIG_B43 && "
       "(CONFIG_HW_RANDOM || "
       "((CONFIG_HW_RANDOM && CONFIG_B43) || "
       "(CONFIG_HW_RANDOM_MODULE && CONFIG_B43_MODULE) || "
       "(!CONFIG_HW_RANDOM && !CONFIG_B43 && "
       "!CONFIG_HW_RANDOM_MODULE && !CONFIG_B43_MODULE)))"));
    teststrings.push_back(std::make_pair(
       "SSB_POSSIBLE && SSB && (PCI || PCI=SSB)",
       "CONFIG_SSB_POSSIBLE && (CONFIG_SSB_MODULE || CONFIG_SSB) && "
         "((CONFIG_PCI_MODULE || CONFIG_PCI) || ((CONFIG_PCI && CONFIG_SSB) || "
           "(CONFIG_PCI_MODULE && CONFIG_SSB_MODULE) || "
           "(!CONFIG_PCI && !CONFIG_SSB && !CONFIG_PCI_MODULE && !CONFIG_SSB_MODULE)))"));

    teststrings.push_back(std::make_pair(
       "THINKPAD_ACPI && SND && (SND=y || THINKPAD_ACPI=SND)",
       "CONFIG_THINKPAD_ACPI && CONFIG_SND && "
         "(CONFIG_SND || ((CONFIG_THINKPAD_ACPI && CONFIG_SND) || "
         "(CONFIG_THINKPAD_ACPI_MODULE && CONFIG_SND_MODULE) || "
         "(!CONFIG_THINKPAD_ACPI && !CONFIG_SND && !CONFIG_THINKPAD_ACPI_MODULE && !CONFIG_SND_MODULE)))"));

    for (StringPairList::iterator i = teststrings.begin();
         i != teststrings.end(); ++i) {
        const char *input     = (*i).first.c_str();
        const char *reference = (*i).second.c_str();
        char *output          = strdup(db.rewriteExpressionPrefix(input).c_str());

        ck_assert_str_eq(output, reference);
        free(output);
    }

//    db.dumpAllItems(std::cout);
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
