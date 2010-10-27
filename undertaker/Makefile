# DEBUG = -g3 -DDEBUG
# to compile with debug, compile like this:
# make DEBUG="-g3 -DDEBUG"
CFLAGS = -Wall -Wextra -O2 -I../ziz -I../limmat $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
LDLIBS = -lbdd -lboost_regex -lboost_filesystem libparser.a ../ziz/libziz.a  ../limmat/liblimmat.a -lboost_wave-mt
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = CloudContainer.o ExpressionParser.o \
	    VariableToBddMap.o RsfBlocks.o KconfigBdd.o \
	    KconfigRsfDb.o KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o
HEADERS = $(wildcard *.h)
PROGS = undertaker cpppc

all: ../ziz/libziz.a $(PROGS)
undertaker: ../limmat/liblimmat.a ../ziz/libziz.a libparser.a
undertaker2: libparser.a
KconfigIntersect: libparser.a
cpppc: libparser.a ../ziz/libziz.a

../ziz/libziz.a: FORCE
	$(MAKE) -C ../ziz libziz.a

../limmat/liblimmat.a: FORCE
	cd ../limmat/; test -e Makefile || ./configure
	$(MAKE) -C ../limmat liblimmat.a

$(PARSEROBJ): $(HEADERS)
libparser.a: $(PARSEROBJ) $(HEADERS)
	ar r $@ $(PARSEROBJ)

clean: clean-check
	rm -rf *.o *.a

test: test-SatChecker
test-%: %.cpp
	$(CXX) $(CXXFLAGS) -DTEST_$* -o $@ $< $(LDFLAGS) $(LDLIBS)

run-test-%: %
	for f in tests/*.c; do printf "$$f: examining with $<\n\n"; ./$< $$f; done

run-test-SatChecker: test-SatChecker
	for t in $^; do ./$$t; done

run-all-tests: run-test-SatChecker run-test-cpppc run-test-undertaker

check: $(PROGS)
	cd validation && ./test-suite

clean-check:
	find validation/ \( -name "*.c.output.expected" \
	                 -o -name "*.c.output.got" \
	                 -o -name "*.c.output.diff" \
	                 -o -name "*.c.error.expected" \
	                 -o -name "*.c.error.got" \
	                 -o -name "*.c.error.diff" \
	                 -o -name "*.dead" \
	                 -o -name "*.undead" \
	                 \) -exec rm {} \;
docs:
	doxygen

.PHONY: all clean FORCE run-% docs validation
