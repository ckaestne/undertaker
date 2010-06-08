DEBUG = -g3
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)
LDFLAGS =
LDLIBS = libziz.a

ZIZOBJ = Ziz.o
HEADERS = $(wildcard *.h)

all: zizler

zizler: ZizTest.cpp $(HEADERS) $(LDLIBS)
	$(CXX) -o $@ $(CXXFLAGS) $^

$(ZIZOBJ): $(HEADERS)
libziz.a: $(ZIZOBJ) $(HEADERS)
	ar r $@ $(ZIZOBJ)

check: all
	@cd validation && ./test-suite.sh

clean:
	rm -rf *.o

.PHONY: check
