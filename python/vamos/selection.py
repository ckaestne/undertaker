#
#   vamos - common auxiliary functionality
#
# Copyright (C) 2011-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
#

import copy

def unique(seq):
    # order preserving
    noDupes = []
    _ = [noDupes.append(i) for i in seq if not noDupes.count(i)]
    return noDupes

class Selection:
    """A selection is a selection of symbols (derived from kconfig
    features). """

    def __init__(self, other_selection = None):
        """A selection can be empty, or can be copied from another
        selection. So this is somewhat like a copy constructor in c++.

        other_selection == None: empty selection
        other_selection == [{},{}]: selection from tuple
        other_selection == Selection(..): copy selection from other selection"""

        # expr == (SYMBOL || SYMBOL) && (SYMBOL || SYMBOL)
        # inner sets: alternatives
        # outer list: conjugations

        self.expr = []

        # All mentioned symbols in this selection
        self.symbols = set([])

        if other_selection:
            # Copy the other expression
            if type(other_selection) == list:
                self.expr = copy.deepcopy(other_selection)
            else:
                self.expr = copy.deepcopy(other_selection.expr)

        for or_expr in self.expr:
            for symbol in or_expr:
                self.symbols.add(symbol[0])

    def push_down(self):
        """Add a new level of or_expressions to the conjugation"""
        self.expr.append([])


    def add_alternative(self, alternative, value):
        """Add an alternative to the current "top of stack" or expression"""
        self.expr[-1].append((alternative, value))
        self.symbols.add(alternative)


    def feature_in_selection(self, feature):
        """ Tests if feature ({,_MODULE}) is already in this selection"""
        return feature in self.symbols

    def __len__(self):
        return len(self.expr)

    def __str__(self):
        """Format the selection to a (normalized) propositional formula
        (return-type: string)"""
        or_exprs = []
        for or_expr in self.expr:
            if len(or_expr) == 0:
                continue
            features = sorted(["%s=%s" % x for x in or_expr])
            if len(or_expr) == 1:
                or_exprs.append(features[0])
            else:
                or_exprs.append("(" + " || ".join(features) + ")")

        if len(or_exprs) > 0:
            return " && ".join(sorted(or_exprs))
        else:
            return ""

    __repr__ = __str__

    def __hash__(self):
        return hash(str(self))
    def __eq__(self, other):
        return str(self) == str(other)

    def to_dict(self):
        ret = {}
        for or_expr in self.expr:
            for f in or_expr:
                ret[f[0]] = f[1]
        return ret

    def better_than(self, other_selection):
        """One Selection is better than another if it is a subset of
        the other (and if it is shorter)"""
        if len(self.expr) < len(other_selection.expr):
            # When our expression is shorter we can test if our
            # expression is a subset of the other expression

            # This means all our expressions are within the other
            # expression
            for or_expr in self.expr:
                within = False
                for or_expr_1 in other_selection.expr:
                    if set(or_expr_1) == set(or_expr):
                        within = True
                if within == False:
                    return False
            return True
        else:
            return False


    def merge(self, other):
        """Expressions can be merged into one selection if they differ
        only in or_expression"""
        if len(self.expr) != len(other.expr):
            return None

        new_expr = []
        differences = 0
        different_or_expr = []

        for or_expr in self.expr:
            within = False
            for or_expr_1 in other.expr:
                if set(or_expr_1) == set(or_expr):
                    new_expr.append(or_expr)
                    within = True
            if not within:
                differences += 1
                different_or_expr = unique(different_or_expr + or_expr)
            if differences > 1:
                return None


        for or_expr in other.expr:
            within = False
            for or_expr_1 in self.expr:
                if set(or_expr_1) == set(or_expr):
                    within = True
            if not within:
                different_or_expr = unique(different_or_expr + or_expr)

        new_expr.append(different_or_expr)

        return Selection(new_expr)
