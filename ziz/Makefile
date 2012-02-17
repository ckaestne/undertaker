CXX=g++
LDXX=g++
DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LDLIBS = -lboost_regex -lboost_wave-mt -lboost_system-mt $(LDCOV)

ZIZOBJ = Ziz.o
HEADERS = $(wildcard *.h)

all: zizler

Zizler.o: Zizler.cpp $(HEADERS)

zizler: Zizler.o libziz.a
	$(LDXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(ZIZOBJ): $(HEADERS)
libziz.a: $(ZIZOBJ) $(HEADERS)
	ar r $@ $(ZIZOBJ)

clean: clean-check
	rm -rf *.o *.a zizzler

clean-check:
	rm -rf test/out

check test: all
	@mkdir -p test/out
	@cd test && ./test-suite.sh

test-clean:
	rm -rf test/out/*

.PHONY: check test
