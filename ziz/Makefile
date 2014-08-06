CXX=g++
LDXX=g++
DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS) -std=gnu++11
LDFLAGS =
LDLIBS = -lboost_regex -lboost_wave -lboost_system $(LDCOV)

HEADERS = $(wildcard *.h)

all: zizler

Zizler.o: Zizler.cpp $(HEADERS)

zizler: Zizler.o Ziz.o
	$(LDXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

Ziz.o: $(HEADERS)

clean: clean-check
	rm -rf *.o *.a zizler

clean-check:
	rm -rf test/out

check test: all
	@mkdir -p test/out
	@cd test && ./test-suite.sh

test-clean:
	rm -rf test/out/*

.PHONY: check test
