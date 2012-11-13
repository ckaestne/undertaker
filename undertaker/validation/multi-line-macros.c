xyz
#ifdef A
a
#else
a2
#endif
b
#ifdef CONFIG_SMP
c
#define SBDMA_NEXTBUF(d,f) ((((d)->f+1) == (d)->sbdma_dscrtable_end) ? \
			  (d)->sbdma_dscrtable : (d)->f+1)
d
#elif CONFIG_NUMA

#define NEXT_MULTILINE foo \
    bar
e
#endif
f
#ifdef CONFIG_FOO
g
#else
h
#endif

/*
 * check-name: Normalize multi-line macros without shifting line numbers
 * check-command: undertaker -v -j coverage -O combined $file; wc -l $file.source0
 * check-output-start
I: Removed 0 leftovers for multi-line-macros.c
I: multi-line-macros.c, Found Solutions: 4, Coverage: 7/7 blocks enabled (100%)
34 multi-line-macros.c.source0
 * check-output-end
 */
