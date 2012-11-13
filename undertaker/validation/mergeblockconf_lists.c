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
 * check-name: calculate configuration from solvable multiple-file block preconditions with white- and blacklist
 * check-command: undertaker -v -j mergeblockconf mergeblockconf_lists.worklist -m mergeblockconf.model -W mergeblockconf.whitelist -B mergeblockconf.blacklist
 * check-output-start
I: loaded 1 items to whitelist
I: loaded 1 items to blacklist
I: loaded rsf model for mergeblockconf
I: Using mergeblockconf as primary model
I: Processing block B0_mergeblockconf_lists.c
I: Processing block B4_mergeblockconf_lists.c
I: Solution found, result:
# B0_mergeblockconf_lists.c=y
# B3_mergeblockconf_lists.c=n
# B4_mergeblockconf_lists.c=y
# FILE_mergeblockconf_lists.c=y
CONFIG_MERGEBLOCK_1=y
CONFIG_MERGEBLOCK_2=n
CONFIG_UNTOUCHEDBLOCK=n
 * check-output-end
 */
