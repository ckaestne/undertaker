CXX=g++
LDXX=g++
DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDXXFLAGS =

ZIZOBJ = Ziz.o
HEADERS = $(wildcard *.h)

all: zizler

ZizTest.o: ZizTest.cpp $(HEADERS)

zizler: ZizTest.o libziz.a
	$(LDXX) -o $@ $(LDXXFLAGS) $^

$(ZIZOBJ): $(HEADERS)
libziz.a: $(ZIZOBJ) $(HEADERS)
	ar r $@ $(ZIZOBJ)

check: all
	@cd validation && ./test-suite.sh

clean:
	rm -rf *.o *.a

test-clean:
	rm -rf validation/*.got
	rm -rf validation/*.diff
	rm -rf validation/*.expected

.PHONY: check
