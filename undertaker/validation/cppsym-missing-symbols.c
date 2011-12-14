#ifdef CONFIG_RT_GROUP_SCHED
#endif

#ifdef CONFIG_CGROUP_SCHED
#endif

#ifdef CONFIG_FAIR_GROUP_SCHED
#endif

#ifdef CONFIG_SPARC
#endif

#ifdef CONFIG_SND_HDA_INTEL
#endif

#ifdef CONFIG_SND_HDA_INTEL_MODULE
#endif

#if PAGE_SIZE % 512==0
#else
#endif

/*
 * check-name: cppsym: duplicate elimination with model and tristate symbols
 * check-command: undertaker -j cppsym -m models/x86.model $file
 * check-output-start
I: Using x86 as primary model
I: loaded rsf model for x86
0 (MISSING, NOT_CONFIG_LIKE)
512 (MISSING, NOT_CONFIG_LIKE)
CONFIG_CGROUP_SCHED (BOOLEAN)
CONFIG_FAIR_GROUP_SCHED (BOOLEAN)
CONFIG_RT_GROUP_SCHED (BOOLEAN)
CONFIG_SND_HDA_INTEL (TRISTATE)
CONFIG_SPARC (MISSING)
PAGE_SIZE (MISSING, NOT_CONFIG_LIKE)
 * check-output-end
 */
