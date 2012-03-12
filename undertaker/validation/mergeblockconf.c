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
I: No block at ./mergeblockconf.c:8 was found.
I: Block B2_mergeblockconf.c | Defect: no | Global: 0
I: Solution found, result was written to _block.config
D: ---- Dumping new assignment map
D: considering MERGEBLOCK_1
D: considering MERGEBLOCK_2
I: Content of '_block.config':
CONFIG_MERGEBLOCK_1=n
CONFIG_MERGEBLOCK_2=y
# B0_mergeblockconf.c=n
# B1_mergeblockconf.c=y
# B2_mergeblockconf.c=y
# FILE_mergeblockconf.c=y

 * check-output-end
 */