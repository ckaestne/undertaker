
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
Checking 1 configuration(s):
" -DCONFIG_A=1":	1 errors
  ---- sparse: ----
coverage-3.c:8: warning: incorrect type in return expression (different base types) | expected int | got char static *<noident>
  in 1 configs. e.g. in " -DCONFIG_A=1"
  -------------
 * check-output-end
 */
