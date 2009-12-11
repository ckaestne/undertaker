#!/usr/bin/python

import os
import os.path

filename = 'some-results-for-nur-in-source'

arches = ["PPC", "PPC64", "X86", "ARM", "SPARC",
          "AMIGA", "AVR32", "M68K", "S390", "BLACKFIN",
          "SPARC64", "IA64", "MIPS", "PPC32", "PARISC", 
          "SPARC32"]

false_pos = ["IP_PIMSM",]

conl = file('ttt', 'w')

class Tags:
    def __init__(self, filename):
        self.taglist = dict()
        taglines = [ x[:-1] for x in file(filename).readlines() ]
        for tl in taglines:
            tagline = tl.split()
            if tagline[0] in self.taglist:
                import sys
                sys.stderr.write('Config %s appears twice in tagfile' % tagline[0])
                sys.exit(-1)
            else:
                self.taglist[tagline[0]] = tagline[1:]

        for i in self.taglist.items():
            import sys
            sys.stderr.write( str(i) + '\n' )

    def __getitem__(self, config):
        if config in self.taglist:
            return self.taglist[config]
        else:
            return []

    def getconfigs(self, tagname):
        result = []
        for i in self.taglist.items():
            if tagname in i[1]:
                result.append(i[0])
        return result

    def alltags(self):
        result = set()
        for i in self.taglist.values() :
            result |= set(i)
        return result

class Config:
    def __init__(self, name, tags):
        self.name = name
        self.tags = tags[name]

    def __str__(self):
        return '%s (%s)' % ( self.name, ", ".join(self.tags) )
        
    def __hash__(self):
        return self.name.__hash__()

class Dir:
    def __init__(self, name, tags, configs=[], files=[], dirs=[]):
        self.name = name
        self.configs = set(configs)
        self.files = files
        self.dirs = dirs
        self.tags = tags

    def add_dir(self, rempath, config):
        sfor = rempath[0]
        for d in self.dirs:
            if d.name == sfor:
                d.add(rempath[1:], config)
                return

        d = Dir(sfor, self.tags, [], [], [])
        d.add(rempath[1:], config)
        self.dirs.append(d)

    def add_file(self, name, config):
        for f in self.files:
            if f.name == name:
                f.add(config)
                return
        
        self.files.append(File(name, self.tags, [config]))

    def add(self, rempath, config):
        self.configs.add(config)

        if len(rempath) == 1:
            self.add_file(rempath[0], config)
        else:
            self.add_dir(rempath, config)

    def output(self, indent):
        return ''.join(('<li>%s (%d)' % (self.name, len(self.configs)),
                        '<div><a href="." class="point">Configs</a><ul class="sub"><li>',
                        '</li><li>'.join(sorted([str(Config(x, self.tags)) for x in self.configs])),
                        '</li></ul></div><div><a href="." class="point">Subdirs</a>',
                        '<ul class="sub">',
                        ''.join([ i.output(indent + '>') for i in self.files]),
                        ''.join([ i.output(indent + '>') for i in self.dirs]),
                        "</ul></div></li>"))

class File:
    def __init__(self, name, tags, configs=[]):
        self.name = name
        self.configs = set(configs)
        self.tags = tags

    def output(self, indent):
        return "".join(("<li>%s (%d)" % (self.name, len(self.configs)),
                        '<div><a href="." class="point">Configs</a><ul class="sub"><li>',
                        '</li><li>'.join(sorted([str(Config(x, self.tags)) for x in self.configs])),
                        '</li></ul></div></li>'))
#        print indent, '-- File ', self.name, " : ", ",".join(self.configs)

    def add(self, config):
        self.configs.add(config)

class Tree:
    def __init__(self, tags):
        self.root = Dir('.', tags)
        self.tags = tags

    def add_node(self, path, config):
        steps = path.split('/')
        self.root.add(steps[1:], config)

    def parse(self, filename):
        lines = [ x[:-1] for x in file(filename).readlines() ]
        conf = ""
        for line in lines:
            if line[:7] == "CONFIG_":
                conf = line[7:]
                continue
            else:
                if conf not in false_pos + arches:
                    conl.write('%s\n' % conf)
                    self.add_node(line, conf)

    def output(self, ondisk):
        return self.root.output("")

tags = Tags('tags')
tree = Tree(tags)
tree.parse("some-results-for-nur-in-source")

graph = file("graph.html", "w+")

graph.write('''
<html>
<head>
<style type="text/css">
.point {
font-size: 0.5em;
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
<ul>
%s
</ul>
</body>
</html>''' % tree.output(graph))



import sys
sys.stderr.write( str(list(tags.alltags())) )

print '\n%s\n' % ('#'*60, ) * 2
for i in tags.alltags():
    print ('<<<%s>>>\n\t' % i),
    print '\n\t'.join(tags.getconfigs(i))
