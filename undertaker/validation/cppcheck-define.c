#ifdef FOO
#define BAR
#ifdef BAR
void bar() {
    return;
}
#else
int bar() {
    return 1;
}
#endif
#endif

int main(void) {
    bar();
    return 0;
}

/*
 * check-name: http://sourceforge.net/apps/trac/cppcheck/ticket/1467
 * check-command: undertaker -v $file
 * check-output-start
I: creating cppcheck-define.c.B1.no_kconfig.globally.undead
I: creating cppcheck-define.c.B2.no_kconfig.globally.dead
 * check-output-end
 */
