/*
 * check-name: FormatCommented on blocks without following code
 * check-command: undertaker -j coverage -O exec:cat $file
 * check-output-start
 * check-output-end
 *
 * Note: the segfault happens on files that end with a preprocessor directive
 */
#ifdef DUMMY
#endif

