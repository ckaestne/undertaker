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
 * check-name: multiple-file block preconditions conflicting with whitelist
 * check-command: undertaker -v -j mergeblockconf mergeblockconf_lists_conflict.worklist -m mergeblockconf.model -W mergeblockconf.whitelist -B mergeblockconf.blacklist
 * check-output-start
I: loaded 1 items to whitelist
I: loaded 1 items to blacklist
I: loaded rsf model for mergeblockconf
I: Using mergeblockconf as primary model
I: Processing block B2_mergeblockconf_lists_conflict.c
I: Processing block B4_mergeblockconf_lists_conflict.c
I: Solution found, result:
# B3_mergeblockconf_lists_conflict.c=n
# B4_mergeblockconf_lists_conflict.c=y
# FILE_mergeblockconf_lists_conflict.c=y
CONFIG_MERGEBLOCK_1=y
CONFIG_MERGEBLOCK_2=n
CONFIG_UNTOUCHEDBLOCK=n
 * check-output-end
 * check-error-start
W: Code constraints for B2_mergeblockconf_lists_conflict.c not satisfiable - override by black-/whitelist
 * check-error-end
 */
