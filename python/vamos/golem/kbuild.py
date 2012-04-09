#
#   golem - analyzes feature dependencies in Linux makefiles
#
# Copyright (C) 2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
# Copyright (C) 2012 Andreas Ruprecht <rupran@einserver.de>
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

from vamos.Config import Config
from vamos.tools import execute, CommandFailed, execute

from tempfile import mkstemp
from glob import glob

import logging
import os
import re
import sys

import vamos

class TreeNotConfigured(RuntimeError):
    """ Indicates that this Linux tree is not configured yet """
    pass


class NotALinuxTree(RuntimeError):
    """ Indicates we are not in a Linux tree """
    pass


def find_autoconf():
    """ returns the path to the autoconf.h file in this linux tree
    """

    if vamos.golem.autoconf_h:
        return vamos.golem.autoconf_h

    (autoconf, _) = execute("find include -name autoconf.h", failok=False)
    autoconf = filter(lambda x: len(x) > 0, autoconf)
    if len(autoconf) != 1:
        logging.error("Found %d autoconf.h files (%s)",
                      len(autoconf), ", ".join(autoconf))
        raise RuntimeError("Not exactly one autoconf.h was found")
    vamos.golem.autoconf_h = autoconf[0]
    return vamos.golem.autoconf_h


def apply_configuration(arch=None, subarch=None, filename=None):
    """
    Applies the current configuration

    This method updates 'include/config/auto.conf' and
    'include/generated/autoconf.h' to match the current configuration.
    Expects a complete configuration in '.config'. If it does not exist,
    the standard configuration 'allyesconfig' is configured.

    If not applying for the default architecture 'x86', the optional
    parameters "arch" (and possibly "subarch") need to be specified.

    If an optional filename is passed, then architecture and subarch are
    guessed using the guess_arch_from_filename() function. Overriding
    either arch or subarch remains possible by explicitly setting arch
    or subarch.
    """

    if filename:
        (guessed_arch, guessed_subarch) = guess_arch_from_filename(filename)
        if not arch:
            arch = guessed_arch
        if not subarch:
            subarch = guessed_subarch

    if not arch:
        logging.warning("No architecture selected. Defaulting to x86")
        arch = 'x86'

    if not os.path.exists(".config"):
        call_linux_makefile("allyesconfig", arch=arch, subarch=subarch)

    # this catches unset defaults. Since boolean and tristate have
    # implicit defaults, this can effectively only happen for integer
    # and hex items. Both are fine with a setting of '0'
    execute('sed -i s,=$,=0,g .config', failok=False)
    try:
        call_linux_makefile('silentoldconfig', arch=arch, subarch=subarch,
                            failok=False)
    except CommandFailed as e:
        if e.returncode == 2:
            raise TreeNotConfigured("target 'silentoldconfig' failed")
        else:
            raise

def guess_source_for_target(target):
    """
    for the given target, try to determine its source file.

    return None if no source file could be found
    """
    for suffix in ('.c', '.S', '.s', '.l', '.y', '.ppm'):
        sourcefile = target[:-2] + suffix
        if os.path.exists(sourcefile):
            return sourcefile
    return None


