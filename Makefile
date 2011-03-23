PROGS = scripts/kconfig/dumpconf undertaker/undertaker rsf2model/rsf2model
TEMPLATED = rsf2model/undertaker-kconfigdump
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
PYTHONLIBDIR ?= $(LIBDIR)/python2.6/dist-packages
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

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker clean
	$(MAKE) -C ziz clean
	$(MAKE) -C rsf2model clean
	@rm -f $(TEMPLATED)

docs:
	$(MAKE) -C undertaker docs

check:
	$(MAKE) -C undertaker $@
	$(MAKE) -C rsf2model $@

install: all $(TEMPLATED)
	@install -d -v $(DESTDIR)$(PREFIX)/bin

	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker 
	@install -d -v $(DESTDIR)$(PYTHONLIBDIR)/undertaker 
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker
	@install -v rsf2model/rsf2model $(DESTDIR)$(LIBDIR)/undertaker
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(LIBDIR)/undertaker

	@install -v undertaker/undertaker $(DESTDIR)$(PREFIX)/bin
	@install -v rsf2model/undertaker-kconfigdump $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(PREFIX)/bin

	@install -v -m 0644 contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v -t $(DESTDIR)$(PYTHONLIBDIR)/undertaker \
		rsf2model/undertaker/__init__.py \
		rsf2model/undertaker/BoolRewriter.py \
		rsf2model/undertaker/RsfReader.py \
		rsf2model/undertaker/TranslatedModel.py \
		rsf2model/undertaker/tools.py \


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
