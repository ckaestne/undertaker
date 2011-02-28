#ifdef X

#ifdef A
#undef C
#endif

#ifdef B
#define C
#endif

#endif

#ifdef C
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
&& (  ( (C) && !(B5 | B2) ) -> C_ )
&& (  ( (C_)  && !(B5 | B2) ) -> C )
&& ( ( B2 && !( B5 ) ) -> !C_ )
&& ( B5 -> C_ )
&& B00
 * check-output-end
 */
