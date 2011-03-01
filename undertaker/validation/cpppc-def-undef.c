#ifdef X

#undef X

#define X

#ifdef X
#endif

#endif
/*
 * check-name: Useless defines on the same depth
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-def-undef.c
( B0 <-> X )
&& ( B2 <->  ( B0 )  && X. )
&& (  ( (X) && !(B0) ) -> X. )
&& (  ( (X.)  && !(B0) ) -> X )
&& ( B0 -> X. )
&& B00
 * check-output-end
 */
