RSF2MODEL=../../rsf2model/rsf2model
RSF_FILES=$(shell echo kconfig-*)
MODELS=$(patsubst kconfig-%.rsf,models/%.model,$(RSF_FILES))

all: $(RSF2MODEL)
	@if [ "$(MODELS)" = "kconfig-*" ]; then echo  "\n\nPlease use git checkout for checking, no kconfig dumps found\n\n"; exit 1; fi
	@$(MAKE) build_models

build_models: $(MODELS)

models/%.model: kconfig-%.rsf
	@mkdir -p models
	$(RSF2MODEL) $< > $@


FORCE:

.PHONY: FORCE
