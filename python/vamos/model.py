#
#   vamos - common auxiliary functionality
#
# Copyright (C) 2011-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2011-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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


from vamos.rsf2model.RsfReader import RsfReader

import logging
import re

class Model(dict):
    def __init__(self, path, rsf = None):
        dict.__init__(self)
        self.path = path
        self.always_on_items = set()

        with open(path) as fd:
            self.parse(fd)

        if not rsf and path.endswith(".model"):
            rsf = path[:-len(".model")] + ".rsf"

        if rsf:
            try:
                with open(rsf) as f:
                    self.rsf = RsfReader(f)
            except IOError:
                logging.warning ("no rsf file for model %s was found", path)

    def parse(self, fd):
        for line in fd.readlines():
            line = line.strip()
            if line.startswith("UNDERTAKER_SET ALWAYS_ON"):
                line = line.split(" ")[2:]
                always_on_items = [ l.strip(" \t\"") for l in line]
                for item in always_on_items:
                    self[item] = None
                    self.always_on_items.add(item)
                continue
            elif line.startswith("I:") or line.startswith("UNDERTAKER_SET"):
                continue
            line = line.split(" ", 1)
            if len(line) == 1:
                self[line[0]] = None
            elif len(line) == 2:
                self[line[0]] = line[1].strip(" \"\t\n")


    def mentioned_items(self, key):
        """
        @return list of mentioned items of the key's implications
        """
        expr = self.get(key, "")
        if expr == None:
            return []
        expr = re.split("[()&!|><-]", expr)
        expr = [x.strip() for x in expr]
        return [x for x in expr if len(x) > 0]

    def leaf_features(self):
        """
        Leaf features are feature that aren't mentioned anywhere else

        @return list of leaf features in model
        """

        # Dictionary of mentioned items
        features = dict([(key, False) for key in self.keys()
                        if not key.endswith("_MODULE")])
        for key in features.keys():
            items = self.mentioned_items(key)
            items += self.mentioned_items(key + "_MODULE")
            for mentioned in items:
                # Strip _MODULE POSTFIX
                if mentioned.endswith("_MODULE"):
                    mentioned = mentioned[:-len("_MODULE")]

                # A Leaf can't "unleaf" itself. This is important for
                # circular relations like:
                #
                # CONFIG_A -> !CONFIG_A_MODULE
                # CONFIG_A_MODULE -> !CONFIG_A
                if key != mentioned and mentioned in features:
                    features[mentioned] = True
        return sorted([key for key in features if features[key] == False])

    def get_type(self, symbol):
        """
        RuntimeError gets raised if no corresponding rsf is found.
        @return data type of symbol.
        """
        if not self.rsf:
            raise RuntimeError("No corresponding rsf file found.")
        if symbol.startswith("CONFIG_"):
            symbol = symbol[len("CONFIG_"):]
        return self.rsf.get_type(symbol)


    def is_bool_tristate(self, symbol):
        """
        Returns true if symbol is tristate, otherwise false is returned.

        Raises RuntimeError if no corresponding rsf-file is found.
        """

        if not self.rsf:
            raise RuntimeError("No corresponding rsf file found.")

        if symbol.startswith("CONFIG_"):
            symbol = symbol[len("CONFIG_"):]

        return self.rsf.is_bool_tristate(symbol)

    def slice_symbols(self, initial):
        """
        Apply the slicing algorithm to the given set of symbols
        returns a list of interesting symbol
        """
        if type(initial) == list:
            stack = initial
        else:
            stack = [initial]

        visited = set()

        while len(stack) > 0:
            symbol = stack.pop()
            visited.add(symbol)
            for new_symbols in self.mentioned_items(symbol):
                if not new_symbols in visited:
                    stack.append(new_symbols)
        return list(visited)
