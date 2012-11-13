#ifdef HAVE_SMP
#endif

#ifdef CONFIG_SMP
#define HAVE_SMP
#endif

#ifndef HAVE_SMP
#else
#endif

/*
 * check-name: cppsym: Count rewrites correctly with model
 * check-command: undertaker -j cppsym -m models/x86.model $file
 * check-output-start
CONFIG_SMP, 1, 0, PRESENT, BOOLEAN
HAVE_SMP, 2, 1, MISSING, NOT_CONFIG_LIKE
 * check-output-end
 */
