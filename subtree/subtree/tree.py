#!/usr/bin/python

import config

class TreeNode:
    def __init__(self, name, tags, configs):
        self.name = name
        self.tags = tags
        self.configs = set(configs)

    def output(self, indent):
        return '''%s (%d)
<div><a href="." class="point">Configs</a><ul class="sub">
<li>%s</li>
</ul></div>''' % (self.name,
                   len(self.configs),
                   '</li><li>'.join(sorted([str(config.Config(x, self.tags))
                                            for x in self.configs])))

class Dir(TreeNode):
    def __init__(self, name, tags, configs=[], files=[], dirs=[]):
        TreeNode.__init__(self, name, tags, configs)
        self.files = files
        self.dirs = dirs

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

    def output_subs(self, indent):
        return '''<div><a href="." class="point">Subdirs</a>
<ul class="sub">
%s
%s
</ul></div>''' % (''.join([ i.output(indent + '>') for i in self.files]),
                  ''.join([ i.output(indent + '>') for i in self.dirs]))

    def output(self, indent):
        return '''<li>%s%s</li>''' % (TreeNode.output(self, indent),
                                      self.output_subs(indent))

class File(TreeNode):
    def output(self, indent):
        return '<li>%s</li>' % TreeNode.output(self, indent)

    def add(self, config):
        self.configs.add(config)

class Tree:
    def __init__(self, tags):
        self.root = Dir('.', tags)
        self.tags = tags

    def add_node(self, path, config):
        steps = path.split('/')
        self.root.add(steps[1:], config)

    def parse(self, filename, ignores):
        lines = [ x[:-1] for x in file(filename).readlines() ]
        conf = ""
        for line in lines:
            if line[:7] == "CONFIG_":
                conf = line[7:]
                continue
            else:
                if conf not in ignores:
#                    conl.write('%s\n' % conf)
                    self.add_node(line, conf)

    def output(self, ondisk):
        return self.root.output("")
