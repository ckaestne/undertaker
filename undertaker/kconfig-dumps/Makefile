RSF2MODEL=../../rsf2model/rsf2model
RSF_FILES=$(shell echo kconfig-*)
MODELS=$(patsubst kconfig-%.rsf,models/%.model,$(RSF_FILES))

all: $(RSF2MODEL) $(MODELS)

models/%.model: kconfig-%.rsf
	@mkdir -p models
	$(RSF2MODEL) $< > $@

$(RSF2MODEL): FORCE
	$(MAKE) -s -C ../../rsf2model rsf2model

FORCE:

.PHONY: FORCE