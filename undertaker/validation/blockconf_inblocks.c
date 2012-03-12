WITHOUT BLOCK AT BEGIN
#ifdef CONFIG_TOPLEVEL
WITHIN TOPLEVEL
#ifdef CONFIG_A
WITHIN A
#else
OUT OF A
#endif
BETWEEN A AND B
#ifdef CONFIG_B
WITHIN B
#else
OUT OF B
#endif
BETWEEN B AND C
#ifdef CONFIG_C
WITHIN C
#else
OUT OF C
#endif
AFTER C
#else
OUT OF TOPLEVEL
#endif
WITHOUT BLOCK AT END

/*
 * check-name: calculate configuration from solvable block preconditions at one location
 * check-command: undertaker -v -j blockconf $file:13
 * check-output-start
I: Block B4 | Defect: no | Global: 0
blockconf_inblocks.c:13.config
D: ---- Dumping new assignment map
D: considering B
D: considering TOPLEVEL
I: Content of 'blockconf_inblocks.c:13.config':
CONFIG_B=n
CONFIG_TOPLEVEL=y

 * check-output-end
 */
