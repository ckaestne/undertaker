#ifdef CONFIG_A	//0
#elif CONFIG_B	//1
#elif CONFIG_C	//2
#else		//3
#endif
/*
 * check-name: Nested elifs
 * check-command: undertaker -j cpppc $file
 * check-output-start
( B0 <-> CONFIG_A )
&& ( B1 <-> CONFIG_B && ( ! (B0) ) )
&& ( B2 <-> CONFIG_C && ( ! (B1 || B0) ) )
&& ( B3 <-> ( ! (B2 || B1 || B0) ) )
&& B00
 * check-output-end
 */
