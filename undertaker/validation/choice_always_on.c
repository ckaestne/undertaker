#if !CONFIG_TREE_RCU && !CONFIG_TREE_PREEMPT_RCU && !CONFIG_TINY_RCU && !CONFIG_TINY_PREEMPT_RCU
#endif

#if !CONFIG_TREE_PREEMPT_RCU && !CONFIG_TINY_RCU && !CONFIG_TINY_PREEMPT_RCU
#endif

/*
 * check-name: Check that choice items are always on
 * check-output-start
I: loaded rsf model for alpha
I: loaded rsf model for arm
I: loaded rsf model for avr32
I: loaded rsf model for blackfin
I: loaded rsf model for cris
I: loaded rsf model for frv
I: loaded rsf model for h8300
I: loaded rsf model for ia64
I: loaded rsf model for m32r
I: loaded rsf model for m68k
I: loaded rsf model for m68knommu
I: loaded rsf model for microblaze
I: loaded rsf model for mips
I: loaded rsf model for mn10300
I: loaded rsf model for parisc
I: loaded rsf model for powerpc
I: loaded rsf model for s390
I: loaded rsf model for score
I: loaded rsf model for sh
I: loaded rsf model for sparc
I: loaded rsf model for tile
I: loaded rsf model for x86
I: loaded rsf model for xtensa
I: found 23 rsf models
I: Using x86 as primary model
I: creating choice_always_on.c.B0.kconfig.globally.dead
 * check-output-end
 */
