#ifdef CONFIG_RT_GROUP_SCHED
#endif

#ifdef CONFIG_CGROUP_SCHED

#ifdef CONFIG_FAIR_GROUP_SCHED
#endif

#ifdef CONFIG_RT_GROUP_SCHED
#endif

#define root_task_group init_task_group

#ifdef CONFIG_FAIR_GROUP_SCHED

#ifdef CONFIG_SMP
#endif

# define INIT_TASK_GROUP_LOAD	NICE_0_LOAD
#define MIN_SHARES	2
#define MAX_SHARES	(1UL << 18)
#endif

#endif	/* CONFIG_CGROUP_SCHED */
#ifdef CONFIG_FAIR_GROUP_SCHED
#ifdef CONFIG_SMP
#endif
#endif
#if defined CONFIG_SMP || defined CONFIG_RT_GROUP_SCHED
#ifdef CONFIG_SMP
#endif
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_SMP
#endif
#endif
#ifdef CONFIG_NO_HZ
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_SCHED_HRTICK
#ifdef CONFIG_SMP
#endif
#endif
#ifdef CONFIG_SCHEDSTATS
#endif
#ifdef CONFIG_SMP
#else
#endif

#ifdef CONFIG_CGROUP_SCHED
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#else /* CONFIG_CGROUP_SCHED */
#endif /* CONFIG_CGROUP_SCHED */
#ifdef CONFIG_SCHED_DEBUG
# define const_debug __read_mostly
#else
# define const_debug static const
#endif
#undef SCHED_FEAT
#undef SCHED_FEAT
#ifdef CONFIG_SCHED_DEBUG
#undef SCHED_FEAT
#endif
#define sched_feat(x) (sysctl_sched_features & (1UL << __SCHED_FEAT_##x))
#ifndef prepare_arch_switch
# define prepare_arch_switch(next)	do { } while (0)
#endif
#ifndef finish_arch_switch
# define finish_arch_switch(prev)	do { } while (0)
#endif
#ifndef __ARCH_WANT_UNLOCKED_CTXSW
#ifdef CONFIG_DEBUG_SPINLOCK
#endif
#else /* __ARCH_WANT_UNLOCKED_CTXSW */
#ifdef CONFIG_SMP
#else
#endif
#ifdef CONFIG_SMP
#endif
#ifdef __ARCH_WANT_INTERRUPTS_ON_CTXSW
#else
#endif
#ifdef CONFIG_SMP
#endif
#ifndef __ARCH_WANT_INTERRUPTS_ON_CTXSW
#endif
#endif /* __ARCH_WANT_UNLOCKED_CTXSW */
#ifdef CONFIG_SCHED_HRTICK
#ifdef CONFIG_SMP
#else
#endif /* CONFIG_SMP */
#ifdef CONFIG_SMP
#endif
#else	/* CONFIG_SCHED_HRTICK */
#endif	/* CONFIG_SCHED_HRTICK */
#ifdef CONFIG_SMP
#ifndef tsk_is_polling
#define tsk_is_polling(t) test_tsk_thread_flag(t, TIF_POLLING_NRFLAG)
#endif
#ifdef CONFIG_NO_HZ
#endif /* CONFIG_NO_HZ */
#else /* !CONFIG_SMP */
#endif /* CONFIG_SMP */
#if BITS_PER_LONG == 32
# define WMULT_CONST	(~0UL)
#else
# define WMULT_CONST	(1UL << 32)
#endif
#define WMULT_SHIFT	32
#define SRR(x, y) (((x) + (1UL << ((y) - 1))) >> (y))
#define WEIGHT_IDLEPRIO                3
#define WMULT_IDLEPRIO         1431655765
#ifdef CONFIG_CGROUP_CPUACCT
#else
#endif
#if (defined(CONFIG_SMP) && defined(CONFIG_FAIR_GROUP_SCHED)) || defined(CONFIG_RT_GROUP_SCHED)
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_FAIR_GROUP_SCHED
#else
#endif
#ifdef CONFIG_PREEMPT
#else
#endif /* CONFIG_PREEMPT */
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#ifdef CONFIG_SMP
#endif
#endif
#ifdef CONFIG_SMP
#endif
#define sched_class_highest (&rt_sched_class)
#ifdef CONFIG_SCHED_DEBUG
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_SCHED_DEBUG
#endif
#endif /* CONFIG_SMP */
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_SCHEDSTATS
#endif /* CONFIG_SCHEDSTATS */
#endif /* CONFIG_SMP */
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_SCHEDSTATS
#endif
#ifdef CONFIG_PREEMPT_NOTIFIERS
#endif
#if defined(CONFIG_SCHEDSTATS) || defined(CONFIG_TASK_DELAY_ACCT)
#endif
#if defined(CONFIG_SMP) && defined(__ARCH_WANT_UNLOCKED_CTXSW)
#endif
#ifdef CONFIG_PREEMPT
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_PREEMPT_NOTIFIERS
#else /* !CONFIG_PREEMPT_NOTIFIERS */
#endif /* CONFIG_PREEMPT_NOTIFIERS */
#ifdef __ARCH_WANT_INTERRUPTS_ON_CTXSW
#endif /* __ARCH_WANT_INTERRUPTS_ON_CTXSW */
#ifdef __ARCH_WANT_INTERRUPTS_ON_CTXSW
#endif /* __ARCH_WANT_INTERRUPTS_ON_CTXSW */
#ifdef CONFIG_SMP
#else
#endif
#ifdef __ARCH_WANT_UNLOCKED_CTXSW
#endif
#ifndef __ARCH_WANT_UNLOCKED_CTXSW
#endif
#ifdef CONFIG_NO_HZ
#else
#endif
#ifdef CONFIG_SMP
#endif
#ifndef CONFIG_VIRT_CPU_ACCOUNTING
#endif
#ifdef CONFIG_VIRT_CPU_ACCOUNTING
#else
#ifndef nsecs_to_cputime
# define nsecs_to_cputime(__nsecs)	nsecs_to_jiffies(__nsecs)
#endif
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_DEBUG_PREEMPT
#endif
#ifdef CONFIG_DEBUG_PREEMPT
#endif
#ifdef CONFIG_DEBUG_PREEMPT
#endif
#endif
#ifdef CONFIG_SCHEDSTATS
#endif
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
#ifdef CONFIG_DEBUG_PAGEALLOC
#else
#endif
#endif
#ifdef CONFIG_PREEMPT
#endif /* CONFIG_PREEMPT */
#ifdef CONFIG_RT_MUTEXES
#endif
#ifdef __ARCH_WANT_SYS_NICE
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#if BITS_PER_LONG == 32
#else
#endif
#ifdef CONFIG_DEBUG_STACK_USAGE
#endif
#if BITS_PER_LONG == 32
#else
#endif
#ifdef CONFIG_SCHED_DEBUG
#endif
#if defined(CONFIG_SMP) && defined(__ARCH_WANT_UNLOCKED_CTXSW)
#endif
#if defined(CONFIG_PREEMPT)
#else
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_HOTPLUG_CPU
#endif /* CONFIG_HOTPLUG_CPU */
#if defined(CONFIG_SCHED_DEBUG) && defined(CONFIG_SYSCTL)
#else
#endif
#ifdef CONFIG_HOTPLUG_CPU
#endif
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_SCHED_DEBUG
#else /* !CONFIG_SCHED_DEBUG */
# define sched_domain_debug(sd, cpu) do { } while (0)
#endif /* CONFIG_SCHED_DEBUG */
#define SD_NODES_PER_DOMAIN 16
#ifdef CONFIG_NUMA
#endif /* CONFIG_NUMA */
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_SCHED_SMT
#endif /* CONFIG_SCHED_SMT */
#ifdef CONFIG_SCHED_MC
#endif /* CONFIG_SCHED_MC */
#if defined(CONFIG_SCHED_MC) && defined(CONFIG_SCHED_SMT)
#elif defined(CONFIG_SCHED_MC)
#endif
#ifdef CONFIG_SCHED_MC
#elif defined(CONFIG_SCHED_SMT)
#else
#endif
#ifdef CONFIG_NUMA
#endif /* CONFIG_NUMA */
#ifdef CONFIG_NUMA
#else /* !CONFIG_NUMA */
#endif /* CONFIG_NUMA */
#ifdef CONFIG_SCHED_DEBUG
#else
#endif
#define	SD_INIT(sd, type)	sd_init_##type(sd)
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_MC
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_SCHED_MC
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_MC
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_MC
#endif
#ifdef CONFIG_NUMA
#endif
#ifdef CONFIG_SCHED_SMT
#elif defined(CONFIG_SCHED_MC)
#else
#endif
#if defined(CONFIG_SCHED_MC) || defined(CONFIG_SCHED_SMT)
#ifdef CONFIG_SCHED_MC
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_SMT
#endif
#ifdef CONFIG_SCHED_MC
#endif
#endif /* CONFIG_SCHED_MC || CONFIG_SCHED_SMT */
#ifndef CONFIG_CPUSETS
#endif
#if defined(CONFIG_NUMA)
#endif
#ifndef CONFIG_CPUSETS
#endif
#else
#endif /* CONFIG_SMP */
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#if defined CONFIG_SMP || defined CONFIG_RT_GROUP_SCHED
#ifdef CONFIG_SMP
#endif
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#ifdef CONFIG_CPUMASK_OFFSTACK
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
#endif /* CONFIG_RT_GROUP_SCHED */
#ifdef CONFIG_CPUMASK_OFFSTACK
#endif /* CONFIG_CPUMASK_OFFSTACK */
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif /* CONFIG_RT_GROUP_SCHED */
#ifdef CONFIG_CGROUP_SCHED
#endif /* CONFIG_CGROUP_SCHED */
#if defined CONFIG_FAIR_GROUP_SCHED && defined CONFIG_SMP
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#ifdef CONFIG_CGROUP_SCHED
#endif
#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
#ifdef CONFIG_CGROUP_SCHED
#endif
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_PREEMPT_NOTIFIERS
#endif
#ifdef CONFIG_SMP
#endif
#ifdef CONFIG_RT_MUTEXES
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_NO_HZ
#endif
#endif /* SMP */
#ifdef CONFIG_DEBUG_SPINLOCK_SLEEP
#ifdef in_atomic
#endif
#endif
#ifdef CONFIG_MAGIC_SYSRQ
#ifdef CONFIG_SCHEDSTATS
#endif
#endif /* CONFIG_MAGIC_SYSRQ */
#if defined(CONFIG_IA64) || defined(CONFIG_KGDB_KDB)
#endif /* defined(CONFIG_IA64) || defined(CONFIG_KGDB_KDB) */
#ifdef CONFIG_IA64
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#else /* !CONFG_FAIR_GROUP_SCHED */
#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
#else /* !CONFIG_RT_GROUP_SCHED */
#endif /* CONFIG_RT_GROUP_SCHED */
#ifdef CONFIG_CGROUP_SCHED
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#endif /* CONFIG_CGROUP_SCHED */
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#else /* !CONFIG_RT_GROUP_SCHED */
#endif /* CONFIG_RT_GROUP_SCHED */
#ifdef CONFIG_CGROUP_SCHED
#ifdef CONFIG_RT_GROUP_SCHED
#else
#endif
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
#endif /* CONFIG_RT_GROUP_SCHED */
#ifdef CONFIG_FAIR_GROUP_SCHED
#endif
#ifdef CONFIG_RT_GROUP_SCHED
#endif
#endif	/* CONFIG_CGROUP_SCHED */
#ifdef CONFIG_CGROUP_CPUACCT
#ifndef CONFIG_64BIT
#else
#endif
#ifndef CONFIG_64BIT
#else
#endif
#ifdef CONFIG_SMP
#define CPUACCT_BATCH	SOMETHING
#else
#define CPUACCT_BATCH	0
#endif
#endif	/* CONFIG_CGROUP_CPUACCT */
#ifndef CONFIG_SMP
#else /* #ifndef CONFIG_SMP */
#endif /* #else #ifndef CONFIG_SMP */

