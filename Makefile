
# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoids hard-to-debug behaviour);
# o  print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
  endif
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# Cancel implicit rules on top Makefile
$(CURDIR)/Makefile Makefile: ;

# That's our default target when none is given on the command line
PHONY := _all
_all:

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

# Make variables (CC, etc...)
HOSTCC       = gcc
HOSTCXX      = g++
HOSTCFLAGS   = -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCXXFLAGS = -O2

export HOSTCC HOSTCXX HOSTCFLAGS HOSTCXXFLAGS

KBUILD_KCONFIG = $(CURDIR)/fm/main.fm
export KBUILD_KCONFIG

srctree		:= $(CURDIR)
objtree		:= $(CURDIR)
src		:= $(srctree)
obj		:= $(objtree)

export srctree objtree

# We need some generic definitions (do not try to remake the file).
$(srctree)/scripts/Kbuild.include: ;
include $(srctree)/scripts/Kbuild.include

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

undertaker: FORCE
	$(Q)$(MAKE) -C undertaker $@ MAKEFLAGS=

config: scripts_basic FORCE
	$(Q)mkdir -p include/linux include/config
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

dumpconf: scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@
	scripts/kconfig/dumpconf $(KBUILD_KCONFIG)

clean:
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name 'Module.markers' -o -name '.tmp_*.o.*' \) \
		-type f -print | xargs rm -f -v

PHONY += FORCE
FORCE:

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
