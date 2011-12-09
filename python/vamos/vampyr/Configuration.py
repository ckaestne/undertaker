#
#   utility classes for working in source trees
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

from vamos.vampyr.Messages import SparseMessage, GccMessage, ClangMessage
from vamos.tools import execute

import logging
import re
import os
import shutil

class ExpansionError(RuntimeError):
    """ Base class of all sort of Expansion errors """
    pass


class ExpansionSanityCheckError(ExpansionError):
    """ Internal kernel config sanity checks failed, like `make silentoldconfig` """
    pass


def arch_from_file(filename, default="x86"):
    """Determine the needed architecture for a specified file"""

    m = re.search("arch/([^/]+)/", filename)
    if m:
        return m.group(1)
    return default


class Configuration:
    def __init__(self):
        pass

    def switch_to(self):
        raise NotImplementedError

    def call_sparse(self, on_file):
        raise NotImplementedError


class BareConfiguration(Configuration):
    def __init__(self, framework, cpp_flags):
        Configuration.__init__(self)

        self.cpp_flags = cpp_flags
        self.framework = framework


    def __repr__(self):
        return '"' + self.cpp_flags + '"'


    def switch_to(self):
        pass


    def __call_compiler(self, compiler, args, on_file):
        cmd = compiler + " " + args
        if compiler in self.framework.options['args']:
            cmd += " " + self.framework.options['args'][compiler]
        cmd += " " + self.cpp_flags
        cmd += " '" + on_file + "'"
        (out, returncode) = execute(cmd)
        if returncode == 127:
            raise RuntimeError(compiler + " not found on this system?")
        else:
            return (out, returncode)


    def call_sparse(self, on_file):
        (messages, statuscode) = self.__call_compiler("sparse", "", on_file)
        if statuscode != 0:
            messages.append(on_file +
                            ":1: error: cannot compile file [ARTIFICIAL, rc=%d]" % statuscode)

        messages = SparseMessage.preprocess_messages(messages)
        messages = map(lambda x: SparseMessage(self, x), messages)
        return messages


    def call_gcc(self, on_file):
        (messages, statuscode) = self.__call_compiler("gcc", "-o/dev/null -c", on_file)
        if statuscode != 0:
            messages.append(on_file +
                            ":1: error: cannot compile file [ARTIFICIAL, rc=%d]" % statuscode)

        messages = GccMessage.preprocess_messages(messages)
        messages = map(lambda x: GccMessage(self, x), messages)
        return messages


    def call_clang(self, on_file):
        (messages, statuscode) = self.__call_compiler("clang", "--analyze", on_file)

        if statuscode != 0:
            messages.append(on_file +
                            ":1: error: cannot compile file [ARTIFICIAL, rc=%d]" % statuscode)

        messages = ClangMessage.preprocess_messages(messages)
        messages = map(lambda x: ClangMessage(self, x), messages)
        return messages
