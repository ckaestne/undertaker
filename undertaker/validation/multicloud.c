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
 * check-name: multicloud
 * check-command: undertaker -v $file
 * check-output-start
I: creating multicloud.c.B4.no_kconfig.globally.dead
 * check-output-end
 */
