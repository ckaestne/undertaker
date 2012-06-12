/*
 *   boolframwork - boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "bool.h"
#include "PicosatCNF.h"
#include <iostream>
#include <check.h>

using namespace kconfig;

START_TEST(simpleModel) {

    PicosatCNF cnf;
    // building the model

    // v2 -> v1
    // -2 1 0
    cnf.pushVar(-2);
    cnf.pushVar(1);
    cnf.pushClause();

    // v3 -> v1
    // -3 1 0
    cnf.pushVar(-3);
    cnf.pushVar(1);
    cnf.pushClause();

    //try some assumptions
    cnf.pushAssumption(3);
    cnf.pushAssumption(2);

    fail_unless(cnf.checkSatisfiable());
    fail_unless(cnf.deref(1) == true);
    fail_unless(cnf.deref(2) == true);
    fail_unless(cnf.deref(3) == true);

    //try other assumptions
    cnf.pushAssumption(-1);

    fail_unless(cnf.checkSatisfiable());
    fail_unless(cnf.deref(1) == false);
    fail_unless(cnf.deref(2) == false);
    fail_unless(cnf.deref(3) == false);

    //unsatisfiable assumptions
    cnf.pushAssumption(2);
    cnf.pushAssumption(-1);

    fail_if(cnf.checkSatisfiable());


} END_TEST;

START_TEST(moreComplexModel) {
    PicosatCNF cnf;

    // v2 -> v1
    // -2 1 0
    cnf.pushVar(-2);
    cnf.pushVar(1);
    cnf.pushClause();

    // v3 -> v1
    // -3 1 0
    cnf.pushVar(-3);
    cnf.pushVar(1);
    cnf.pushClause();

    // v4 -> v1
    // -4 1 0
    cnf.pushVar(-4);
    cnf.pushVar(1);
    cnf.pushClause();

    // v5 -> (v2 || !v3)
    //-5 2 -3 0
    cnf.pushVar(-5);
    cnf.pushVar(2);
    cnf.pushVar(-3);
    cnf.pushClause();

    // v6 -> (v2 || v4)
    //-6 2 4 0
    cnf.pushVar(-6);
    cnf.pushVar(2);
    cnf.pushVar(4);
    cnf.pushClause();

    // !v2
    //-2 0
    cnf.pushVar(-2);
    cnf.pushClause();

    // v5
    //5 0
    cnf.pushVar(5);
    cnf.pushClause();

    // v6
    //6 0
    cnf.pushVar(6);
    cnf.pushClause();

    fail_unless(cnf.checkSatisfiable());

} END_TEST;

START_TEST(sequentialUsage) {
    PicosatCNF satisfiable;  // build a satisfiable model


    // v1 || !v1
    satisfiable.pushVar(1);
    satisfiable.pushVar(-1);
    satisfiable.pushClause();

    fail_unless(satisfiable.checkSatisfiable());

    PicosatCNF unsatisfiable; // build an unsatisfiable model

    // v1 && !v1
    unsatisfiable.pushVar(1);
    unsatisfiable.pushClause();
    unsatisfiable.pushVar(-1);
    unsatisfiable.pushClause();

    fail_if(unsatisfiable.checkSatisfiable());

    PicosatCNF satisfiable1;   // building the model

    // v1 || !v1
    satisfiable1.pushVar(1);
    satisfiable1.pushVar(-1);
    satisfiable1.pushClause();

    fail_unless(satisfiable1.checkSatisfiable());

} END_TEST;

START_TEST(parallelUsage) {

    PicosatCNF satisfiable;
    PicosatCNF unsatisfiable;
    // building the model

    // v1 || !v1
    satisfiable.pushVar(1);
    satisfiable.pushVar(-1);
    satisfiable.pushClause();

     // v1 && !v1
    unsatisfiable.pushVar(1);
    unsatisfiable.pushClause();
    unsatisfiable.pushVar(-1);
    unsatisfiable.pushClause();

    fail_unless(satisfiable.checkSatisfiable());
    fail_if(unsatisfiable.checkSatisfiable());

    fail_unless(satisfiable.checkSatisfiable());
    fail_if(unsatisfiable.checkSatisfiable());

    fail_unless(satisfiable.checkSatisfiable());
    fail_unless(satisfiable.checkSatisfiable());
    fail_unless(satisfiable.checkSatisfiable());

    fail_if(unsatisfiable.checkSatisfiable());
    fail_if(unsatisfiable.checkSatisfiable());

    fail_unless(satisfiable.checkSatisfiable());
    fail_if(unsatisfiable.checkSatisfiable());

} END_TEST;

Suite *cond_block_suite(void) {

    Suite *s  = suite_create("PicosatCNF-test");
    TCase *tc = tcase_create("PicosatCNF");
    tcase_add_test(tc, simpleModel);
    tcase_add_test(tc, moreComplexModel);
    tcase_add_test(tc, sequentialUsage);
    tcase_add_test(tc, parallelUsage);

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
