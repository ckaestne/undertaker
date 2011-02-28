#undef X
#define BUFFER 1000
#define macro \
1000
#ifdef CONFIG_A	
#define CONFIG_C
#endif
#ifdef CONFIG_C
#endif
/*
 * check-name: Complex Conditions
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-define.c
( B1 <-> CONFIG_A )
&& ( B3 <-> CONFIG_C_ )
&& (  ( (CONFIG_C) && !(B1) ) -> CONFIG_C_ )
&& (  ( (CONFIG_C_)  && !(B1) ) -> CONFIG_C )
&& ( B1 -> CONFIG_C_ )
&& B00
 * check-output-end
 */
