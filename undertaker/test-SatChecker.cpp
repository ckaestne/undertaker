/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
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

#include <assert.h>
#include <typeinfo>

#include <string>

#include <set>

#include "SatChecker.h"

#include <check.h>

int cnf_test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);

    if (error == 0)  {
        fail_unless(checker() == result,
                    "%s should evaluate to %d",
                    s.c_str(), (int) result);
        return checker.countClauses();
    } else {
        try {
            fail_unless(checker() == result,
                        "%s should evaluate to %d",
                        s.c_str(), (int) result);
            return checker.countClauses();
        } catch (std::runtime_error &e) {
            fail_unless(typeid(*error) == typeid(e),
                        "%s didn't throw the right exception should be %s, but is %s.",
                        typeid(*error).name(), typeid(e).name());
            return checker.countClauses();
        }
    }
    return checker.countClauses();
}

START_TEST(format_config_items_simple) {
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", true));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_HURZ", true));

    std::stringstream ss;
    int c = m.formatKconfig(ss, dummy);
    ck_assert_int_eq(3, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=y\n"
                     "CONFIG_HURZ=y\n");
} END_TEST

START_TEST(format_config_items_module) {
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", false));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_FOO_MODULE", true));

    std::stringstream ss;
    int c = m.formatKconfig(ss, dummy);
    ck_assert_int_eq(2, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=m\n");
} END_TEST

START_TEST(format_config_items_module_not_valid_in_kconfig) {
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", true));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_FOO_MODULE", true));

    std::stringstream ss;
    int c = m.formatKconfig(ss, dummy);
    ck_assert_int_eq(2, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=m\n");
} END_TEST

START_TEST(test_base_expression) {
    BaseExpressionSatChecker sat("X && Y  && !Z",0);
    std::set<std::string> a1;
    a1.insert("X");
    fail_if(!sat(a1));

    a1.insert("Z");
    fail_if(sat(a1));
} END_TEST

Suite * satchecker_suite(void) {
    Suite *s  = suite_create("SatChecker");
    TCase *tc = tcase_create("SatChecker");
    tcase_add_test(tc, format_config_items_simple);
    tcase_add_test(tc, format_config_items_module);
    tcase_add_test(tc, format_config_items_module_not_valid_in_kconfig);
    tcase_add_test(tc, test_base_expression);

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
