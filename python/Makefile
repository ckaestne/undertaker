PYTHONFILES=golem rsf2model undertaker-calc-coverage vampyr $(wildcard vamos/*/*.py) $(wildcard vamos/*.py)

all:

pylint:
	-pylint --rcfile pylint.rc $(PYTHONFILES)

check: all
	$(MAKE) -s clean-check
	@for test in vamos/*/*_test.py vamos/*_test.py; do \
	   echo "Running python unittest $$test"; \
	   env PYTHONPATH=$(CURDIR) python "$$test"; \
	done
	cd validation && ./test-suite
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
                     \) -delete

FORCE:
.PHONY: all clean clean-check pylint FORCE
