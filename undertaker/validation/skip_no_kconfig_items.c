#define SKIP_ME_FIRST
#define SKIP_ME_AFTER

#ifdef SKIP_ME_FIRST
    //B0: NOT REPORTED
#endif

#ifdef CONFIG_X86
    //B1: X86 UNDEAD
#else
    //B2: X86 DEAD
#endif

#ifdef SKIP_ME_AFTER
    //B3: NOT REPORTED
#endif

/*
 * check-name: skip no_kconfig (un)deads
 * check-command: undertaker -svj dead -m models $file
 * check-output-start
I: loaded rsf model for alpha
I: loaded rsf model for arm
I: loaded rsf model for avr32
I: loaded rsf model for blackfin
I: loaded rsf model for cris
I: loaded rsf model for frv
I: loaded rsf model for h8300
I: loaded rsf model for hexagon
I: loaded rsf model for ia64
I: loaded rsf model for m32r
I: loaded rsf model for m68k
I: loaded rsf model for microblaze
I: loaded rsf model for mips
I: loaded rsf model for mn10300
I: loaded rsf model for openrisc
I: loaded rsf model for parisc
I: loaded rsf model for powerpc
I: loaded rsf model for s390
I: loaded rsf model for score
I: loaded rsf model for sh
I: loaded rsf model for sparc
I: loaded rsf model for tile
I: loaded rsf model for um
I: loaded rsf model for unicore32
I: loaded rsf model for x86
I: loaded rsf model for xtensa
I: found 26 models
I: Using x86 as primary model
I: creating skip_no_kconfig_items.c.B1.kconfig.x86.undead
I: creating skip_no_kconfig_items.c.B2.kconfig.x86.dead
 * check-output-end
 */
