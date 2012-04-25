#
# Copyright (C) 2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

import vamos

from vamos.tools import execute, CommandFailed
from vamos.golem.kbuild import *


class testTools(t.TestCase):
    def setUp(self):
        pass

    def test_execute(self):
        (output, rc) = execute("false", failok=True)
        self.assertEqual(rc, 1)
        self.assertIs(len(output), 1)
        self.assertIs(len(output[0]), 0)
        with self.assertRaises(CommandFailed):
            execute("false", failok=False)

    def test_guessfilename(self):
        vamos.prefer_32bit = False

        (arch, subarch) = guess_arch_from_filename('arch/x86/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'x86_64')

        vamos.prefer_32bit = False
        (arch, subarch) = guess_arch_from_filename('arch/x86/../x86/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'x86_64')

        (arch, subarch) = guess_arch_from_filename('kernel/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'x86_64')

        (arch, subarch) = guess_arch_from_filename('././kernel/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'x86_64')

        vamos.prefer_32bit = True
        (arch, subarch) = guess_arch_from_filename('arch/x86/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'i386')

        (arch, subarch) = guess_arch_from_filename('arch/sparc/init.c')
        self.assertEqual(arch, 'sparc')
        self.assertEqual(subarch, 'sparc')

        (arch, subarch) = guess_arch_from_filename('arch/h8300/kernel/timer/tpu.c')
        self.assertEqual(arch, 'h8300')
        self.assertEqual(subarch, 'h8300')

        (arch, subarch) = guess_arch_from_filename('kernel/init.c')
        self.assertEqual(arch, 'x86')
        self.assertEqual(subarch, 'i386')

    def test_call_linux_makefile(self):

        vamos.prefer_32bit = True

        (cmd, rc) = call_linux_makefile('', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=i386', 'SUBARCH=i386'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('allnoconfig', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', 'allnoconfig', ' ARCH=i386', 'SUBARCH=i386'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('silentoldconfig', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', 'silentoldconfig', 'ARCH=i386', 'SUBARCH=i386'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('', filename='./arch/x86/init.o', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', 'ARCH=i386', 'SUBARCH=i386'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('', filename='./arch/um/init.o', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=um', 'SUBARCH=i386'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('arch/h8300/kernel/timer/tpu.o',
                                        filename='arch/h8300/kernel/timer/tpu.c',
                                        extra_variables="C=2",
                                        dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=h8300', 'SUBARCH=h8300', 'C=2', 'tpu.o'):
            self.assertIn(needle, cmd)


        vamos.prefer_32bit = False
        (cmd, rc) = call_linux_makefile('', filename='./arch/x86/init.o', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=x86_64', 'SUBARCH=x86_64'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('', filename='./arch/um/init.o', dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=um', 'SUBARCH=x86_64'):
            self.assertIn(needle, cmd)

        (cmd, rc) = call_linux_makefile('arch/h8300/kernel/timer/tpu.o',
                                        filename='arch/h8300/kernel/timer/tpu.c',
                                        extra_variables="C=2",
                                        dryrun=True)
        self.assertEqual(rc, 0)
        for needle in ('make', ' ARCH=h8300', 'SUBARCH=h8300', 'C=2', 'tpu.o'):
            self.assertIn(needle, cmd)

if __name__ == '__main__':
    t.main()
