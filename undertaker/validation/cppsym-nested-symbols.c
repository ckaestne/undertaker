#ifdef CONFIG_A
#ifdef CONFIG_B
#ifdef CONFIG_C
#endif
#endif
#endif

#ifdef CONFIG_A
#elif CONFIG_B
#else
#endif

/*
 * check-name: cppsym: duplicate elimination without a model
 * check-command: undertaker -j cppsym $file
 * check-output-start
CONFIG_A, 2, 0, NO_MODEL, CONFIG_LIKE
CONFIG_B, 2, 0, NO_MODEL, CONFIG_LIKE
CONFIG_C, 1, 0, NO_MODEL, CONFIG_LIKE
 * check-output-end
 */
