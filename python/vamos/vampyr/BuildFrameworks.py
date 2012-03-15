#
#   BuildFrameworks - utility classes for working in source trees
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2011-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

from vamos.vampyr.Configuration import BareConfiguration, LinuxConfiguration
from vamos.vampyr.utils import find_configurations, \
    get_conditional_blocks, get_block_to_linum_map
from vamos.golem.kbuild import apply_configuration, file_in_current_configuration, \
    guess_arch_from_filename, guess_subarch_from_arch, \
    call_linux_makefile, find_autoconf, \
    get_linux_version, NotALinuxTree
from vamos.tools import execute

import os.path
import logging

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
            if options.has_key('args'):
                assert(isinstance(options['args'], dict))
            self.options = options
        else:
            self.options = None

    def calculate_configurations(self, filename):
        raise NotImplementedError

    def analyze_configuration_coverage(self, filename, autoconf_h):
        """
        Analyzes the given file 'filename' for its configuration coverage.

        This method also includes statistics about blocks covered by 'allyeconfig'

        returns a dict with the following members:
         - lines_total: the total number of LOC in the file
         - lines_covered: the number of covered LOC in all configurations
         - lines_conditional_total: the number of LOC in #ifdef blocks
         - lines_conditional_covered: the number of LOC in selected #ifdef blocks
         - linum_map: the map as returned by 'get_block_to_linum_map(filename)'
         - blocks: a dict of configuration to the set of cond. blocks it contains
         - blocks_total: contains a set of all blocks in the file
         - blocks_covered: contains the set of all covered blocks over all configurations
         - blocks_uncovered: contains the set of blocks that are not in any configuration

        ..note: the dict blocks contains the configuration name as
                key. The name has the form 'configN', with N being a
                running number. The length if this dict is equal to the
                number of found confiurations of this file.

        """

        return_dict = {
            'blocks': {},
            'lines': {}}

        total_blocks = set(get_conditional_blocks(filename, None,
                                                  configuration_blocks=False))
        total_blocks.add("B00")
        return_dict['blocks_total'] = total_blocks

        conf_blocks = set(get_conditional_blocks(filename, None,
                                                 configuration_blocks=True))
        conf_blocks.add("B00")
        return_dict['configuration_blocks'] = conf_blocks

        covered_blocks = set()
        configs = self.calculate_configurations(filename)
        logging.info("%s: found %d configurations", filename, len(configs))

        return_dict['all_configs'] = find_configurations(filename)
        return_dict['all_config_count'] = len(return_dict['all_configs'])
        return_dict['expandable_configs'] = len(configs)

        for config in configs:
            try:
                config.switch_to()
                old_len = len(covered_blocks)
                return_dict['blocks'][config.config] = \
                    set(get_conditional_blocks(filename, autoconf_h,
                                               configuration_blocks=False)) \
                                               | set(["B00"])
                covered_blocks |= return_dict['blocks'][config.config]

                print "Config %s added %d additional blocks" % \
                    (config.config, len(covered_blocks) - old_len)

            except RuntimeError as e:
                logging.error("Failed to process config '%s': %s", filename, e)

        return_dict['blocks_covered'] = covered_blocks
        return_dict['blocks_uncovered'] = total_blocks - covered_blocks

        linum_map = get_block_to_linum_map(filename)
        return_dict['linum map'] = linum_map

        total_lines = sum(linum_map.values())
        covered_lines = 0
        for block in covered_blocks:
            covered_lines += linum_map[block]

        return_dict['lines_total'] = total_lines
        return_dict['lines_covered'] = covered_lines
        return_dict['lines_conditional total'] = total_lines - linum_map["B00"]
        if "B00" in covered_blocks:
            return_dict['lines_conditional covered'] = covered_lines - linum_map["B00"]
        else:
            return_dict['lines_conditional covered'] = 0

        return return_dict


class BareBuildFramework(BuildFramework):
    """ For use without Makefiles, works directly on source files """

    def __init__(self, options=None):
        BuildFramework.__init__(self, options)

    def calculate_configurations(self, filename):
        undertaker = "undertaker -q -j coverage -C min -O combined"
        if 'undertaker' in self.options['args']:
            undertaker += " " + self.options['args']['undertaker']
        undertaker += " '" + filename + "'"
        execute(undertaker, failok=False)

        configs = list()
        for cfgfile in find_configurations(filename):
            assert '.config' in cfgfile
            cppflagsfile = cfgfile.replace('.config', '.cppflags')
            with open(cppflagsfile) as f:
                flags = " ".join([s.rstrip() for s in f.readlines()])
            configs.append(BareConfiguration(self, cfgfile, flags))
        return configs


