#ifdef CONFIG_FB_OMAP_MANUAL_UPDATE
#else
#endif
#ifdef CONFIG_ARCH_OMAP1
#else
#endif
#ifdef CONFIG_FB_OMAP_LCDC_HWA742
#endif
#ifdef CONFIG_FB_OMAP_LCDC_BLIZZARD
#endif
#ifdef CONFIG_FB_OMAP_LCDC_EXTERNAL
#ifdef CONFIG_ARCH_OMAP1
#else
#endif
#endif
#ifdef DEBUG
#endif
#define _C(x) (fbdev->ctrl->x != NULL)
#define _P(x) (fbdev->panel->x != NULL)
#undef _P
#undef _C
#ifdef CONFIG_ARCH_OMAP1
#ifdef CONFIG_FB_OMAP_LCDC_EXTERNAL
#endif
#else	/* OMAP2 */
#ifdef CONFIG_FB_OMAP_LCDC_EXTERNAL
#endif
#endif
#ifdef CONFIG_FB_OMAP_DMA_TUNE
#endif
#ifndef MODULE
#endif
#ifndef MODULE
#endif

/*
 * check-name: omapfb_main.c
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
I: creating omapfb_main-structure.c.B0.x86.missing.dead
I: creating omapfb_main-structure.c.B2.x86.missing.dead
I: creating omapfb_main-structure.c.B4.x86.missing.dead
I: creating omapfb_main-structure.c.B5.x86.missing.dead
I: creating omapfb_main-structure.c.B6.x86.missing.dead
I: creating omapfb_main-structure.c.B7.x86.missing.dead
I: creating omapfb_main-structure.c.B8.x86.missing.dead
I: creating omapfb_main-structure.c.B11.x86.missing.dead
I: creating omapfb_main-structure.c.B12.x86.missing.dead
I: creating omapfb_main-structure.c.B14.x86.missing.dead
I: creating omapfb_main-structure.c.B15.x86.missing.dead
 * check-output-end
 */
