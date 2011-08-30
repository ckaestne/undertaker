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
 * check-command: undertaker -q $file
 * check-output-start
I: creating cppcheck-define.c.B2.code.globally.undead
I: creating cppcheck-define.c.B4.code.globally.dead
 * check-output-end
 */
