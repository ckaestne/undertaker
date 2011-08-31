#include "simple-expression.h"
/*
 * check-name: simple #include statement
 * check-command: undertaker -q -j cpppc -Iinclude $file
 * check-output-start
I: CPP Precondition for simple-include.c
( B0 <-> CONFIG_A )
&& B00
 * check-output-end
 */
