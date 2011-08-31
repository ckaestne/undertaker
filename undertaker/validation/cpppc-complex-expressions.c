#if BITS_PER_LONG == 32 &&  defined( CONFIG_SMP )
#else
#endif
/*
 * check-name: Handle Comparators
 * check-command: undertaker -q -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-complex-expressions.c
( B0 <-> BITS_PER_LONG == 32 &&  ( CONFIG_SMP ) )
&& ( B1 <-> ( ! (B0) ) )
&& B00
 * check-output-end
 */
