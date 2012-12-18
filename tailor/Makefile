CC = gcc
CXX = g++
CSTD = c99
CFLAGS = -O3 -Wall -pedantic -Wno-unused-result -ffast-math -ftracer -funroll-loops -fforce-addr
CXXSTD = c++0x
CXXFLAGS = -lboost_regex
TARGET=undertaker-traceutil
TESTDIR = sample
SRCDIR = src
VARIANT = native

.PHONY: all native stdio boost clean test testclean check install

all: $(VARIANT)

native: $(TARGET)

$(TARGET): $(SRCDIR)/traceutil-native.c
	$(CC) -std=$(CSTD) $(CFLAGS) $^ -o $(TARGET)

stdio: $(SRCDIR)/traceutil-stdio.c
	$(CC) -std=$(CSTD) $(CFLAGS) $^ -o $(TARGET)

boost: $(SRCDIR)/traceutil-boost.cc
	$(CXX) -std=$(CXXSTD) $(CFLAGS) $^ $(CXXFLAGS) -o $(TARGET)

clean: testclean
	rm -f $(TARGET)
	rm -f ftrace-initramfs.img
	rm -f initrd.img-*
	rm -f sample/test_ignore sample/test_out

check: $(TARGET) testclean
	@echo "Running trace..."
	./$(TARGET) $(TESTDIR)/test_trace $(TESTDIR)/test_out $(TESTDIR)/test_ignore $(TESTDIR)/test_modules
	@echo "Checking results..."
	@test -f $(TESTDIR)/test_out.`uname -m` || echo File $(TESTDIR)/test_out.`uname -m` not available... Ignoring.
	@test ! -f $(TESTDIR)/test_out.`uname -m` || diff $(TESTDIR)/test_out $(TESTDIR)/test_out.`uname -m`
	@diff $(TESTDIR)/test_ignore $(TESTDIR)/test_ignore.all
	@echo "(done)"

testclean:
	rm -f $(TESTDIR)/test_out $(TESTDIR)/test_ignore

test: check
	@echo -n "Checking if current booted kernel supports ftrace... "
	@cat /boot/config-`uname -r` | grep -q "CONFIG_FTRACE=y" && echo "Yes" \
	|| echo "No - you are not able to use this kernel without modification."
