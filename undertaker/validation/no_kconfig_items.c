#define DEAD

#ifdef DEAD
    //B0: UNDEAD
    #undef DEAD
#else
    //B1: DEAD
#endif

#ifdef DEAD
    //B2: DEAD
#endif

#ifdef CONFIG_X86
    //B3: X86 UNDEAD
#else
    //B4: X86 DEAD
#endif

/*
 * check-name: no_kconfig (un)deads
 * check-command: undertaker -vj dead -m models $file
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
I: found 23 models
I: Using x86 as primary model
I: creating no_kconfig_items.c.B0.code.no_kconfig.undead
I: creating no_kconfig_items.c.B1.code.no_kconfig.dead
I: creating no_kconfig_items.c.B2.code.no_kconfig.dead
I: creating no_kconfig_items.c.B3.kconfig.x86.undead
I: creating no_kconfig_items.c.B4.kconfig.x86.dead
 * check-output-end
 */
