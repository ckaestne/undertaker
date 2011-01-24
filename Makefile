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

	@install -d -v $(DESTDIR)$(PREFIX)/lib/undertaker 
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(PREFIX)/lib/undertaker
	@install -v rsf2model/rsf2model $(DESTDIR)$(PREFIX)/lib/undertaker
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(PREFIX)/lib/undertaker

	@install -v undertaker/undertaker $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-kconfigdump $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(PREFIX)/bin

	@install -v contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

dist: clean
	tar -C .. -czvf ../undertaker-$(VERSION).tar.gz vamos --exclude=*.rsf --exclude=*.model \
		--exclude-vcs --exclude="*nfs*" --exclude="*git*" --exclude=*.tar.gz --exclude="*.html"

undertaker-lcov:
	$(MAKE) -C undertaker run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
