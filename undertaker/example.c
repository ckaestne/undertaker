#if defined CONFIG_A || defined  CONFIG_B || defined (CONFIG_C) 
# if defined ( CONFIG_A && CONFIG_B )
# elif defined ( CONFIG_A && CONFIG_C )
# elif defined ( CONFIG_B && CONFIG_C )
#   ifdef CONFIG_C
#   else
#   endif
# else
# endif
#endif
