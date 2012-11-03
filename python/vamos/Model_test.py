#!/usr/bin/env python
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2011 Valentin Rothberg <valentinrothberg@googlemail.com>
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

import unittest2 as t
from vamos.model import RsfModel, CnfModel
import tempfile

class TestModel(t.TestCase):
    def setUp(self):
        self.model_file = tempfile.NamedTemporaryFile()
        self.model_file.file.write("""I: Items-Count: 12399
I: Format: <variable> [presence condition]
UNDERTAKER_SET SCHEMA_VERSION 1.1
UNDERTAKER_SET ALWAYS_ON "CONFIG_B1 "CONFIG_B2"
UNDERTAKER_SET ALWAYS_OFF "CONFIG_B3" "CONFIG_B5"
CONFIG_T1
CONFIG_B3 "CONFIG_T2 -> CONFIG_FOO && (CONFIG_BAR)"
CONFIG_B4 CONFIG_T1
CONFIG_B5
CONFIG_T2 "!CONFIG_T2_MODULE && CONFIG_FOO"
CONFIG_T2_MODULE "!CONFIG_T2 && CONFIG_BAR && CONFIG_MODULES"
CONFIG_T3 "CONFIG_BAR"
""")
        self.model_file.file.flush()

        self.rsf_file = tempfile.NamedTemporaryFile()
        self.rsf_file.file.write("""Item B1 boolean
Default "y"
Item B2 boolean
Default "y"
Item T1 tristate
Default T1 "y"
Item B3 boolean
HasPrompts B3 0
Default B3 "FOO && BLA" "y"
Item B4 boolean
Default B4 "A" "y"
Item T2 tristate
Depends T2 "BAR"
Item T3 tristate
Depends T3 "BAR"
""")
        self.rsf_file.file.flush()

        self.cnf_file = tempfile.NamedTemporaryFile()
        self.cnf_file.file.write("""c File Format Version: 2.0
c Type info:
c c sym <symbolname> <typeid>
c with <typeid> being an integer out of:
c enum {S_BOOLEAN=1, S_TRISTATE=2, S_INT=3, S_HEX=4, S_STRING=5, S_OTHER=6}
c variable names:
c c var <variablename> <cnfvar>
c meta_value ALWAYS_ON CONFIG_FOO
c meta_value ALWAYS_OFF CONFIG_BAR
c sym BAR 1
c sym FOO 2
c var CONFIG_BAR 1
c var CONFIG_FOO 2
""")
        self.cnf_file.file.flush()

    def test_all_symbols_included(self):
        model = RsfModel(self.model_file.name)
        for symbol in ("CONFIG_B1", "CONFIG_B2", "CONFIG_B3", "CONFIG_B4",
                       "CONFIG_T1", "CONFIG_T2_MODULE", "CONFIG_T2", "CONFIG_T3"):
            self.assertIn(symbol, model)

        for symbol in ("CONFIG_ALWAYS_ON", "ALWAYS_ON"):
            self.assertNotIn(symbol, model)

    def test_mentioned_items(self):
        model = RsfModel(self.model_file.name)
        self.assertEqual(model.mentioned_items("CONFIG_T1"), [])
        self.assertEqual(model.mentioned_items("CONFIG_FOO"), [])
        self.assertEqual(model.mentioned_items("CONFIG_B4"), ["CONFIG_T1"])
        self.assertEqual(set(model.mentioned_items("CONFIG_B3")), set(["CONFIG_T2",
                "CONFIG_FOO", "CONFIG_BAR"]))
        self.assertEqual(set(model.mentioned_items("CONFIG_T2")), set(["CONFIG_T2_MODULE",
                "CONFIG_FOO"]))
        self.assertEqual(set(model.mentioned_items("CONFIG_T2_MODULE")), set(["CONFIG_T2",
                "CONFIG_MODULES","CONFIG_BAR"]))
        self.assertEqual(set(model.mentioned_items("CONFIG_T3")), set(["CONFIG_BAR"]))

    def test_symbol_always_on(self):
        model = RsfModel(self.model_file.name)
        for symbol in ("CONFIG_ALWAYS_ON", "CONFIG_BARFOO",
                       "CONFIG_A", "CONFIG_B", "CONFIG_C", "CONFIG_MODULE"):
            self.assertNotIn(symbol, model.always_on_items)

        for symbol in ("CONFIG_B1", "CONFIG_B2"):
            self.assertIn(symbol, model.always_on_items)

    def test_symbol_always_off(self):
        model = RsfModel(self.model_file.name)
        for symbol in ("CONFIG_B1", "B2", "CONFIG_B2"):
            self.assertNotIn(symbol, model.always_off_items)

        for symbol in ("CONFIG_B3", "CONFIG_B5"):
            self.assertIn(symbol, model.always_off_items)

    def test_slice_symbols(self):
        model = RsfModel(self.model_file.name)
        self.assertEqual(set(model.slice_symbols(["CONFIG_B1"])),set(["CONFIG_B1"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_B2"])),set(["CONFIG_B2"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_B3"])),set(["CONFIG_B3",
                "CONFIG_T2", "CONFIG_FOO", "CONFIG_BAR", "CONFIG_T2_MODULE",
                "CONFIG_MODULES"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_B4"])),set(["CONFIG_B4",
                "CONFIG_T1"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_T1"])),set(["CONFIG_T1"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_T2"])),set(["CONFIG_T2",
                "CONFIG_T2_MODULE", "CONFIG_FOO", "CONFIG_BAR", "CONFIG_MODULES"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_T2_MODULE"])),
                set(["CONFIG_T2_MODULE", "CONFIG_T2", "CONFIG_BAR", "CONFIG_MODULES",
                "CONFIG_FOO"]))

    def test_leaf_features(self):
        model = RsfModel(self.model_file.name)
        for x in ("CONFIG_B1", "CONFIG_B2", "CONFIG_B3", "CONFIG_B4", "CONFIG_T3"):
            self.assertIn(x, model.leaf_features())

    def test_none_leaf_features(self):
        model = RsfModel(self.model_file.name)
        self.assertNotIn(set(["CONFIG_FOO", "CONFIG_BAR", "CONFIG_T1", "CONFIG_T2"]),
                set(model.leaf_features()))

    def test_types(self):
        model = RsfModel(self.model_file.name, self.rsf_file.name)
        for x in ("CONFIG_B1", "CONFIG_B2", "CONFIG_B3", "CONFIG_B4"):
            self.assertEqual(model.get_type(x), "boolean")
        for x in ("CONFIG_T1", "CONFIG_T2"):
            self.assertEqual(model.get_type(x), "tristate")

    def test_cnf_symbols(self):
        model = CnfModel(self.cnf_file.name)
        for symbol in ('CONFIG_FOO', 'CONFIG_BAR'):
            self.assertIn(symbol, model)

        for symbol in ('FOO', 'BAR', 'CONFIG_BAR_MODULE'):
            self.assertNotIn(symbol, model)

    def test_cnf_types(self):
        model = CnfModel(self.cnf_file.name)
        self.assertEqual(model.get_type('CONFIG_BAR'), "boolean")
        self.assertEqual(model.get_type('CONFIG_FOO'), "tristate")

    def test_cnf_metaflgas(self):
        model = CnfModel(self.cnf_file.name)
        self.assertNotIn('CONFIG_FOO', model.always_off_items)
        self.assertNotIn('CONFIG_BAR', model.always_on_items)
        self.assertIn('CONFIG_BAR', model.always_off_items)
        self.assertIn('CONFIG_FOO', model.always_on_items)

    def test_cnf_is_bool_stristate(self):
        model = CnfModel(self.cnf_file.name)
        for symbol in ('CONFIG_FOO', 'CONFIG_BAR', 'FOO', 'BAR'):
            st = model.get_type(symbol)
            self.assertTrue(model.is_bool_tristate(symbol),
                            "symbol %s has type %s " % (symbol, st))

        with self.assertRaises(RuntimeError):
            self.assertFalse(model.is_bool_tristate('HURZ'))

if __name__ == '__main__':
    t.main()
