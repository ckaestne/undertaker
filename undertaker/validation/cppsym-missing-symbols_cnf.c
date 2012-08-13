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
 * check-name: CNF: cppsym: duplicate elimination with model and tristate symbols
 * check-command: undertaker -v -j cppsym -m cnfmodels/x86.cnf $file
 * check-output-start
I: Using x86 as primary model
I: loaded cnf model for x86
CONFIG_CGROUP_SCHED, 1, 0, PRESENT, BOOLEAN
CONFIG_FAIR_GROUP_SCHED, 1, 0, PRESENT, BOOLEAN
CONFIG_RT_GROUP_SCHED, 1, 0, PRESENT, BOOLEAN
CONFIG_SND_HDA_INTEL, 2, 0, PRESENT, TRISTATE
CONFIG_SPARC, 1, 0, MISSING, CONFIG_LIKE
PAGE_SIZE, 1, 0, MISSING, NOT_CONFIG_LIKE
 * check-output-end
 */
