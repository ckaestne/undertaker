#!/usr/bin/env python
#
#   vampyr - extracts presence implications from kconfig dumps
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


import unittest as t

from vamos.vampyr.Messages import SparseMessage, SpatchMessage

class TestMessage(t.TestCase):

    def setUp(self):
        self.sparse_output = """\
kernel/fork.c:709: warning: incorrect type in argument 1 (different signedness) | expected unsigned int [noderef] [usertype] <asn:1>*uaddr | got int [noderef] <asn:1>*clear_child_tid
include/linux/mm.h:723: warning: potentially expensive pointer subtraction
kernel/fork.c:85: warning: symbol 'max_threads' was not declared. Should it be static?
kernel/fork.c:210: warning: symbol 'fork_init' was not declared. Should it be static?
make[1]: *** [kernel/fork.o] Error 42
"""
        messages = SparseMessage.preprocess_messages(self.sparse_output.splitlines())
        self.sparse_messages = [SparseMessage(None, x) for x in messages]


        self.spatch_output = """\
net/core/skbuff.c:935:24-33: reference preceded by free on line 920
net/core/skbuff.c.source1:935:24-33: reference preceded by free on line 920
net/core/skbuff.c.source0:935:24-33: reference preceded by free on line 920
Killed
"""

        messages = SpatchMessage.preprocess_messages(self.spatch_output.splitlines())
        self.spatch_messages = [SpatchMessage(None, x, "net/core/skbuff.c", "dummy.cocci") for x in messages]

    def test_number_of_messages_sparse(self):
        self.assertEqual(len(self.sparse_output.splitlines()),
                         len(self.sparse_messages) + 1 ) # ignore the make error

    def test_equality_sparse(self):
        self.assertNotEqual(self.sparse_messages[1], self.sparse_messages[2])

    def test_number_of_messages_spatch(self):
        self.assertEqual(len(self.spatch_messages) + 1, # ignore the 'Killed' line
                         len(self.spatch_output.splitlines()))

    def test_non_equality_spatch(self):
        reference = self.spatch_messages[0]
        for m in self.spatch_messages[1:]:
            self.assertEqual(reference.get_message(), m.get_message(),
                             "Messages not correctly normalized:\n%s" % 
                             "\n".join(x.get_message() for x in self.spatch_messages))

if __name__ == '__main__':
    t.main()
