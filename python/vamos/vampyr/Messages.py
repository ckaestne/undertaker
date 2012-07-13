#
#   utility classes for working in source trees
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2011-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
# Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
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

import re
import logging
import os.path

class MessageContainer(set):
    def add_message(self, configuration, message):
        for msg in self:
            if msg == message:
                msg.message_is_in_configuration(configuration)
                return
        # Message isn't in configuration already, so just add it
        self.add(message)


class SparseMessage:
    @staticmethod
    def preprocess_messages(lines):
        """Remove the linebreaks introduced by gcc and sparse depending on the line prefix"""
        last_prefix = None
        result = []
        # Remove empty lines
        lines = filter(lambda x: len(x) > 0, lines)
        # Strip coloumn numbers
        lines = map(lambda x: re.sub(r'(:\d+):\d+: ', r'\1: ', x), lines)
        for line in lines:
            m = re.match('^\s*([^ \t]+)([ \t]+)(.*)', line)
            if not m:
                logging.error("Failed to parse '%s'", line)
                continue

            if 'originally declared here' in line:
                continue

            if '***' in line:
                continue

            if last_prefix != None and m.group(1) == last_prefix:
                result[-1] += " | " + m.group(3)
                continue
            last_prefix = m.group(1)
            result.append(line)

        return result

    def __init__(self, configuration, line):
        self.configuration = configuration
        self.bare_message = line
        self.in_configurations = set([configuration])
        self.message = ""
        self.location = ""
        self.is_error = False
        self.parse()

    def message_is_in_configuration(self, configuration):
        """Tell this message that it is also in another configuration"""
        self.in_configurations.add(configuration)

    def parse(self):
        message = self.bare_message.split(" ", 2)

        if 'error: ' in self.bare_message:
            self.is_error = True
        elif 'warning: ' in self.bare_message:
            self.is_error = False
        else:
            raise RuntimeError("Sparse: Failed to determine criticality of '%s'" % self.bare_message)

        self.location = message[0]
        self.message = message[2]

    def __repr__(self):
        return "<CompileMessage: error: " + str(self.is_error) + \
            " loc: " + self.location + " msg: " + self.message + ">"

    def __hash__(self):
        return hash(self.bare_message)

    def __eq__(self, other):
        if not isinstance(other, SparseMessage):
            return False
        return self.bare_message == other.bare_message

    def get_message(self):
        return self.bare_message

    def get_report(self):
        return self.bare_message + "\n  in %d configs. e.g. in %s" \
               % ( len(self.in_configurations), str(self.configuration))


class GccMessage(SparseMessage):
    @staticmethod
    def preprocess_messages(messages):
        # Remove [-W<flag>]$ messages
        expr = re.compile("\s*\[(-W[^[]+|enabled by default)\]$")
        messages = map(lambda x: re.sub(expr, "", x), messages)
        messages = SparseMessage.preprocess_messages(messages)
        messages = filter(lambda x: re.match(".*:[0-9]+: (fatal )?(warning|error):", x), messages)
        messages = map(lambda x: re.sub(r'(:\d+):\d+: ', r'\1: ', x), messages)
        return messages

    def __init__(self, configuration, line):
        SparseMessage.__init__(self, configuration, line)

    def __hash__(self):
        return self.location.__hash__()

    def __eq__(self, other):
        if not isinstance(other, GccMessage):
            return False
        ret = self.bare_message == other.bare_message

        if self.configuration == other.configuration \
           and self.location == other.location:
            ret = True

        return ret


class ClangMessage(SparseMessage):
    @staticmethod
    def preprocess_messages(messages):
        # Remove [-W<flag>]$ messages
        expr = re.compile("\s*\[(-W[^[]+|enabled by default)\]$")
        messages = map(lambda x: re.sub(expr, "", x), messages)

        messages = filter(lambda x: re.match(".*:[0-9]+:.*(warning|error):", x), messages)
        return messages


    def __init__(self, configuration, line):
        SparseMessage.__init__(self, configuration, line)


    def parse(self):
        message = self.bare_message.split(" ", 2)
        if "error:" == message[1] or "fatal" == message[1]:
            self.is_error = True
        elif message[1] == "warning:" or message[1] == "note:":
            self.is_error = False
        else:
            raise RuntimeError("Clang: Couldn't parse " + self.bare_message)

        self.location = message[0]
        self.message = message[2]

        self.in_configurations = set([self.configuration])


class SpatchMessage(SparseMessage):
    @staticmethod
    def preprocess_messages(lines):
        # Remove unwanted, boring lines
        lines = filter(lambda x: len(x) > 0, lines)
        lines = filter(lambda x: not x.startswith("  "), lines)
        lines = filter(lambda x: not "***" in x, lines)
        lines = filter(lambda x: not "Killed" == x , lines)        
        return lines

    def __init__(self, configuration, line, filename, test):
        self.location = re.sub('.source([\d]+)$', '', filename)
        self.test = test
        SparseMessage.__init__(self, configuration,
                               re.sub('.source([\d]+)', '', line))

    def parse(self):
        try:
            _, information = self.bare_message.split(":", 1)
            line, message = information.split(" ", 1)
            self.location = "%s:%s" % (self.location, line)
            self.message = message
        except ValueError:
            self.message = ""

    def get_message(self):
        return "%s [%s]" % (self.bare_message, os.path.basename(self.test))

    def get_report(self):
        return self.bare_message + " [%s]\n  in %d configs. e.g. in %s" \
               % ( self.test, len(self.in_configurations), str(self.configuration))
