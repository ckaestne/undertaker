#ifdef HAVE_SMP
#endif

#ifdef CONFIG_SMP
#define HAVE_SMP
#endif

#ifndef HAVE_SMP
#else
#endif

/*
 * check-name: CNF: cppsym: Count rewrites correctly with model
 * check-command: undertaker -v -j cppsym -m cnfmodels/x86.cnf $file
 * check-output-start
I: Using x86 as primary model
I: loaded cnf model for x86
CONFIG_SMP, 1, 0, PRESENT, BOOLEAN
HAVE_SMP, 2, 1, MISSING, NOT_CONFIG_LIKE
 * check-output-end
 */
