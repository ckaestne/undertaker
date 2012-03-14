#
#   golem - analyzes feature dependencies in Linux makefiles
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

from vamos.golem.kbuild import files_for_selected_features
import logging

class FileSet:
    """
    A FileSet is a list of files and directories, compiled by a given selection.
    """

    def __init__(self, selection, arch, subarch=None):
        self.arch = arch
        self.subarch = subarch
        self.selection = selection
        try:
            (self.files, self.dirs) = files_for_selected_features(selection.features(), arch, subarch)
        except RuntimeError as e:
            logging.error("Failed to determine fileset, continuing anyways: %s", e)
            self.files = set()
            self.dirs = set()
            return
        self.files = self.files
        self.dirs = self.dirs

    def compare_to_base(self, other):
        """
        Compare the filesets to a other selection.

        @return ((f_added, f_deleted), (d_added, d_deleted))
        """

        f_added = self.files - other.files
        f_deleted = other.files - self.files
        d_added = self.dirs - other.dirs
        d_deleted = other.dirs - self.dirs

        return ((f_added, f_deleted), (d_added, d_deleted))

class FileSetCache(dict):
    def __init__(self, arch, subarch):
        dict.__init__(self)
        self.arch = arch
        self.subarch = subarch

    def get_fileset(self, selection):
        if not selection in self:
            self[selection] = [FileSet(selection, arch=self.arch, subarch=self.subarch), 1]
        else:
            # hit
            # logging.info("HIT " + str(sys.getsizeof(self)) + " " + str(len(self)))
            self[selection][1] += 1
        return self[selection][0]