class KbuildBuildFramework(BuildFramework):
    """ For use with Linux, considers Kconfig constraints """

    def __init__(self, options=None):
        if not options:
            options=dict()
        BuildFramework.__init__(self, options)
        if not options.has_key('expansion_strategy'):
            options['expansion_strategy'] = 'alldefconfig'
        if not options.has_key('coverage_strategy'):
            options['coverage_strategy'] = 'min'
        if not options.has_key('arch'):
            options['arch'] = None
        if not options.has_key('subarch'):
            options['subarch'] = None

    def guess_arch_from_filename(self, filename):
        """
        Tries to guess the architecture from a filename. Uses golem's
        guess_arch_from_filename() method.
        """
        (arch, subarch) = guess_arch_from_filename(filename)
        self.options['arch'] = arch
        self.options['subarch'] = subarch
        return (arch, subarch)

    def calculate_configurations(self, filename):
        """Calculate configurations for a given file with Kconfig output mode"""

        if self.options['arch'] is None:
            oldsubarch = self.options['subarch']
            self.guess_arch_from_filename(filename)
            if oldsubarch is not None:
                # special case: if only subarch is set, restore it
                self.options['subarch'] = oldsubarch

        cmd = "undertaker -q -j coverage -C %s -O combined" % self.options['coverage_strategy']
        if os.path.isdir("models"):
            cmd += " -m models/%s.model" % self.options['arch']
        else:
            logging.info("No models directory found, running without models")

        logging.info("Calculating configurations for '%s'", filename)
        if self.options and self.options.has_key('args'):
            if 'undertaker' in self.options['args']:
                cmd += " " + self.options['args']['undertaker']

        cmd += " '%s'" % filename.replace("'", "\\'")
        (output, statuscode) = execute(cmd, failok=True)
        if statuscode != 0 or any([l.startswith("E:") for l in output]):
            logging.error("Running undertaker failed: %s", cmd)
            print "--"
            for i in output:
                logging.error(i)
            return set()

        logging.info("Testing which configurations are actually being compiled")
        configs = list()
        for cfgfile in find_configurations(filename):
            config_obj = LinuxConfiguration(self, cfgfile,
                                            arch=self.options['arch'], subarch=self.options['subarch'],
                                            expansion_strategy=self.options['expansion_strategy'])
            config_obj.switch_to()
            if file_in_current_configuration(filename, config_obj.arch, config_obj.subarch) != "n":
                logging.info("Configuration '%s' is actually compiled", cfgfile)
                configs.append(config_obj)
            else:
                logging.info("Configuration '%s' is *not* compiled", cfgfile)

        return configs

    def analyze_configuration_coverage(self, filename, autoconf_h=None):
        """
        Analyzes the given file 'filename' for its configuration coverage.

        This method also includes statistics about blocks covered by 'allyeconfig'

        returns a dict with the following members:
         - arch: the analyzed architecture
         - subarch: the analyzed subarchitecture
         - lines_total: the total number of LOC in the file
         - lines_covered: the number of covered LOC in all configurations
         - lines_conditional_total: the number of LOC in #ifdef blocks
         - lines_conditional_covered: the number of LOC in selected #ifdef blocks
         - linum_map: the map as returned by 'get_block_to_linum_map(filename)'
         - blocks: a dict of configuration to the set of cond. blocks it contains
         - blocks_allyesconfig: contains the set of blocks 'allyesconfig' contains
         - blocks_total: contains a set of all blocks in the file
         - blocks_covered: contains the set of all covered blocks over all configurations
         - blocks_uncovered: contains the set of blocks that are not in any configuration

        ..note: the dict blocks contains the configuration name as
                key. The name has the form 'configN', with N being a
                running number. The length if this dict is equal to the
                number of found confiurations of this file.

        """

        if self.options['arch'] is None:
            oldsubarch = self.options['subarch']
            self.guess_arch_from_filename(filename)
            if oldsubarch is not None:
                # special case: if only subarch is set, restore it
                self.options['subarch'] = oldsubarch

        if autoconf_h is None:
            # sanity check: remove existing configuration to ensure consistent behavior
            if os.path.exists(".config"):
                os.unlink(".config")
            apply_configuration(arch=self.options['arch'], subarch=self.options['subarch'])
            assert os.path.exists('.config')
            autoconf_h=find_autoconf()

        return_dict = BuildFramework.analyze_configuration_coverage(self, filename, autoconf_h)
        return_dict['arch'] = self.options['arch']
        return_dict['subarch'] = self.options['subarch']

        # generate the configuration for 'allyesconfig'
        call_linux_makefile("allyesconfig", failok=False,
                            arch=self.options['arch'], subarch=self.options['subarch'])

        if file_in_current_configuration(filename,
                            arch=self.options['arch'], subarch=self.options['subarch']) != "n":
            return_dict['blocks_allyesconfig'] = \
                set(get_conditional_blocks(filename, autoconf_h, configuration_blocks=False)) \
                    | set(["B00"])
        else:
            return_dict['blocks_allyesconfig'] = set()

        return return_dict


def select_framework(identifier, options):

    frameworks = {'kbuild' : KbuildBuildFramework,
                  'linux'  : KbuildBuildFramework,
                  'bare'   : BareBuildFramework}

    if identifier is None:
        identifier='bare'
        try:
            logging.info("Detected Linux version %s, selecting kbuild framework",
                         get_linux_version())
            identifier='linux'
        except NotALinuxTree:
            pass

    if os.environ.has_key('ARCH'):
        options['arch'] = os.environ['ARCH']

        if os.environ.has_key('SUBARCH'):
            options['subarch'] = os.environ['SUBARCH']
        else:
            options['subarch'] = guess_subarch_from_arch(options['arch'])

    if identifier in frameworks:
        bf = frameworks[identifier](options)
    else:
        raise RuntimeError("Build framework '%s' not found" % \
                               options['framework'])
    return bf
