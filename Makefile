PROGS = scripts/kconfig/dumpconf undertaker/undertaker undertaker/predator undertaker/rsf2cnf \
	python/rsf2model tailor/undertaker-traceutil ziz/zizler
MANPAGES = doc/undertaker.1.gz doc/undertaker-linux-tree.1.gz doc/undertaker-kconfigdump.1.gz \
	doc/undertaker-kconfigpp.1.gz

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
LIBDIR ?= $(PREFIX)/lib
MANDIR ?= $(PREFIX)/share/man
DOCDIR ?= $(PREFIX)/share/doc
ETCDIR ?= $(PREFIX)/etc

VERSION=$(shell cat VERSION)

ifneq (,$(DESTDIR))
SETUP_PY_EXTRA_ARG = --root=$(DESTDIR)
SETUP_PY_INSTALL_EXTRA_ARG = $(SETUP_PY_EXTRA_ARG)
endif

all: $(PROGS)

version.h: generate-version.sh
	./$<

scripts/kconfig/dumpconf scripts/kconfig/conf: FORCE
	$(MAKE) -f Makefile.kbuild $(@F)

picosat/libpicosat.a: FORCE
	cd picosat && ./configure -static -O
	$(MAKE) -C picosat libpicosat.a

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

undertaker/predator: FORCE
	$(MAKE) -C undertaker predator

undertaker/rsf2cnf: FORCE
	$(MAKE) -C undertaker rsf2cnf

tailor/undertaker-traceutil: FORCE
	$(MAKE) -C tailor undertaker-traceutil

ziz/zizler: FORCE
	$(MAKE) -C ziz zizler

%.1.gz: %.1
	gzip < $< > $@

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C python clean
	$(MAKE) -C tailor clean
	test ! -f picosat/makefile || $(MAKE) -C picosat clean
	rm -rf doc/*.gz
	@python setup.py clean

docs:
	$(MAKE) -C undertaker docs

check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C ziz $@
	$(MAKE) -C python $@
	$(MAKE) -C tailor $@

install: all $(MANPAGES)
	@install -d -v $(DESTDIR)$(BINDIR)
	@install -d -v $(DESTDIR)$(SBINDIR)
	@install -d -v $(DESTDIR)$(DOCDIR)
	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker
	@install -d -v $(DESTDIR)$(ETCDIR)/undertaker
	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker/tailor/ubuntu-boot
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker
	@install -d -v $(DESTDIR)$(DOCDIR)/undertaker/tailor
	@install -d -v $(DESTDIR)$(MANDIR)/man1

	@install -v python/undertaker-calc-coverage $(DESTDIR)$(BINDIR)
	@install -v python/undertaker-kconfigdump $(DESTDIR)$(BINDIR)
	@install -v python/vampyr-spatch-wrapper $(DESTDIR)$(BINDIR)
	@install -v python/fakecc $(DESTDIR)$(BINDIR)

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker

	@install -v tailor/undertaker-tailor $(DESTDIR)$(BINDIR)
	@install -v tailor/undertaker-tracecontrol-prepare $(DESTDIR)$(BINDIR)
	@install -v tailor/undertaker-tracecontrol $(DESTDIR)$(BINDIR)
	@install -v tailor/undertaker-traceutil $(DESTDIR)$(SBINDIR)
	@cp -v tailor/lists/* $(DESTDIR)$(ETCDIR)/undertaker/
	@cp -v tailor/HOWTO $(DESTDIR)$(DOCDIR)/undertaker/tailor/
	@cp -v tailor/README $(DESTDIR)$(DOCDIR)/undertaker/tailor/
	@cp -r -v tailor/boot/* $(DESTDIR)$(LIBDIR)/undertaker/tailor/ubuntu-boot

	@install -v undertaker/predator $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-coreboot-tree $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(BINDIR)
	@install -v undertaker/undertaker-busybox-tree $(DESTDIR)$(BINDIR)
	@install -v undertaker/rsf2cnf $(DESTDIR)$(BINDIR)

	@install -v ziz/zizler $(DESTDIR)$(BINDIR)

	@install -v scripts/Makefile.list $(DESTDIR)$(LIBDIR)
	@install -v scripts/Makefile.list_recursion $(DESTDIR)$(LIBDIR)
	@install -v scripts/Makefile.list_fiasco $(DESTDIR)$(LIBDIR)
	@install -v scripts/Makefile.version $(DESTDIR)$(LIBDIR)

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
