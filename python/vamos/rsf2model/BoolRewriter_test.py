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
from vamos.rsf2model.BoolRewriter import BoolRewriter as BR



class TestRsfReader(t.TestCase):
    def setUp(self):
        rsf = """Item A 'boolean'
Item B tristate
ItemFoo B some_value
Item C boolean
Item S string
Item H hex
CRAP CARASDD"""
        self.rsf = RsfReader.RsfReader(StringIO.StringIO(rsf))

    def test_simple_expr(self):
        def rewrite(a, b, to_module = True):
            self.assertEqual(str(BR(self.rsf, a, eval_to_module = to_module).rewrite()), b)

        rewrite("64BIT", "CONFIG_64BIT")
        rewrite("FOO && 64BIT", "(CONFIG_FOO && CONFIG_64BIT)")
        rewrite("FOO && (64BIT)", "(CONFIG_FOO && CONFIG_64BIT)")


        rewrite("A", "CONFIG_A")
        rewrite("!A", "!CONFIG_A")
        rewrite("!(A && C)", "(!CONFIG_A || !CONFIG_C)")

        rewrite("B", "(CONFIG_B_MODULE || CONFIG_B)")
        rewrite("!B", "!CONFIG_B")
        rewrite("B", "CONFIG_B", False)
        rewrite("!B", "(!CONFIG_B_MODULE && !CONFIG_B)", False)

        rewrite("S", "CONFIG_S")
        rewrite("H", "CONFIG_H")

        rewrite("FOO=y", "CONFIG_FOO")

        rewrite("FOO=n", "(!CONFIG_FOO_MODULE && !CONFIG_FOO)")
        rewrite("FOO=m", "CONFIG_FOO_MODULE")
        rewrite("FOO!=y", "!CONFIG_FOO")
        rewrite("FOO!=n", "(CONFIG_FOO_MODULE || CONFIG_FOO)")
        rewrite("FOO!=m", "!CONFIG_FOO_MODULE")

        rewrite("FOO=BAR",
                "((CONFIG_FOO && CONFIG_BAR) || (CONFIG_FOO_MODULE && CONFIG_BAR_MODULE) "
                + "|| (!CONFIG_FOO && !CONFIG_BAR && !CONFIG_FOO_MODULE && !CONFIG_BAR_MODULE))")

        rewrite("FOO!=BAR", "((CONFIG_FOO && !CONFIG_BAR) "
                + "|| (CONFIG_FOO_MODULE && !CONFIG_BAR_MODULE) "
                + "|| (!CONFIG_FOO && CONFIG_BAR) "
                + "|| (!CONFIG_FOO_MODULE && CONFIG_BAR_MODULE))")

        rewrite("FOO && BAR", "(CONFIG_FOO && CONFIG_BAR)")

        rewrite("FOO && BAR=HURZ",
                "(CONFIG_FOO && "
                + "((CONFIG_BAR && CONFIG_HURZ) || (CONFIG_BAR_MODULE && CONFIG_HURZ_MODULE) || "
                + "(!CONFIG_BAR && !CONFIG_HURZ && !CONFIG_BAR_MODULE && !CONFIG_HURZ_MODULE)))")

        rewrite("FOO && BAR=FOO",
                "(CONFIG_FOO && "
                + "((CONFIG_BAR && CONFIG_FOO) || (CONFIG_BAR_MODULE && CONFIG_FOO_MODULE) || "
                + "(!CONFIG_BAR && !CONFIG_FOO && !CONFIG_BAR_MODULE && !CONFIG_FOO_MODULE)))")

        rewrite("B43 && (HW_RANDOM || HW_RANDOM=B43)",
                "(CONFIG_B43 && "
                + "(CONFIG_HW_RANDOM || "
                + "((CONFIG_HW_RANDOM && CONFIG_B43) || "
                + "(CONFIG_HW_RANDOM_MODULE && CONFIG_B43_MODULE) || "
                + "(!CONFIG_HW_RANDOM && !CONFIG_B43 && "
                + "!CONFIG_HW_RANDOM_MODULE && !CONFIG_B43_MODULE))))")

        self.rsf = RsfReader.RsfReader(open("validation/equals-module.fm"))

        rewrite("SSB_POSSIBLE && SSB && (PCI || PCI=SSB)",
                "(CONFIG_SSB_POSSIBLE && (CONFIG_SSB_MODULE || CONFIG_SSB) && "
                + "((CONFIG_PCI_MODULE || CONFIG_PCI) || ((CONFIG_PCI && CONFIG_SSB) || "
                + "(CONFIG_PCI_MODULE && CONFIG_SSB_MODULE) || "
                + "(!CONFIG_PCI && !CONFIG_SSB && !CONFIG_PCI_MODULE && !CONFIG_SSB_MODULE))))")

        rewrite("THINKPAD_ACPI && SND && (SND=y || THINKPAD_ACPI=SND)",
                "(CONFIG_THINKPAD_ACPI && CONFIG_SND && "
                + "(CONFIG_SND || ((CONFIG_THINKPAD_ACPI && CONFIG_SND) || "
                + "(CONFIG_THINKPAD_ACPI_MODULE && CONFIG_SND_MODULE) || "
                + "(!CONFIG_THINKPAD_ACPI && !CONFIG_SND && !CONFIG_THINKPAD_ACPI_MODULE && !CONFIG_SND_MODULE))))")


if __name__ == '__main__':
    t.main()
