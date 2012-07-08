int main(void) {

#ifdef CONFIG_A
    void *p = 0;
#else
    void *p;
#endif

    return (int) p;
}

/*
 * check-name: coverage: signedness error in one configuration
 * check-command: vampyr -a -f bare -W whitelist -m /dev/null $file
 * check-output-start
Config coverage-5.c.config0 added 2 additional blocks
coverage report dumped to coverage-5.c.coverage
coverage-5.c: Covered 2/3 (2/3) blocks, 16/17 (16/17) lines, 0 (0) deads
 * check-output-end
 */
