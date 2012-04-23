#
#   utility classes for working in source trees
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

from vamos.vampyr.Messages import SparseMessage, GccMessage, ClangMessage
from vamos.vampyr.utils import ExpansionError, ExpansionSanityCheckError
from vamos.golem.kbuild import guess_arch_from_filename, call_linux_makefile, \
    apply_configuration, find_autoconf, guess_subarch_from_arch, \
    files_for_current_configuration
from vamos.tools import execute
from vamos.model import Model
from vamos.Config import Config

import logging
import os.path
import re
import shutil

class Configuration:
    def __init__(self, framework, basename, nth):
        self.cppflags = '%s.cppflags%s' % (basename, nth)
        self.source   = '%s.source%s' % (basename, nth)
        self.kconfig  = '%s.config%s' % (basename, nth)
        # Note, undertaker will clean up ".config*", which matches self.config_h
        self.config_h = '%s.config%s.h' % (basename, nth)
        self.framework = framework
        self.basename = basename

        with open(self.config_h, 'w') as fd:
            cppflags = self.get_cppflags()
            flags = cppflags.split("-D")
            logging.debug("Generating %s, found %d items", self.config_h, len(flags))
            for f in flags:
                if f == '': continue # skip empty fields
                try:
                    name, value = f.split('=')
                    fd.write("#define %s %s\n" % (name, value))
                except ValueError:
                    logging.error("%s: Failed to parse flag '%s'", self.config_h, f)

    def switch_to(self):
        raise NotImplementedError

    def call_sparse(self, on_file):
        raise NotImplementedError

    def get_cppflags(self):
        with open(self.cppflags, 'r') as fd:
            return fd.read().strip()

    def filename(self):
        return self.kconfig

    def __copy__(self):
        raise RuntimeError("Object <%s> is not copyable" % self)

    def __deepcopy__(self):
        raise RuntimeError("Object <%s> is not copyable" % self)

    def get_config_h(self):
        """
        Returns the path to a config.h like configuration file
        """
        return self.config_h


class BareConfiguration(Configuration):

    def __init__(self, framework, basename, nth):
        Configuration.__init__(self, framework, basename, nth)

    def __repr__(self):
        return '"' + self.get_cppflags() + '"'

    def switch_to(self):
        pass

    def __call_compiler(self, compiler, args, on_file):
        cmd = compiler + " " + args
        if compiler in self.framework.options['args']:
            cmd += " " + self.framework.options['args'][compiler]
        cmd += " " + self.get_cppflags()
        cmd += " '" + on_file + "'"
        (out, returncode) = execute(cmd, failok=True)
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

    def expand(self, verify=True):
        pass


