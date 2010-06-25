PROGS = scripts/kconfig/dumpconf undertaker/cpppc

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/cpppc: FORCE
	$(MAKE) -C undertaker cpppc 

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean

FORCE:
PHONY: FORCE
