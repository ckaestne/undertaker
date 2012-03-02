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

def tree_change(func, tree):
    """
    Calls func on every subtree and replaces it with the result if
    not None, otherwise we need to go deeper
    """

    if not type(tree) in (list, tuple) or len(tree) < 1:
        return tree
    a = func(tree)
    if a != None:
        return a
    i = 1

    while i < len(tree):
        b = tree_change(func, tree[i])
        if b == []:
            del tree[i]
        else:
            tree[i] = b
            i+= 1
    return tree


class BoolParserException(RuntimeError):
    def __init__(self, value):
        RuntimeError.__init__(self, value)

    def __unicode__(self):
        return "ERROR: " + self.value
    __repr__ = __unicode__

class BoolRewriterException(RuntimeError):
    def __init__(self, value):
        RuntimeError.__init__(self)
        self.value = value

    def __unicode__(self):
        return "ERROR: " + self.value
    __repr__ = __unicode__
