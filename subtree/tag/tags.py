#!/usr/bin/python

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
