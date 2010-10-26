# DEBUG = -g3 -DDEBUG
# to compile with debug, compile like this:
# make DEBUG="-g3 -DDEBUG"
CFLAGS = -Wall -Wextra -O2 -I../ziz -I../picosat $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
LDLIBS = -lbdd -lboost_regex -lboost_filesystem libparser.a ../ziz/libziz.a  ../picosat/libpicosat.a -lboost_wave-mt
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = CloudContainer.o ExpressionParser.o \
	    VariableToBddMap.o RsfBlocks.o KconfigBdd.o \
	    KconfigRsfDb.o KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o
HEADERS = $(wildcard *.h)
PROGS = undertaker cpppc

all: ../ziz/libziz.a $(PROGS)
undertaker: ../limmat/liblimmat.a libparser.a
undertaker2: libparser.a
KconfigIntersect: libparser.a
cpppc: libparser.a ../ziz/libziz.a

../ziz/libziz.a: FORCE
	$(MAKE) -C ../ziz libziz.a

../picosat/libpicosat.a: FORCE
	cd ../picosat/; test -e makefile || ./configure
	$(MAKE) -C ../picosat libpicosat.a

$(PARSEROBJ): $(HEADERS)
libparser.a: $(PARSEROBJ) $(HEADERS)
	ar r $@ $(PARSEROBJ)

clean:
	rm -rfv *.o *.a

test: test-SatChecker
test-%: %.cpp
	$(CXX) $(CXXFLAGS) -DTEST_$* -o $@ $< -lcheck $(LDFLAGS) $(LDLIBS)

run-test-%: %
	for f in tests/*.c; do printf "$$f: examining with $<\n\n"; ./$< $$f; done

run-test-SatChecker: test-SatChecker
	for t in $^; do ./$$t; done

run-all-tests: run-test-SatChecker run-test-cpppc run-test-undertaker

docs:
	doxygen

.PHONY: all clean FORCE run-% docs
