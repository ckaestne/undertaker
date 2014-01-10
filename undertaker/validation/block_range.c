#define A
#define B
#define C

#ifdef A
    //B0
#endif

#ifdef B
    //B1
#elif C
    //B2
#endif

#ifdef C
    //B3
#   ifdef D
    //B4
#   endif
#endif

/*
 * check-name: blockrange test
 * check-command: undertaker -j blockrange $file
 * check-output-start
block_range.c:B00:0:0
block_range.c:B0:5:7
block_range.c:B1:9:11
block_range.c:B2:11:13
block_range.c:B3:15:20
block_range.c:B4:17:19
 * check-output-end
 */
