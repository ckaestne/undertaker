PROGS = scripts/kconfig/dumpconf undertaker/undertaker undertaker/cpppc

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/cpppc: FORCE
	$(MAKE) -C undertaker cpppc 

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean

check:
	$(MAKE) -C undertaker $@

FORCE:
.PHONY: FORCE check
