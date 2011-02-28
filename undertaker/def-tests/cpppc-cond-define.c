#ifdef X
B0

#ifdef A
B2
#undef C
#endif

#ifdef B
B5
#define C
#endif

#endif

#ifdef C
B9
#endif
/*
 * check-name: Complex Conditions
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-cond-define.c
( B0 <-> X )
&& ( B2 <->  ( B0 )  && A )
&& ( B5 <->  ( B0 )  && B )
&& ( B9 <-> C_ )
&& ( ( B2 & !( B5 ) ) -> !C_ )
&& ( B5 -> C_ )
&& ( C -> C_ )
 * check-output-end
 */
