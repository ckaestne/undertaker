#!/usr/bin/env python
#
#   vampyr - extracts presence implications from kconfig dumps
#
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

from vamos.tools import execute, CommandFailed

import logging
import os

import vamos.vampyr

class ExpansionError(RuntimeError):
    """ Base class of all sort of Expansion errors """
    pass


class ExpansionSanityCheckError(ExpansionError):
    """ Internal kernel config sanity checks failed, like `make silentoldconfig` """
    pass

class PredatorError(RuntimeError):
    pass


def find_autoconf():
    """ returns the path to the autoconf.h file in this linux tree
    """

    if vamos.vampyr.autoconf_h:
        return vamos.vampyr.autoconf_h

    (autoconf, _) = execute("find include -name autoconf.h", failok=False)
    autoconf = filter(lambda x: len(x) > 0, autoconf)
    if len(autoconf) != 1:
        logging.error("Found %d autoconf.h files (%s)",
                      len(autoconf), ", ".join(autoconf))
        raise ExpansionSanityCheckError("Not exactly one autoconf.h was found")
    vamos.vampyr.autoconf_h = autoconf[0]
    return vamos.vampyr.autoconf_h


def get_conditional_blocks(filename, selected_only=False, configuration_blocks=True):
    """
    Counts the conditional blocks in the given source file

    The calculation is done using the 'predator' binary from the system
    path.  If the parameter 'actually_selected' is set, then the source
    file is preprocessed with 'cpp' while using current configuration as
    per 'include/generated/autoconf.h'

    If the parameter configuration_blocks is set to true, only
    configuration control conditional blocks will be counted, otherwise
    all blocks. This function will use the 'zizler' tool in the former
    case, and 'predator in the latter case.

    @return a non-empty list of blocks found in the source file

    """

    if configuration_blocks:
        normalizer = 'zizler -cC "%s"' % filename
    else:
        normalizer = 'predator "%s"' % filename

    if selected_only:
        try:
            cmd = '%s | cpp -include %s' % (normalizer, find_autoconf())
        except ExpansionSanityCheckError:
            # maybe we need to do something more aggressive or clever here
            (files, _) = execute("find include -name autoconf.h -print -delete", failok=False)
            logging.warning("Deleted spurious configuration files: %s", ", ".join(files))
            raise
    else:
        cmd = normalizer
    (blocks, rc) = execute(cmd, echo=True, failok=True)

    blocks = filter(lambda x: len(x) != 0 and x.startswith("B"), blocks)
    if rc != 0:
        logging.warning("'%s' signals exitcode: %d", cmd, rc)
        if rc == 127:
            raise CommandFailed(cmd, 127)
    return blocks


def get_loc_coverage(filename):
    """
    Returns LOC of the given file taking the current configuration into account
    """

    assert(os.path.exists(filename))
    cmd = "grep -v -E '^\s*#\s*include' %s | cpp -include %s" % \
        (filename, find_autoconf())
    (lines, _) = execute(cmd, echo=True, failok=True)
    # we ignore the exitcode here as we are not interested in failing
    # for #error directives and similar.
    return len(lines)