def files_for_current_configuration(arch=None, subarch=None, how=False):
    """
    to be run in a Linux source tree.

    Returns a list of files that are compiled with the current
    configuration.

    The parameter 'how', which defaults to False, decides on the mode of
    operation. If it is set to False, this function guesses for each
    found file the corresponding source file. A warning is issued if the
    source file could not be guessed and the file is ignored. If it is
    set to True, the function returns a list of object files and
    additionally indicates in what way (i.e., module or statically
    built-in) the object file is compiled.

    NB: If not working for the default architecture 'x86', the optional
    parameters "arch" (and possibly "subarch") need to be specified!
    """

    # locate directory for supplemental makefiles
    scriptsdir = find_scripts_basedir()
    assert os.path.exists(os.path.join(scriptsdir, 'Makefile.list_recursion'))

    apply_configuration(arch=arch, subarch=subarch)

    make_args="-f %(basedir)s/Makefile.list UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }

    (output, _) = call_linux_makefile('list',
                                      arch=arch,
                                      subarch=subarch,
                                      failok=False,
                                      extra_variables=make_args)

    files = set()
    for line in output:
        # these lines indicate error and warning messages
        if '***' in line: continue
        if len(line) == 0:
            continue
        try:
            if (how):
                files.add(line)
            else:
                objfile = line.split()[0]
                # try to guess the source filename
                sourcefile = guess_source_for_target(objfile)
                if not sourcefile:
                    logging.debug("Failed to guess source file for %s",
                                  objfile)
                else:
                    files.add(sourcefile)
        except IndexError:
            raise RuntimeError("Failed to parse line '%s'" % line)

    return files


def file_in_current_configuration(filename, arch=None, subarch=None):
    """
    to be run in a Linux source tree.

    Returns the mode (as a string) the file is compiled in the current
    configuration:

        "y" - statically compiled
        "m" - compiled as module
        "n" - not compiled into the kernel

    NB: this function expects the current configuration to be already
        applied with the apply_configuration() function

    """

    # locate directory for supplemental makefiles
    scriptsdir = find_scripts_basedir()
    assert(os.path.exists(os.path.join(scriptsdir, 'Makefile.list_recursion')))

    # normalize filename
    filename = os.path.normpath(filename)

    basename = filename.rsplit(".", 1)[0]
    logging.debug("checking file %s", basename)

    make_args = "-f %(basedir)s/Makefile.list UNDERTAKER_SCRIPTS=%(basedir)s compiled='%(filename)s'" % \
        { 'basedir' : scriptsdir,
          'filename': filename.replace("'", "\'")}

    (make_result, _) = call_linux_makefile('list',
                                           filename=filename,
                                           arch=arch,
                                           subarch=subarch,
                                           failok=False,
                                           extra_variables=make_args)

    for line in make_result:
        # these lines indicate error and warning messages
        if '***' in line: continue
        if line == '': continue
        if line.startswith(basename):
            try:
                return os.path.normpath(line.split(" ")[1])
            except IndexError:
                logging.error("Failed to parse line: '%s'",  line)
                return "n"

    return "n"


def determine_buildsystem_variables(arch=None):
    """
    returns a list of kconfig variables that are mentioned in Linux Makefiles
    """
    if arch:
        cmd = r"find . \( -name Kbuild -o -name Makefile \) " + \
              r"\( ! -path './arch/*' -o -path './arch/%(arch)s/*' \) " + \
              r"-exec sed -n '/CONFIG_/p' {} \+"
        cmd = cmd % {'arch': arch}
    else:
        cmd = r"find . \( -name Kbuild -o -name Makefile \) -exec sed -n '/CONFIG_/p' {} \+"
    find_result = execute(cmd, failok=False)

    ret = set()
    # line might look like this:
    # obj-$(CONFIG_MODULES)           += microblaze_ksyms.o module.o
    for line in find_result[0]:
        for m in re.finditer(r'CONFIG_(?P<feature>[A-Za-z0-9_]+)', line):
            config_variable = m.group('feature')
            ret.add(config_variable)
    return ret


def determine_buildsystem_variables_in_directory(directory):
    filenames = []
    kbuild = os.path.join(directory, "Kbuild")
    makefile = os.path.join(directory, "Makefile")

    if os.path.exists(kbuild):
        filenames.append(kbuild)

    if os.path.exists(makefile):
        filenames.append(makefile)

    if os.path.exists(kbuild + ".platforms"):
        filenames.append(kbuild + ".platforms")
        filenames += glob(directory + "/*/Platform")

    ret = set()

    for filename in filenames:
        with open(filename) as fd:
            for line in fd:
                for m in re.finditer(r'CONFIG_(?P<feature>[A-Za-z0-9_]+)', line):
                    config_variable = m.group('feature')
                    ret.add("CONFIG_" + config_variable)

    return ret


