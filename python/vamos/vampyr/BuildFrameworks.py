#
#   BuildFrameworks - utility classes for working in source trees
#
# Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2011-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
# Copyright (C) 2012 Christoph Egger <siccegge@informatik.uni-erlangen.de>
# Copyright (C) 2012 Valentin Rothberg <valentinrothberg@googlemail.com>
# Copyright (C) 2012 Manuel Zerpies <manuel.f.zerpies@ww.stud.uni-erlangen.de>
# Copyright (C) 2012 Stefan Hengelein <stefan.hengelein@informatik.stud.uni-erlangen.de>
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

import vamos

from vamos.vampyr.Configuration \
    import BareConfiguration, \
    LinuxConfiguration, LinuxStdConfiguration, LinuxPartialConfiguration, \
    BusyboxConfiguration, BusyboxStdConfiguration, BusyboxPartialConfiguration, \
    CorebootConfiguration, CorebootStdConfiguration, CorebootPartialConfiguration
from vamos.vampyr.utils import find_configurations, \
    get_conditional_blocks, get_block_to_linum_map
from vamos.golem.kbuild import apply_configuration, file_in_current_configuration, \
    guess_subarch_from_arch, \
    call_linux_makefile, find_autoconf, get_linux_version, NotALinuxTree, \
    call_makefile_generic, NotABusyboxTree, get_busybox_version, \
    NotACorebootTree, get_coreboot_version, coreboot_get_config_for
from vamos.tools import execute
from vamos.model import parse_model, get_model_for_arch

import os.path
import logging

