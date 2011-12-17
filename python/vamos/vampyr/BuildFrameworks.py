#
#   BuildFrameworks - utility classes for working in source trees
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

from vamos.vampyr.Configuration import BareConfiguration
from vamos.tools import execute


class BuildFramework:
    """ Base class for all Build System Frameworks"""

    def __init__(self, options=None):
        """
        options: Dictonary that control the used helper tools. The
        following keys are supported:

        args: Dictionary with extra arguments for various tools (see note below)

        exclude_others: boolean, if set, #include statements will be ignored.

        loglevel: sets the loglevel, see the logging package.

        Note: in the args dictionary, the 'undertaker' key can be used
        to pass extra arguments to undertaker when calculating the
        configurations. For instance, pass "-C simple" to select the
        non-greedy algorithm for caluculating configurations.

        For the tools 'sparse', 'gcc' and 'clang', this field allows you
        to enable additional warnings (e.g., -Wextra).
        """

        if options:
            assert(isinstance(options, dict))
            if options['args']:
                assert(isinstance(options['args'], dict))
            self.options = options

    def calculate_configurations(self, filename):
        raise NotImplementedError


class BareBuildFramework(BuildFramework):
    """ For use without Makefiles, works directly on source files """

    def __init__(self, options=None):
        BuildFramework.__init__(self, options)

    def calculate_configurations(self, filename):
        undertaker = "undertaker -j coverage -O cpp -q"
        if 'undertaker' in self.options['args']:
            undertaker += " " + self.options['args']['undertaker']
        undertaker += " '" + filename + "'"
        (output, _) = execute(undertaker, failok=False)
        flags_list = filter(lambda x: not x.startswith("I:"), output)
        configs = [BareConfiguration(self, x) for x in flags_list]
        return configs
