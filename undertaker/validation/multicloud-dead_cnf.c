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
 * check-name: CNF: multicloud
 * check-command: undertaker -v -m epsilon.cnf $file
 * check-output-start
I: loaded cnf model for epsilon
I: Using epsilon as primary model
I: creating multicloud-dead_cnf.c.B0.code.no_kconfig.undead
I: creating multicloud-dead_cnf.c.B1.code.no_kconfig.dead
 * check-output-end
 */