class EmptyLinumMapException(RuntimeError):
    """ Indicates an Empty Linum Map in Buildframeworks """
    pass


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
            self.options = dict()
        if not self.options.has_key('coverage_strategy'):
            self.options['coverage_strategy'] = 'min'

    def make_configuration(self, basename, nth):
        raise NotImplementedError

    def make_partial_configuration(self, filename):
        raise NotImplementedError

    def apply_configuration(self):
        """
        Ensures that a newly set configuration is active.
        """
        pass

    def cleanup_autoconf_h(self):
        """
        deletes various autoconf.h files
        returns a list of deleted files
        """
        return list()

    def find_autoconf(self):
        return find_autoconf()

    def calculate_configurations(self, filename):
        """Calculate configurations for the given file

        returns a list of 'Configuration' objects that match the
        buildsystem class.
        """

        cmd = "undertaker -q -j coverage -C %s -O combined" % self.options['coverage_strategy']
        if self.options.has_key('model') and self.options['model']:
            cmd += " -m %s" % self.options['model']
        else:
            logging.info("No model specified, running without models")

        if self.options.has_key('whitelist'):
            cmd += " -W %s" % self.options['whitelist']

        if self.options.has_key('blacklist'):
            cmd += " -B %s" % self.options['blacklist']

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

        return self.verify_configurations(filename)

    def verify_configurations(self, filename):
        raise NotImplementedError

    def get_stdconfig(self, filename, verify=True):
        # pylint: disable=W0613
        return None

    def file_in_current_configuration(self, filename):
        raise NotImplementedError

    def analyze_configuration_coverage(self, filename):
        """
        Analyzes the given file 'filename' for its configuration coverage.

        If all_cpp_blocks is set to True, all CPP blocks will be considered,
        only configuration conditional blocks otherwise.
        This method also includes statistics about blocks covered by 'allyesconfig'

        returns a dict with the following members:
         - lines_total: the total number of LOC in the file
         - lines_covered: the number of covered LOC in all configurations
         - lines_conditional_total: the number of LOC in #ifdef blocks
         - lines_conditional_covered: the number of LOC in selected #ifdef blocks
         - linum_map: the map as returned by 'get_block_to_linum_map(filename, True)'
         - blocks_total: contains a set of all blocks in the file
         - configuration_blocks: a set of configuration depentend conditional blocks
                   (i.e., blocks that contain an item starting with 'CONFIG_')
         - blocks: a dict that maps a configuration to the set of
                   conditional blocks that it selects
                   (NB: this contains *all* blocks)
         - blocks_covered: contains the set of all covered blocks over all configurations
         - blocks_uncovered: contains the set of blocks that are not in any configuration
         - all_configs: a set all of all found configurations, including unexpandable ones
         - expandable_configs: the set of configuration that expanded successfully

        ..note: the dict 'blocks' contains the configuration name as
                key. The name has the form 'configN', with N being a
                running number. The length if this dict is equal to the
                number of found confiurations of this file.

        """

        return_dict = {
            'lines': {},
            'blocks': {},
            'blocks_total':
                set(get_conditional_blocks(filename, all_cpp_blocks=True)),
            'configuration_blocks':
                set(get_conditional_blocks(filename, all_cpp_blocks=False)),
            }

        covered_blocks = set()

        if self.options.has_key('configfile'):
            config_h = self.options['configfile']
            logging.info("Analyzing Configuration %s", config_h)
            covered_blocks = return_dict['blocks'][config_h] = \
                set(get_conditional_blocks(filename, config_h, all_cpp_blocks=True))
        else:
            if self.options.has_key('configurations'):
                if not isinstance(self.options["configurations"], set):
                    raise RuntimeError("Internal Error, " + \
                                       "self.options['configuration'] is not a set")
                configs = list()
                return_dict['all_configs'] =  list()
                for config_path in self.options["configurations"]:
                    config_obj = self.make_partial_configuration(config_path)
                    cfgfile = config_obj.kconfig
                    return_dict['all_configs'].append(cfgfile)
                    config_obj.switch_to()
                    if file_in_current_configuration(filename,
                                                     config_obj.arch,
                                                     config_obj.subarch) != "n":
                        logging.info("Configuration '%s' is actually compiled", cfgfile)
                        configs.append(config_obj)
                    else:
                        logging.info("Configuration '%s' is *not* compiled", cfgfile)

            else:
                configs = self.calculate_configurations(filename)
                logging.info("%s: found %d configurations", filename, len(configs))
                return_dict['all_configs'] = find_configurations(filename)

            return_dict['all_config_count'] = len(return_dict['all_configs'])
            return_dict['expandable_configs'] = len(configs)

            for config in configs:
                try:
                    config.switch_to()
                    autoconf_h=config.get_config_h()
                    old_len = len(covered_blocks)
                    return_dict['blocks'][config.kconfig] = \
                        set(get_conditional_blocks(filename, autoconf_h,
                                                   all_cpp_blocks=True))
                    covered_blocks |= return_dict['blocks'][config.kconfig]

                    print "Config %s added %d additional blocks" % \
                        (config.kconfig, len(covered_blocks) - old_len)

                except RuntimeError as e:
                    logging.error("Failed to process config '%s': %s", filename, e)

        return_dict['blocks_covered']   = covered_blocks
        return_dict['blocks_uncovered'] = return_dict['blocks_total'] - covered_blocks

        linum_map = get_block_to_linum_map(filename, all_cpp_blocks=True)
        if len(linum_map) == 0:
            raise EmptyLinumMapException("linum_map contains no elements")
        return_dict['linum_map'] = linum_map

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

    def identifier(self):
        return "bare"

    def make_configuration(self, basename, nth):
        return BareConfiguration(self, basename, nth)

    def make_partial_configuration(self, filename):
        raise RuntimeError("BareBuildFramework does not create partial configurations")

    def file_in_current_configuration(self, filename):
        # pointless for BareBuildFramework
        return "y"

    def verify_configurations(self, filename):
        configs = list()
        for cfgfile in find_configurations(filename):
            # here, we rely on the fact that find_configurations returns filenames such as
            # 'kernel/sched.c.config42' (i.e., filenames end on the number)
            assert '.config' in cfgfile
            basename, ext = os.path.splitext(cfgfile)
            nth = int(ext[len('.config'):])
            configs.append(self.make_configuration(basename, nth))
        return configs


