PYTHONFILES=rsf2model undertaker-calc-coverage $(wildcard vamos/*/*.py) $(wildcard vamos/*.py)

all:

pylint:
	-pylint --rcfile pylint.rc $(PYTHONFILES)

check: all
	$(MAKE) -s clean-check
	@for test in *_test.py; do python "$$test"; done
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