class LinuxConfiguration(Configuration):
    """
    This class represents a (partial) Linux configuration.

    The expand method uses Kconfig to fill up the remaining
    variables. The parameter expansion_strategy of the __init__ method
    selects how partial configurations get expanded. In the default
    mode, 'alldefconfig', the strategy is to set the remaining variables
    to their Kconfig defined defaults. With 'allnoconfig', the strategy
    is to enable as few features as possible.

    The attribute 'model' of this class is allocated in the expand()
    method on demand.

    """
    def __init__(self, framework, basename, nth,
                 arch=None, subarch=None,
                 expansion_strategy='alldefconfig'):
        Configuration.__init__(self, framework, basename, nth)

        self.expanded = None
        self.expansion_strategy = expansion_strategy
        self.model = None
        try:
            os.unlink(self.kconfig + '.expanded')
        except OSError:
            pass

        if arch:
            self.arch = arch

            if subarch:
                self.subarch = subarch
            else:
                self.subarch = guess_subarch_from_arch(arch)
        else:
            oldsubarch = subarch
            self.arch, self.subarch = guess_arch_from_filename(self.kconfig)
            if oldsubarch:
                self.subarch = oldsubarch

        self.result_cache = {}

    def get_config_h(self):
        return find_autoconf()

    def __repr__(self):
        return '<LinuxConfiguration "' + self.kconfig + '">'

    def expand(self, verify=False):
        """
        @raises ExpansionError if verify=True and expanded config does
                               not patch all requested items
        """
        logging.debug("Trying to expand configuration " + self.kconfig)

        if not os.path.exists(self.kconfig):
            raise RuntimeError("Partial configuration %s does not exist" % self.kconfig)

        (files, _) = execute("find include -name autoconf.h -print -delete", failok=False)
        if len(files) > 1:
            logging.error("Deleted spurious configuration files: %s", ", ".join(files))

        target = self.expansion_strategy
        extra_var = 'KCONFIG_ALLCONFIG="%s"' % self.kconfig
        call_linux_makefile(target, extra_variables=extra_var,
                            arch=self.arch, subarch=self.subarch,
                            failok=False)
        apply_configuration(arch=self.arch, subarch=self.subarch)

        expanded_config = self.kconfig + '.expanded'
        shutil.copy('.config', expanded_config)
        self.expanded = expanded_config

        if verify:
            if not self.model:
                modelf = "models/%s.model" % self.arch
                self.model = Model(modelf)
                logging.info("Loaded %d items from %s", len(self.model), modelf)

            all_items, violators = self.verify(expanded_config)
            if len(violators) > 0:
                for v in violators:
                    logging.debug("violating item: %s", v)
                logging.warning("%d/%d mismatched items", len(violators), len(all_items))
                raise ExpansionError("Config %s failed to expand" % self.kconfig)


    def verify(self, expanded_config='.config'):
        """
        verifies that the given expanded configuration satisfies the
        constraints of the given partial configuration.

        @return (all_items, violators)
          all_items: set of all items in partial configuration
          violators: list of items that violate the partial selection
        """

        partial_config = Config(self.kconfig)
        config = Config(expanded_config)
        conflicts = config.getConflicts(partial_config)

        return (partial_config.keys(), conflicts)


    def switch_to(self):
        if not self.expanded:
            logging.debug("Expanding partial configuration %s", self.kconfig)
            self.expand()
            return

        # now replace the old .config with our 'expanded' one
        shutil.copyfile(self.expanded, '.config')

        call_linux_makefile('oldconfig', failok=False,
                            arch=self.arch, subarch=self.subarch)


        (_, returncode) = call_linux_makefile('silentoldconfig', failok=True,
                                              arch=self.arch, subarch=self.subarch)
        if returncode != 0:
            # with the satconfig approach, this always worked
            m = "silentoldconfig failed while switching to config %s (rc: %d)" \
                % (self.expanded, returncode)
            raise ExpansionSanityCheckError(m)


    def __call_make(self, on_file, extra_args):
        on_object = on_file[:-1] + "o"
        try:
            os.unlink(on_object)
        except OSError:
            pass

        (messages, statuscode) = \
            call_linux_makefile(on_object, failok=True, arch=self.arch, subarch=self.subarch,
                                extra_variables=extra_args)

        state = None
        CC = []
        CHECK = []

        while len(messages) > 0:
            if re.match("^\s*CC\s*(\[M\]\s*)?" + on_object, messages[0]):
                state = "CC"
                del messages[0]
                continue
            if re.match("^\s*CHECK\s*(\[M\]\s*)?" + on_file, messages[0]):
                state = "CHECK"
                del messages[0]
                continue
            if re.match("fixdep: [\S]* is empty", messages[0]):
                del messages[0]
                continue

            # Skip lines before "    CC"
            if state == None:
                pass
            elif state == "CC":
                CC.append(messages[0])
            elif state == "CHECK":
                CHECK.append(messages[0])
            else:
                raise RuntimeError("Should never been reached")

            # Remove line
            del messages[0]

        if statuscode != 0:
            logging.error("Running checker %s on file %s failed", extra_args, on_file)

        logging.debug("contents of CC:")
        logging.debug(CC)
        logging.debug("contents of CHECK:")
        logging.debug(CHECK)
        return (CC, CHECK)


    def call_gcc(self, on_file):
        """Call Gcc on the given file"""
        if "CC" in self.result_cache:
            return self.result_cache["CC"]

        extra_args = "KCFLAGS='%s'" % self.framework.options['args']['gcc']
        (CC, _) = self.__call_make(on_file, extra_args)

        messages = GccMessage.preprocess_messages(CC)
        messages = [GccMessage(self, x) for x in messages]
        self.result_cache["CC"] = messages
        return messages


    def call_sparse(self, on_file):
        """Call Sparse on the given file"""
        if "SPARSE" in self.result_cache:
            return self.result_cache["SPARSE"]

        sparse = "ulimit -t 30; sparse"
        if 'sparse' in self.framework.options['args']:
            sparse += " " + self.framework.options['args']['sparse']

        (CC, CHECK) = self.__call_make(on_file, "C=2 CC='fakecc' CHECK='%s'" % sparse.replace("'", "\\'"))

        # GCC messages
        messages = GccMessage.preprocess_messages(CC)
        messages = map(lambda x: GccMessage(self, x), messages)
        self.result_cache["CC"] = messages

        # Sparse messages
        messages = SparseMessage.preprocess_messages(CHECK)
        messages = map(lambda x: SparseMessage(self, x), messages)
        self.result_cache["SPARSE"] = messages

        return messages


class LinuxStdConfiguration(LinuxConfiguration):
    """
    This option creates a configuration object for a standard Linux
    configuration, such as 'allyesconfig' or 'allnoconfig'.

    Instantiating this class will not change the current working tree,
    immediately.
    """

    def __init__(self, framework, basename, arch, subarch):
        assert framework.options.has_key('stdconfig')
        assert arch
        assert subarch
        logging.debug("arch: %s, subarch: %s", arch, subarch)
        configuration = ".%s" % framework.options['stdconfig']
        LinuxConfiguration.__init__(self, framework,
                                    arch=arch, subarch=subarch,
                                    basename=basename, nth=configuration)

        self.cppflags = '/dev/null'
        self.source   = '/dev/null'
        self.kconfig  = '/dev/null'

    def expand(self, verify=False):

        arch    = self.framework.options['arch']
        subarch = self.framework.options['subarch']

        call_linux_makefile(self.framework.options['stdconfig'],
                            arch=arch, subarch=subarch,
                            failok=False)

        if not self.framework.options.has_key('stdconfig_files'):
            self.framework.options['stdconfig_files'] \
                = set(files_for_current_configuration(arch, subarch))
        else:
            apply_configuration(arch=self.arch, subarch=self.subarch)
        self.kconfig = '.vampyr.' + self.framework.options['stdconfig']
        shutil.copy('.config', self.kconfig)

    def get_cppflags(self):
        return ""

    def filename(self):
        return self.basename + '.' + self.framework.options['stdconfig']
