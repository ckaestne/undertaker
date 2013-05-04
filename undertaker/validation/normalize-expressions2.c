/*
 * code snippet from coreutils/lib/c-strtod.c
 */

#if defined LC_ALL_MASK && (LONG ? HAVE_STRTOLD_L : HAVE_STRTOD_L)
#else
#endif


/*
 * check-name: Gracefully handle complicated constructions from coreutils: ? operator
 * check-output-start:
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
 * check-output-end
 */
