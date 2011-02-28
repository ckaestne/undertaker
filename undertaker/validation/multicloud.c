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
 * check-command: undertaker $file
 * check-output-start
I: creating multicloud.c.B10.code.globally.dead
 * check-output-end
 */
