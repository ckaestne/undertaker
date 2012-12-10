#define CONFIG_FOO 0

#if CONFIG_FOO
#endif

#define CONFIG_BOO 1

#if CONFIG_BOO
#endif

/*
 * check-name: Transform #define FOO 0 -> #undef FOO
 * check-command: undertaker -j cpppc $file
 * check-output-start
( B0 <-> CONFIG_FOO. )
&& ( B1 <-> CONFIG_BOO. )
&& (B00 -> CONFIG_BOO.)
&& (!B00 -> (CONFIG_BOO <-> CONFIG_BOO.))
&& (B00 -> !CONFIG_FOO.)
&& (!B00 -> (CONFIG_FOO <-> CONFIG_FOO.))
&& B00
 * check-output-end
 */
