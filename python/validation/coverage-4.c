
int main(void) {
#ifdef CONFIG_A
#endif
        int a;
        return 42;
}

/*
 * We have to set the right LC_ALL enviroment variable to collect correct english
 * error messages. And we have to remove the '[-W<flag>]$' at the end of gcc-4.6
 * warning messages in order to get consistent test-suite result over different
 * versions.
 *
 * The sed expression in the new testcase is a result of different error message
 * formats in gcc4.4 and gcc4.6. The first lacks the column information, which is
 * provided by the second. The sed expression removes the column information for
 * this testcase to make it consistent.
 *
 * check-name: coverage: use correct localisation
 * hint: we need this sed expression to remove the column from gcc4.6
 *       messages to remain compatible with gcc4.4 in this unit-test
 * check-command: vampyr -f bare -Cgcc -Ogcc,-Wunused-variable  $file
 * check-output-start
coverage-4.c: Checking 1 configuration(s): coverage-4.c.config0
  ---- Found 1 messages with gcc in coverage-4.c ----
coverage-4.c:5: warning: unused variable 'a' (in configs: coverage-4.c.config0)
  --------------------------------------------------
 * check-output-end
 */
