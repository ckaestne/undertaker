#!/usr/bin/env python
#
#   rsf2model - extracts presence implications from kconfig dumps
#
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
import StringIO
from vamos.rsf2model import RsfReader


class TestRsfReader(t.TestCase):
    def setUp(self):
        rsf = """Item A 'boolean'
Item B tristate
ItemFoo B some_value
Item C boolean
Item H hex
Item S string
CRAP CARASDD"""
        self.rsf = RsfReader.RsfReader(StringIO.StringIO(rsf))

    def test_correct_options(self):
        opts = set()
        for name in self.rsf.options():
            opt = self.rsf.options()[name]
            if opt.name == "A":
                opts.add("A")
                self.assertEqual(opt.tristate(), False, "Item state wrong parsed")
            if opt.name == "B":
                opts.add("B")
                self.assertEqual(opt.tristate(), True, "Item state wrong parsed")
            if opt.name == "C":
                opts.add("C")
                self.assertEqual(opt.tristate(), False, "Item state wrong parsed")
            if opt.name == "H":
                opts.add("H")
                self.assertEqual(opt.hex(), True, "Item state wrong")
            if opt.name == "S":
                opts.add("S")
                self.assertEqual(opt.string(), True, "Item state wrong")

    def test_symbol_generation(self):
        self.assertEqual(self.rsf.options()["A"].symbol(), "CONFIG_A")
        self.assertRaises(RsfReader.OptionNotTristate, self.rsf.options()["A"].symbol_module)
        self.assertEqual(self.rsf.options()["B"].symbol_module(), "CONFIG_B_MODULE")
        self.assertEqual(self.rsf.options()["S"].symbol(), "CONFIG_S")
        self.assertEqual(self.rsf.options()["H"].symbol(), "CONFIG_H")


if __name__ == '__main__':
    t.main()
