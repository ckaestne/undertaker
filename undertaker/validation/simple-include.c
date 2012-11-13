#include "simple-expression.h"
/*
 * check-name: simple #include statement
 * check-command: undertaker -j cpppc -Iinclude $file
 * check-output-start
( B0 <-> CONFIG_A )
&& B00
 * check-output-end
 */
