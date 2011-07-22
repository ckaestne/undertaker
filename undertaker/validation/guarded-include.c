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
( B0 <-> ! _INCLUDE_GUARD )
&& (B0 -> _INCLUDE_GUARD.)
&& ((_INCLUDE_GUARD  && !(B0)) -> _INCLUDE_GUARD.)
&& ((_INCLUDE_GUARD. && !(B0)) -> _INCLUDE_GUARD )
&& B00
&& ( B1 <-> B0 && CONFIG_A )
 * check-output-end
 */
