#if CONFIG_BLUB0
B0

#if CONFIG_BLUB1
B1
#elif CONFIG_BLUB2
B2
#endif

#if CONFIG_BLUB3
B3
#if CONFIG_BLUB5
B4
#endif
#if CONFIG_BLUB6
B5
#endif

#elif CONFIG_BLUB4
B6

#if CONFIG_BLUB7
B7
#endif

#if CONFIG_BLUB8
B8
#endif

#endif // config_blub4
#endif // config_blub0

#if CONFIG_BLUB9
B9
#elif CONFIG_BLUB10
B10
#endif

#if CONFIG_BLUB11
B11
#endif

/*
 * check-name: Coverage on a more complex structure, this test ensures the blocks get inserted right
 * into the CppFile list with "-C simple_decision"
 * check-command: undertaker -v -j coverage -C simple_decision $file
 * check-output-start
I: Removed 0 leftovers for coverage-ifdefs-simple_decision.c
I: coverage-ifdefs-simple_decision.c, Found Solutions: 13, Coverage: 22/22 blocks enabled (100%)
 * check-output-end
 */
