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
 * check-command: undertaker -v -j mergeblockconf mergeblockconf_fail.worklist
 * check-output-start
I: No block found at ./mergeblockconf_fail.c:8
I: Processing block B0_mergeblockconf_fail.c
I: Processing block B2_mergeblockconf_fail.c
 * check-output-end
 * check-error-start
E: Wasn't able to generate a valid configuration
 * check-error-end
 */