class KbuildBuildFramework(BuildFramework):
    """ For use with Kbuild-like projects, considers Kconfig constraints """

    def __init__(self, options=None):
        if options:
            self.options=options
        else:
            self.options=dict()
        BuildFramework.__init__(self, self.options)
        self.always_on_items = set()
        self.always_off_items = set()
        if self.options.has_key("model") and self.options['model']:
            m = parse_model(self.options['model'])
            self.always_on_items = m.always_on_items
            self.always_off_items = m.always_off_items

    def identifier(self):
        return "kbuild"

    def make_configuration(self, filename, nth):
        raise NotImplementedError

    def make_partial_configuration(self, filename):
        raise NotImplementedError

    def make_std_configuration(self, basename):
        raise NotImplementedError

    def call_makefile(self, target, extra_env="", extra_variables="", failok=False):
        raise NotImplementedError

    def file_in_current_configuration(self, filename):
        return file_in_current_configuration(filename, arch=self.options['arch'])

    def apply_configuration(self):
        """
        Ensures that a newly set configuration is active.
        """
        raise NotImplementedError

    def cleanup_autoconf_h(self):
        """
        deletes various autoconf.h files
        returns a list of deleted files
        """
        (files, _) = execute("find include -name autoconf.h -print -delete",
                             failok=False)
        return files

    def verify_configurations(self, filename):
        logging.info("Testing which configurations are actually being compiled for '%s'",
                     filename)
        configs = list()
        for cfgfile in find_configurations(filename):
            # here, we rely on the fact that find_configurations returns filenames such as
            # 'kernel/sched.c.config42' (i.e., filenames end on the number)
            assert '.config' in cfgfile
            basename, ext = os.path.splitext(cfgfile)
            nth = int(ext[len('.config'):])
            config_obj = self.make_configuration(basename, nth)
            config_obj.switch_to()
            if self.file_in_current_configuration(filename) != 'n':
                logging.info("Configuration '%s' is actually compiled", cfgfile)
                configs.append(config_obj)
            else:
                logging.info("Configuration '%s' is *not* compiled", cfgfile)

        return configs

    def get_stdconfig(self, filename, verify=True):
        """ returns a StdConfiguration object for the given filename.
        If verify is set to true, only return an object if the stdconfig actually builds it.
        returns None otherwise."""

        if not self.options.has_key('stdconfig'):
            logging.info("No reference configuration given")
            return None

        # cache for speedup, filled by Configuration.expand()
        if verify and self.options.has_key('stdconfig_files') and \
                not filename in self.options['stdconfig_files']:
            logging.info("stdconfig '%s' does not build %s (cached)",
                         self.options['stdconfig'], filename)
            return None

        c = self.make_std_configuration(filename)

        if not verify:
            return c

        c.switch_to()
        if self.file_in_current_configuration(filename) != "n":
            return c
        else:
            logging.info("stdconfig '%s' does not build '%s'",
                         self.options['stdconfig'], filename)
            return None

    def analyze_configuration_coverage(self, filename):
        """
        Analyzes the given file 'filename' for its configuration coverage.

        This method also includes statistics about blocks covered by 'allyesconfig'

        Returns a dict with all members that the base class returns,
        plus the following in addition:
         - arch: the analyzed architecture
         - subarch: the analyzed subarchitecture
         - blocks_allyesconfig': All blocks that are selected by allyesconfig

        """

        return_dict = BuildFramework.analyze_configuration_coverage(self, filename)

        return_dict['arch'] = self.options.get('arch', None)
        return_dict['subarch'] = self.options.get('subarch', None)

        # generate the configuration for 'allyesconfig'
        allyesconfig = self.make_std_configuration(filename)
        allyesconfig.switch_to()
        autoconf_h=self.find_autoconf()

        if self.file_in_current_configuration(filename) != 'n':
            return_dict['blocks_allyesconfig'] = set(["B00"]) | \
                set(get_conditional_blocks(filename, autoconf_h, all_cpp_blocks=True))
        else:
            return_dict['blocks_allyesconfig'] = set()

        print "Config %s reaches %d blocks" % \
            (allyesconfig, len(return_dict['blocks_allyesconfig']))

        return return_dict

    def apply_black_white_lists(self, ignoreset):
        """
        This function creates the "allno.config" and "allyes.config" files in
        the root directory of the current tree, which ensure, when calling the
        respective "make all{no, yes}config", the always_off_items to be always
        off and the always_on_items to be always on, even when calling "make
        all{no,yes}config" without using the framework.

        Be careful though, when having an item without prompt in Kconfig, the
        previously described behaviour CANNOT be guaranteed!
        """

        # Sanity check
        if len(self.always_on_items & self.always_off_items) > 0:
            raise RuntimeError("Intersection between always_on_items and \
            always_off_items is non-empty: %s" %(self.always_on_items & \
            self.always_off_items))

        del_cmds = list()
        for item in self.always_on_items | self.always_off_items:
            if any([i in item for i in ignoreset]): continue
            del_cmds.append("/^%s=/d" % item)
        if len(del_cmds) > 0:
            sed_commands = ";".join(del_cmds)
            execute("sed '%s' .config > allno.config" % sed_commands)
            self.call_makefile("allnoconfig")

        del_cmds = list()
        for item in self.always_on_items:
            if any([i in item for i in ignoreset]): continue
            del_cmds.append("/^%s=/d" % item)
        if len(del_cmds) > 0:
            sed_commands = ";".join(del_cmds)
            execute("sed '%s' .config > allyes.config" % sed_commands)
            self.call_makefile("allyesconfig")
        else:
            self.call_makefile("silentoldconfig")


