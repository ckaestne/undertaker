#!/usr/bin/python

import os
import os.path
import time

import tag.info
import tag.tags
import subtree.tree

filename = 'missing-in-source-annotated'

arches = ["PPC", "PPC64", "X86", "ARM", "SPARC",
          "AMIGA", "AVR32", "M68K", "S390", "BLACKFIN",
          "SPARC64", "IA64", "MIPS", "PPC32", "PARISC",
          "SPARC32"]

false_pos = []

conl = file('ttt', 'w')

tags = tag.tags.Tags('tags.k-s')
tree = subtree.tree.Tree(tags)
tree.parse("missing-in-source-annotated", false_pos + arches)

graph = file("graph2.html", "w+")

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

graph.write('''
<html>
<head>
<style type="text/css">
.point {
font-size: 0.5em;
}
td {
border: solid 1px;
} 
</style>

<script type="text/javascript" src="jquery.js"></script>
<script type="text/javascript">
$(document).ready(function(){
	$(".point").toggle(function(){
		$(this).siblings().show('slow');
	},function(){
		$(this).siblings().hide('slow');
	});
	$(".sub").hide();
	return false;
});
</script>
</head>
<body>
<div><a href="." class="point">Tagstats</a>
<table class="sub" style="border: solid 1px;">
%s
</table>
</div>
<ul>
%s
</ul>
<p>%s</p>
</body>
</html>''' % (tagtable(), tree.output(graph), time.ctime()))



import sys
sys.stderr.write( str(list(tags.alltags())) )

print '\n%s\n' % ('#'*60, ) * 2
for i in tags.alltags():
    print ('<<<%s>>>\n\t' % i),
    print '\n\t'.join(tags.getconfigs(i))
