#!/usr/bin/env python
#
#   vampyr - extracts presence implications from kconfig dumps
#
# Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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


import os.path
from pprint import pprint
from tempfile import NamedTemporaryFile

import unittest2 as t

from vamos.vampyr.BuildFrameworks import BareBuildFramework
from vamos.vampyr.utils import get_conditional_blocks
from vamos.tools import setup_logging

class Coverage_test(t.TestCase):
    def setUp(self):
        setup_logging(1) # set to 2 for more debug
        self.bf = BareBuildFramework()

    def test_coverage_1(self):
        with NamedTemporaryFile() as f:
            f.write("""
int main(void) {

#ifdef CONFIG_A
    void *p = 0;
#else

#if HAVE_FOO
    void *p;
#endif
#endif
}
""")
            f.flush()
            assert os.path.exists(f.name)

            self.assertEqual(['B0', 'B1', 'B00'], get_conditional_blocks(f.name))
            results = self.bf.analyze_configuration_coverage(f.name)
            self.assertEqual(results['all_config_count'], 2)
            self.assertEqual(len(results['all_configs']),2)
            for b in ['B0', 'B00', 'B1']:
                self.assertIn(b, results['configuration_blocks'])
            for b in ['B2']:
                self.assertNotIn(b, results['configuration_blocks'])
            for b in ['B0', 'B00', 'B1', 'B2']:
                self.assertIn(b, results['blocks_covered'])

            # now test detected blocks
            blocks = results['blocks']
            self.assertEqual(len(blocks), 2)
            for s in blocks.values():
                self.assertIn('B00', s)

            linum_map = results['linum_map']
            LOC = 0
            for _, v in linum_map.items():
                LOC += v
            self.assertEqual(LOC, results['lines_total'])
            self.assertEqual(results['lines_total'], results['lines_covered'])

            if 0:
                pprint(results)

if __name__ == '__main__':
    t.main()
