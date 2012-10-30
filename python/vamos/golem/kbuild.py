#
#   golem - analyzes feature dependencies in Linux makefiles
#
# Copyright (C) 2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
# Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
# Copyright (C) 2012 Andreas Ruprecht <rupran@einserver.de>
# Copyright (C) 2012 Stefan Hengelein <stefan.hengelein@informatik.stud.uni-erlangen.de>
# Copyright (C) 2012 Manuel Zerpies <manuel.f.zerpies@ww.stud.uni-erlangen.de>
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
from vamos.tools import execute, CommandFailed

from tempfile import mkstemp
from glob import glob

import logging
import os
import re
import shutil
import sys

import vamos

# matches lines like:
# obj-$(CONFIG_MODULES)           += microblaze_ksyms.o module.o
# must not match lines like:
# foo_CFLAGS += -DCONFIG=foo.h

BUILDSYSTEM_VARIABLE_REGEX = \
    r'(^CONFIG_|[^A-Za-z0-9_]CONFIG_)(?P<feature>[A-Za-z0-9_]+)'


class TreeNotConfigured(RuntimeError):
    """ Indicates that this Linux tree is not configured yet """
    pass


class NotALinuxTree(RuntimeError):
    """ Indicates we are not in a Linux tree """
    pass


class NotABusyboxTree(RuntimeError):
    """ Indicates we are not in a Busybox tree """
    pass

class NotACorebootTree(RuntimeError):
    """ Indicates we are not in a Coreboot tree """
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

def linux_files_for_current_configuration(arch=None, subarch=None, how=False):
    """
    Returns a list of files that are compiled with the current
    configuration.

    to be run in a Linux source tree.

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

def coreboot_get_config_for(subarch=None):
    if subarch != None:
        subarch_regex = re.compile("([a-zA-Z0-9-]+)/([a-zA-Z_0-9_-]+)")
        m             = subarch_regex.match(subarch)

        if m:
            vendor    = m.group(1)
            mainboard = m.group(2)
            logging.debug("Using Vendor '%s', Mainboard '%s'", vendor, mainboard)

            cmd = './util/abuild/abuild -B -C -t %s/%s' % (vendor, mainboard)
            if not os.path.isdir('./coreboot-builds/%s_%s' % (vendor, mainboard)):
                execute(cmd, failok=True)

            if not os.path.isdir('./coreboot-builds/%s_%s' % (vendor, mainboard)):
                raise RuntimeError('%s failed. ' % cmd + \
                    'Maybe Vendor and/or Mainboard does not exist?')
        else:
            raise RuntimeError('SUBARCH (%s) given but has invalid syntax, \
                use "Vendor/Mainboard" instead' % subarch)

        shutil.copy('./coreboot-builds/%s_%s/config.build' % (vendor, mainboard),
                    '.config')

def coreboot_files_for_current_configuration(subarch=None):
    """
    Returns a list of files that are compiled with the current
    configuration.

    Unlike in the Linux case, the parameter 'subarch' specifies the
    selected vendor and mainboard in the form "VENDOR/MAINBOARD".

    If there is no current configuration, (i.e., '.config' does not
    exist), then qemu-x86 is selected as side effect.

    To be run in a coreboot source tree.
    """

    if subarch == None and not os.path.exists('.config'):
        logging.warning('.config does not exist, defaulting to qemu-x86')
        subarch = "emulation/qemu-x86"

    coreboot_get_config_for(subarch)
    (output, _) = call_makefile_generic('printall', failok=False)

    files = set()
    for line in output:
        if line.startswith("allsrcs="):
            for f in line[len('allsrcs=')+1:].split(): # skip first item
                files.add(f)
    return files

def files_for_current_configuration(arch, subarch, how=False):
    """
    Returns a list of files that are compiled with the current
    configuration.

    The parameter 'how', which defaults to False, decides on the mode of
    operation. If it is set to False, this function guesses for each
    found file the corresponding source file. A warning is issued if the
    source file could not be guessed and the file is ignored. If it is
    set to True, the function returns a list of object files and
    additionally indicates in what way (i.e., module or statically
    built-in) the object file is compiled.

    Distinguishes if current tree is linux or coreboot and then
    returns a list of files that are compiled with the current
    configuration.
    """
    if arch == 'coreboot':
        return coreboot_files_for_current_configuration(subarch)
    else:
        return linux_files_for_current_configuration(arch, subarch, how)


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

    if arch == 'busybox':
        (make_result, _) = call_makefile_generic('list',
                                                 failok=False,
                                                 extra_variables=make_args)
    elif arch == 'coreboot':
        make_result = coreboot_files_for_current_configuration(subarch)
        for line in make_result:
            if filename in line:
                return "y"
        return "n"
    # fallback to Linux
    else:
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
    for line in find_result[0]:
        for m in re.finditer(BUILDSYSTEM_VARIABLE_REGEX, line):
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
                for m in re.finditer(BUILDSYSTEM_VARIABLE_REGEX, line):
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


def call_makefile_generic(target, failok=False, dryrun=False, **kwargs):
    """
    Invokes 'make'.

    This variant is intended to work in a generic way. For project
    specific adaptations, a wrapper function may call this.

    If dryrun is True, then the command line is returned instead of the
    command's execution output. This is mainly useful for testing.

    returns a tuple with
     1. the command's standard output as list of lines
     2. the exitcode
    """

    njobs = kwargs.get('njobs', None)

    if njobs is None:
        njobs = int(os.sysconf('SC_NPROCESSORS_ONLN') * 1.20 + 0.5)

    cmd = "env %(extra_env)s make -j%(njobs)s %(target)s %(extra_variables)s " %\
        {
        'target': target,
        'njobs': njobs,
        'extra_env': kwargs.get('extra_env', ""),
        'extra_variables': kwargs.get('extra_variables', ""),
        }

    if dryrun:
        return (cmd, 0)
    else:
        return execute(cmd, failok=failok)


def call_linux_makefile(target, extra_env="", extra_variables="",
                        filename=None, arch=None, subarch=None,
                        failok=True, dryrun=False, njobs=None):
    # pylint: disable=R0912
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

    if arch == 'x86':
        # x86 is special - set default first
        if vamos.prefer_32bit:
            variant = 'i386'
        else:
            variant = 'x86_64'
        # do we need to override manually?
        if subarch == 'i386':   variant = 'i386'
        if subarch == 'x86_64': variant = 'x86_64'
        extra_env += " ARCH=%s" % variant
    else:
        extra_env += " ARCH=%s" % arch
    extra_env += " SUBARCH=%s" % subarch

    if not 'KERNELVERSION=' in extra_variables:
        if not vamos.kernelversion:
            (output, rc) = execute("git describe", failok=True)
            if rc == 0:
                vamos.kernelversion = output[-1]
        if vamos.kernelversion:
            extra_env += ' KERNELVERSION="%s"' % vamos.kernelversion

    return call_makefile_generic(target, failok=failok, njobs=njobs,
                                 dryrun=dryrun,
                                 extra_env=extra_env,
                                 extra_variables=extra_variables)


def get_linux_version():
    """
    Check that the current working directory is actually a Linux tree

    If we are in a git tree, return that kernel version. Otherwise,
    use a custom Makefile to retrieve the current kernel version.

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
            logging.debug("Execution of '%s' command failed, analyzing the Makefile instead",
                          cmd)

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
        raise NotALinuxTree("Only versions 2.6 and 3.x are supported, but not %s",
                            version)
    else:
        return version