/*
 * check-name: structure of kernel/sched.c from linux 2.6.35 (edited)
 * check-command: undertaker -w /dev/null $file
 * check-output-start
I: loaded rsf model for alpha
I: loaded rsf model for arm
I: loaded rsf model for avr32
I: loaded rsf model for blackfin
I: loaded rsf model for cris
I: loaded rsf model for frv
I: loaded rsf model for h8300
I: loaded rsf model for ia64
I: loaded rsf model for m32r
I: loaded rsf model for m68k
I: loaded rsf model for m68knommu
I: loaded rsf model for microblaze
I: loaded rsf model for mips
I: loaded rsf model for mn10300
I: loaded rsf model for parisc
I: loaded rsf model for powerpc
I: loaded rsf model for s390
I: loaded rsf model for score
I: loaded rsf model for sh
I: loaded rsf model for sparc
I: loaded rsf model for tile
I: loaded rsf model for x86
I: loaded rsf model for xtensa
I: found 23 rsf models
I: loaded 0 items to whitelist
I: creating scheduler-structure.c.B21.x86.code.globally.undead
I: creating scheduler-structure.c.B124.x86.missing.dead
I: creating scheduler-structure.c.B236.x86.kconfig.globally.undead
I: creating scheduler-structure.c.B239.x86.kconfig.globally.undead
I: creating scheduler-structure.c.B254.x86.missing.dead
 * check-output-end
 */
