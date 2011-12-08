PROGS = scripts/kconfig/dumpconf undertaker/undertaker undertaker/predator python/rsf2model ziz/zizler
MANPAGES = doc/undertaker.1.gz doc/undertaker-linux-tree.1.gz doc/undertaker-kconfigdump.1.gz

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
MANDIR ?= $(PREFIX)/share/man

VERSION=$(shell cat VERSION)

ifneq (,$(DESTDIR))
SETUP_PY_EXTRA_ARG = --root=$(DESTDIR)
endif

all: $(PROGS)

version.h: generate-version.sh
	./$<

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

undertaker/predator: FORCE
	$(MAKE) -C undertaker predator

ziz/zizler: FORCE
	$(MAKE) -C ziz zizler

%.1.gz: %.1
	gzip < $< > $@

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C python clean
	@python setup.py clean

docs:
	$(MAKE) -C undertaker docs

check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C python $@

install: all $(MANPAGES)
	@install -d -v $(DESTDIR)$(BINDIR)
	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker 
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker
	@install -d -v $(DESTDIR)$(MANDIR)/man1

	@install -v python/undertaker-calc-coverage $(DESTDIR)$(BINDIR)
	@install -v python/undertaker-kconfigdump $(DESTDIR)$(BINDIR)

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker

	@install -v undertaker/predator $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(BINDIR)

	@install -v ziz/zizler $(DESTDIR)$(BINDIR)

	@install -v scripts/Makefile.list $(DESTDIR)/$(LIBDIR)
	@install -v scripts/Makefile.list_recursion $(DESTDIR)/$(LIBDIR)
	@install -v scripts/Makefile.version $(DESTDIR)/$(LIBDIR)

	@install -v -m 0644 contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v -m 0644 -t $(DESTDIR)$(MANDIR)/man1 $(MANPAGES)

	@python setup.py build $(SETUP_PY_BUILD_EXTRA_ARG)
	@python setup.py install --prefix=$(PREFIX) $(SETUP_PY_INSTALL_EXTRA_ARG)

dist: clean
	tar -czvf ../undertaker-$(VERSION).tar.gz . \
		--show-transformed-names \
		--transform 's,^./,undertaker-$(VERSION)/,'\
		--exclude=*~ \
		--exclude=*.rsf \
		--exclude=*.model \
		--exclude-vcs \
		--exclude="*nfs*" \
		--exclude="*git*" \
		--exclude=*.tar.gz \
		--exclude="*.html"

undertaker-lcov:
	$(MAKE) -C undertaker run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
.NOTPARALLEL:
