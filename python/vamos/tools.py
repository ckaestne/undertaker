#
#   vamos - common auxiliary functionality
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
#

import logging
import os.path

from subprocess import *


class CommandFailed(RuntimeError):
    """ Indicates that some command failed

    Attributes:
        command    -- the command that failed
        returncode -- the exitcode of the failed command
    """
    def __init__(self, command, returncode):
        assert(returncode != 0)
        self.command = command
        self.returncode = returncode
        self.repr = "Command %s failed to execute (returncode: %d)" % \
            (command, returncode)
        RuntimeError.__init__(self, self.repr)
    def __str__(self):
        return self.repr


def setup_logging(log_level):
    """ setup the logging module with the given log_level """

    l = logging.WARNING # default
    if log_level == 1:
        l = logging.INFO
    elif log_level >= 2:
        l = logging.DEBUG

    logging.basicConfig(level=l)


def execute(command, echo=True, failok=True):
    """
    executes 'command' in a shell.

    optional parameter echo can be used to suppress emitting the command
    to logging.debug.

    if failok is set to false, an RuntimeError will be raised with the
    full commandname and exitcode.

    returns a tuple with
     1. the command's standard output as list of lines
     2. the exitcode
    """
    os.environ["LC_ALL"] = "C"

    if echo:
        logging.debug("executing: " + command)
    p = Popen(command, stdout=PIPE, stderr=STDOUT, shell=True)
    (stdout, _)  = p.communicate() # stderr is merged into STDOUT
    if not failok and p.returncode != 0:
        raise CommandFailed(command, p.returncode)
    if len(stdout) > 0 and stdout[-1] == '\n':
        stdout = stdout[:-1]
    return (stdout.__str__().rsplit('\n'), p.returncode)


def calculate_worklist(args, batch_mode=False):
    """
    Calculates a sanitizes worklist from a list of given arguments

    This method is intended to be passed lists from the command line. It
    is passed a list of filenames, which are tested individually for
    existance. Non-existing filenames are skipped with a warning.

    Optionally, the given files can be interpreted as worklist. In this
    case, each line in the given files is tested for existance and
    considered for the worklist.

    In any case, the caller can rely on a set of filenames which are
    good for processing.
    """

    worklist = set()

    if batch_mode:
        for worklist_file in args:
            oldlen = len(worklist)
            try:
                with open(worklist_file) as fd:
                    for line in fd:
                        f = os.path.normpath(line.rstrip())
                        if os.path.exists(f):
                            worklist.add(f)
                        else:
                            logging.warning("Skipping non-existent file %s", f)
            except IOError:
                logging.warning("failed to process worklist %s", worklist_file)
            logging.info("Added %d files from worklist %s",
                         len(worklist) - oldlen, worklist_file)
    else:
        for filename in args:
            f = os.path.normpath(filename)
            if os.path.exists(f):
                worklist.add(f)
            else:
                logging.warning("Skipping non-existent file %s", f)
    logging.info("Processing %d files in total", len(worklist))

    return worklist


def check_tool(tool):
    """
    checks if the specified tool is available and executable

    NB: the specified command will be executed and should therefore not
    have any side effects. For gcc or make pass an option such as '--version'.
    """

    try:
        execute(tool, failok=False)
    except CommandFailed:
        logging.error("Failed to locate %s, check your installation", tool)
        return False

    return True
