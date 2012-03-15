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

import unittest as t

import re
import os.path
import glob

from vamos.tools import execute

def find_blocks(cmd, regex):
    (output, _) = execute(cmd, failok=False)
    blocks = set()
    blocks.add(0)
    for line in output:
        m = re.match(regex, line)
        if m:
            blocks.add(int(m.group(1)))

    return blocks

class testVamos(t.TestCase):
    def check_highest_blockno(self, filename):

        self.assertTrue(os.path.exists(filename))

        b1 = find_blocks("zizler -c %s" % filename, r"^B(\d+)")
        b2 = find_blocks("undertaker -j cpppc %s" % filename,
                              r"^&& \( B(\d+) <->")
        self.assertEqual(max(b1), max(b2),
                         "File %s different highest blockno (%d vs %d)" \
                             % (filename, max(b1), max(b2)))

        self.assertEqual(b1, b2,
                         "zizler and undertaker disagree on found blocks")
        return len(b1)

    def check_blocksets(self, filename):
        b1 = find_blocks("zizler -cC %s" % filename, r"^B(\d+)")
        b2 = find_blocks("undertaker -j cpppc %s" % filename,
                              r"^&& \( B(\d+) <->")
        self.assertEqual(len(b1 - b2), 0)

    def test_highest_blockno(self):
        files = set()
        for f in glob.glob("../undertaker/validation/*.c"):
            self.check_highest_blockno(f)
            files.add(f)
        print "\nhighest blockno: Checked %d files" % len(files)

    def test_blocksets(self):
        files = set()
        for f in glob.glob("../undertaker/validation/*.c"):
            self.check_blocksets(f)
            files.add(f)
        print "\nblocksets: Checked %d files" % len(files)

if __name__ == '__main__':
    t.main()
