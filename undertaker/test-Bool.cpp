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
#include <iostream>
#include <check.h>

using namespace kconfig;
using namespace std;

void parse_test(std::string input, bool good)
{
    BoolExp *e = BoolExp::parseString(input);
    fail_unless(good == (e!=0), input.c_str());
    delete e;
}

//from test-SatChecker
START_TEST(bool_parser_test)
{
        parse_test("", false);
    parse_test("A", true);
    parse_test("! A", true);
    parse_test("--0--", false);
    parse_test("A && B", true);
    parse_test("A  ||   B", true);
    parse_test("A &&", false);
    parse_test("(A && B) || C", true);
    parse_test("A && B && C && D", true);
    parse_test("A || C && B", true);
    parse_test("C && B || A", true);
    parse_test("! ( ! (A))", true);
    parse_test("!!!!!A", true);
    parse_test("A -> B", true);
    parse_test(" -> B", false);
    parse_test("(A -> B) -> A -> A", true);
    parse_test("(A <-> ! B) || ( B <-> ! A)", true);
    parse_test("A -> B -> C -> (D -> C)", true);
    parse_test("A && !A || B && !B", true);
    parse_test("A -> B -> C", true);
    parse_test("A <-> B", true);
    parse_test("( B23 <->  ( B1 )  && ( MAX_DMA_CHANNELS >= 12 ) >> 2 )", true);
    parse_test("( B0 <-> CONFIG_TTYS0_BASE == 0x2f8 )", true);
    parse_test("( B172 <-> B0 && (FAMILY_MMIO_BASE_MASK < 0xFFFFFF0000000000ull) )", true);
}

END_TEST

START_TEST(parseFunc)
{
    //function tests
    parse_test("foo(x)", true);
    parse_test("foo(!x)", true);
    parse_test("foo()", true);
    parse_test("foo(x,y)", true);
    parse_test("foo(x,y,z)", true);
    parse_test("foo(x,y) || bar(x,z)", true);
    parse_test("foo(bar(x))", true);
    parse_test("B00 && ( B0 <-> FOO( BAR(1,2), 3) ) && ( B1 <-> ( ! (B0) ) ) && B00 && ( B00 <-> FILE_normalize_expressions5.c )", true);
    //                                                               ^
    parse_test("B00 && ( B0 <-> ON. && A > 23 ) && ( B1 <-> ! ON. || 12 + (24 & 12) ) && (B00 -> ON.) && (!B00 -> (ON <-> ON.)) && B00 && ( B00 <-> FILE_comparator.c )", true);

} END_TEST;

START_TEST(parseBool)
{
    //simple tests
    BoolExp *e0 = BoolExp::parseString("X || Y && Z");
    BoolExp *e1 = BoolExp::parseString("(X || Y) && Z");
    BoolExp *e2 = BoolExp::parseString("(X || !Y) && Z");
    BoolExp *e3 = BoolExp::parseString("(X||\n !Y) \n\t     &&Z");
    BoolExp *e4 = BoolExp::parseString("ExFalso->Quodlibet");
    BoolExp *e5 = BoolExp::parseString("CONFIG_NO_HZ -> (CONFIG_GENERIC_TIME && CONFIG_GENERIC_CLOCKEVENTS)");
    BoolExp *e6 = BoolExp::parseString("0 || 1 || 01");
    BoolExp *e7 = BoolExp::parseString("0 || 1 || 'r'");

    fail_if(!e0 || e0->str() != "X || Y && Z", "cannot parse 'X || Y && Z'");
    fail_if(!e1 || e1->str() != "(X || Y) && Z", "cannot parse '(X || Y) && Z'");
    fail_if(!e2 || e2->str() != "(X || !Y) && Z", "cannot parse '(X || !Y) && Z'");
    fail_if(!e3 || e3->str() != "(X || !Y) && Z", "does not like spaces");
    fail_if(!e4 || e4->str() != "ExFalso -> Quodlibet", "Implication fails");
    fail_if(!e5 || e5->str() != "CONFIG_NO_HZ -> CONFIG_GENERIC_TIME && CONFIG_GENERIC_CLOCKEVENTS", "");
    fail_if(!e6 || e6->str() != "0 || 1 || 1", "Consts");
    fail_if(!e7 || e7->str() != "0 || 1 || 1", "Char Consts");

    delete e0;
    delete e1;
    delete e2;
    delete e3;
    delete e4;
    delete e5;
    delete e6;
    delete e7;

} END_TEST;

Suite *cond_block_suite(void)
{

    Suite *s  = suite_create("Suite");
    TCase *tc = tcase_create("Bool");
    tcase_add_test(tc, parseBool);
    tcase_add_test(tc, bool_parser_test);
    tcase_add_test(tc, parseFunc);
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