def get_busybox_version():
    """
    Check that the current working directory is actually a Busybox tree

    If we are in a git tree, return that kernel version. Otherwise,
    use a custom Makefile to retrieve the current kernel version.

    Raises a 'NotABusyboxTree' exception if the version could not be retrieved.
    """

    scriptsdir = find_scripts_basedir()

    if not os.path.exists('Makefile'):
        raise NotABusyboxTree("No 'Makefile' found")

    if os.path.isdir('.git'):
        cmd = "git describe"
        (output, ret) = execute(cmd)
        git_version = output[0]
        if (ret > 0):
            git_version = ""
            logging.debug("Execution of '%s' command failed, analyzing the Makefile instead",
                          cmd)

        # 'standard' Busybox repository descriptions start with 1_
        if git_version.startswith("1_"):
            return git_version
        else:
            raise NotABusyboxTree("Git does not indicate a supported busybox version")

    extra_vars = "-f %(basedir)s/Makefile.version UNDERTAKER_SCRIPTS=%(basedir)s" % \
        { 'basedir' : scriptsdir }

    (output, ret) = call_makefile_generic('', extra_variables=extra_vars)
    if ret > 0:
        raise NotABusyboxTree("The call to Makefile.version failed")

    version = output[-1] # use last line, if not configured we get additional warning messages
    if not version.startswith("1."):
        raise NotABusyboxTree("Only 1.x versions are supported, but not %s",
                              version)
    else:
        return version


def get_coreboot_version():
    """
    Check that the current working directory is actually a Coreboot tree.

    If we are in a git tree or a tarball with build/autoconf.h, return that bios
    version if it starts with 4., else raise a NotACorebootTree-Exception
    or return "coreboot-UNKNOWN" if in a bare tarball.
    """

    if not os.path.exists('Makefile') and not os.path.exists('Makefile.inc') \
            and not os.path.exists('src/Kconfig'):
        raise NotACorebootTree("No 'Makefile', 'Makefile.inc' or 'src/Kconfig' found")

    if os.path.isdir('.git'):
        cmd = "git describe"
        (output, ret) = execute(cmd)
        git_version = output[0]
        if (ret > 0):
            git_version = ""
            logging.debug("Execution of '%s' command failed, analyzing the Makefile instead",
                          cmd)
        # 'standard' Coreboot repository descriptions start with 4.
        if git_version.startswith("4."):
            return git_version
        raise NotACorebootTree("Only 4.x versions are supported, but not %s",
                              git_version)

    if os.path.exists('build/autoconf.h'):
        regx = re.compile(" \* coreboot version: ([a-zA-Z_0-9_.-]+)")
        with open('build/autoconf.h') as conf:
            for line in conf:
                if regx.match(line):
                    m = regx.match(line)
                    version = m.group(1)
                    break

        # 'standard' Coreboot repository descriptions start with 4.
        if version.startswith("4."):
            return version
        raise NotACorebootTree("Only 4.x versions are supported, but not %s",
                              version)

    # at this stage, we are "sure" to have a coreboot tree, since Makefile and
    # Makefile.inc and src/Kconfig files exist, but no valid version is known
    return "coreboot-UNKNOWN"

def find_scripts_basedir():
    executable = os.path.realpath(sys.argv[0])
    base_dir   = os.path.dirname(executable)
    for d in [ '../lib', '../scripts', '../../scripts', '../../../scripts']:
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
                                               extra_variables=make_args,
                                               njobs=1)
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
