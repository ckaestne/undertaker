import tools
import ast
import re

class BoolParserException:
    def __init__(self, value):
        self.value = value
    def __unicode__(self):
        return "ERROR: " + self.value
    __repr__ = __unicode__

class BoolParser (ast.NodeTransformer):
    AND = "and"
    OR  = "or"
    NOT = "not"
    EQUAL = "=="
    NEQUAL = "!="
    def __init__(self, expr):
        try:
            expr = expr.replace("&&", " and ").replace("||", "or")
            expr = re.sub("!(?=[^=])", " not ", expr)
            expr = re.sub("(?<=[^!])=", " == ", expr)
            expr = re.sub("^ *", "", expr)
            self.tree = ast.parse(expr)
        except:
            raise BoolParserException("Parsing failed")

        if self.tree.__class__ != ast.Module \
                or len(self.tree.body) > 1 \
                or self.tree.body[0].__class__ != ast.Expr:
            raise BoolParserException("No valid boolean expression")
        self.tree = self.tree.body[0]

    def new_node(self, type, childs):
        return [type] + childs

    def visit_Expr(self, node):
        return self.visit(node.value)

    def visit_BoolOp(self, node):
        if type(node.op) == ast.And:
            return self.new_node(self.AND, map(self.visit, node.values))
        if type(node.op) == ast.Or:
            return self.new_node(self.OR,  map(self.visit, node.values))
        raise BoolParserException("Unkown boolean expression")

    def visit_UnaryOp(self, node):
        if type(node.op) == ast.Not:
            return self.new_node(self.NOT, [self.visit(node.operand)])
        raise BoolParserException("Unkown unary operation")

    def visit_Compare(self, node):
        if len(node.ops) == 1 and len(node.comparators) == 1:
            if type(node.ops[0]) in [ast.Eq, ast.NotEq]:
                left = self.visit(node.left)
                right = self.visit(node.comparators[0])
                if type(left) != str or type(right) != str:
                    raise BoolParserException("Too complex comparison")
                if type(node.ops[0]) == ast.Eq:
                    return self.new_node(self.EQUAL, [left, right])
                else:
                    return self.new_node(self.NEQUAL, [left, right])
        raise BoolParserException("Unkown compare operation")

    def visit_Name(self, node):
        return node.id

    def visit(self, node):
        func = "visit_" + node.__class__.__name__
        if hasattr(self, func):
            return (getattr(self, func))(node)
        else:
            raise BoolParserException("Boolean Parsing failed, unkown expression: " + repr(node))
    def to_bool(self):
        return self.visit(self.tree)


class BoolRewriterException(Exception):
    def __init__(self, value):
        self.value = value
    def __unicode__(self):
        return "ERROR: " + self.value
    __repr__ = __unicode__

def tree_change(func, tree):
    """Calls func on every subtree and replaces it with the result if not None, otherwise we need to go deeper"""
    if not type(tree) in (list, tuple) or len(tree) < 1:
        return tree
    a = func(tree)
    if a != None:
        return a
    ret = [tree[0]]
    i = 1

    while i < len(tree):
        b = tree_change(func, tree[i])
        if b == []:
            del tree[i]
        else:
            tree[i] = b
            i+= 1
    return tree


