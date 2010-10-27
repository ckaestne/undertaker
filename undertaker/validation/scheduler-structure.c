
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
I: loaded rsf model for x86
I: found 1 rsf models
 * check-output-end
 */
