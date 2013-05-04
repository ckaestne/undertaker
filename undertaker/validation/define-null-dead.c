
#define CONFIG_HURZ 0
#define CONFIG_FURZ 1

#if 1

#if CONFIG_HURZ
// dead
#else
// undead
#endif

#if CONFIG_FURZ
// undead
#else
// dead
#endif

#endif

#if CONFIG_HURZ
// dead
#else
// undead
#endif

#if CONFIG_FURZ
// undead
#else
// dead
#endif


/*
 * check-name: Complex Conditions
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
I: creating define-null-dead.c.B1.code.globally.dead
I: creating define-null-dead.c.B2.code.globally.undead
I: creating define-null-dead.c.B3.code.globally.undead
I: creating define-null-dead.c.B4.code.globally.dead
I: creating define-null-dead.c.B5.code.globally.dead
I: creating define-null-dead.c.B6.code.globally.undead
I: creating define-null-dead.c.B7.code.globally.undead
I: creating define-null-dead.c.B8.code.globally.dead
 * check-output-end
 */
