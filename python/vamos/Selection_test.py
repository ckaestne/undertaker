# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
        self.one_item = Selection([set(["CONFIG_PM"])])
        self.numa_item = Selection([set(["CONFIG_NUMA"])])
        self.two_item = Selection([set(["CONFIG_PM"]), set(["CONFIG_ACPI"])])
        self.two_item_module = Selection([set(["CONFIG_PM"]), set(["CONFIG_ACPI_MODULE"])])

        self.alternative = Selection([set(["CONFIG_PM"]), set(["CONFIG_ACPI", "CONFIG_SOUND"])])

        self.not_better_than_one_item = Selection([set(["CONFIG_X86"]), set(["CONFIG_SOUND"])])


    def test_equal(self):
        new = Selection(self.empty)
        self.assertEqual(str(self.empty), "")
        self.assertEqual(str(new), "")
        self.assertEqual(self.one_item, self.one_item)
        self.assertNotEqual(self.one_item, self.two_item)
        self.assertNotEqual(self.one_item, None)
        # can only compare to other selections
        self.assertNotEqual(Selection([set(["CONFIG_FOO"])]), "CONFIG_FOO")



    def test_to_str(self):
        self.assertEqual(str(self.one_item), "CONFIG_PM")
        self.assertEqual(str(self.two_item), "CONFIG_ACPI && CONFIG_PM")
        self.assertEqual(str(self.alternative), "(CONFIG_ACPI || CONFIG_SOUND) && CONFIG_PM")


    def test_better_than(self):
        self.assertTrue(self.one_item.better_than(self.two_item))
        self.assertTrue(self.one_item.better_than(self.alternative))

        self.assertFalse(self.one_item.better_than(self.not_better_than_one_item))
        self.assertFalse(self.two_item.better_than(self.one_item))

    def test_merge_selections(self):
        # Two unmergable expressions
        self.assertEqual(self.two_item.merge(self.not_better_than_one_item), None)

        merged = self.two_item.merge(self.two_item_module)
        self.assertNotEqual(merged, None)
        self.assertEqual(str(merged), "(CONFIG_ACPI || CONFIG_ACPI_MODULE) && CONFIG_PM")
        failedmerge = self.one_item.merge(self.two_item)
        self.assertEqual(failedmerge, None)

    def test_features(self):
        self.assertEqual(self.empty.features(), {})
        self.assertEqual(self.one_item.features(), {'CONFIG_PM': 'y'})
        self.assertEqual(self.two_item.features(), {'CONFIG_PM': 'y', 'CONFIG_ACPI':'y'})
        self.assertEqual(self.two_item_module.features(), {'CONFIG_PM': 'y', 'CONFIG_ACPI':'m'})

        alternate = self.alternative.features()
        self.assertTrue( alternate == {'CONFIG_PM': 'y', 'CONFIG_SOUND':'y'} or \
                         alternate == {'CONFIG_PM': 'y', 'CONFIG_ACPI':'y'})

        self.assertTrue(self.two_item_module.feature_in_selection("CONFIG_ACPI"))
        self.assertTrue(self.two_item_module.feature_in_selection("CONFIG_ACPI_MODULE"))

    def test_alternatives(self):
        newitem = self.one_item
        newitem.add_alternative("CONFIG_NUMA")
        self.assertEqual(str(newitem), "(CONFIG_NUMA || CONFIG_PM)")
        newitem.push_down()
        newitem.add_alternative("CONFIG_ACPI")
        self.assertEqual(str(newitem), "(CONFIG_NUMA || CONFIG_PM) && CONFIG_ACPI")

    def test_selection_invariance(self):
        a = Selection([set(["CONFIG_BARFOO"])])
        self.assertEqual(a.symbols, set(["CONFIG_BARFOO"]))

        self.assertEqual(self.two_item_module.symbols, set(["CONFIG_PM", "CONFIG_ACPI_MODULE"]))


if __name__ == '__main__':
    t.main()
