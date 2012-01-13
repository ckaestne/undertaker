#
#   golem - analyzes feature dependencies in Linux makefiles
#
# Copyright (C) 2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
import os

from vamos.golem.kbuild import determine_buildsystem_variables_in_directory, apply_configuration

from vamos.selection import Selection
from vamos.golem.FileSet import FileSet, FileSetCache

def objects_in_dir(directory):
    """
    Returns object files and subdirectories in a specific directory

    @return tuple(set(object_files), set(subdirectories))
    """

    ret = (set(), set())

    for name in os.listdir(directory):
        name = os.path.join(directory, name)
        # is a directory
        if os.path.isdir(name):
            ret[1].add(name)
        elif name.endswith(".c"):
            name = name[:-len(".c")] + ".o"
            ret[0].add(name)
    return ret


class FileSymbolInferencer:
    def __init__(self, model, arch, subarch=None, subdir=""):
        """
        subdir is used to limit the subdirectories, on which the
        inference algorithm operates.

        """

        self.arch = arch
        self.subarch = subarch
        self.model = model

        apply_configuration(arch=self.arch, subarch=self.subarch)

        self.empty_selection = Selection()
        self.empty_fileset = FileSet(self.empty_selection, arch=arch, subarch=subarch)

        self.file_selections = {}

        for filename in self.empty_fileset.files:
            self.file_selections[filename] = None

        # Directories that caused a recursion loop
        self.visited_dirs = set()
        self.directory_prefix = subdir

        self.file_set_cache = FileSetCache(arch=arch, subarch=subarch)

    def calculate(self):
        directories = self.empty_fileset.dirs

        for directory in directories:
            symbol_slice = determine_buildsystem_variables_in_directory(directory)
            self.__calculate(directory, symbol_slice,
                             self.empty_selection,
                             self.empty_fileset)

        for objfile in self.file_selections:
            sourcefile = objfile[:-2] + '.c'
            if not os.path.exists(sourcefile):
                logging.warning("Failed to guess source file for %s", objfile)
                sourcefile = objfile # makes these cases easier to identify
            if self.file_selections[objfile]:
                print "FILE_%s \"%s\"" % (sourcefile, self.file_selections[objfile])
            else:
                print "FILE_%s" % sourcefile

    def __subdirectory_is_worth_working_on(self, directory):
        """
        Determine if it is worth to work on a specific directory,
        or if all object files and all directories within are already visited
        """

        (objects, dirs) = objects_in_dir(directory)

        if not directory.startswith(self.directory_prefix):
            logging.info("Skipping %s, not in scope", directory)
            return False

        # Are all objects visited?
        for obj in objects:
            if not obj in self.file_selections:
                return True

        # Are all directories visited
        for dirname in dirs:
            if not dirname in self.visited_dirs:
                return True

        logging.info("Skipping %s, already processed", directory)
        return False


    def __check_subdir(self, subdir, symbols):
        """
        @return a set of new symbols found in subdir
        """

        new_symbols = set()
        new_symbols.update(determine_buildsystem_variables_in_directory(subdir))
        new_symbols.update(symbols)

        # For tristate features also add the _MODULE
        # symbol
        to_be_removed = set()
        to_be_added = set()
        for symbol in symbols:
            if (symbol + "_MODULE") in self.model:
                to_be_added.add(symbol + "_MODULE")
            # Remove missing items in the first place
            if not symbol in self.model:
                to_be_removed.add(symbol)
                to_be_removed.add(symbol + "_MODULE")
        new_symbols = (new_symbols - to_be_removed).union(to_be_added)

        return new_symbols


    def __copy_selections(self, directory, base_symbols, current_selection, current_fileset):
        """
        @return a list of tulples: (subdir, symbols, selection, fileset)
        """
        file_selections = {}
        recursive_selections = []
        new_visited_dirs = self.visited_dirs

        for symbol in base_symbols:

            if symbol in current_selection.symbols:
                continue

            if not symbol in self.model:
                continue

            # Copy Selection
            new_selection = Selection(current_selection)
            new_selection.push_down()
            new_selection.add_alternative(symbol)
            new_fileset = self.file_set_cache.get_fileset(new_selection)

            ((f_added, _), (d_added, _)) = new_fileset.compare_to_base(current_fileset)

            for filename in f_added: # f_added
                if not filename in file_selections:
                    file_selections[filename] = Selection(new_selection)
                else:
                    file_selections[filename].add_alternative(symbol)

            # Is there a directory we haven't done recursion on yet,
            # so lets do a recursion step here.
            # We only consider subdirectories of the current directory
            new_subdirectories = d_added - self.visited_dirs
            new_subdirectories = set([dirname for dirname in new_subdirectories
                                      if dirname.startswith(directory) and
                                      dirname.startswith(self.directory_prefix)])
            if len(new_subdirectories) > 0:
                new_visited_dirs = new_visited_dirs.union(set(new_subdirectories))

                # If we want to go down recursive in a directory, we
                # add all symbols which are mentioned within the
                # directories Kbuild/Makefile
                for subdir in new_subdirectories:
                    new_symbols = self.__check_subdir(subdir, new_selection.symbols)
                    recursive_selections.append( tuple([subdir,
                                                        new_symbols,
                                                        new_selection,
                                                        new_fileset]))

        self.visited_dirs = new_visited_dirs

        return (recursive_selections, file_selections)


    def __calculate(self, directory, base_symbols, current_selection, current_fileset):
        """
        The heart of the inferencer, here the recursion is done.
        """

        if not self.__subdirectory_is_worth_working_on(directory):
            return

        logging.info("Starting __calculate on %s with %d symbols (%d files so far)",
                     directory, len(base_symbols), len(self.file_selections))

        (recursive_selections, file_selections) =  \
            self.__copy_selections(directory, base_symbols,
                                   current_selection, current_fileset)

        # Collection phase
        for (filename, selection) in file_selections.items():
            if not filename in self.file_selections:
                self.file_selections[filename] = selection
            else:
                if selection.better_than(self.file_selections[filename]):
                    self.file_selections[filename] = selection
                else:
                    # Sometimes Expressions can be merged if they only
                    # differ in one or_expression
                    merged = self.file_selections[filename].merge(selection)
                    if merged:
                        self.file_selections[filename] = merged

        # RECURSION!
        for (subdir, symbols, selection, fileset) in recursive_selections:
            self.__calculate(subdir, symbols, selection, fileset)
