#ifdef CONFIG_A		//0
# ifdef CONFIG_B	//1
# else CONFIG_C		//2
# endif
#elif CONFIG_B		//3
#elif CONFIG_C		//4
#else			//5
# ifdef CONFIG_D	//6
# elif CONFIG_A		//7
#  ifdef CONFIG_E	//8
#  endif
# else CONFIG_B		//9
# endif
#endif
