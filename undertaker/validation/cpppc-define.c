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
( B0 <-> CONFIG_A )
&& ( B1 <-> CONFIG_C. )
&& (B0 -> CONFIG_C.)
&& (!B0 -> (CONFIG_C <-> CONFIG_C.))
&& (!B00 -> (BUFFER <-> BUFFER.))
&& (!B00 -> (X <-> X.))
&& (!B00 -> (macro <-> macro.))
&& (B00 -> !X.)
&& (B00 -> BUFFER.)
&& (B00 -> macro.)
&& B00
 * check-output-end
 */
