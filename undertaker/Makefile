DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LINK.o = $(LINK.cpp)
LDLIBS = -lssl -lbdd -lboost_regex libparser.a
TESTLDFLAGS = -lboost_unit_test_framework
PARSEROBJ = BddContainer.o SatContainer.o ExpressionParser.o \
	    VariableToBddMap.o RsfBlocks.o KconfigBdd.o \
	    KconfigRsfDb.o KconfigRsfDbFactory.o SatChecker.o CodeSatStream.o
HEADERS = $(wildcard *.h)

all: undertaker KconfigIntersect
undertaker: libparser.a
undertaker2: libparser.a
KconfigIntersect: libparser.a
sat: libparser.a

$(PARSEROBJ): $(HEADERS)
libparser.a: $(PARSEROBJ) $(HEADERS)
	ar r $@ $(PARSEROBJ)

# undertaker: $(obj)/undertaker undertaker.cpp

# hostprogs-y	:= undertaker undertaker2
# always		:= $(hostprogs-y)
