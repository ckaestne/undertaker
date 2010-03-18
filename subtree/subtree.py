#!/usr/bin/python

import os
import os.path
import time
import sys

import tag.info
import tag.tags
import subtree.tree

if (len(sys.argv) < 4):
    sys.exit(-1)

template = file("%s/graph.tmpl" % os.path.dirname(__file__)).read()

arches = ["PPC", "PPC64", "X86", "ARM", "SPARC",
          "AMIGA", "AVR32", "M68K", "S390", "BLACKFIN",
          "SPARC64", "IA64", "MIPS", "PPC32", "PARISC",
          "SPARC32"]

false_pos = []

tags = tag.tags.Tags(sys.argv[2])
tree = subtree.tree.Tree(tags)
tree.parse(sys.argv[1], false_pos + arches)

graph = file(sys.argv[3], "w+")

def tagtable():
    import re
    r = re.compile('[0-9a-f]{40}')

    f = lambda x: x if not x[len(x) - 1] == "?" else x[:-1]

    taglist = list(set( [f(i) for i in tags.alltags() if not r.match(i)]))
    return '\n'.join(['<tr><td>%s</td><td>%s</td></tr>' % (i, '<br />'.join(tags.getconfigs(i) +
                                                                            tags.getconfigs('%s?' % i)))
                      for i in taglist])

def tagtable_():
    import re
    r = re.compile('[0-9a-f]{40}')

    taglist = [i for i in tags.alltags() if r.match(i)]
    result = '<tr>'

    for i in taglist:
        result += '<td>%s</td>' % i

    result += '</tr>\n<tr>'

    for i in taglist:
        result += '\n<td>%s</td>' % '<br />'.join(tags.getconfigs(i))

    return result

graph.write(template % (tagtable(), tree.output(graph), time.ctime()))

sys.stderr.write( str(list(tags.alltags())) )

print '\n%s\n' % ('#'*60, ) * 2
for i in tags.alltags():
    print ('<<<%s>>>\n\t' % i),
    print '\n\t'.join(tags.getconfigs(i))
