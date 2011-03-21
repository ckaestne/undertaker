PROGS = scripts/kconfig/dumpconf undertaker/undertaker rsf2model/rsf2model
TEMPLATED = rsf2model/undertaker-kconfigdump
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/undertaker: FORCE
	$(MAKE) -C undertaker undertaker

rsf2model/rsf2model: FORCE
	$(MAKE) -C rsf2model

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
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker
	@install -v rsf2model/rsf2model $(DESTDIR)$(LIBDIR)/undertaker
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(LIBDIR)/undertaker

	@install -v undertaker/undertaker $(DESTDIR)$(PREFIX)/bin
	@install -v rsf2model/undertaker-kconfigdump $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker-linux-tree $(DESTDIR)$(PREFIX)/bin

	@install -v contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker

dist: clean
	tar -C .. -czvf ../undertaker-$(VERSION).tar.gz $(shell basename `pwd`) --exclude=*.rsf --exclude=*.model \
		--exclude-vcs --exclude="*nfs*" --exclude="*git*" --exclude=*.tar.gz --exclude="*html*" \

undertaker-lcov:
	$(MAKE) -C undertaker run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
