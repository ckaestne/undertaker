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
&& ( B2 <-> B0 && A )
&& ( B5 <-> B0 && B )
&& ( B9 <-> C.. )
&& (B2 -> !C.)
&& ((C  && !(B2)) -> C.)
&& ((C. && !(B2)) -> C )
&& (( B2 && !(B5)) -> !C..)
&& (B5 -> C..)
&& ((C.  && !(B2 || B5)) -> C..)
&& ((C.. && !(B2 || B5)) -> C. )
&& B00
 * check-output-end
 */
