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
 * check-name: CNF: Coverage on trivial (#if 1 / #if 0) code with "-C cmin"
 * check-command: undertaker -v -j coverage -m epsilon.cnf -C min $file
 * check-output-start
I: loaded cnf model for epsilon
I: Using epsilon as primary model
I: Removed 0 leftovers for coverage-trivial-ifdefs-cmin_cnf.c
I: coverage-trivial-ifdefs-cmin_cnf.c, Found Solutions: 1, Coverage: 3/5 blocks enabled (60%)
 * check-output-end
 */

