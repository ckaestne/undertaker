#
#   rsf2model - extracts presence implications from kconfig dumps
#
# Copyright (C) 2012 Manuel Zerpies <manuel.f.zerpies@ww.stud.uni-erlangen.de>
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
#

import ast
import re
from vamos.rsf2model.helper import BoolParserException

class BoolParser (ast.NodeTransformer):
    AND = "and"
    OR  = "or"
    NOT = "not"
    EQUAL = "=="
    NEQUAL = "!="

    def __init__(self, bool_expr):
        ast.NodeTransformer.__init__(self)

        try:
            expr = bool_expr.replace("&&", " and ").replace("||", "or")
            expr = re.sub("!(?=[^=])", " not ", expr)
            expr = re.sub("(?<=[^!])=", " == ", expr)
            expr = re.sub("((?<=[( &|])|^)(?=[0-9])", "VAMOS_MAGIC_", expr)
            expr = re.sub("^ *", "", expr)
            self.tree = ast.parse(expr)
        except:
            raise BoolParserException("Parsing failed: '" + expr + "'")

        if self.tree.__class__ != ast.Module \
                or len(self.tree.body) > 1 \
                or self.tree.body[0].__class__ != ast.Expr:
            raise BoolParserException("No valid boolean expression")
        self.tree = self.tree.body[0]

    @staticmethod
    def new_node(node_type, childs):
        return [node_type] + childs

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

    @staticmethod
    def visit_Name(node):
        if node.id.startswith("VAMOS_MAGIC_"):
            return node.id[len("VAMOS_MAGIC_"):]
        return node.id

    def visit(self, node):
        func = "visit_" + node.__class__.__name__
        if hasattr(self, func):
            return (getattr(self, func))(node)
        else:
            raise BoolParserException("Boolean Parsing failed, unkown expression: " + repr(node))
    def to_bool(self):
        return self.visit(self.tree)
