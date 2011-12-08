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
import sys

from subprocess import *

class NotALinuxTree(RuntimeError):
    """ Indicates we are not in a Linux tree """
    pass

def setup_logging(log_level):
    """ setup the logging module with the given log_level """

    l = logging.WARNING # default
    if log_level == 1:
        l = logging.INFO
    elif log_level >= 2:
        l = logging.DEBUG

    logging.basicConfig(level=l)


def execute(command, echo=True):
    """
    executes 'command' in a shell

    returns a tuple with
     1. the command's standard output as list of lines
     2. the exitcode
    """
    os.environ["LC_ALL"] = "C"

    if echo:
        logging.debug("executing: " + command)
    p = Popen(command, stdout=PIPE, stderr=STDOUT, shell=True)
    (stdout, _)  = p.communicate() # ignore stderr
    if len(stdout) > 0 and stdout[-1] == '\n':
        stdout = stdout[:-1]
    return (stdout.__str__().rsplit('\n'), p.returncode)


def get_linux_version():
    """
    Checks that the current working directory is actually a Linux Tree

    Uses a custom Makefile to retrieve the current kernel version. If we
    are in a git tree, additionally compare the git version with the
    version stated in the Makefile for plausibility.

    Raises a 'NotALinuxTree' exception if the version could not be retrieved.
    """

    scriptsdir = find_scripts_basedir()

    if not os.path.exists('Makefile'):
        raise NotALinuxTree("No 'Makefile' found")

    cmd = "make -f %(basedir)s/Makefile.version UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }

    (output, ret) = execute(cmd)
    if ret > 0:
        raise NotALinuxTree("Makefile does not indicate a Linux version")

    version = output[-1] # use last line, if not configured we get additional warning messages
    if os.path.isdir('.git'):
        cmd = "git describe"
        (output, ret) = execute(cmd)
        git_version = output[0]
        if (ret > 0):
            return 'v' + version

        if (not git_version.startswith('v')):
            raise NotALinuxTree("Git does not indicate a Linux version ('%s')" % \
                                    git_version)

        if git_version[1:].startswith(version[0:3]):
            return git_version
        else:
            raise NotALinuxTree("Git version does not look like a Linux version ('%s' vs '%s')" % \
                                    (git_version, version))
    else:
        return 'v' + version


def find_scripts_basedir():
    executable = os.path.realpath(sys.argv[0])
    base_dir   = os.path.dirname(executable)
    for d in [ '../lib', '../scripts']:
        f = os.path.join(base_dir, d, 'Makefile.list')
        if os.path.exists(f):
            return os.path.realpath(os.path.join(base_dir, d))
    raise RuntimeError("Failed to locate Makefile.list")
