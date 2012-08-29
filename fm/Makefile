SATYR=$(CURDIR)/../undertaker/satyr
CONF=$(CURDIR)/../scripts/kconfig/conf
MODELS=$(wildcard *.fm)
CNFs=$(MODELS:.fm=.cnf)
ALLYESCONFIGS=$(MODELS:.fm=.allyesconfig)
ALLMODCONFIGS=$(MODELS:.fm=.allmodconfig)
ALLNOCONFIGS=$(MODELS:.fm=.allnoconfig)
ALLDEFCONFIGS=$(MODELS:.fm=.alldefconfig)
ALLCONFIGS=$(ALLYESCONFIGS) $(ALLNOCONFIGS) $(ALLMODCONFIGS) $(ALLDEFCONFIGS)

all: $(SATYR)
	$(MAKE) check

$(SATYR):
	$(MAKE) -s -C $(@D) $(@F)

$(CONF):
	$(MAKE) -s -C .. $(@F)

%.cnf: %.fm
	$(SATYR) $< -c $@

%.allyesconfig: %.fm $(CONF)
	env KCONFIG_CONFIG=$@ $(CONF) $< --allyesconfig
	$(SATYR) $< -a $@

%.allnoconfig: %.fm $(CONF)
	env KCONFIG_CONFIG=$@ $(CONF) $< --allnoconfig
	$(SATYR) $< -a $@

%.allmodconfig: %.fm $(CONF)
	env KCONFIG_CONFIG=$@ $(CONF) $< --allmodconfig
	$(SATYR) $< -a $@

%.alldefconfig: %.fm $(CONF)
	env KCONFIG_CONFIG=$@ $(CONF) $< --alldefconfig
	$(SATYR) $< -a $@

check: $(ALLCONFIGS)
	@echo $@ checking $<

clean:
	rm -rf $(CNFs) $(ALLCONFIGS) *.*.old


.PHONY: check clean all
