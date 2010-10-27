#ifdef CONFIG_A
#ifdef CONFIG_B
#ifdef CONFIG_C
#endif
#endif
#endif
/*
 * check-name: Basic Example, 3 nested blocks
 * check-command: cpppc $file
 * check-output-start
( B0 <-> CONFIG_A )
& ( B1 <->  ( B0 )  & CONFIG_B )
& ( B2 <->  ( B1 )  & CONFIG_C )
 * check-output-end
 */
