#ifdef A
B0
#endif

#ifdef X
B3
#define A
#endif

#ifdef Y
B6
#undef A
#endif

#ifdef A
B9
#endif

#undef A

#ifdef A
B12
#endif
