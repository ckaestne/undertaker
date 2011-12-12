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
I: Using x86 as primary model
I: loaded rsf model for x86
CONFIG_FOO (MISSING)
CONFIG_SMP (BOOLEAN)
HAVE_NUMA (MISSING, NOT_CONFIG_LIKE)
__SPARC__ (MISSING, NOT_CONFIG_LIKE)
 * check-output-end
 */
