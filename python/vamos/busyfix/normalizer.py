#!/usr/bin/env python
#
# busyfix - normalize the '#if ENABLE', 'IF' and 'IF_NOT' statements
#
# Copyright (C) 2012 Manuel Zerpies <manuel.f.zerpies@ww.stud.uni-erlangen.de>
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

import re
import logging

# global variable for usage in normalize_IF only
# count open brackets
open_brackets = 0

def normalize_ENABLE_inline(line):
    """replaces ENABLE lines not in macros
        "if (ENABLE_FEATURE_FANCY) {
            // do something
        }"
        is replaced by
        "if (
        #if defined CONFIG_FEATURE_FANCY
        1
        #else
        0
        #endif
        ) {
            // do something
        }"
    """
    m = re.search(r"^#\s*if", line)
    m2= re.search(r"^#\s*ifdef", line)
    if m or m2:
        logging.debug("not normalizing line %s - found #if ", line)
        return line

    if "_ENABLE_" in line:
        logging.debug("the only case in busybox where '_ENABLE_' is found - returning")
        return line

    m = re.search(r"ENABLE_([A-Za-z0-9_]+)", line)
    m2 = re.search(r"(?<=_)ENABLE_([A-Za-z0-9_]+)", line)
    if m and not m2:
        logging.debug("normalizing line %s - found ENABLE_*", line)
        l = line[:m.start(0)] + \
            "\n#if defined CONFIG_" + line[m.start(1):m.end(1)]
        l += "\n1\n#else\n0\n#endif\n"
        l += line[m.end(1):]
        return l
    else:
        return line

def normalize_defined_ENABLE_macro(line):
    """replaces '#if defined ENABLE_FOO' -> '#if defined CONFIG_FOO'
       #if defined(CONFIG_FOO)' doesn't actually happen in busybox at the moment
       by codingstyle guidelines"""
    m = re.search(r"defined ENABLE_", line)
    if m:
        logging.debug("normalizing line %s - found defined ENABLE_*", line)
        return line.replace('defined ENABLE_', 'defined CONFIG_')
    else:
        return line

def normalize_ENABLE_macro(line):
    """replaces '#if ENABLE_FOO' -> '#if defined CONFIG_FOO' """
    m = re.match(r"^#\s*if[\s()!]+ENABLE_([^\s])+", line)
    m2 = re.match(r"^#\s*elif[\s()!]+ENABLE_([^\s])+", line)
    m3 = re.search(r"[A-Z]+(_ENABLE_)[A-Z0-9^\s\|&_]+", line)
    m4 = re.search(r"^\s*if[\s{}]+[A-Z0-9_]+", line)
    if m or m2:
        logging.debug("we are in a #(el)if ENABLE_* line! - %s", line)
        if m4:
            return line
        if m3:
            return line.replace('ENABLE_', 'defined CONFIG_', 1)
        else:
            return line.replace('ENABLE_', 'defined CONFIG_')
    else:
        return line

def normalize_IF(line):
    """normalizes every possible 'IF_...' expression by a equal '#if defined
    ...' expression
    for example:
    'long IF_NOT_LONG_ENOUGH(long int) beta = 0;'
    is replaced by
    'long 
    #if !defined CONFIG_LONG_ENOUGH
    long int
    #endif
     beta = 0;'
    """
    #pylint: disable=R0912
    global open_brackets

    if open_brackets > 0:
        l = ""
        pos = 0
        while open_brackets > 0 and pos < len(line):
            if line[pos] == '(':
                open_brackets += 1
            if line[pos] == ')':
                open_brackets -= 1
            if open_brackets == 0:
                l = line[0:pos]
                if len(l) > 0:
                    l += '\n'
                l += '#endif'
                return l
            pos = pos + 1

    m = re.search(r"\s*(IF_NOT)_([A-Za-z0-9_]+)\(", line)
    if m:
        l1 = line[:m.start(1)]
        l2 = "IF" + line[m.end(1):]
        l2 = normalize_IF(l2)
        l2 = l2.replace('defined CONFIG_', r'!defined CONFIG_')
        line = l1 + l2

    m = re.search(r"\s*IF_([A-Za-z0-9_]+)\(", line)
    if m:
        l = line[0:m.start(0)]
        if (line.endswith('(')):
            l += "\n#if defined CONFIG_" + m.group(1)
        else:
            l += "\n#if defined CONFIG_" + m.group(1) + "\n"
        pos = m.end(0)
        old_level = open_brackets
        open_brackets += 1
        while open_brackets > old_level and pos < len(line):
            # count opening and closing brakets
            if line[pos] == '(':
                open_brackets = open_brackets + 1
            if line[pos] == ')':
                open_brackets = open_brackets - 1
            if open_brackets > old_level:
                l += line[pos]
            pos = pos + 1
        if pos <= len(line):
            if open_brackets == old_level:
                if line.endswith(')') and pos == len(line):
                    l += '\n#endif'
                else:
                    l += '\n#endif\n' + line[pos:]
            return l
        else:
            logging.error("normalize_IF failed on line %s, pos %d > len %d",
                          line, pos, len(line))

    return line

def do_not_normalize_def_undef(line):
    """ if a line contains #undef or #define, don't normalize it """
    if re.search(r"^#\s*undef\s", line):
        return True
    if re.search(r"^#\s*define\s", line):
        return True
    return False

def normalize_line(line):
    #this function is special here. disable pylint "too many return statements-warning
    # pylint: disable=R0911
    """ normalize one line and check if the line has to be normalized """

    assert not line.endswith('\n') or line.endswith('#endif\n'), "line: %s" % line

    if len(line) == 0:
        return line

    if do_not_normalize_def_undef(line):
        return line

    m = re.search("//", line)
    if m:
        c = m.start(0)
        return normalize_line(line[:c]) + line[c:]

    m = re.search(r"#\s*.*/\*", line)
    if m:
        c = m.start(0)
        return normalize_line(line[:c]) + line[c:]

    l = normalize_defined_ENABLE_macro(line)
    if l != line:
        return normalize_line(l)

    l = normalize_ENABLE_macro(line)
    if l != line:
        return normalize_line(l)

    l = normalize_ENABLE_inline(line)
    if l != line:
        return normalize_line(l)

    l = normalize_IF(line)
    if l != line:
        return normalize_line(l)

    return l


def substitute_line_continuation(infile):
    """ substitue line continuations because they make life really hard """

    start = -1
    end = -1
    retour = []
    for i in range(len(infile)):
        if infile[i].endswith("\\\n"):
            if start == -1:
                start = i
            end = i
            continue
        else:
            end = i
        if end == i and not start == -1:
            retour.append([''.join(infile[start:(end+1)])][0].replace("\\\n",""))
            start = -1
            end = -1
        else:
            retour.append(infile[i])
    return retour

def normalize_file(infile):
    """
    normalizes each line in infile.

    returns an array of lines
    """

    global open_brackets

    with open(infile, 'r') as fd:
        a = fd.readlines()
        logging.info("file %s contains %d lines", infile, len(a))
        ret = list()

        a = substitute_line_continuation(a)

        for i in range(len(a)):
            try:
                res_line = a[i].splitlines()[0]
                out = normalize_line(res_line)
                if len(out) > 0:
                    lines = out.splitlines()
                    for l in lines:
                        ret.append(l + '\n')
                else:
                    ret.append(out + '\n')
            except:
                """ at the moment we catch every exception here """
                # pylint: disable=W0702
                logging.error("Failed to process %s:%s, skipping line", infile, i)
                continue

        open_brackets = 0
        return ret
