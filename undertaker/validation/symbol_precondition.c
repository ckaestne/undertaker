/*
 * check-name: find symbol precondition for a given symbol
 * check-command: undertaker -v -j symbolpc -m preconditions.model CONFIG_LEVEL_C_B
 * check-output-start
I: loaded rsf model for preconditions
I: Using preconditions as primary model
I: Symbol Precondition for `CONFIG_LEVEL_C_B'
(CONFIG_LEVEL_C_B -> ((CONFIG_NOT_MISSING && !CONFIG_TOPLEVEL_C)))
&& (CONFIG_TOPLEVEL_C -> ((CONFIG_I_DONT_GIVE_A_BLOODY_HELL_PRECONDITION)))

&&
( ! ( CONFIG_I_DONT_GIVE_A_BLOODY_HELL_PRECONDITION ) )
 * check-output-end
 */

