#ifdef CONFIG_A
#ifdef CONFIG_B
#ifdef CONFIG_C
#endif
#endif
#endif
/*
 * check-name: Basic Example, 3 nested blocks
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-nested-blocks.c
( B0 <-> CONFIG_A )
&& ( B1 <-> B0 && CONFIG_B )
&& ( B2 <-> B1 && CONFIG_C )
&& B00
 * check-output-end
 */
