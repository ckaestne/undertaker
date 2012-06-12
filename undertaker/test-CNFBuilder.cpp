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
#include "CNFBuilder.h"
#include "PicosatCNF.h"
#include <iostream>
#include <check.h>
#include <string>

using namespace kconfig;
using namespace std;

START_TEST(buildCNFVarUsedMultipleTimes)
{
    BoolExp *e = BoolExp::parseString("x && !x");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

    fail_if(cnf->checkSatisfiable());

} END_TEST;


START_TEST(buildCNFOr)
{
    BoolExp *e = BoolExp::parseString("x || y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFAnd)
{
    BoolExp *e = BoolExp::parseString("x && y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFImplies)
{
    BoolExp *e = BoolExp::parseString("x -> y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFEqual)
{
    BoolExp *e = BoolExp::parseString("x <-> y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFNot)
{
    BoolExp *e = BoolExp::parseString("x <-> !y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFMultiNot0)
{
    BoolExp *e = BoolExp::parseString("!!!x");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

    cnf->pushAssumption("x",true);
    fail_if(cnf->checkSatisfiable());

    cnf->pushAssumption("x",false);
    fail_if(!cnf->checkSatisfiable());

} END_TEST;

START_TEST(buildCNFMultiNot1)
{
    BoolExp *e = BoolExp::parseString("x <-> !!!y");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFConst)
{
    BoolExp *e = BoolExp::parseString("(x || 0) && (y && 1)");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);

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

START_TEST(buildCNFComplex0)
{
    BoolExp *e = BoolExp::parseString("a -> (b || !c && d)");
    PicosatCNF *cnf = new PicosatCNF();
    CNFBuilder builder;
    builder.cnf = cnf;
    builder.pushClause(e);
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
    BoolExp *e = BoolExp::parseString(expression);
    PicosatCNF *cnf;

    if (!e) {
        fail("String %s could not be parsed", expression);
        return;
    }

    // now bind '0' and '1' to false/true
    cnf = new PicosatCNF();
    CNFBuilder builder1(false, CNFBuilder::BOUND);
    builder1.cnf = cnf;
    builder1.pushClause(e);
    if (bound_strategy)
        fail_unless(builder1.cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: BOUND), but should be", expression);
    else
        fail_if(builder1.cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: BOUND), should not be", expression);
    delete cnf;
    delete e;

    // now check again with free strategy

    // NB: We need to parse again to start from scratch wrt. CNF
    // variable numberings.
    e = BoolExp::parseString(expression);
    if (!e) {
        fail("String %s could not be parsed", expression);
        return;
    }

    cnf = new PicosatCNF();
    CNFBuilder builder2(false, CNFBuilder::FREE);
    builder2.cnf = cnf;
    builder2.pushClause(e);
    if (free_strategy)
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: FREE), but should be", expression);
    else
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: FREE), should not be", expression);
    delete cnf;
    delete e;

    // now bind '0' and '1' to false/true, again
    e = BoolExp::parseString(expression);
    if (!e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    cnf = new PicosatCNF();
    CNFBuilder builder3(false, CNFBuilder::BOUND);
    builder3.cnf = cnf;
    builder3.pushClause(e);
    if (bound_strategy)
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: BOUND, pass2), but should be", expression);
    else
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: BOUND, pass2), should not be", expression);
    delete cnf;
    delete e;

    // now check again with free strategy
    e = BoolExp::parseString(expression);
    if (!e) {
        fail("String %s could not be parsed", expression);
        return;
    }
    cnf = new PicosatCNF();
    CNFBuilder builder4(false, CNFBuilder::FREE);
    builder4.cnf = cnf;
    builder4.pushClause(e);
    if (free_strategy)
        fail_unless(cnf->checkSatisfiable(),
                    "%s is unsatisfiable (strategy: FREE), but should be", expression);
    else
        fail_if(cnf->checkSatisfiable(),
                    "%s is satisfiable (strategy: FREE), should not be", expression);
    delete cnf;
    delete e;
}

START_TEST(buildAndNull)
{
    build_and_evaluate_strategy("A && 0", true, false);
} END_TEST;

START_TEST(buildImplNull)
{
    build_and_evaluate_strategy("A && (A <-> 0)", true, false);

} END_TEST;

START_TEST(literals)
{
    build_and_evaluate_strategy("0l", true, false);

    // FIXME: these below should succeed but currently do not
    //build_and_evaluate_strategy("0x0", true, false);
    //build_and_evaluate_strategy("0x0ull", true, false);
} END_TEST;

Suite *cond_block_suite(void)
{

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
    tcase_add_test(tc, literals);
    tcase_add_test(tc, buildCNFVarUsedMultipleTimes);
    suite_add_tcase(s, tc);
    return s;
}

int main()
{

    Suite *s = cond_block_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