def guess_subarch_from_arch(arch):
    """
    For the given architecture, try to guess the best matching subarchitecture.

    This choice is not so obvious for architectures like 'x86', where valid
    values include 'x86_64' for 64bit CPUs and 'i386' for 32bit CPUs.
    """

    subarch = arch
    if arch == 'x86' or arch == 'um':
        if vamos.prefer_32bit:
            subarch = 'i386'
        else:
            subarch = 'x86_64'
    else:
        assert(arch==subarch)

    return subarch


def guess_arch_from_filename(filename):
    """
    Guesses the 'best' architecture for the given filename.

    If the file is in the Linux HAL (e.g., 'arch/arm/init.c', then the
    architecture is deduced from the filename

    Defaults to 'vamos.default_architecture' (default: 'x86')

    returns a tuple (arch, subarch) with the architecture identifier (as
    in the subdirectory part) and the preferred "subarchitecture". The
    latter is used e.g. to disable CONFIG_64BIT on 'make allnoconfig'
    when compiling on a 64bit host (default behavior), unless
    vamos.prefer_32bit is set to False.
    """

    m = re.search("^arch/([^/]*)/", os.path.normpath(filename))
    if m:
        arch = m.group(1)
    else:
        arch = vamos.default_architecture

    subarch = guess_subarch_from_arch(arch)

    forced_64bit = False
    forced_32bit = False

    if arch =='x86' and '.config' in filename:
        c = Config(filename)
        if c.valueOf('CONFIG_X86_64') == 'y' or c.valueOf('CONFIG_64BIT') == 'y':
            subarch = 'x86_64'
            logging.debug("Config %s forces subarch x86_64", filename)
            forced_64bit = True

        if c.valueOf('CONFIG_X86_64') == 'n' or c.valueOf('CONFIG_64BIT') == 'n' \
                or c.valueOf('CONFIG_X86_32') == 'y':
            subarch = 'i386'
            logging.debug("Config %s forces subarch i386", filename)
            forced_32bit = True

    if forced_32bit and forced_64bit:
        logging.error("%s is inconsistent: cannot enable 64bit and 32bit together (using %s)",
                      filename, subarch)

    return (arch, subarch)


