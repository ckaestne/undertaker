all:

check: all
	@$(MAKE) -s clean-check
	@for test in *_test.py; do python "$$test"; done
	cd validation && ./test-suite

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
.PHONY: all clean clean-check FORCE
