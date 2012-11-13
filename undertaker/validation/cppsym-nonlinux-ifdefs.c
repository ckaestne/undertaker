#ifdef CONFIG_SMP
#endif

#ifdef CONFIG_FOO
#endif

#ifdef HAVE_NUMA
#elif __SPARC__
#else
#endif

/*
 * check-name: cppsym: ensure status and type of detected symbols with x86 model
 * check-command: undertaker -j cppsym -m models/x86.model $file
 * check-output-start
CONFIG_FOO, 1, 0, MISSING, CONFIG_LIKE
CONFIG_SMP, 1, 0, PRESENT, BOOLEAN
HAVE_NUMA, 1, 0, MISSING, NOT_CONFIG_LIKE
__SPARC__, 1, 0, MISSING, NOT_CONFIG_LIKE
 * check-output-end
 */
