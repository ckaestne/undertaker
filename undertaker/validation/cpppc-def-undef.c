#ifdef X

#undef X

#define X

#ifdef X
#endif

#endif
/*
 * check-name: Define and undefine in same level
 * check-command: undertaker -j cpppc $file
 * check-output-start
( B0 <-> X )
&& (B0 -> !X.)
&& (B0 -> X..)
&& (!B0 -> (X <-> X.))
&& (!B0 -> (X. <-> X..))
&& B00
&& ( B1 <-> B0 && X.. )
 * check-output-end
 */
