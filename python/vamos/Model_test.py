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

import unittest as t
from vamos.model import Model
import tempfile

class TestModel(t.TestCase):
    def setUp(self):
        self.model_file = tempfile.NamedTemporaryFile()
        self.model_file.file.write("""I: Items-Count: 12399
I: Format: <variable> [presence condition]
UNDERTAKER_SET SCHEMA_VERSION 1.1
UNDERTAKER_SET ALWAYS_ON "CONFIG_ALWAYS_ON" "CONFIG_BARFOO"
CONFIG_A
CONFIG_X "CONFIG_C -> CONFIG_FOO && (CONFIG_BLA)"
CONFIG_B CONFIG_A
CONFIG_C "!CONFIG_C_MODULE"
CONFIG_C_MODULE "!CONFIG_C && CONFIG_MODULE"
""")
        self.model_file.file.flush()

    def test_all_symbols_included(self):
        model = Model(self.model_file.name)
        for symbol in ["CONFIG_ALWAYS_ON", "CONFIG_BARFOO", "CONFIG_A", "CONFIG_B",
                       "CONFIG_C", "CONFIG_C_MODULE", "CONFIG_X"]:
            self.assertTrue(symbol in model, msg="%s not in model" % symbol)

        self.assertEqual(model["CONFIG_B"], "CONFIG_A")
        self.assertEqual(model["CONFIG_C"], "!CONFIG_C_MODULE")

    def test_leaf_features(self):
        model = Model(self.model_file.name)
        self.assertEqual(set(model.leaf_features()),set(['CONFIG_X', 'CONFIG_ALWAYS_ON', 'CONFIG_BARFOO', 'CONFIG_B']) )

    def test_mentioned_items(self):
        model = Model(self.model_file.name)
        self.assertEqual(model.mentioned_items("CONFIG_A"), [])
        self.assertEqual(model.mentioned_items("CONFIG_XYZ"), [])
        self.assertEqual(model.mentioned_items("CONFIG_B"), ["CONFIG_A"])
        self.assertEqual(set(model.mentioned_items("CONFIG_X")), set(["CONFIG_C", "CONFIG_FOO", "CONFIG_BLA"]))
        self.assertEqual(set(model.mentioned_items("CONFIG_C")), set(["CONFIG_C_MODULE"]))
        self.assertEqual(set(model.mentioned_items("CONFIG_C_MODULE")), set(["CONFIG_C", "CONFIG_MODULE"]))

    def test_slice_symbols(self):
        model = Model(self.model_file.name)
        self.assertEqual(set(model.slice_symbols(["CONFIG_B"])),set(["CONFIG_A", "CONFIG_B"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_A"])),set(["CONFIG_A"]))
        self.assertEqual(set(model.slice_symbols(["CONFIG_X"])),set(["CONFIG_X", "CONFIG_C", "CONFIG_FOO", "CONFIG_BLA", "CONFIG_C_MODULE", "CONFIG_MODULE"]))

if __name__ == '__main__':
    t.main()
