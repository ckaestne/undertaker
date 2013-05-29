#if CONFIG_FOO
B0
#endif

/*
 * check-name: Coverage on a minimal testcase (Coverage-Mode: min)
 * check-command: undertaker -v -j coverage -O combined -C min $file
 * check-output-start
I: Removed 0 leftovers for coverage-minimaltest-cmin.c
I: coverage-minimaltest-cmin.c, Found Solutions: 1, Coverage: 2/2 blocks enabled (100%)
 * check-output-end
 */

