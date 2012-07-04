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
import copy
import Queue
import threading
import thread
import time

from vamos.selection import Selection
from vamos.tools import get_online_processors
from vamos.golem.FileSet import FileSetCache
from vamos.golem.inference_atoms import *


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

def all_variations(seq_SEQ):
    if len(seq_SEQ) == 0:
        return [[]]
    rest = all_variations(seq_SEQ[1:])
    ret = []
    for i in seq_SEQ[0]:
        for r in rest:
            ret.append([i] + r)
    return ret

def unique(seq):
   # order preserving
   noDupes = []
   [noDupes.append(i) for i in seq if not noDupes.count(i)]
   return noDupes

class Counter:
    def __init__(self):
        self.lock = threading.Lock()
        self.int = 0
    def inc(self):
        with self.lock:
            self.int += 1
    def dec(self):
        with self.lock:
            self.int -= 1
    def isZero(self):
        with self.lock:
            return self.int == 0


class Inferencer:
    def __init__(self, atoms):
        # The atom
        self.atoms = atoms
        self.cache = FileSetCache(self.atoms)
        self.lock = threading.Lock()

        self.visited_povs = {}
        self.var_impl_selections = {}

        self.pov_queue = Queue.Queue()
        self.work_queue = Queue.Queue()

        self.on_the_run = Counter()
        self.running = True

    def observer(self):
        while not self.on_the_run.isZero():
            time.sleep(0.2)
        with self.lock:
            self.running = False


    def work_queue_worker(self):
        while self.running:
            try:
                args = self.work_queue.get(True, 1)
                self.__test_selection(*args)
                self.on_the_run.dec()
            except Queue.Empty:
                pass
            except Exception as e:
                self.running = False
                logging.error(str(e))
                raise

    def generate_variations(self, base_select, pov):
        var_ints = self.atoms.OP_features_in_pov(pov)
        ret = []
        for var_group in var_ints:
            group = []
            for var_int in var_group:
                if type(var_int) == tuple:
                    group.append([var_int])
                else:
                    values = self.atoms.OP_domain_of_variability_intention(var_int) \
                        - set([self.atoms.OP_default_value_of_variability_intention(var_int)])
                    group.append([(var_int, value) for value in values])

            base_select_dict = base_select.to_dict()
            for variation in all_variations(group):
                d = dict(variation)
                skip = False
                delete_from_var = []
                for i in d:
                    base_select_value = base_select_dict.get(i, None)
                    if base_select_value != None:
                        if base_select_value != d[i]:
                            skip = True
                        else:
                            delete_from_var.append(i)

                variation = filter(lambda (var_int, value): not var_int in delete_from_var, variation)

                if not skip:
                    ret.append(variation)
        return unique(ret)

    def calculate(self):
        empty_selection = Selection()
        base_var_impl = self.cache.get_fileset(empty_selection)
        empty_var_impl = copy.deepcopy(base_var_impl)
        empty_var_impl.var_impl = set()
        for var_impl in base_var_impl.var_impl:
            self.var_impl_selections[var_impl] = [empty_selection]

        for pov in empty_var_impl.pov:
            self.on_the_run.inc()
            self.pov_queue.put(tuple([empty_selection, base_var_impl, pov]))

        thread.start_new_thread(self.observer, tuple())

        for i in range(0, int(get_online_processors() * 1.5)):
            thread.start_new_thread(self.work_queue_worker, tuple())

        while self.running:
            try:
                (base_select, base_var_impl, pov) = self.pov_queue.get(True, 1)
            except Queue.Empty:
                continue

            base_select_is_superset = any([x.better_than(base_select) for x in self.visited_povs.get(pov, [])])

            if not base_select_is_superset and self.atoms.pov_worth_working_on(pov):
                if not pov in self.visited_povs:
                    self.visited_povs[pov] = []
                self.visited_povs[pov].append(base_select)
                logging.info("Visiting POV: %s", pov)

                for variation in self.generate_variations(base_select, pov):
                    new_selection = Selection(base_select)
                    assert all([not var_int in new_selection.symbols
                                for (var_int, value) in variation])
                    for (var_int, value) in variation:
                        new_selection.push_down()
                        new_selection.add_alternative(var_int, value)

                    self.on_the_run.inc()
                    self.work_queue.put(tuple([pov, new_selection, base_var_impl]))

            self.on_the_run.dec()

        # Cleanup bad alternatives
        for var_impl in self.var_impl_selections:
            selection = self.var_impl_selections[var_impl]
            i = 0
            for i in range(0, len(selection)):
                for x in range(0, len(selection)):
                    if x != i and selection[i] and selection[x] \
                            and selection[i].better_than(selection[x]):
                        selection[x] = None

            again = True
            while again:
                again = False
                for i in range(0, len(selection)):
                    for x in range(0, len(selection)):
                        if x != i and selection[i] and selection[x]:
                            m = selection[i].merge(selection[x])
                            if m:
                                again = True
                                selection[i] = m
                                selection[x] = None

            self.var_impl_selections[var_impl] = filter(lambda x: x, selection)

        for i in self.var_impl_selections:
            if len(self.var_impl_selections[i]) > 0:
                print '%s "%s"' % (self.atoms.format_var_impl(i),
                                   self.atoms.format_selections(self.var_impl_selections[i]))
            else:
                print self.atoms.format_var_impl(i)

    def __test_selection(self, pov, current_selection, base_var_impl):
        new_var_impl = self.cache.get_fileset(current_selection)

        ((var_impl_added, _), (pov_added, _)) = new_var_impl.compare_to_base(base_var_impl)

        with self.lock:
            for var_impl in var_impl_added:
                if not var_impl in self.var_impl_selections:
                    self.var_impl_selections[var_impl] = [current_selection]
                else:
                    self.var_impl_selections[var_impl].append(current_selection)

        for pov in pov_added:
            self.on_the_run.inc()
            self.pov_queue.put(tuple([current_selection, new_var_impl, pov]))
