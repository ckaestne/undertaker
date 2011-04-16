#!/usr/bin/python

import sys
import subprocess
import getopt

VERBOSITY=1
filename = ""

def call_undertaker(filename, mode = "simple"):
    """ Generates the presence conditions for `filename' in one line """
    cpppc = subprocess.Popen(['undertaker', '-j', 'coverage', '-O', 'cpp',
                              '-C', mode, filename],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
    cpppc.stdin.close()
    (stdout_got, stderr_got) = (cpppc.stdout.readlines(), cpppc.stderr.readlines())
    cpppc.wait()

    if 0 != cpppc.returncode or len(stderr_got) > 0:
        print "STDOUT:", stdout_got
        print "STDERR:", stderr_got
        raise RuntimeError("calling undertaker failed")

    return filter(lambda x: not x.startswith("I:"), stdout_got)

class Main:
    def __init__(self):
        self.vars = set()
        self.blocks = set()
        self.content = ""

    def cpp_results(self, cpp_args):
        """ generating preprocessor result for specific cpp command line argument"""
        flags = ['cpp'] + cpp_args.strip().split(" ")
        if VERBOSITY > 1:
            print flags

        p = subprocess.Popen(flags,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        blob = p.communicate(self.content)[0]
        if 0 != p.returncode:
            raise RuntimeError("error running cpp with args: " + str(cpp_args))
        return set([ i.strip() for i in blob.split('\n') if i.startswith('B') ])


    def calc_coverage(self, mode = "simple"):
        configurations = call_undertaker(filename, mode)
        covered_set = set()
        for config in configurations:
            covered_set = covered_set.union(self.cpp_results(config))

        print "  COVERAGE %.2f%%, %d configs, %s" % (float(len(covered_set))/float(len(self.blocks)) * 100,
                                                    len(configurations), mode)
        if self.blocks != covered_set:
            print "    DIFF: " + str(list(self.blocks.difference(covered_set)))

        return float(len(covered_set))/float(len(self.blocks))

    def check(self):
        print "TEST:", filename
        return not(self.calc_coverage("simple") == self.calc_coverage("min"))


    def main(self):
        p = subprocess.Popen(['zizler', '-c', filename], stdout=subprocess.PIPE)
        self.content = p.communicate()[0]
        if p.returncode != 0:
            raise RuntimeError("Error while running zizler")

        continue_with_next = False

        def extract_block(line):
            if line.startswith('B'):
                self.blocks.add(line.strip())


        for line in self.content.split('\n'):
            if VERBOSITY == 2:
                print line
            extract_block(line)

        if VERBOSITY == 2:
            print self.blocks

def usage():
    print "%s [-v level] filename" % sys.argv[0]
    print "%s -h" % sys.argv[0]
    print
    print "-v: Verbosity"
    print "  -v 0: only print differences"
    print "  -v 1: print result for all combinations"
    print "  -v 2: noisy debug output"
    print
    print "Return values:"
    print " 1: Operation Error"
    print " 2: Some checks didn't pass"

if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:], "hv:")

    for opt,arg in opts:
        if opt in ('-v', '--verbose'):
            VERBOSITY=int(arg)
            if VERBOSITY < 0 or VERBOSITY > 2:
                print "invalid verbosity level"
                usage()
                sys.exit(1)
        elif opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        else:
            usage()
            assert False, "unknown option"

    if len(args) != 1:
        print "(only) expecting filename as argument"
        usage()
        sys.exit(1)

    filename = args[0]

    m = Main()
    m.main()
    sys.exit(2 if m.check() else 0)
