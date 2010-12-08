#if defined ( CONFIG_A ) && ( defined CONFIG_B )
#elif defined ( CONFIG_A )
#endif
/*
 * check-name: Handle 'defined' keyword
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-handle-defined.c
( B0 <-> defined ( CONFIG_A ) && ( defined CONFIG_B ) )
&& ( B1 <-> defined ( CONFIG_A ) && ( ! (B0) )  )
 * check-output-end
 */
