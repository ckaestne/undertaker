# Add .d to Make's recognized suffixes.
SUFFIXES += .d

#We don't need to clean up when we're making these targets
NODEPS:=clean clean-check

# DEBUG = -g3 -DDEBUG
# to compile with debug, compile like this:
# make DEBUG="-g3 -DDEBUG"
CFLAGS = -Wall -Wextra -O2 -I../ziz $(DEBUG)
CXXFLAGS = $(CFLAGS)

LDFLAGS =
# LDCOV = -coverage
LDLIBS = -lstdc++ -lboost_system -lboost_regex -lboost_filesystem libparser.a ../ziz/libziz.a  -lpicosat -lboost_wave-mt $(LDCOV)

PARSEROBJ = CloudContainer.o RsfReader.o KconfigWhitelist.o ConfigurationModel.o	\
	    ModelContainer.o SatChecker.o CodeSatStream.o BlockDefectAnalyzer.o

DEPFILES:=$(patsubst %.o,%.d,$(PARSEROBJ)) undertaker.d

PROGS = undertaker
TESTPROGS = test-SatChecker

all: ../ziz/libziz.a $(PROGS) $(TESTPROGS)

undertaker: ../ziz/libziz.a libparser.a
KconfigIntersect: libparser.a

# we require FORCE here because we don't want to duplicate all
# dependencies from libziz' Makefiles
../ziz/zizler ../ziz/libziz.a: FORCE
	$(MAKE) -C ../ziz libziz.a
	$(MAKE) -C ../ziz zizler

libparser.a: $(DEPFILES) $(PARSEROBJ)
	ar r $@ $(PARSEROBJ)

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    #Chances are, these files don't exist.  GMake will create them and
    #clean up automatically afterwards
    -include $(DEPFILES)
endif

#This is the rule for creating the dependency files
%.d: %.cpp
	 @$(CXX) $(CXXFLAGS) -MM $< > $@

clean: clean-check
	rm -rf *.o *.a *.gcda *.gcno *.d

test-%: test-%.cpp libparser.a
	$(CXX) $(CXXFLAGS) -o $@ $< -lcheck $(LDFLAGS) $(LDLIBS)

run-libcheck: test-SatChecker
	@for t in $^; do ./$$t; done

# coverage analysis will create validation/sched.c.config*
run-coveragecheck: undertaker
	@$(MAKE) -C kconfig-dumps all
	./undertaker -c -m kconfig-dumps/models/x86.model validation/sched.c
	# we expect between 25 and 30 solutions
	test `find validation -name 'sched.c.config*' | wc -l` -gt 5
	test `find validation -name 'sched.c.config*' | wc -l` -lt 30
	md5sum validation/sched.c.config* | awk '// {print $$1}' | sort | uniq -c \
		| awk '// { if ($$1 != 1) { print "duplicate solutions found" ; exit 1 } }' \
		|| md5sum validation/sched.c.config* | sort
	if grep -q ^CONFIG_IA64 validation/sched.c.config*; then echo "IA64 not selectable!"; false ; fi
	if grep -q ^CONFIG_CHOICE validation/sched.c.config*; then echo "must not contain CONFIG_CHOICE*"; false ; fi

check: $(PROGS) ../ziz/zizler
	@$(MAKE) -s clean-check
	@$(MAKE) run-coveragecheck
	@$(MAKE) -C kconfig-dumps all
	cd def-tests && ./run-tests
	cd validation && ./test-suite
	@$(MAKE) -s run-libcheck

clean-check:
	find validation/ \( -name "*.c.output.expected" \
	                 -o -name "*.c.output.got" \
	                 -o -name "*.c.output.diff" \
	                 -o -name "*.c.error.expected" \
	                 -o -name "*.c.error.got" \
	                 -o -name "*.c.error.diff" \
	                 -o -name "*.c.config*" \
	                 -o -name "*.dead" \
	                 -o -name "*.undead" \
	                 \) -exec rm {} \;
docs:
	doxygen

run-lcov:
	$(MAKE) -B DEBUG="-g -O0 -fprofile-arcs -ftest-coverage" LDCOV="-coverage" -C ../ziz zizler libziz.a
	$(MAKE) -B DEBUG="-g -O0 -fprofile-arcs -ftest-coverage" LDCOV="-coverage"
	rm -rf coverage-html ; mkdir coverage-html
	lcov --directory $(CURDIR) --zerocounters
	-$(MAKE) check
	lcov --directory $(CURDIR) --capture --output-file coverage-html/undertaker.info
	genhtml -o coverage-html coverage-html/undertaker.info

FORCE:
.PHONY: all clean clean-check FORCE run-% docs validation run-lcov run-coveragecheck
