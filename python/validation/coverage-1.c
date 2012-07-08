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
 * check-command: vampyr -f bare -C sparse -Osparse,"-Wsparse,all" $file
 * check-output-start
coverage-1.c: Checking 2 configuration(s): coverage-1.c.config0, coverage-1.c.config1
  ---- Found 1 messages with sparse in coverage-1.c ----
coverage-1.c:4: warning: Using plain integer as NULL pointer (in configs: coverage-1.c.config0)
  --------------------------------------------------
 * check-output-end
 */
