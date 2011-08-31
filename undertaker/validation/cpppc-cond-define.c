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
 * check-command: undertaker -q -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-cond-define.c
( B0 <-> X )
&& ( B1 <-> B0 && A )
&& ( B2 <-> B0 && B )
&& ( B3 <-> C.. )
&& (B1 -> !C.)
&& ((C  && !(B1)) -> C.)
&& ((C. && !(B1)) -> C )
&& (( B1 && !(B2)) -> !C..)
&& (B2 -> C..)
&& ((C.  && !(B1 || B2)) -> C..)
&& ((C.. && !(B1 || B2)) -> C. )
&& B00
 * check-output-end
 */
