# DEBUG = -g3 -DDEBUG
# to compile with debug, compile like this:
# make DEBUG="-g3 -DDEBUG"
CFLAGS = -Wall -Wextra -O2 -I../ziz -I../picosat $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
# LDCOV = -coverage
LDLIBS = -lbdd -lboost_regex -lboost_filesystem libparser.a ../ziz/libziz.a  ../picosat/libpicosat.a -lboost_wave-mt $(LDCOV)
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = CloudContainer.o ExpressionParser.o VariableToBddMap.o	\
	    RsfBlocks.o KconfigBdd.o KconfigWhitelist.o KconfigRsfDb.o	\
	    KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o
HEADERS = $(wildcard *.h)
PROGS = undertaker cpppc

all: ../ziz/libziz.a $(PROGS)
undertaker: ../picosat/libpicosat.a ../ziz/libziz.a libparser.a
undertaker2: libparser.a
KconfigIntersect: libparser.a
cpppc: libparser.a ../ziz/libziz.a

../ziz/libziz.a: FORCE
	$(MAKE) -C ../ziz libziz.a

../picosat/libpicosat.a: FORCE
	cd ../picosat/; test -s makefile || ./configure
	$(MAKE) -C ../picosat libpicosat.a

$(PARSEROBJ): $(HEADERS)
libparser.a: $(PARSEROBJ) $(HEADERS)
	ar r $@ $(PARSEROBJ)

clean: clean-check
	rm -rf *.o *.a *.gcda *.gcno


test-%: %.cpp libparser.a
	$(CXX) $(CXXFLAGS) -DTEST_$* -o $@ $< -lcheck $(LDFLAGS) $(LDLIBS)

run-libcheck: test-SatChecker
	@for t in $^; do ./$$t; done

check: $(PROGS)
	@$(MAKE) -s clean-check
	cd validation && ./test-suite
	@$(MAKE) -s run-libcheck

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

run-lcov:
	$(MAKE) -B DEBUG="-g -O0 -fprofile-arcs -ftest-coverage" LDCOV="-coverage"
	rm -rf coverage-html ; mkdir coverage-html
	lcov --directory $(CURDIR) --zerocounters
	-$(MAKE) check
	lcov --directory $(CURDIR) --capture --output-file coverage-html/undertaker.info
	genhtml -o coverage-html coverage-html/undertaker.info

FORCE:
.PHONY: all clean FORCE run-% docs validation run-lcov
