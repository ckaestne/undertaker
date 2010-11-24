PROGS = scripts/kconfig/dumpconf undertaker/undertaker undertaker/cpppc rsf2model/rsf2model

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/cpppc: FORCE
	$(MAKE) -C undertaker cpppc 

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

rsf2model/rsf2model: FORCE
	$(MAKE) -C rsf2model


clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C rsf2model clean


check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C rsf2model $@


undertaker-lcov:
	$(MAKE) -C undertaker run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
