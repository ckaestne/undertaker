DEBUG = -g3 -DDEBUG -g -ggdb
CFLAGS = -Wall -Wextra -O0 -I../ziz -I../limmat $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
LDLIBS = -lbdd -lboost_regex -lboost_filesystem libparser.a ../ziz/libziz.a  ../limmat/liblimmat.a -lboost_wave-mt
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = CloudContainer.o ExpressionParser.o \
	    VariableToBddMap.o RsfBlocks.o KconfigBdd.o \
	    KconfigRsfDb.o KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o 

HEADERS = $(wildcard *.h)
PROGS = undertaker

all: ../limmat/liblimmat.a ../ziz/libziz.a $(PROGS)
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

clean:
	rm -rfv *.o *.a 

test: test-SatChecker

test-%: %.cpp
	$(CXX) $(CXXFLAGS) -DTEST_$* -o $@ $< $(LDFLAGS) $(LDLIBS) 

.PHONY: all clean FORCE
