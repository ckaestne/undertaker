#!/usr/bin/env python
#
#   vampyr - extracts presence implications from kconfig dumps
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


import unittest as t

from vamos.vampyr.Messages import SparseMessage

class TestSparseMessage(t.TestCase):

    def setUp(self):
        self.sparse_output = """\
kernel/fork.c:709: warning: incorrect type in argument 1 (different signedness) | expected unsigned int [noderef] [usertype] <asn:1>*uaddr | got int [noderef] <asn:1>*clear_child_tid
include/linux/mm.h:723: warning: potentially expensive pointer subtraction
kernel/fork.c:85: warning: symbol 'max_threads' was not declared. Should it be static?
kernel/fork.c:210: warning: symbol 'fork_init' was not declared. Should it be static?
"""
        messages = SparseMessage.preprocess_messages(self.sparse_output.splitlines())
        self.messages = [SparseMessage(None, x) for x in messages]

    def test_number_of_messages(self):
        self.assertEqual(len(self.sparse_output.splitlines()),
                         len(self.messages))

    def test_equality(self):
        self.assertNotEqual(self.messages[1], self.messages[2])

if __name__ == '__main__':
    t.main()
