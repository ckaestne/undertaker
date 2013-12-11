#define FOO
#define BAR

// UNDEAD if, DEAD else
#ifdef FOO
    //B0: UNDEAD
    #undef FOO
#else
    //B1: DEAD
#endif

// DEAD if, UNDEAD else
#ifdef FOO
    //B2: DEAD
#else
    //B3: UNDEAD
#endif

// DEAD if, UNDEAD elif, DEAD else
#ifdef FOO
    //B4: DEAD
#elif BAR
    //B5: UNDEAD
#else
    //B6: DEAD
#endif

// UNDEAD if, DEAD elif, DEAD else
#ifdef BAR
    //B7: UNDEAD
#elif FOO
    //B8: DEAD
#else
    //B9: DEAD
#endif

// UNDEAD if, DEAD elif
#ifdef CONFIG_X86
    //B10: UNDEAD
#elif FOO
    //B11: DEAD
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
I: creating no_kconfig_items.c.B0.no_kconfig.globally.undead
I: creating no_kconfig_items.c.B1.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B2.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B3.no_kconfig.globally.undead
I: creating no_kconfig_items.c.B4.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B5.no_kconfig.globally.undead
I: creating no_kconfig_items.c.B6.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B7.no_kconfig.globally.undead
I: creating no_kconfig_items.c.B8.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B9.no_kconfig.globally.dead
I: creating no_kconfig_items.c.B10.kconfig.x86.undead
I: creating no_kconfig_items.c.B11.no_kconfig.globally.dead
 * check-output-end
 */
