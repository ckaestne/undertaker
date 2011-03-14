#!/usr/bin/python

import sys
import subprocess

VERBOSE = False

def call_undertaker(filename):
    """ Generates the presence conditions for `filename' in one line """
    cpppc = subprocess.Popen(['undertaker', '-j', 'cpppc', filename],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    return ' '.join(cpppc.split('\n')[1:])

def call_limboole(selection, cpppc):
    """ returns True if (selection) && (cpppc) is satisfiable """
    limboole_in = ' && '.join([selection, cpppc]).replace('&&', '&').replace("||", "|")

    if VERBOSE:
        print limboole_in

    return not ' UNSATIS' in subprocess.Popen(['limboole', '-s'],
                                              stdin=subprocess.PIPE,
                                              stdout = subprocess.PIPE,
                                              stderr = subprocess.PIPE).communicate(limboole_in)[0]

def for_all_combinations(length, fn):
    """ applies `fn' to all combinations of a boolean string of length `length' """
    def recursor(rest):
        if rest == 1:
            return [[ False ], [ True ]]
        else:
            return [ [False] + i for i in recursor(rest-1) ] +\
                [ [True] + i for i in recursor(rest-1) ]

    combinations = recursor(length)
    return [ fn(i) for i in combinations ]

class Main:
    def __init__(self):
        self.vars = set()
        self.blocks = set()
        self.content = ""

    def cpp_results(self):
        """ generates list containing all selectable sets of cpp blocks
            [ set(['B1', 'B5', 'B12']), set(['B2', 'B5']) ]
        """
        def tester(combination):
            """ generating preprocessor result for specific set of Macros
                [ True, False, True ] -> cpp -DA -DC -> set(['B1', 'B3', 'B4', 'B12'])
            """
            flags = ['cpp']
            for i in zip(combination, list(self.vars)):
                if i[0]:
                    flags.append('-D%s' % i[1])
            flags.append(sys.argv[1])
            blob = subprocess.Popen(flags, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
            return set([ i.strip() for i in blob.split('\n') if i.startswith('B') ])

        return for_all_combinations(len(self.vars), tester)

    def create_combination(self, combination):
        """ assembles a subset of existing blocks using `combination'
            combination -> [ True, True, False, True ]
            self.blocks -> [ 'B0', 'B3', 'B5', 'B12' ]
            -> [ 'B0', 'B3', 'B12' ] , [ 'B5' ]
        """
        needed = []
        notneeded = []
        for i in zip(combination, self.blocks):
            if i[0]:
                needed.append(i[1])
            else:
                notneeded.append(i[1])

        return needed, notneeded

    def check(self):
        cpppc = call_undertaker(sys.argv[1])
        cppergs = self.cpp_results()

        def printer(combination):
            needed, notneeded = self.create_combination(combination)
            cppresult = set(needed) in cppergs

            # [ 'B0', 'B3', 'B12' ] , [ 'B5' ] -> "B0 && B3 && B12 && !B5"
            blockstring = ' && '.join(needed + [ '!%s' % i for i in notneeded ] )
            undertakerresult = call_limboole(blockstring, cpppc)

            print blockstring.ljust(7*(len(needed) + len(notneeded))),
            print '\t1' if cppresult else '\t0',
            print '\t1' if undertakerresult else '\t0',
            print '\tFINE' if cppresult == undertakerresult else '\tDIFFERENT'
            return cppresult == undertakerresult

        return False in for_all_combinations(len(self.blocks), printer)

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
    if len(sys.argv) > 2 and sys.argv[2] == 'v':
        VERBOSE = True

    m = Main()
    m.main()
    sys.exit(23 if m.check() else 0)
