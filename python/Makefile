PYTHONFILES=golem rsf2model undertaker-calc-coverage vampyr $(wildcard vamos/*/*.py) $(wildcard vamos/*.py)
SEARCHPATH="$(CURDIR)/../undertaker:$(CURDIR):$(PATH)"

all:

pylint:
	-pylint --rcfile pylint.rc $(PYTHONFILES)

check: all
	$(MAKE) -s clean-check
	@for test in vamos/*/*_test.py vamos/*_test.py; do \
	   echo "Running python unittest $$test"; \
	   env PATH="$(SEARCHPATH)" PYTHONPATH=$(CURDIR) python "$$test"; \
	done
	cd validation && env PATH="$(SEARCHPATH)" ./test-suite
	$(MAKE) -s pylint

clean: clean-check

clean-check:
	find validation/ \( -name "*.output.expected" \
                     -o -name "*.output.got" \
                     -o -name "*.output.diff" \
                     -o -name "*.error.expected" \
                     -o -name "*.error.diff" \
                     -o -name "*.error.got" \
                     -o -name "*.dead" \
                     -o -name "*.undead" \
                     -o -name "*.plist" \
                     \) -delete

FORCE:
.PHONY: all clean clean-check pylint FORCE
