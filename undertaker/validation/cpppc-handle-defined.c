#if defined ( CONFIG_A ) && ( defined CONFIG_B )
#elif defined ( CONFIG_A )
#endif
/*
 * check-name: Handle 'defined' keyword
 * check-command: cpppc $file
 * check-output-start
( B0 <-> defined ( CONFIG_A ) && ( defined CONFIG_B ) )
&& ( B1 <-> defined ( CONFIG_A ) && ( ! (B0) )  )
 * check-output-end
 */
