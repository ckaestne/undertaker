#ifdef CONFIG_A		//0
# ifdef CONFIG_B	//1
# else CONFIG_C		//2
# endif
#elif CONFIG_B		//3
#elif CONFIG_C		//4
#else			//5
# ifdef CONFIG_D	//6
# elif CONFIG_A		//7
#  ifdef CONFIG_E	//8
#  endif
# else CONFIG_B		//9
# endif
#endif
/*
 * check-name: Complex Conditions
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-complex.c
( B0 <-> CONFIG_A )
&& ( B1 <->  ( B0 )  && CONFIG_B )
&& ( B2 <->  ( B0 )  && CONFIG_C && ( ! (B1) )  )
&& ( B3 <-> CONFIG_B && ( ! (B0) )  )
&& ( B4 <-> CONFIG_C && ( ! (B3 || B0) )  )
&& ( B5 <-> ( ! (B4 || B3 || B0) )  )
&& ( B6 <->  ( B5 )  && CONFIG_D )
&& ( B7 <->  ( B5 )  && CONFIG_A && ( ! (B6) )  )
&& ( B8 <->  ( B7 )  && CONFIG_E )
&& ( B9 <->  ( B5 )  && CONFIG_B && ( ! (B7 || B6) )  )
 * check-output-end
 */
