#ifdef FOO
B0
#define BAR
#ifdef BAR
B2
#else
B4
#endif
#endif
/*
 * check-name: http://sourceforge.net/apps/trac/cppcheck/ticket/1467
 * check-command: undertaker $file
 * check-output-start
I: creating cppcheck-define.c.B2.code.globally.undead
I: creating cppcheck-define.c.B4.code.globally.dead
 * check-output-end
 */
