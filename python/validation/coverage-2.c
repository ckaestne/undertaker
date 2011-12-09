int main(char argc, char *argv[]) {

#ifdef CONFIG_A
    int i = 1;
#else
    float i = 2;
#endif
    return i;
}

/*
 * check-name: coverage: call clang --analyze
 * check-command: vampyr -f bare -C clang $file
 * check-output-start
Checking 2 configuration(s):
" -DCONFIG_A=1":	0 errors
"":	2 errors
Found 0 configuration dependent errors
 * check-output-end
 */
