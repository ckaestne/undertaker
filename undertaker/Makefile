DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 -I../ziz $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
LDLIBS = -lssl -lbdd -lboost_regex libparser.a ../ziz/libziz.a -lboost_wave-mt
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = BddContainer.o CloudContainer.o ExpressionParser.o \
	    VariableToBddMap.o RsfBlocks.o KconfigBdd.o \
	    KconfigRsfDb.o KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o
HEADERS = $(wildcard *.h)
PROGS = undertaker cpppc # undertaker2  <- currently broken

all: ../ziz/libziz.a $(PROGS)
undertaker: libparser.a
undertaker2: libparser.a
KconfigIntersect: libparser.a
cpppc: libparser.a ../ziz/libziz.a

../ziz/libziz.a: FORCE
	$(MAKE) -C ../ziz libziz.a

$(PARSEROBJ): $(HEADERS)
libparser.a: $(PARSEROBJ) $(HEADERS)
	ar r $@ $(PARSEROBJ)

clean:
	rm -rfv *.o *.a 

.PHONY: all clean FORCE
