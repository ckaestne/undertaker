#if CONFIG_FOO
B0
#endif

/*
 * check-name: Coverage on a minimal testcase (Coverage-Mode: simple)
 * check-command: undertaker -v -j coverage -O combined -C simple $file
 * check-output-start
I: Removed 0 leftovers for coverage-minimaltest-simple.c
I: coverage-minimaltest-simple.c, Found Solutions: 1, Coverage: 2/2 blocks enabled (100%)
 * check-output-end
 */

