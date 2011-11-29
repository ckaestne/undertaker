#include "guarded-expression.h"
#include "guarded-expression.h"
#include "guarded-expression.h"
#include "guarded-expression.h"
#include "guarded-expression.h"

/*
 * check-name: Guarded include
 * check-command: undertaker -j cpppc -Iinclude $file
 * check-output-start
I: CPP Precondition for guarded-include.c
( B0 <-> CONFIG_A )
&& B00
 * check-output-end
 */
