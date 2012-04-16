int main(int argc, char *argv[]) {

#ifdef CONFIG_A
    int i;
#else
    int i = 2;
#endif
    return i;
}

/*
 * check-name: coverage: call clang --analyze
 * check-command: vampyr -f bare -C clang $file
 * check-output-start
Checking 2 configuration(s):
" -DCONFIG_A=1":	1 errors
"":	0 errors
   Not in all configs: coverage-2.c:8:5: warning: Undefined or garbage value returned to caller
  in 1 configs. e.g. in " -DCONFIG_A=1"
Found 1 configuration dependent errors
 * check-output-end
 */
