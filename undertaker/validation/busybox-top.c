#if ENABLE_FEATURE_TOP_SMP_PROCESS
#endif

#if ENABLE_FEATURE_TOPMEM
#endif

#if ENABLE_FEATURE_TOP_SMP_CPU
#endif

#if ENABLE_FEATURE_USE_TERMIOS
#endif

#if !ENABLE_FEATURE_TOP_CPU_USAGE_PERCENTAGE
#else
#endif

#if ENABLE_FEATURE_TOP_CPU_USAGE_PERCENTAGE
#    if !ENABLE_FEATURE_TOP_SMP_CPU
#    else
#    endif
#endif /* FEATURE_TOP_CPU_USAGE_PERCENTAGE */

#if ENABLE_FEATURE_TOP_CPU_GLOBAL_PERCENTS && ENABLE_FEATURE_TOP_DECIMALS
#endif

#if ENABLE_FEATURE_TOP_CPU_GLOBAL_PERCENTS
#    if ENABLE_FEATURE_TOP_SMP_CPU
#    endif

#    if ENABLE_FEATURE_TOP_DECIMALS
#    else
#    endif

#    if !ENABLE_FEATURE_TOP_SMP_CPU
#    else
#    endif

#    if ENABLE_FEATURE_TOP_SMP_CPU
#    else
#    endif
#else /* !ENABLE_FEATURE_TOP_CPU_GLOBAL_PERCENTS */
#endif

#if ENABLE_FEATURE_TOP_DECIMALS
#else
#endif

#if ENABLE_FEATURE_TOPMEM
#else
#endif /* TOPMEM */

#if ENABLE_FEATURE_TOP_CPU_USAGE_PERCENTAGE
#else
#endif

#if !ENABLE_FEATURE_USE_TERMIOS
#else
#    if ENABLE_FEATURE_TOP_CPU_USAGE_PERCENTAGE
#    endif

#    if ENABLE_FEATURE_SHOW_THREADS
#    endif

#    if ENABLE_FEATURE_TOP_CPU_USAGE_PERCENTAGE
#        if ENABLE_FEATURE_TOPMEM
#        endif

#        if ENABLE_FEATURE_TOP_SMP_CPU
#        endif
#    endif
#endif /* FEATURE_USE_TERMIOS */

/*
 * check-name: coverage analysis for busybox top
 * check-command: undertaker -q -j coverage -O stdout -m  busybox-top.model $file | grep -v '^ENABLE'
 * check-output-start
I: Set configuration space regex to '^(ENABLE_|CONFIG_)[^ ]*$'
I: loaded rsf model for busybox-top.model
I: Using busybox-top.model as primary model
I: Found 6 assignments
I: In all assignments the following symbols are equally set
CONFIG_FEATURE_SHOW_THREADS=1
CONFIG_FEATURE_TOP_SMP_PROCESS=1
CONFIG_TOP=1
I: All differences in the assignments
I: Config 0
CONFIG_FEATURE_TOPMEM=1
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=1
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=1
CONFIG_FEATURE_TOP_DECIMALS=1
CONFIG_FEATURE_TOP_SMP_CPU=1
CONFIG_FEATURE_USE_TERMIOS=1
I: Config 1
CONFIG_FEATURE_TOPMEM=1
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=0
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=0
CONFIG_FEATURE_TOP_DECIMALS=0
CONFIG_FEATURE_TOP_SMP_CPU=0
CONFIG_FEATURE_USE_TERMIOS=1
I: Config 2
CONFIG_FEATURE_TOPMEM=1
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=0
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=1
CONFIG_FEATURE_TOP_DECIMALS=0
CONFIG_FEATURE_TOP_SMP_CPU=0
CONFIG_FEATURE_USE_TERMIOS=1
I: Config 3
CONFIG_FEATURE_TOPMEM=1
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=1
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=1
CONFIG_FEATURE_TOP_DECIMALS=0
CONFIG_FEATURE_TOP_SMP_CPU=0
CONFIG_FEATURE_USE_TERMIOS=1
I: Config 4
CONFIG_FEATURE_TOPMEM=0
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=1
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=1
CONFIG_FEATURE_TOP_DECIMALS=0
CONFIG_FEATURE_TOP_SMP_CPU=0
CONFIG_FEATURE_USE_TERMIOS=1
I: Config 5
CONFIG_FEATURE_TOPMEM=0
CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS=1
CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE=1
CONFIG_FEATURE_TOP_DECIMALS=0
CONFIG_FEATURE_TOP_SMP_CPU=0
CONFIG_FEATURE_USE_TERMIOS=0
 * check-output-end
 */