class BoolRewriter(tools.UnicodeMixin):
    ELEMENT = "in"

    def __init__(self, rsf, expr, eval_to_module = True):
        self.expr = BoolParser(expr).to_bool()
        self.rsf = rsf
        self.eval_to_module = eval_to_module

    def rewrite_not(self):
        self.expr = tree_change(self.__rewrite_not, self.expr)
        return self.expr

    def __rewrite_not(self, tree):
        if tree[0] == BoolParser.NOT and type(tree[1]) == list:
            tree = tree[1]
            if tree[0] == BoolParser.AND:
                tree = [BoolParser.OR] + map(lambda x: [BoolParser.NOT, x], tree[1:])
                return tree_change(self.__rewrite_not, tree)
            elif tree[0] == BoolParser.OR:
                tree = [BoolParser.AND] + map(lambda x: [BoolParser.NOT, x], tree[1:])
                return tree_change(self.__rewrite_not, tree)
            elif tree[0] == BoolParser.NOT:
                return tree_change(self.__rewrite_not, tree[1])
            elif tree[0] == BoolParser.EQUAL:
                tree[0] = BoolParser.NEQUAL
                return tree
            elif tree[0] == BoolParser.NEQUAL:
                tree[0] = BoolParser.EQUAL
                return tree

    def rewrite_tristate(self):
        self.expr = tree_change(self.__rewrite_tristate, self.expr)
        return self.expr

    def __rewrite_tristate(self, tree):
        def tristate_not(symbol):
            if symbol in self.rsf.options() and self.rsf.options()[symbol].tristate():
                if self.eval_to_module:
                    return [BoolParser.NEQUAL, symbol, "y"]
                else:
                    return [BoolParser.EQUAL,  symbol, "n"]
            return [BoolParser.NOT, symbol]
        def tristate(symbol):
            if symbol in self.rsf.options() and self.rsf.options()[symbol].tristate():
                if self.eval_to_module:
                    return [BoolParser.NEQUAL, symbol, "n"]
                else:
                    return [BoolParser.EQUAL,  symbol, "y"]
            return symbol

        if tree[0] in [BoolParser.AND, BoolParser.OR]:
            for i in range(1, len(tree)):
                if type(tree[i]) == list:
                    if tree[i][0] == BoolParser.NOT:
                        tree[i] = tristate_not(tree[i][1])
                    else:
                        tree[i] = tree_change(self.__rewrite_tristate, tree[i])
                else:
                    tree[i] = tristate(tree[i])
            return tree

    def rewrite_symbol(self):
        self.expr = tree_change(self.__rewrite_symbol, self.expr)
        return self.expr

    def __rewrite_symbol(self, tree):
        def to_symbol(tree):
            if type(tree) in [list, tuple]:
                return tree_change(self.__rewrite_symbol, tree)
            return self.rsf.symbol(tree)

        if tree[0] in [BoolParser.NOT, BoolParser.AND, BoolParser.OR]:
            return [tree[0]] + map(to_symbol, tree[1:])
        if tree[0] == BoolParser.EQUAL:
            left = tree[1]
            right = tree[2]
            left_y = self.rsf.symbol(left)
            left_m = self.rsf.symbol_module(left)
            right_y = self.rsf.symbol(right)
            right_m = self.rsf.symbol_module(right)

            if left.lower() in ["y", "n", "m"]:
                right, left = left, right
            if left.lower() in ["y", "n", "m"]:
                raise BoolRewriterException("compare literal with literal")
            if right == "y":
                return left_y
            elif right == "m":
                return left_m
            elif right == "n":
                return [BoolParser.AND,
                        [BoolParser.NOT, left_y],
                        [BoolParser.NOT, left_m]]
            else:
                # Symbol == Symbol
                return [BoolParser.OR,
                        [BoolParser.AND, left_y, right_y], # Either both y
                        [BoolParser.AND, right_y, right_m], # Or both
                        [BoolParser.AND, # Or everything disabled
                         [BoolParser.NOT, left_y], [BoolParser.NOT, left_m],
                         [BoolParser.NOT, right_y], [BoolParser.NOT, right_m]]]


        if tree[0] == BoolParser.NEQUAL:
            left = tree[1]
            right = tree[2]
            left_y = self.rsf.symbol(left)
            left_m = self.rsf.symbol_module(left)
            right_y = self.rsf.symbol(right)
            right_m = self.rsf.symbol_module(right)

            if left.lower() in ["y", "n", "m"]:
                right, left = left, right
            if left.lower() in ["y", "n", "m"]:
                raise BoolRewriterException("compare literal with literal")
            if right == "y":
                return [BoolParser.NOT, left_y]
            elif right == "m":
                return [BoolParser.NOT, left_m]
            elif right == "n":
                return [BoolParser.OR, left_y, left_m]
            else:
                # y -> !y =  !y1 or !y2
                # m -> !m =  !m1 or !m2
                # n -> !n =  !(y1 |m1) or (y2 | m2)
                return [BoolParser.AND,
                        [BoolParser.OR, [BoolParser.NOT, left_y], [BoolParser.NOT, right_y]],
                        [BoolParser.OR, [BoolParser.NOT, left_m], [BoolParser.NOT, right_m]],
                        [BoolParser.OR, [BoolParser.NOT, [BoolParser.OR, left_y, left_m]],
                         [BoolParser.OR, right_y, right_m]]]


    def dump(self):
        def __concat(tree):
            elements = []
            for e in tree[1:]:
                if type(e) in [list, tuple]:
                    if e[0] == BoolParser.NOT:
                        elements.append("!" + e[1])
                    else:
                        elements.append(__concat(e))
                else:
                    elements.append(e)
            cat = " && "
            if tree[0] == BoolParser.OR:
                cat = " || "
            return "(" + cat.join(elements) + ")"
        return __concat(self.expr)
    def rewrite(self):
        self.rewrite_not()
        self.rewrite_tristate()
        self.rewrite_symbol()
        return self
    def __unicode__(self):
        return self.dump()

