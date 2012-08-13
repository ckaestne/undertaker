#ifdef CONFIG_SMP
#endif

#ifdef CONFIG_FOO
#endif

#ifdef HAVE_NUMA
#elif __SPARC__
#else
#endif

/*
 * check-name: CNF: cppsym: ensure status and type of detected symbols with x86 model
 * check-command: undertaker -v -j cppsym -m cnfmodels/x86.cnf $file
 * check-output-start
I: Using x86 as primary model
I: loaded cnf model for x86
CONFIG_FOO, 1, 0, MISSING, CONFIG_LIKE
CONFIG_SMP, 1, 0, PRESENT, BOOLEAN
HAVE_NUMA, 1, 0, MISSING, NOT_CONFIG_LIKE
__SPARC__, 1, 0, MISSING, NOT_CONFIG_LIKE
 * check-output-end
 */
