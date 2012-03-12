# Copyright (C) 2012 Valentin Rothberg <valentinrothberg@googlemail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

#!/usr/bin/env python

import unittest2 as t
import tempfile
from vamos.Config import Config

class TestConfig(t.TestCase):
    def setUp(self):
        self.configA = tempfile.NamedTemporaryFile()
        self.configB = tempfile.NamedTemporaryFile()

        self.configA.file.write("""CONFIG_A=y
CONFIG_B=y
CONFIG_C=n
CONFIG_M=m""")
        self.configA.file.flush()

        self.configB.file.write("""CONFIG_A=y
CONFIG_B=n
CONFIG_C=y
CONFIG_M=n""")
        self.configB.file.flush()

    def test_readConfigFile(self):
        confA = Config(self.configA.name)
        self.assertTrue(confA.contains("CONFIG_A"))

    def test_valueOf(self):
        confA = Config()
        confA.readConfigFile(self.configA.name)
        self.assertEqual(confA.valueOf("CONFIG_A"),"y")

    def test_addFeature(self):
        confA = Config()
        confA.addFeature("CONFIG_FOO","m")
        self.assertEqual(confA.valueOf("CONFIG_FOO"),"m")

    def test_conflict(self):
        confA = Config()
        confA.readConfigFile(self.configA.name)

        confB = Config()
        confB.readConfigFile(self.configB.name)

        self.assertTrue(confA.conflict(confB))

    def test_getConflicts(self):
        confA = Config()
        confA.readConfigFile(self.configA.name)

        confB = Config()
        confB.readConfigFile(self.configB.name)

        conflicts = confA.getConflicts(confB)
        self.assertIn("CONFIG_B",conflicts)
        self.assertIn("CONFIG_C",conflicts)
        self.assertIn("CONFIG_M",conflicts)

if __name__ == '__main__':
    t.main()
