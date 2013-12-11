#define FOO 1

#ifdef FOO
#define BAR
#endif

#ifndef BAR
int bar()
#endif

int main(void) {
    bar();
    return 0;
}

/*
 * check-name: multicloud
 * check-command: undertaker -v $file
 * check-output-start
I: creating multicloud-dead.c.B0.no_kconfig.globally.undead
I: creating multicloud-dead.c.B1.no_kconfig.globally.dead
 * check-output-end
 */
