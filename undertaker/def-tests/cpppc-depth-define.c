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
I: CPP Precondition for cpppc-depth-define.c
( B0 <-> C )
&& ( B2 <-> A )
&& ( B4 <-> B2 && C. )
&& ( B6 <-> B2 && C. )
&& ( B9 <-> B2 && C.. )
&& (  ( (C) && !(B2) ) -> C. )
&& (  ( (C.)  && !(B2) ) -> C )
&& (  ( (C.) && !(B2 || B6) ) -> C.. )
&& (  ( (C..)  && !(B2 || B6) ) -> C. )
&& ( ( B6 && !( B2 ) ) -> C.. )
&& ( B2 -> !C.. )
&& ( B2 -> C. )
&& B00
 * check-output-end
 */
