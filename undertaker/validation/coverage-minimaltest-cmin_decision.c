#if CONFIG_FOO
B0
#endif

/*
 * check-name: Coverage on a minimal testcase (Coverage-Mode: min_decision)
 * check-command: undertaker -v -j coverage -O combined -C min_decision $file
 * check-output-start
I: Removed 0 leftovers for coverage-minimaltest-cmin_decision.c
I: coverage-minimaltest-cmin_decision.c, Found Solutions: 2, Coverage: 3/3 blocks enabled (100%)
 * check-output-end
 */

