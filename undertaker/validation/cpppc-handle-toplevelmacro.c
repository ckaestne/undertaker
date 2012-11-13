#define both(a, b) ((a) && (b))
#if both(CONFIG_A, CONFIG_B)
#endif

#undef both
#define both(a, b) ((a) || (b))
#define foo_enabled() 1

#if XXXX
#undef both
#endif

#if both(CONFIG_A, CONFIG_B) && foo_enabled()
#endif
/*
 * check-name: Handle toplevel macros
 * check-command: undertaker -j cpppc $file
 * check-output-start
( B0 <-> ((CONFIG_A) && ( CONFIG_B)) )
&& ( B1 <-> XXXX )
&& ( B2 <-> both(CONFIG_A, CONFIG_B) && 1 )
&& B00
 * check-output-end
 */
