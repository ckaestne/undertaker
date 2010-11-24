#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
#else
#endif
/*
 * check-name: Handle Comparators
 * check-command: cpppc $file
 * check-output-start
( B0 <-> BITS_PER_LONG == 32 && defined ( CONFIG_SMP ) )
&& ( B1 <-> ( ! (B0) )  )
 * check-output-end
 */
