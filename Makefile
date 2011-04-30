PROGS = scripts/kconfig/dumpconf undertaker/undertaker rsf2model/rsf2model
TEMPLATED = rsf2model/undertaker-kconfigdump
MANPAGES = doc/undertaker.1.gz doc/undertaker-linux-tree.1.gz doc/undertaker-kconfigdump.1.gz

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
MANDIR ?= $(PREFIX)/share/man

VERSION=$(shell cat VERSION)

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

%: %.in
	@echo "Template: $< -> $@"
	@sed "s#@LIBDIR@#${LIBDIR}#g" < $< > $@
	@chmod --reference="$<" $@

%.1.gz: %.1
	gzip < $< > $@

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C rsf2model clean
	@python setup.py clean
	@rm -f $(TEMPLATED)

docs:
	$(MAKE) -C undertaker docs

check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C rsf2model $@

install: all $(TEMPLATED) $(MANPAGES)
	@install -d -v $(DESTDIR)$(PREFIX)/bin

	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker 
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker
	@install -d -v $(DESTDIR)$(MANDIR)/man1

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker
	@install -v rsf2model/rsf2model $(DESTDIR)$(LIBDIR)/undertaker
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(LIBDIR)/undertaker

	@install -v undertaker/undertaker $(DESTDIR)$(PREFIX)/bin
	@install -v rsf2model/undertaker-kconfigdump $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-calc-coverage $(DESTDIR)$(PREFIX)/bin

	@install -v -m 0644 contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v -m 0644 -t $(DESTDIR)$(MANDIR)/man1 $(MANPAGES)

	@python setup.py install --prefix=$(PREFIX)

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
