#ifdef X

#undef X

#define X

#ifdef X
#endif

#endif
/*
 * check-name: Define and undefine in same level
 * check-command: undertaker -q -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-def-undef.c
( B0 <-> X )
&& (B0 -> !X.)
&& ((X  && !(B0)) -> X.)
&& ((X. && !(B0)) -> X )
&& (( B0 && !(B0)) -> !X..)
&& (B0 -> X..)
&& ((X.  && !(B0)) -> X..)
&& ((X.. && !(B0)) -> X. )
&& B00
&& ( B2 <-> B0 && X.. )
 * check-output-end
 */
