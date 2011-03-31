#ifdef A
#ifdef B
#define C
#endif
#endif

#ifdef X
#ifdef Y
#undef C
#endif
#endif

#ifdef Y
#ifdef C
#endif
#endif
