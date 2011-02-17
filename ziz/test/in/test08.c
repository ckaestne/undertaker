start();
#ifdef A
bar();
#else
 #  define B
#endif
do_stuff();
#if B
cleanup();
#undef B
#endif
