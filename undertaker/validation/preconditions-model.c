#ifdef CONFIG_TOPLEVEL_A
WITHIN TOPLEVEL_A
#ifdef CONFIG_LEVEL_A_B
WITHIN A AND B
#endif 
WITHIN A
#endif

#ifdef CONFIG_TOPLEVEL_C
WITHIN TOPLEVEL_C
#ifdef CONFIG_LEVEL_C_B
WITHIN C AND B
#endif 
WITHIN C
#else
OUT OF C
#endif

No CPP Precond
/*
 * check-name: calculate block preconditions with model
 * check-command: undertaker -q -j blockpc -m preconditions.model $file:12
 * check-output-start
I: loaded rsf model for preconditions
I: Using preconditions as primary model
I: Block B3 | Defect: dead/kconfig | Global: 1
B3
&& ( B2 <-> CONFIG_TOPLEVEL_C )
&& ( B3 <-> B2 && CONFIG_LEVEL_C_B )
&& B00
&& (CONFIG_LEVEL_C_B -> ((CONFIG_NOT_MISSING && !CONFIG_TOPLEVEL_C)))
&& (CONFIG_NOT_MISSING -> ((!CONFIG_NOT_MISSING_MODULE)))
&& (CONFIG_NOT_MISSING_MODULE -> ((!CONFIG_NOT_MISSING)))
&& (CONFIG_TOPLEVEL_C -> ((CONFIG_I_DONT_GIVE_A_BLOODY_HELL_PRECONDITION)))
 * check-output-end
 */
