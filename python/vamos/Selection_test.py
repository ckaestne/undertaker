# Copyright (C) 2011-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import unittest as t

from vamos.selection import Selection


class testSelection(t.TestCase):
    def setUp(self):
        self.empty = Selection()
        self.one_item = Selection([[("CONFIG_PM", "y")]])
        self.numa_item = Selection([[("CONFIG_NUMA", "y")]])
        self.two_item = Selection([[("CONFIG_PM", "y")], [("CONFIG_ACPI", "y")]])
        self.two_item_module = Selection([[("CONFIG_PM", "y")], [("CONFIG_ACPI", "m")]])

        self.alternative   = Selection([[("CONFIG_PM", "y")], [("CONFIG_ACPI", "y"), ("CONFIG_SOUND", "y")]])
        self.alternative_2 = Selection([[("CONFIG_PM", "y")], [("CONFIG_ACPI", "y"), ("CONFIG_SOUND", "m")]])


        self.not_better_than_one_item = Selection([[("CONFIG_X86", "y")], [("CONFIG_SOUND", "y")]])


    def test_equal(self):
        new = Selection(self.empty)
        self.assertEqual(str(self.empty), "")
        self.assertEqual(str(new), "")
        self.assertEqual(self.one_item, self.one_item)
        self.assertNotEqual(self.one_item, self.two_item)
        self.assertNotEqual(self.one_item, None)
        # can only compare to other selections
        self.assertNotEqual(Selection([[("CONFIG_FOO","y")]]), "CONFIG_FOO")


    def test_to_str(self):
        self.assertEqual(str(self.one_item), "CONFIG_PM=y")
        self.assertEqual(str(self.two_item), "CONFIG_ACPI=y && CONFIG_PM=y")
        self.assertEqual(str(self.alternative), "(CONFIG_ACPI=y || CONFIG_SOUND=y) && CONFIG_PM=y")


    def test_better_than(self):
        self.assertTrue(self.one_item.better_than(self.two_item))
        self.assertTrue(self.one_item.better_than(self.alternative))

        self.assertFalse(self.one_item.better_than(self.not_better_than_one_item))
        self.assertFalse(self.two_item.better_than(self.one_item))

    def test_merge_selections(self):
        # Two unmergable expressions
        self.assertEqual(self.two_item.merge(self.not_better_than_one_item), None)

        self.assertEqual(str(self.two_item.merge(self.two_item)), str(self.two_item))

        merged = self.two_item.merge(self.two_item_module)
        self.assertNotEqual(merged, None)
        self.assertEqual(str(merged), "(CONFIG_ACPI=m || CONFIG_ACPI=y) && CONFIG_PM=y")
        failedmerge = self.one_item.merge(self.two_item)
        self.assertEqual(failedmerge, None)

        merged = self.alternative.merge(self.alternative_2)
        self.assertEqual(str(merged), "(CONFIG_ACPI=y || CONFIG_SOUND=m || CONFIG_SOUND=y) && CONFIG_PM=y")

    def test_features(self):
        self.assertTrue(self.two_item.feature_in_selection("CONFIG_ACPI"))
        self.assertTrue(self.two_item_module.feature_in_selection("CONFIG_ACPI"))

    def test_alternatives(self):
        newitem = Selection(self.one_item)
        newitem.add_alternative("CONFIG_NUMA", "y")
        self.assertEqual(str(newitem), "(CONFIG_NUMA=y || CONFIG_PM=y)")
        newitem.push_down()
        newitem.add_alternative("CONFIG_ACPI", "y")
        self.assertEqual(str(newitem), "(CONFIG_NUMA=y || CONFIG_PM=y) && CONFIG_ACPI=y")

    def test_selection_invariance(self):
        a = Selection([[("CONFIG_BARFOO", "y")]])
        self.assertEqual(a.symbols, set(["CONFIG_BARFOO"]))

        self.assertEqual(self.two_item_module.symbols, set(["CONFIG_PM", "CONFIG_ACPI"]))


if __name__ == '__main__':
    t.main()
