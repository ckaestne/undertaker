#ifdef Z

#endif

#ifdef X
#define A

#endif


#ifdef W
#undef A

#endif

#ifdef W
#ifdef A
#endif
#endif


/*
 * check-name: CNF: multicloud
 * check-command: undertaker -v -m epsilon.cnf $file
 * check-output-start
I: loaded cnf model for epsilon
I: Using epsilon as primary model
I: creating multicloud_cnf.c.B4.no_kconfig.globally.dead
 * check-output-end
 */
