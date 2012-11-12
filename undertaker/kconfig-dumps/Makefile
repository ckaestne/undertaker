RSF2MODEL=../../python/rsf2model
RSF2CNF=../../undertaker/rsf2cnf
RSF_FILES=$(shell echo kconfig-*)
MODELS=$(patsubst kconfig-%.rsf,models/%.model,$(RSF_FILES))
CNF_MODELS=$(patsubst kconfig-%.rsf,cnfmodels/%.cnf,$(RSF_FILES))

all: $(RSF2MODEL) $(RSF2CNF)
	@if [ "$(MODELS)" = "kconfig-*" ]; then echo  "\n\nPlease use git checkout for checking, no kconfig dumps found\n\n"; exit 1; fi
	@$(MAKE) build_models

build_models: $(MODELS) $(CNF_MODELS)

models/%.model: kconfig-%.rsf
	@mkdir -p models
	@cp -lv $< models/$*.rsf
	$(RSF2MODEL) $< > $@

cnfmodels/%.cnf: models/%.model kconfig-%.rsf
	@mkdir -p cnfmodels
	$(RSF2CNF) -m models/$*.model -r models/$*.rsf >$@

FORCE:

.PHONY: FORCE
