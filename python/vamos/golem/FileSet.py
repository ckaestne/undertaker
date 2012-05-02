#
#   golem - analyzes feature dependencies in Linux makefiles
#
# Copyright (C) 2011-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
#

import logging

class FileSet:
    """
    A FileSet is a list of files and directories, compiled by a given selection.
    """

    def __init__(self, atoms, selection):
        self.selection = selection
        self.atoms = atoms
        try:
            (self.var_impl, self.pov) = self.atoms.OP_list(selection)
        except RuntimeError as e:
            logging.error("Failed to determine fileset, continuing anyways: %s", e)
            self.var_impl = set()
            self.pov = set()
            return
    def compare_to_base(self, other):
        """
        Compare the filesets to a other selection.

        @return ((f_added, f_deleted), (d_added, d_deleted))
        """

        var_impl_added = self.var_impl - other.var_impl
        var_impl_deleted = other.var_impl - self.var_impl
        pov_added = self.pov - other.pov
        pov_deleted = other.pov - self.pov

        return ((var_impl_added, var_impl_deleted), (pov_added, pov_deleted))

class FileSetCache(dict):
    def __init__(self, atoms):
        dict.__init__(self)
        self.atoms = atoms

    def get_fileset(self, selection):
        if not selection in self:
            self[selection] = [FileSet(self.atoms, selection), 1]
        else:
            # hit
            # logging.info("HIT " + str(sys.getsizeof(self)) + " " + str(len(self)))
            self[selection][1] += 1
        return self[selection][0]


