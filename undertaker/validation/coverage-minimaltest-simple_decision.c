#if CONFIG_FOO
B0
#endif

/*
 * check-name: Coverage on a minimal testcase (Coverage-Mode: simple_decision)
 * check-command: undertaker -v -j coverage -O combined -C simple_decision $file
 * check-output-start
I: Removed 0 leftovers for coverage-minimaltest-simple_decision.c
I: coverage-minimaltest-simple_decision.c, Found Solutions: 2, Coverage: 3/3 blocks enabled (100%)
 * check-output-end
 */

