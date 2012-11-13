#ifdef C
#endif 

#ifdef A


#define C

#ifdef C
#endif

#ifdef C
#define C
#endif


#undef C

#ifdef C
#endif

#endif
/*
 * check-name: Defines with the same depth and a cond block in between
 * check-command: undertaker -j cpppc $file
 * check-output-start
( B0 <-> C )
&& (B1 -> C.)
&& (B3 -> C..)
&& (B1 -> !C...)
&& (!B1 -> (C <-> C.))
&& (!B1 -> (C.. <-> C...))
&& (!B3 -> (C. <-> C..))
&& ( B1 <-> A )
&& B00
&& ( B3 <-> B1 && C. )
&& ( B2 <-> B1 && C. )
&& ( B4 <-> B1 && C... )
 * check-output-end
 */
