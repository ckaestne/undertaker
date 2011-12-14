#
#   golem - analyzes feature dependencies in Linux makefiles
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

from vamos.tools import find_scripts_basedir, execute

import logging
import os
import re

class TreeNotConfigured(RuntimeError):
    """ Indicates that this Linux tree is not configured yet """
    pass


def apply_configuration():
    """
    Applies the current configuration

    Expects a complete configuration in '.config'. Updates
    'include/config/auto.conf' and 'include/generated/autoconf.h' to
    match the current configuration.
    """

    cmd = "make silentoldconfig"
    (_, ret) = execute(cmd)
    if ret == 2:
        raise TreeNotConfigured("target 'silentoldconfig' failed")
    elif ret != 0:
        raise RuntimeError("Failed to run: '%s', exitcode: %d" % (cmd, ret))


def files_for_current_configuration(how=False):
    """
    to be run in a Linux source tree.

    Returns a list of files that are compiled with the current
    configuration. If the optional parameter 'how' is added, the
    returned list additionally indicates in what way (i.e., module or
    statically compiled) the file is included.
    """

    # locate directory for supplemental makefiles
    scriptsdir = find_scripts_basedir()
    assert os.path.exists(os.path.join(scriptsdir, 'Makefile.list_recursion'))

    apply_configuration()

    cmd = "make -f %(basedir)s/Makefile.list UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }

    (output, rc) = execute(cmd)
    if rc != 0:
        raise RuntimeError("Failed to run: '%s', exitcode: %d" % (cmd, rc))

    files = set()
    for line in output:
        if len(line) == 0:
            continue
        try:
            if (how):
                objfile = line
            else:
                objfile = line.split()[0]
            # try to guess the source filename
            sourcefile = objfile[:-2] + '.c'
            if os.path.exists(sourcefile):
                files.add(sourcefile)
            else:
                logging.warning("Failed to guess source file for %s", objfile)
        except IndexError:
            raise RuntimeError("Failed to parse line '%s'" % line)

    return files


def file_in_current_configuration(filename):
    """
    to be run in a Linux source tree.

    Returns the mode (as a string) the file is compiled in the current
    configuration:

        "y" - statically compiled
        "m" - compiled as module
        "n" - not compiled into the kernel

    """

    # locate directory for supplemental makefiles
    scriptsdir = find_scripts_basedir()
    assert(os.path.exists(os.path.join(scriptsdir, 'Makefile.list_recursion')))

    # normalize filename
    filename = os.path.normpath(filename)

    basename = filename.rsplit(".", 1)[0]
    logging.debug("checking file %s", basename)

    apply_configuration()

    cmd = "make -f %(basedir)s/Makefile.list UNDERTAKER_SCRIPTS=%(basedir)s compiled='%(filename)s'" % \
        { 'basedir' : scriptsdir,
          'filename': filename.replace("'", "\'")}

    (make_result, returncode) = execute(cmd)
    if returncode != 0:
        raise RuntimeError("Failed to run: '%s', exitcode: %d" % (cmd, returncode))

    for line in make_result:
        if line.startswith(basename):
            return line.split(" ")[1]

    return "n"


def determine_buildsystem_variables():
    """
    returns a list of kconfig variables that are mentioned in Linux Makefiles
    """
    cmd = "find . -name Kbuild -o -name Makefile -exec sed -n '/^obj-\$(CONFIG_/p' {} \+"
    find_result = execute(cmd)

    ret = set()
    # line might look like this:
    # obj-$(CONFIG_MODULES)           += microblaze_ksyms.o module.o
    for line in find_result[0]:
        m = re.search(r'^obj-\$\(CONFIG_(\w+)\)', line)
        if not m: continue
        config_variable = m.group(1)
        if (config_variable):
            ret.add(config_variable)

    return ret
