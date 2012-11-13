#if 0
  some code
#else
  other code
#endif

#if 1
  some code
#else
  other code
#endif

/*
 * check-name: Coverage on trivial (#if 1 / #if 0) code with "-C simple"
 * check-command: undertaker -v -j coverage -C simple $file
 * check-output-start
I: Removed 0 leftovers for coverage-trivial-ifdefs-simple.c
I: coverage-trivial-ifdefs-simple.c, Found Solutions: 1, Coverage: 3/5 blocks enabled (60%)
 * check-output-end
 */

