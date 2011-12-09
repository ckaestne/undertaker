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
Checking 2 configuration(s):
" -DCONFIG_A=1":	1 errors
"":	0 errors
   Not in all configs: coverage-1.c:4: warning: Using plain integer as NULL pointer
  in 1 configs. e.g. in " -DCONFIG_A=1"
Found 1 configuration dependent errors
 * check-output-end
 */
