
int main(void) {
        int a;
#ifdef CONFIG_A
#endif
        char *ptr = (char *)0;
        a = *ptr;
        return "ABC";
}

#include "include/clang-analyze-error.h"

/*
 * check-name: coverage: exclude messages from other files
 * check-command: vampyr -f bare -Csparse --exclude-others $file
 * check-output-start
coverage-3.c: Checking 1 configuration(s): coverage-3.c.config0
  ---- Found 1 messages with sparse in coverage-3.c ----
coverage-3.c:8: warning: incorrect type in return expression (different base types) | expected int | got char static *<noident> (in configs: coverage-3.c.config0)
  --------------------------------------------------
 * check-output-end
 */
