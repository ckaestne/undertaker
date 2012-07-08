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
coverage-2.c: Checking 2 configuration(s): coverage-2.c.config0, coverage-2.c.config1
  ---- Found 1 messages with clang in coverage-2.c ----
coverage-2.c:8:5: warning: Undefined or garbage value returned to caller (in configs: coverage-2.c.config0)
  --------------------------------------------------
 * check-output-end
 */
