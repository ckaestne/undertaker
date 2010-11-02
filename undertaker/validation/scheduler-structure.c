
#ifdef CONFIG_GROUP_SCHED

  #ifdef CONFIG_CGROUP_SCHED
  #endif

  #ifdef CONFIG_FAIR_GROUP_SCHED
  #endif

  #ifdef CONFIG_RT_GROUP_SCHED
  #endif

  #ifdef CONFIG_USER_SCHED

     #ifdef CONFIG_FAIR_GROUP_SCHED
     #endif /* CONFIG_FAIR_GROUP_SCHED */

     #ifdef CONFIG_RT_GROUP_SCHED
     #endif /* CONFIG_RT_GROUP_SCHED */
     #else /* !CONFIG_USER_SCHED */
     #endif /* CONFIG_USER_SCHED */


    #ifdef CONFIG_FAIR_GROUP_SCHED
      #ifdef CONFIG_USER_SCHED
      #else /* !CONFIG_USER_SCHED */
      #endif /* CONFIG_USER_SCHED */
    #endif

    #ifdef CONFIG_USER_SCHED
    #elif defined(CONFIG_CGROUP_SCHED)
    #else
    #endif

    #ifdef CONFIG_FAIR_GROUP_SCHED
    #endif

    #ifdef CONFIG_RT_GROUP_SCHED
    #endif

#else

#endif  /* CONFIG_GROUP_SCHED */

/*
 * check-name: schedule structure
 * check-output-start
I: loaded rsf model for mn10300
I: loaded rsf model for avr32
I: loaded rsf model for parisc
I: loaded rsf model for frv
I: loaded rsf model for m32r
I: loaded rsf model for ia64
I: loaded rsf model for alpha
I: loaded rsf model for mips
I: loaded rsf model for blackfin
I: loaded rsf model for cris
I: loaded rsf model for sh
I: loaded rsf model for s390
I: loaded rsf model for m68k
I: loaded rsf model for arm
I: loaded rsf model for powerpc
I: loaded rsf model for h8300
I: loaded rsf model for xtensa
I: loaded rsf model for sparc
I: loaded rsf model for score
I: loaded rsf model for microblaze
I: loaded rsf model for tile
I: loaded rsf model for x86
I: found 23 rsf models
 * check-output-end
 */
