#ifdef CONFIG_MERGEBLOCK_1
    WITHIN A
#else
#ifdef CONFIG_MERGEBLOCK_2
    WITHIN !A AND B
#endif
#endif
OUT
#ifdef CONFIG_UNTOUCHEDBLOCK
    WITHIN UNTOUCHEDBLOCK
#else
    WITHOUT UNTOUCHEDBLOCK
#endif

/*
 * check-name: try to calculate configuration from unsolvable multiple-file block preconditions
 * check-command: undertaker -j mergeblockconf mergeblockconf_fail.worklist
 * check-output-start
I: No block at ./mergeblockconf_fail.c:8 was found.
I: Block B0_mergeblockconf_fail.c | Defect: no | Global: 0
I: Block B2_mergeblockconf_fail.c | Defect: no | Global: 0
E: Wasn't able to generate a valid configuration
 * check-output-end
 */