/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
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

#include <string>
#include <iostream>
#include <typeinfo>

#include "PresetList.h"
#include "Logging.h"

#include <stdlib.h>
#include <assert.h>
#include <check.h>
#include <istream>

START_TEST(preset_add_test){
    PresetList *l = PresetList::getInstance();

    std::stringstream inputwl,inputbl;
    inputwl << "A" << std::endl << "B" << std::endl << std::endl << "C" << std::endl;
    inputbl << "H" << std::endl << "I" << std::endl << std::endl << "J" << std::endl;

    fail_unless(inputwl.good());
    fail_unless(inputbl.good());

    l->loadWhitelist(inputwl);

    ck_assert_str_eq(l->toSatStr().c_str(),"A && B && C");

    fail_if(l->isWhitelisted("D"));
    fail_if(l->isBlacklisted("D"));

    l->addToBlacklist("D");
    fail_if(l->isWhitelisted("D"));
    fail_unless(l->isBlacklisted("D"));

    l->addToWhitelist("E");
    fail_unless(l->isWhitelisted("E"));
    fail_if(l->isBlacklisted("E"));

    l->addToBlacklist("E");
    fail_unless(l->isWhitelisted("E"));
    fail_unless(l->isBlacklisted("E"));
    ck_assert_str_eq(l->toSatStr().c_str(),"A && B && C && !D && E && !E");

    l->loadBlacklist(inputbl);
    ck_assert_str_eq(l->toSatStr().c_str(),"A && B && C && !D && E && !E && !H && !I && !J");

    l->addToBlacklist("!F");
    ck_assert_str_eq(l->toSatStr().c_str(),"A && B && C && !D && E && !E && !H && !I && !J");

    l->addToWhitelist("!G");
    ck_assert_str_eq(l->toSatStr().c_str(),"A && B && C && !D && E && !E && !H && !I && !J");
}
END_TEST;

Suite * cond_block_suite(void) {
    Suite *s  = suite_create("PresetListSuite");
    TCase *tc = tcase_create("PresetList");
    tcase_add_test(tc, preset_add_test);
    suite_add_tcase(s, tc);

    return s;
}

int main(void) {
    logger.init(std::cout, Logging::LOG_INFO, Logging::LOG_INFO);
    Suite *s = cond_block_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}