class LinuxBuildFramework(KbuildBuildFramework):
    """ For use with Linux, considers Kconfig constraints """

    def __init__(self, options=None):
        if options:
            self.options=options
        else:
            self.options=dict()
        if not options.has_key('expansion_strategy'):
            self.options['expansion_strategy'] = 'alldefconfig'
        if not options.has_key('arch'):
            self.options['arch'] = vamos.default_architecture
        if not options.has_key('subarch'):
            self.options['subarch'] = guess_subarch_from_arch(self.options['arch'])
        self.options['model'] = get_model_for_arch(self.options['arch'])
        KbuildBuildFramework.__init__(self, self.options)

    def make_configuration(self, basename, nth):
        return LinuxConfiguration(self, basename, nth)

    def make_partial_configuration(self, filename):
        return LinuxPartialConfiguration(self, filename)

    def apply_configuration(self):
        """
        Ensures that a newly set configuration is active.
        """
        try:
            apply_configuration(arch=self.options['arch'],
                                subarch=self.options['subarch'])
        except RuntimeError as e:
            logging.error("Expanding configuration failed: %s", e.message)

    def make_std_configuration(self, basename):
        return LinuxStdConfiguration(self, basename)

    def call_makefile(self, target, extra_env="", extra_variables="", failok=False):
        return call_linux_makefile(target, extra_env=extra_env, extra_variables=extra_variables,
                                   arch=self.options['arch'], subarch=self.options['subarch'],
                                   failok=failok)


class BusyboxBuildFramework(KbuildBuildFramework):
    """ For use with Busybox, considers Kconfig constraints """

    def __init__(self, options=None):
        if options:
            self.options=options
        else:
            self.options=dict()
        if not options.has_key('expansion_strategy'):
            self.options['expansion_strategy'] = 'allyesconfig'
        if self.options['expansion_strategy'] == 'alldefconfig':
            # This is set by default but doesn't work in busybox,
            # therfore let's handle this gracefully
            logging.info("expansion_strategy alldefconfig doesn't exist in busybox, " +\
                             "overriding to allyesconfig")
            self.options['expansion_strategy'] = 'allyesconfig'
        self.options['arch'] = 'busybox'
        self.options['subarch'] = 'busybox'
        self.options['model'] = get_model_for_arch('busybox')
        KbuildBuildFramework.__init__(self, self.options)

    def make_configuration(self, basename, nth):
        return BusyboxConfiguration(self, basename, nth)

    def make_partial_configuration(self, filename):
        return BusyboxPartialConfiguration(self, filename)

    def make_std_configuration(self, basename):
        return BusyboxStdConfiguration(self, basename)

    def apply_configuration(self):
        """
        Ensures that a newly set configuration is active.

        First configure without any set or unset special item. Then, use
        kconfig copy all remaining items to a temporary configuration
        'allno.config'. This file is magically used by kconfig to read
        in a configuration selection but set all not mentioned items in
        there to 'no'. After that, repeat the game again with
        'allyes.config' for all items that should be set to yes.
        """

        # these item prefixes indicate harmless items that shall be
        # skipped to improve performance
        ignoreset = ("CONFIG_CHOICE_", "CONFIG_HAVE_DOT_CONFIG")

        self.apply_black_white_lists(ignoreset)

    def call_makefile(self, target, extra_env="", extra_variables="", failok=False):
        return call_makefile_generic(target, extra_env=extra_env,
                                     extra_variables=extra_variables,
                                     failok=failok)


