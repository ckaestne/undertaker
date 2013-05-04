SATYR=$(CURDIR)/../undertaker/satyr
CONF=$(CURDIR)/../scripts/kconfig/conf
MODELS=$(wildcard *.fm)
CNFs=$(MODELS:.fm=.cnf)
ALLYESCONFIGS=$(MODELS:.fm=.allyesconfig)
ALLMODCONFIGS=$(MODELS:.fm=.allmodconfig)
ALLNOCONFIGS=$(MODELS:.fm=.allnoconfig)
ALLDEFCONFIGS=$(MODELS:.fm=.alldefconfig)
ALLCONFIGS=$(ALLYESCONFIGS) $(ALLNOCONFIGS) $(ALLMODCONFIGS) $(ALLDEFCONFIGS)

all: check

$(SATYR):
	$(MAKE) -s -C $(@D) $(@F)

$(CONF):
	$(MAKE) -s -C .. $(@F)

%.cnf: %.fm
	$(SATYR) $< -c $@

%.allyesconfig: %.fm
	env KCONFIG_CONFIG=$@ $(CONF) $< --allyesconfig > /dev/null || (echo "Calculation of $@ failed" && exit 1)
	$(SATYR) $< -a $@ 2> /dev/null || (echo "call of satyr on $@ failed" && exit 1)

%.allnoconfig: %.fm
	env KCONFIG_CONFIG=$@ $(CONF) $< --allnoconfig > /dev/null || (echo "Calculation of $@ failed" && exit 1)
	$(SATYR) $< -a $@ 2> /dev/null || (echo "call of satyr on $@ failed" && exit 1)

%.allmodconfig: %.fm
	env KCONFIG_CONFIG=$@ $(CONF) $< --allmodconfig > /dev/null || (echo "Calculation of $@ failed" && exit 1)
	$(SATYR) $< -a $@ 2> /dev/null || (echo "call of satyr on $@ failed" && exit 1)

%.alldefconfig: %.fm
	env KCONFIG_CONFIG=$@ $(CONF) $< --alldefconfig > /dev/null || (echo "Calculation of $@ failed" && exit 1)
	$(SATYR) $< -a $@ 2> /dev/null || (echo "call of satyr on $@ failed" && exit 1)

check: $(SATYR) $(CONF) $(ALLCONFIGS)

clean:
	rm -rf $(CNFs) $(ALLCONFIGS) *.*.old


.PHONY: check clean all
