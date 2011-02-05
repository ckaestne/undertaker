import unittest as t
import StringIO
import RsfReader
from BoolRewriter import BoolParser as BP
from BoolRewriter import BoolRewriter as BR



class TestRsfReader(t.TestCase):
    def setUp(self):
        rsf = """Item A 'boolean'
Item B tristate
ItemFoo B XXX
Item C boolean
CRAP CARASDD"""
        self.rsf = RsfReader.RsfReader(StringIO.StringIO(rsf))

    def test_simple_expr(self):
        def rewrite(a, b, to_module = True):
            self.assertEqual(str(BR(self.rsf, a, eval_to_module = to_module).rewrite()), b)

        rewrite("A", "CONFIG_A")
        rewrite("!A", "!CONFIG_A")
        rewrite("!(A && C)", "(!CONFIG_A || !CONFIG_C)")

        rewrite("B", "(CONFIG_B_MODULE || CONFIG_B)")
        rewrite("!B", "!CONFIG_B")
        rewrite("B", "CONFIG_B", False)
        rewrite("!B", "(!CONFIG_B_MODULE && !CONFIG_B)", False)






if __name__ == '__main__':
    t.main()

