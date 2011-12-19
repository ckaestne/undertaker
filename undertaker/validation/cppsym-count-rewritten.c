#ifdef X

#undef X

#define X

#ifdef X
#endif

#endif

/*
 * check-name: cppsym: Count rewrites correctly without model
 * check-command: undertaker -j cppsym $file
 * check-output-start
X, 2, 2, NO_MODEL, NOT_CONFIG_LIKE
 * check-output-end
 */
