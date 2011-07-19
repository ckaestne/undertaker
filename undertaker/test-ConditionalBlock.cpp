/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

#include "ConditionalBlock.h"

#include <stdlib.h>
#include <assert.h>
#include <check.h>




CppFile file("validation/conditional-block-test");

ConditionalBlock *block_a, *block_b, *block_ifdef, *block_elsif;

START_TEST(cond_parse_test)
{


    fail_unless(file.good());
    fail_unless(file.size() == 4);

    // two top level blocks
    fail_unless(file.topBlock()->size() == 2);


    fail_unless(block_a->size() == 0); // Block a has no children
    fail_unless(block_b->size() == 2); // Block b has two children (if+else)

    // Parents
    fail_unless(file.topBlock()->getParent() == 0);
    fail_unless(block_a->getParent() == file.topBlock());
    fail_unless(block_b->getParent() == file.topBlock());

    fail_unless(block_a->getPrev() == 0);
    fail_unless(block_b->getPrev() == 0);

    fail_unless(block_ifdef->size() == 0);
    fail_unless(block_elsif->size() == 0);
    fail_unless(block_ifdef->getParent() == block_b);
    fail_unless(block_elsif->getParent() == block_b);
    fail_unless(block_elsif->getPrev() == block_ifdef);

}
END_TEST;

START_TEST(cond_getConstraints) {
    ck_assert_str_eq(file.topBlock()->getConstraintsHelper().c_str(),
                     "B00");

    ck_assert_str_eq(block_a->getConstraintsHelper().c_str(),
                     "( B1 <-> ! A. )");

    ck_assert_str_eq(block_b->getConstraintsHelper().c_str(),
                     "( B4 <-> B.. )");

    ck_assert_str_eq(block_ifdef->getConstraintsHelper().c_str(),
                     "( B5 <-> B4 && X. )");

    ck_assert_str_eq(block_elsif->getConstraintsHelper().c_str(),
                      "( B7 <-> B4 && ( ! (B5) ) )");

} END_TEST;

Suite *
cond_block_suite(void) {
    ConditionalBlock::iterator i = file.topBlock()->begin();
    block_a = *i;
    i++;
    block_b = *i;

    ConditionalBlock::iterator j = block_b->begin();
    block_ifdef = *j;
    j++;
    block_elsif = *j;


    Suite *s  = suite_create("ConditionalBlock");
    TCase *tc = tcase_create("Conditional");
    tcase_add_test(tc, cond_parse_test);
    tcase_add_test(tc, cond_getConstraints);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        CppFile f(argv[1]);
        ConditionalBlock::iterator i = f.topBlock()->begin();
        block_a = *i;

        std::cout << f.topBlock()->getCodeConstraints() << std::endl;
    }

    Suite *s = cond_block_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
