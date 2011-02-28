#ifdef Z
B0
#endif

#ifdef X
#define A
B3
#endif


#ifdef W
#undef A
B6
#endif

#ifdef A
B9
#endif


/*
 * check-name: multicloud-variant-b
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: creating multicloud-dead.c.B2.code.globally.undead
 * check-output-end
 */
