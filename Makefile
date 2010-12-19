PROGS = scripts/kconfig/dumpconf undertaker/undertaker rsf2model/rsf2model
PREFIX ?= /usr/local

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

rsf2model/rsf2model: FORCE
	$(MAKE) -C rsf2model


clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C rsf2model clean

docs:
	$(MAKE) -C undertaker docs

check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C rsf2model $@

install: all
	@install -d -v $(DESTDIR)$(PREFIX)/bin
	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/dump-rsf.sh $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/go.sh $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/scan-head $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/do-coverage.sh $(DESTDIR)$(PREFIX)/bin
	@install -v rsf2model/rsf2model $(DESTDIR)$(PREFIX)/bin

undertaker-lcov:
	$(MAKE) -C undertaker run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
