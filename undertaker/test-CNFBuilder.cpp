/*
 *   boolean framework for undertaker and satyr
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
#include "CNFBuilder.h"
#include "PicosatCNF.h"
#include "exceptions/CNFBuilderError.h"

#include <iostream>
#include <check.h>

using namespace kconfig;

START_TEST(buildCNFVarUsedMultipleTimes) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x && !x");

    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFOr) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x || y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFAnd) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x && y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFImplies) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x -> y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFEqual) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x <-> y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFNot) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x <-> !y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFMultiNot0) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "!!!x");

    cnf->pushAssumption("x",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    fail_if(!cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFMultiNot1) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "x <-> !!!y");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFConst) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "(x || 0) && (y && 1)");

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",true);
    cnf->pushAssumption("y",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    cnf->pushAssumption("y",false);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

START_TEST(buildCNFComplex0) {
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder(cnf, "a -> (b || !c && d)");

    /*
      A B C D  |  A -> (B v (~C & D))
      ---------+---------------------
      1 1 1 1  |    *1 x
      1 1 1 0  |    *1 x
      1 1 0 1  |    *1
      1 1 0 0  |    *1
      1 0 1 1  |    *0 x (shows, wether not does work)
      1 0 1 0  |    *0
      1 0 0 1  |    *1
      1 0 0 0  |    *0 x
      0 1 1 1  |    *1
      0 1 1 0  |    *1
      0 1 0 1  |    *1
      0 1 0 0  |    *1
      0 0 1 1  |    *1
      0 0 1 0  |    *1
      0 0 0 1  |    *1
      0 0 0 0  |    *1
    */

    cnf->pushAssumption("a",true);
    cnf->pushAssumption("b",true);
    cnf->pushAssumption("c",true);
    cnf->pushAssumption("d",true);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("a",true);
    cnf->pushAssumption("b",true);
    cnf->pushAssumption("c",true);
    cnf->pushAssumption("d",false);
    fail_if(!cnf->checkSatisfiable());

    cnf->pushAssumption("a",true);
    cnf->pushAssumption("b",false);
    cnf->pushAssumption("c",true);
    cnf->pushAssumption("d",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("a",true);
    cnf->pushAssumption("b",false);
    cnf->pushAssumption("c",false);
    cnf->pushAssumption("d",false);
    fail_if(cnf->checkSatisfiable());
} END_TEST;

void build_and_evaluate_strategy(const char *expression,
                                 bool free_strategy, bool bound_strategy) {
    PicosatCNF *cnf;

    // now bind '0' and '1' to false/true
    cnf = new PicosatCNF();
    try {
        CNFBuilder builder1(cnf, expression, false, CNFBuilder::ConstantPolicy::BOUND);
    } catch (CNFBuilderError &e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    if (bound_strategy) {
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: BOUND), but should be", expression);
    } else {
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: BOUND), should not be", expression);
    }
    delete cnf;

    // now check again with free strategy

    // NB: We need to parse again to start from scratch wrt. CNF
    // variable numberings.

    cnf = new PicosatCNF();
    try {
        CNFBuilder builder2(cnf, expression, false, CNFBuilder::ConstantPolicy::FREE);
    } catch (CNFBuilderError &e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    if (free_strategy) {
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: FREE), but should be", expression);
    } else {
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: FREE), should not be", expression);
    }
    delete cnf;

    // now bind '0' and '1' to false/true, again
    cnf = new PicosatCNF();
    try {
        CNFBuilder builder3(cnf, expression, false, CNFBuilder::ConstantPolicy::BOUND);
    } catch (CNFBuilderError &e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    if (bound_strategy) {
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: BOUND, pass2), but should be", expression);
    } else {
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: BOUND, pass2), should not be", expression);
    }
    delete cnf;

    // now check again with free strategy
    cnf = new PicosatCNF();
    try {
        CNFBuilder builder4(cnf, expression, false, CNFBuilder::ConstantPolicy::FREE);
    } catch (CNFBuilderError &e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    if (free_strategy) {
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: FREE), but should be", expression);
    } else {
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: FREE), should not be", expression);
    }
    delete cnf;
}

START_TEST(buildAndNull) {
    build_and_evaluate_strategy("A && 0", true, false);
} END_TEST;

START_TEST(buildImplNull) {
    build_and_evaluate_strategy("A && (A <-> 0)", true, false);
} END_TEST;

START_TEST(literals) {
    build_and_evaluate_strategy("0l", true, false);

    // FIXME: these calls below should succeed but currently don't
//  build_and_evaluate_strategy("0x0", true, false);
//  build_and_evaluate_strategy("0x0ull", true, false);
} END_TEST;

Suite *cond_block_suite(void) {
    Suite *s  = suite_create("Suite: test-CNFBuilder");
    TCase *tc = tcase_create("CNFBuilder");
    tcase_add_test(tc, buildCNFOr);
    tcase_add_test(tc, buildCNFAnd);
    tcase_add_test(tc, buildCNFImplies);
    tcase_add_test(tc, buildCNFEqual);
    tcase_add_test(tc, buildCNFNot);
    tcase_add_test(tc, buildCNFMultiNot0);
    tcase_add_test(tc, buildCNFMultiNot1);
    tcase_add_test(tc, buildCNFComplex0);
    tcase_add_test(tc, buildCNFConst);
    tcase_add_test(tc, buildAndNull);
    tcase_add_test(tc, buildImplNull);
    tcase_add_test(tc, buildCNFVarUsedMultipleTimes);
    tcase_add_test(tc, literals);
    tcase_add_test(tc, buildCNFVarUsedMultipleTimes);
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
