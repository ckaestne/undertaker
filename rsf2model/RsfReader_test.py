import unittest as t
import StringIO
from vamos.rsf2model import RsfReader


class TestRsfReader(t.TestCase):
    def setUp(self):
        rsf = """Item A 'boolean'
Item B tristate
ItemFoo B XXX
Item C boolean
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

    def test_symbol_generation(self):
        self.assertEqual(self.rsf.options()["A"].symbol(), "CONFIG_A")
        self.assertRaises(RsfReader.OptionNotTristate, self.rsf.options()["A"].symbol_module)
        self.assertEqual(self.rsf.options()["B"].symbol_module(), "CONFIG_B_MODULE")


if __name__ == '__main__':
    t.main()

