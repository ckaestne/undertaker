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
 * check-name: calculate configuration from solvable multiple-file block preconditions
 * check-command: undertaker -v -j mergeblockconf mergeblockconf.worklist
 * check-output-start
I: No block found at ./mergeblockconf.c:8
I: Processing block B2_mergeblockconf.c
I: Solution found, result:
CONFIG_MERGEBLOCK_1=n
CONFIG_MERGEBLOCK_2=y
# B0_mergeblockconf.c=n
# B1_mergeblockconf.c=y
# B2_mergeblockconf.c=y
# FILE_mergeblockconf.c=y
 * check-output-end
 */
