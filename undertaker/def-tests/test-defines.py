#!/usr/bin/python

import sys
import subprocess

class Main:
    def __init__(self):
        self.vars = set()
        self.blocks = set()

    def for_all_combinations(self, length, fn):
        def recursor(rest):
            if rest == 1:
                return [[ False ], [ True ]]
            else:
                return [ [False] + i for i in recursor(rest-1) ] +\
                       [ [True] + i for i in recursor(rest-1) ]

        combinations = recursor(length)

        return [ fn(i) for i in combinations ]

    def check(self):
        def tester(combination):
            flags = ['cpp']
            for i in zip(combination, list(self.vars)):
                if i[0]:
                    flags.append('-D%s' % i[1])
            flags.append(sys.argv[1])
            return subprocess.Popen(flags, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
        ergs = self.for_all_combinations(len(self.vars), tester)
        ergs = sorted([ sorted([ i.strip() for i in j.split('\n') if i.startswith('B') ]) for j in ergs ])

        def undertaker_test(conditionstring):
            cpppc = subprocess.Popen(['/proj/i4vamos/tools/bin/undertaker', '-j', 'cpppc', sys.argv[1]],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

            cpppc = ' '.join(cpppc.split('\n')[1:])
            
            limboole_in = ' && '.join([conditionstring, cpppc]).replace('&&', '&')

            if len(sys.argv) > 2 and sys.argv[2] == 'v':
                print limboole_in

            undertakererg = subprocess.Popen(['limboole', '-s'],
                                             stdin=subprocess.PIPE,
                                             stdout = subprocess.PIPE,
                                             stderr = subprocess.PIPE).communicate(limboole_in)[0]
            return not ' UNSATIS' in undertakererg

        def printer(combination):
            needed = []
            stringparts = []
            for i in zip(combination, self.blocks):
                if i[0]:
                    needed.append(i[1])
                    stringparts.append(i[1])
                else:
                    stringparts.append("!%s" % i[1])
            boolstring = ' && '.join(stringparts)
            cppresult = sorted(needed) in ergs
            undertakerresult = undertaker_test(boolstring)
            
            print boolstring.ljust(7*len(stringparts)),
            print '\t1' if cppresult else '\t0',
            print '\t1' if undertakerresult else '\t0',
            print '\tFINE' if cppresult == undertakerresult else '\tDIFFERENT'

        self.for_all_combinations(len(self.blocks), printer)
            


    def main(self):
        if len(sys.argv) < 2:
            print "need a filename"
            sys.exit(23)
        with open(sys.argv[1]) as f:
            for line in f:
                if line.startswith('B'):
                    self.blocks.add(line.strip())
                elif line.startswith('#'):
                    try:
                        self.vars.add(line.split()[1])
                    except:
                        pass

if __name__ == '__main__':
    m = Main()
    m.main()
    m.check()
