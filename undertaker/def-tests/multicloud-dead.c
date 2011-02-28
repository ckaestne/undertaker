#define FOO 1

#ifdef FOO
#define BAR
B1
#endif

#ifndef BAR
B4
#endif
/*
 * check-name: multicloud
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: creating multicloud-dead.c.B2.code.globally.undead
 * check-output-end
 */