def call_linux_makefile(target, extra_env="", extra_variables="",
                        filename=None, arch=None, subarch=None,
                        failok=True, dryrun=False):
    """
    Invokes 'make' in a Linux Buildtree.

    This utility function hides details how to set make and environment
    variables that influence kbuild. An important variable is 'ARCH'
    (and possibly later 'SUBARCH'), which can be via the corresponding
    variables.

    If a target points to an existing file (or the optional target
    filename is given)the environment variable for ARCH is derived
    according to the follwing rules:

      - if the file is inside an "arch/$ARCHNAME/", use $ARCHNAME
      - if the "arch" variable is set, use that
      - by default use 'default_arch'
      - if the arch is set to 'x86', set ARCH to 'i386', unless the
        "prefer_64bit" parameter is set to 'True'

    If dryrun is True, then the command line is returned instead of the
    command's execution output. This is mainly useful for testing.

    returns a tuple with
     1. the command's standard output as list of lines
     2. the exitcode
    """

    cmd = "make"
    njobs = int(os.sysconf('SC_NPROCESSORS_ONLN') * 1.20 + 0.5)

    if extra_env and "ARCH=" in extra_env or extra_variables and "ARCH=" in extra_variables:
        logging.debug("Detected manual (SUB)ARCH override in extra arguments '(%s, %s)'",
                      extra_env, extra_variables)
    else:
        if os.path.exists(target):
            filename = target
        if filename:
            (guessed_arch, guessed_subarch) = guess_arch_from_filename(filename)

            if not arch:
                arch = guessed_arch

            if not subarch:
                subarch = guessed_subarch

    if not arch:
        (arch, subarch) = guess_arch_from_filename('Makefile')

    if not subarch:
        subarch = arch

    extra_env += " ARCH=%s" % arch
    extra_env += " SUBARCH=%s" % subarch

    if not 'KERNELVERSION=' in extra_variables:
        if not vamos.kernelversion:
            (output, rc) = execute("git describe", failok=True)
            if rc == 0:
                vamos.kernelversion = output[-1]
        if vamos.kernelversion:
            extra_env += ' KERNELVERSION="%s"' % vamos.kernelversion

    cmd = "env %(extra_env)s make -j%(njobs)s %(target)s %(extra_variables)s " % \
        { 'njobs': njobs,
          'extra_env': extra_env,
          'target': target,
          'extra_variables': extra_variables }

    if dryrun:
        return (cmd, 0)
    else:
        return execute(cmd, failok=failok)


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

    if os.path.isdir('.git'):
        cmd = "git describe"
        (output, ret) = execute(cmd)
        git_version = output[0]
        if (ret > 0):
            git_version = ""
            logging.info("Execution of '%s' command failed, analyzing the Makefile instead", cmd)

        # 'standard' Linux repository descriptions start with v
        if git_version.startswith(("v3.", "v2.6")):
            return git_version

    extra_vars = "-f %(basedir)s/Makefile.version UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }

    (output, ret) = call_linux_makefile('', extra_variables=extra_vars)
    if ret > 0:
        raise NotALinuxTree("The call to Makefile.version failed")

    version = output[-1] # use last line, if not configured we get additional warning messages
    if not version.startswith(("3.", "2.6")):
        raise NotALinuxTree("Only Linux versions 2.6 and 3.x are supported")
    else:
        return version


def find_scripts_basedir():
    executable = os.path.realpath(sys.argv[0])
    base_dir   = os.path.dirname(executable)
    for d in [ '../lib', '../scripts']:
        f = os.path.join(base_dir, d, 'Makefile.list')
        if os.path.exists(f):
            return os.path.realpath(os.path.join(base_dir, d))
    raise RuntimeError("Failed to locate Makefile.list")


def files_for_selected_features(features, arch, subarch):
    """
    to be run in a Linux source tree.

    The parameter features represents a (partial) Kconfig selection in
    the form of a dict from feature -> value e.g: {'CONFIG_X86': 'y',
    'CONFIG_BARFOO': 'm'}.

    @return a tuple of (files, dirs). The first member 'files' is a list
    of files that are compiled with the selected features
    configuration. The second member 'dirs' is a list if directories
    that are traversed thereby.
    """

    # locate directory for supplemental makefiles
    scriptsdir = find_scripts_basedir()
    assert(os.path.exists(os.path.join(scriptsdir, 'Makefile.list_recursion')))

    (fd, tempfile) = mkstemp()
    with os.fdopen(fd, "w+") as fd:
        logging.info("dumping partial configuration with %d items to %s", len(features.items()), tempfile)
        for (key, value) in features.items():
            fd.write("%s=%s\n" % (key, value))
            logging.info("%s=%s", key, value)

    make_args= "-f %(basedir)s/Makefile.list UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }
    make_args += " print_dirs=y print_files=y auto_conf=%(tempfile)s" % \
           {'tempfile': tempfile }

    try:
        (make_result, _) = call_linux_makefile('list',
                                               arch=arch,
                                               subarch=subarch,
                                               failok=False,
                                               extra_variables=make_args)
    except:
        os.unlink(tempfile)
        raise

    files = set()
    dirs = set()
    for line in make_result:
        # these lines indicate error and warning messages
        if '***' in line: continue
        l = line.split()
        try:
            if line.endswith("/"):
                dirs.add(os.path.normpath(line))
            else:
                files.add(os.path.normpath(l[0]))
        except IndexError:
            raise RuntimeError("Failed to parse line '%s'" % line)

    os.unlink(tempfile)

    return (files, dirs)
