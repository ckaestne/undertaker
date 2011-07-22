#include "simple-expression.h"
/*
 * check-name: block B00 must be solvable
 * check-command: undertaker -j cpppc -Iinclude $file
 * check-output-start
I: CPP Precondition for validation/simple-include.c
(B0 <-> CONFIG_A)
&&
B00
 * check-output-end
 */
