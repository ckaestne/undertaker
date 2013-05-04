#if !CONFIG_TREE_RCU && !CONFIG_TREE_PREEMPT_RCU && !CONFIG_TINY_RCU && !CONFIG_TINY_PREEMPT_RCU
#endif

#if !CONFIG_TREE_PREEMPT_RCU && !CONFIG_TINY_RCU && !CONFIG_TINY_PREEMPT_RCU
#endif

/*
 * check-name: CNF: Check that choice items are always on
 * check-command: undertaker -v -m cnfmodels $file
 * check-output-start
I: loaded cnf model for alpha
I: loaded cnf model for arm
I: loaded cnf model for avr32
I: loaded cnf model for blackfin
I: loaded cnf model for cris
I: loaded cnf model for frv
I: loaded cnf model for h8300
I: loaded cnf model for hexagon
I: loaded cnf model for ia64
I: loaded cnf model for m32r
I: loaded cnf model for m68k
I: loaded cnf model for microblaze
I: loaded cnf model for mips
I: loaded cnf model for mn10300
I: loaded cnf model for openrisc
I: loaded cnf model for parisc
I: loaded cnf model for powerpc
I: loaded cnf model for s390
I: loaded cnf model for score
I: loaded cnf model for sh
I: loaded cnf model for sparc
I: loaded cnf model for tile
I: loaded cnf model for um
I: loaded cnf model for unicore32
I: loaded cnf model for x86
I: loaded cnf model for xtensa
I: found 26 models
I: Using x86 as primary model
I: creating choice_always_on_cnf.c.B0.kconfig.x86.dead
 * check-output-end
 */