class CorebootBuildFramework(KbuildBuildFramework):
    """ For use with Coreboot, considers Kconfig constraints """

    def __init__(self, options=None):
        if options:
            self.options=options
        else:
            self.options=dict()
        self.options['arch'] = 'coreboot'

        self.options['model'] = get_model_for_arch('coreboot')
        if not self.options['model']:
            raise RuntimeError('No model existing! Please run \
                    undertaker-kconfigdump first')
        if os.environ.has_key('SUBARCH'):
            self.options['subarch'] = os.environ['SUBARCH']
        else:
            self.options['subarch'] = "emulation/qemu-x86"
        if not options.has_key('expansion_strategy') or \
            self.options['expansion_strategy'] == 'alldefconfig' or \
            self.options['expansion_strategy'] == 'defconfig':
            self.options['expansion_strategy'] = 'allyesconfig'

        KbuildBuildFramework.__init__(self, options)

    def make_configuration(self, basename, nth):
        return CorebootConfiguration(self, basename, nth)

    def make_partial_configuration(self, filename):
        return CorebootPartialConfiguration(self, filename)

    def make_std_configuration(self, basename):
        return CorebootStdConfiguration(self, basename)

    def apply_configuration(self):
        """
        Ensures that a newly set configuration is active.

        """
        logging.debug("Applying config for %s" ,self.options["subarch"])
        coreboot_get_config_for(self.options['subarch'])

        ignoreset = ("CONFIG_CHOICE_",)

        self.apply_black_white_lists(ignoreset)

    def cleanup_autoconf_h(self):
        """
        deletes various autoconf.h files
        returns a list of deleted files
        """
        (files, _) = execute("find build -name config.h -print -delete",
                             failok=False)
        return files

    def find_autoconf(self):
        if os.path.exists("build/config.h"):
            return "build/config.h"
        else:
            return ""

    def call_makefile(self, target, extra_env="", extra_variables="", failok=False):
        return call_makefile_generic(target, extra_env=extra_env,
                                     extra_variables=extra_variables, failok=failok)


def select_framework(identifier, options):
    frameworks = {'kbuild' : LinuxBuildFramework,
                  'linux'  : LinuxBuildFramework,
                  'bare'   : BareBuildFramework,
                  'busybox': BusyboxBuildFramework,
                  'coreboot': CorebootBuildFramework}

    if identifier is None:
        identifier='bare'
        try:
            logging.info("Detected Linux version %s, selecting kbuild framework",
                         get_linux_version())
            identifier='linux'
        except NotALinuxTree:
            pass
        try:
            logging.info("Detected Busybox version %s; selecting busybox framework",
                         get_busybox_version())
            identifier='busybox'
        except NotABusyboxTree:
            pass
        try:
            logging.info("Detected Coreboot version %s; selecting Coreboot framework",
                         get_coreboot_version())
            identifier='coreboot'
        except NotACorebootTree:
            pass

    if not options.has_key('arch') and os.environ.has_key('ARCH'):
        options['arch'] = os.environ['ARCH']

        if os.environ.has_key('SUBARCH'):
            options['subarch'] = os.environ['SUBARCH']
        else:
            options['subarch'] = guess_subarch_from_arch(options['arch'])

    if options.has_key('arch') and (identifier == 'linux' or identifier == 'kbuild'):
        if options['arch'] == "x86_64":
            options['arch'] = 'x86'
            options['subarch'] =  'x86_64'
            logging.info("overriding arch to %s", options['arch'])

        if options['arch'] == 'i386':
            options['arch'] = 'x86'
            options['subarch'] = 'i386'
            logging.info("overriding arch to %s", options['arch'])

    if identifier in frameworks:
        bf = frameworks[identifier](options)
    else:
        raise RuntimeError("Build framework '%s' not found" % options['framework'])

    return bf
