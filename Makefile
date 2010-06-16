CXX=g++
LDXX=g++
DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDXXFLAGS = -lboost_wave-mt

ZIZOBJ = Ziz.o
HEADERS = $(wildcard *.h)

all: zizler

Zizler.o: Zizler.cpp $(HEADERS)

zizler: Zizler.o libziz.a
	$(LDXX) -o $@ $(LDXXFLAGS) $^

$(ZIZOBJ): $(HEADERS)
libziz.a: $(ZIZOBJ) $(HEADERS)
	ar r $@ $(ZIZOBJ)

clean:
	rm -rf *.o *.a

test: all
	@cd test && ./test-suite.sh

test-clean:
	rm -rf test/out/*

.PHONY: check
