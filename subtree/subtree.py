#!/usr/bin/python

import os
import os.path
import time
import sys


import subtree.tree

def from_anotated_file(args):
    if (len(args) != 2):
        sys.exit(1)

    import tag.info
    import tag.tags

    arches = ["PPC", "PPC64", "X86", "ARM", "SPARC",
              "AMIGA", "AVR32", "M68K", "S390", "BLACKFIN",
              "SPARC64", "IA64", "MIPS", "PPC32", "PARISC",
              "SPARC32"]

    whitelist = []

    def tagtable():
        import re
        r = re.compile('[0-9a-f]{40}')

        f = lambda x: x if not x[len(x) - 1] == "?" else x[:-1]

        taglist = list(set( [f(i) for i in tags.alltags() if not r.match(i)]))
        return '\n'.join(['<tr><td>%s</td><td>%s</td></tr>' % (i, '<br />'.join(tags.getconfigs(i) +
                                                                                tags.getconfigs('%s?' % i)))
                          for i in taglist])

    tags = tag.tags.Tags(args[1])
    tree = subtree.tree.Tree(tags)
    tree.parse([ x[:-1] for x in file(args[0]).readlines() ], whitelist + arches)

    return tree, tagtable()

def from_satdead_tree(args):
    if len(args) != 0:
        sys.exit(1)

    import subprocess

    class Tags:
        def __getitem__(self, config):
            return []

    tags = Tags()
    tree = subtree.tree.Tree(tags)

    def extract(string):
        tmp = string.split(':')[0].split('codesat')
        return (tmp[0][:-1], tmp[1].split('.')[1])

    stuff = [ extract(i) for i in  subprocess.Popen(["grep", "-R", "UNSATISFIABLE", "."], stdout=subprocess.PIPE).communicate()[0].split('\n')[:-1] ]

    satdead = ['CONFIG_SATDEAD']

    for thing in stuff:
        f = [ i.split('\t') for i in file('%s.rsfout' % thing[0]).readlines() ]
        f = [ i[2] for i in f if i[0] == 'CondBlockIsAtPos' and i[1] == thing[1][1:] ]
        satdead += f

    tree.parse(satdead, [])

    return tree, ""

def from_satdead_tree_extract(args):
    if len(args) < 2:
        sys.exit(1)

    import subprocess

    hardcoded_extract_base = '/proj/i4vamos/users/tartler/results/vamos1/'
    date, version = args[:2]
    for i in [ '%s/%s/v%s-%s-files.cpio' % (hardcoded_extract_base, date, version, i) for i in ('dead', 'rsfout') ]:
        subprocess.call(["cpio", '-id'], stdin = file(i))

    return from_satdead_tree(args[2:])

def main():
    if len(sys.argv) < 3:
        sys.exit(1)

    opmode, resultfile = sys.argv[1:3]

    if opmode == '-a':
        tree, tagtable = from_anotated_file(sys.argv[3:])
    elif opmode == '-d':
        tree, tagtable = from_satdead_tree(sys.argv[3:])
    elif opmode == '-e':
        tree, tagtable = from_satdead_tree_extract(sys.argv[3:])
    else:
        sys.stderr.write("Please specify source\n")
        sys.stderr.write("-a for a anotated list\n")
        sys.stderr.write("-d for a tree with .dead files\n")
        sys.exit(1)

    graph = file(resultfile, "w+")

    template = file("%s/graph.tmpl" % os.path.dirname(__file__)).read()

    graph.write(template % (tagtable, tree.output(), time.ctime()))

if __name__ == '__main__':
    main()
