#!/usr/bin/env python
#
#   vampyr - extracts presence implications from kconfig dumps
#
# Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
# Copyright (C) 2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

from vamos.tools import execute, CommandFailed

import glob
import logging
import os
import re

class ExpansionError(RuntimeError):
    """ Base class of all sort of Expansion errors """
    pass


class ExpansionSanityCheckError(ExpansionError):
    """ Internal kernel config sanity checks failed, like `make silentoldconfig` """
    pass


def get_conditional_blocks(filename, autoconf_h=None, all_cpp_blocks=False,
                           strip_linums=True):
    """
    Counts the conditional blocks in the given source file

    The calculation is done using the 'zizler' program from the system
    path.

    If the parameter 'autoconf_h' is set to an existing file, then the
    source file is preprocessed with 'cpp' with the given
    configuration. Usually, this will be some 'autoconf.h'. For Linux,
    this can be detected with the method find_autoconf() from the golem
    package.

    If the parameter all_cpp_blocks is set to false, only configuration
    controlled conditional blocks will be counted, otherwise all
    blocks. Configuration controlled conditional blocks are blocks with
    an CPP expression that contains at least one CPP identifier starting
    with the 'CONFIG_'.

    Implementation detail: This function will use the 'zizler -cC'
    command if all_cpp_blocks is set to false, and 'zizler -c' if set to
    true.

    @return a non-empty list of blocks found in the source file

    """

    if all_cpp_blocks:
        normalizer = 'zizler -c "%s"' % filename
    else:
        normalizer = 'zizler -cC "%s"' % filename

    if autoconf_h and os.path.exists(autoconf_h):
        cmd = '%s | cpp -include %s' % (normalizer, autoconf_h)
    else:
        cmd = normalizer
    (blocks, rc) = execute(cmd, echo=True, failok=True)

    blocks = filter(lambda x: len(x) != 0 and x.startswith("B"), blocks)
    # With never versions of zizler line numbers for each block are
    # also printed. By default they are stripped, to retain backward
    # compatibility
    #  "B00 23" -> "B00"
    if strip_linums and len(blocks) > 0 and " " in blocks[0]:
        blocks = [x.split(" ",1)[0] for x in blocks]
    if rc != 0:
        logging.warning("'%s' signals exitcode: %d", cmd, rc)
        if rc == 127:
            raise CommandFailed(cmd, 127)
    return blocks


def get_block_to_linum_map(filename, all_cpp_blocks=True):
    """Returns a map from configuration controlled block name to a
    line count within the block"""

    blocks = get_conditional_blocks(filename, autoconf_h=False,
                                    all_cpp_blocks=all_cpp_blocks,
                                    strip_linums=False)

    d = dict([tuple(x.split(" ")) for x in blocks])
    for key in d:
        d[key] = int(d[key])
    return d


def get_loc_coverage(filename, autoconf_h=None):
    """
    Returns LOC of the given file taking the current configuration into account

    If the parameter 'autoconf_h' is set to an existing file, then the
    source file is preprocessed with 'cpp' with the given
    configuration. Usually, this will be some 'autoconf.h'. For Linux,
    configuration. Usually, this will be some 'autoconf.h'. If it is not
    set, then the file will not be preprocessed at all.

    The given filename is stripped from '#include' directives, and
    (optionally) configured with a configuration file.

    ..note: For Linux, use the method 'find_autoconf()' from the
            vamos.golem.kbuild package to find a suitable autoconf_h

    ..note: Use '/dev/null' for an empty configuration

    """

    assert(os.path.exists(filename))
    cmd = "grep -v -E '^\s*#\s*include' %s " % filename

    if autoconf_h and os.path.exists(autoconf_h):
        cmd += ' | cpp -include %s' % autoconf_h

    (lines, _) = execute(cmd, echo=True, failok=True)
    # we ignore the exitcode here as we are not interested in failing
    # for #error directives and similar.
    return len(lines)


def find_configurations(filename):
    """
    Collects all configurations that undertaker has created

    To be called right after invoking undertaker's coverage mode.
    """
    ret = list()
    for c in glob.glob(filename + ".config*"):
        if re.match(r".*\.config\d+$", c):
            ret.append(c)
    return sorted(ret)
