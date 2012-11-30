//#include "include/kconfig.h"
//#include <stdio.h>

void printf(char *in) {

}

#if IS_ENABLED(CONFIG_8139CP)
// Block0 -> #if (defined(option) || defined(option##_MODULE))
#endif

#if IS_ENABLED(CONFIG_8139CP) || IS_ENABLED(CONFIG_60XX_WDT)
// Block1 -> #if (defined(option1) || defined(option1##_MODULE)) || (defined(option2) || defined(option2##_MODULE))
#endif

#if IS_ENABLED(CONFIG_8139CP) && IS_ENABLED(CONFIG_60XX_WDT)
// Block2 -> #if (defined(option1) || defined(option1##_MODULE)) && (defined(option2) || defined(option2##_MODULE))
#endif



// IS_BUILTIN
#if IS_BUILTIN(CONFIG_8139CP)
// Block3 -> #if defined(option)
#endif

#if IS_BUILTIN(CONFIG_8139CP) || IS_BUILTIN(CONFIG_60XX_WDT)
// Block4 -> #if defined(option1) || defined(option2)
#endif

#if IS_BUILTIN(CONFIG_8139CP) && IS_BUILTIN(CONFIG_60XX_WDT)
// Block5 -> #if defined(option) && defined(option2)
#endif



// IS_MODULE
#if IS_MODULE(CONFIG_8139CP)
// Block6 -> #if defined(option##_MODULE)
#endif

#if IS_MODULE(CONFIG_8139CP) || IS_MODULE(CONFIG_60XX_WDT)
// Block7 -> #if defined(option1##_MODULE) || defined(option2##_MODULE)
#endif

#if IS_MODULE(CONFIG_8139CP) && IS_MODULE(CONFIG_60XX_WDT)
// Block8 -> #if defined(option1##_MODULE) && defined(option2##_MODULE)
#endif



// CONFLICTING

#if IS_MODULE(CONFIG_8139CP) && IS_BUILTIN(CONFIG_8139CP)
// BLOCK9 -> #if defined(option1##_MODULE) && defined(option2)
#endif

#if IS_BUILTIN(CONFIG_8139CP) && !IS_BUILTIN(CONFIG_8139CP)
// BLOCK10 ->
#endif

#if IS_ENABLED(CONFIG_ACPI_CUSTOM_DSDT_FILE) && IS_ENABLED(CONFIG_STANDALONE)
// missing because _module stuff does not exist in the model..TODO find a better conflicting example
// BLOCK11 ->
#endif



// multiline CPP statements
#if IS_MODULE(CONFIG_8139CP) || \
    (IS_MODULE(CONFIG_60XX_WDT) && \
     IS_BUILTIN(CONFIG_8139CP))
// BLOCK12 -> #if defined(CONFIG_8139CP_MODULE) || (defined(CONFIG_60XX_WDT_MODULE) && defined(CONFIG_8139CP))
#endif

#if IS_MODULE(CONFIG_8139CP) && \
    (IS_MODULE(CONFIG_60XX_WDT) || \
     IS_BUILTIN(CONFIG_8139CP))
// BLOCK13 -> #if defined(CONFIG_8139CP_MODULE) || (defined(CONFIG_60XX_WDT_MODULE) && defined(CONFIG_8139CP))
#endif



// TEST
#if defined(CONFIG_8139CP)
// BLOCK14
#endif

#if (defined(CONFIG_8139CP_MODULE) && defined(CONFIG_8139CP))
// BLOCK15 -> #if defined(option1##_MODULE) && defined(option2)
#elif defined(CONFIG_8139CP_MODULE) || defined(CONFIG_8139CP)
// BLOCK16
#else
// BLOCK17
#endif

#if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) || (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
// BLOCK18

#endif

// non-CPP ifs

int main(int argc, const char *argv[]) {
    int var = 0, var2 = 1;

//=============================================================================
// EASY CASES
//=============================================================================

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP)) {
    //     printf("DEBUG");
    // } else if(IS_MODULE(CONFIG_8139CP))
    //     printf("DEBUG2");
    // else if(IS_BUILTIN(CONFIG_60XX_WDT))
    //    printf("DEBUG3");
    // else if(IS_ENABLED(CONFIG_60XX_WDT))
    //    printf("DEBUG4");
// #else
    // if(IS_ENABLED(CONFIG_8139CP)) {
    //     printf("DEBUG");
    // } else
    // #if defined(CONFIG_8139CP_MODULE)
        // if(IS_MODULE(CONFIG_8139CP))
        //     printf("DEBUG2");
        // else if(var)
        //    printf("DEBUG3");
        // else if(IS_ENABLED(CONFIG_60XX_WDT))
        //    printf("DEBUG4");
    // #else
        // if(IS_MODULE(CONFIG_8139CP))
        //     printf("DEBUG2");
        // else if(var)
        //    printf("DEBUG3");
        // else
        // #if (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
            // if(IS_ENABLED(CONFIG_60XX_WDT))
            //    printf("DEBUG4");
        // #else
            // if(IS_ENABLED(CONFIG_60XX_WDT))
            //    printf("DEBUG4");
        // #endif
    // #endif
// #endif

    if(IS_ENABLED(CONFIG_8139CP)) {
        printf("DEBUG");
    // dead, cannot be reached because of first if condition, which is a problem right now, we lose the semantic connection between the ifs when replacing the way described above
    } else if(IS_MODULE(CONFIG_8139CP))
        printf("DEBUG2");
    else if(var)
        printf("DEBUG3");
    else if(IS_ENABLED(CONFIG_60XX_WDT))
        printf("DEBUG4");


// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) && (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) && IS_ENABLED(CONFIG_60XX_WDT))
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) && IS_ENABLED(CONFIG_60XX_WDT))
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) && IS_ENABLED(CONFIG_60XX_WDT))
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) || (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) || IS_ENABLED(CONFIG_60XX_WDT))
    //      printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) || IS_ENABLED(CONFIG_60XX_WDT))
    //      printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) || IS_ENABLED(CONFIG_60XX_WDT))
        printf("DEBUG");

//=============================================================================
// DIFFICULT CASES
//=============================================================================


    // TODO verschachtelte ifs? rekursiv?
    // TODO NOT expressions?

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) && var) {
    //     printf("DEBUG");
    // } else if(IS_MODULE(CONFIG_8139CP) && var2)
    //     printf("DEBUG2");
    // else if(IS_BUILTIN(CONFIG_60XX_WDT) && var)
    //    printf("DEBUG3");
    // else if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
    //    printf("DEBUG4");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) && var) {
    //     printf("DEBUG");
    // } else
    // #if defined(CONFIG_8139CP_MODULE)
        // if(IS_MODULE(CONFIG_8139CP) && var2)
        //     printf("DEBUG2");
        // else if(IS_BUILTIN(CONFIG_60XX_WDT) && var)
        //    printf("DEBUG3");
        // else if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
        //    printf("DEBUG4");
    // #else
            // if(IS_MODULE(CONFIG_8139CP))
            //     printf("DEBUG2");
            // else
            // #if defined(CONFIG_60XX_WDT)
                // if(IS_BUILTIN(CONFIG_60XX_WDT) && var)
                //    printf("DEBUG3");
                // else if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
                //    printf("DEBUG4");
            // #else
                // if(IS_BUILTIN(CONFIG_60XX_WDT) && var)
                //    printf("DEBUG3");
                // else
                // #if (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
                    // if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
                    //    printf("DEBUG4");
                // #else
                    // if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
                    //    printf("DEBUG4");
                // #endif
            // #endif
        // #endif
// #endif

    if(IS_ENABLED(CONFIG_8139CP) && var) {
        printf("DEBUG");
    // dead, cannot be reached because of first if condition, which is a problem right now, we lose the semantic connection between the ifs when replacing the way described above
    } else if(IS_MODULE(CONFIG_8139CP) && var2)
        printf("DEBUG2");
    else if(IS_BUILTIN(CONFIG_60XX_WDT) && var)
        printf("DEBUG3");
    else if(IS_ENABLED(CONFIG_60XX_WDT) || var2)
        printf("DEBUG4");


// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) && var)
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) && var)
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) && var)
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(var && IS_ENABLED(CONFIG_8139CP))
    //    printf("DEBUG");
// #else
    // if(var && IS_ENABLED(CONFIG_8139CP))
    //    printf("DEBUG");
// #endif
    if(var && IS_ENABLED(CONFIG_8139CP))
        printf("DEBUG");


// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) || var)
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) || var)
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) || var)
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE))
    // if(var || IS_ENABLED(CONFIG_8139CP))
    //     printf("DEBUG");
// #else
    // if(var || IS_ENABLED(CONFIG_8139CP))
    //     printf("DEBUG");
// #endif
    if(var || IS_ENABLED(CONFIG_8139CP))
        printf("DEBUG");



// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) && (defined(CONFIG_60XX_WDT) || definied(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) && (var || IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) && (var || IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) && (var || IS_ENABLED(CONFIG_60XX_WDT)))
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) || (defined(CONFIG_60XX_WDT) || definied(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT)))
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) && (defined(CONFIG_60XX_WDT) || definied(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) && (var && IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) && (var && IS_ENABLED(CONFIG_60XX_WDT)))
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) && (var && IS_ENABLED(CONFIG_60XX_WDT)))
        printf("DEBUG");

// #if (defined(CONFIG_8139CP) || defined(CONFIG_8139CP_MODULE)) || (defined(CONFIG_60XX_WDT) || defined(CONFIG_60XX_WDT_MODULE))
    // if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT) && var2))
    //     printf("DEBUG");
// #else
    // if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT) && var2))
    //     printf("DEBUG");
// #endif
    if(IS_ENABLED(CONFIG_8139CP) || (var && IS_ENABLED(CONFIG_60XX_WDT) && var2))
        printf("DEBUG");




// TESTCASES FOR IF conditions
    if (IS_ENABLED(CONFIG_8139CP))
        if(var)
            printf("BLUB");

    if (IS_ENABLED(CONFIG_60XX_WDT)) {
        if(var)
            printf("BLUB");
    } else if (IS_MODULE(CONFIG_60XX_WDT) && var) {
        if(var2)
            printf("BLUB");
        else if(var)
            printf("BLUB");
        else
            printf("BLUB");
    } else {
        printf("BLUB");
    }
    return 0;
}

/*
 * check-name: Complex Makro replacement test
 * check-command: undertaker -j dead -v $file -m models/x86.model
 * check-output-start
I: Using x86 as primary model
I: creating makro_replace.c.B10.code.globally.dead
I: creating makro_replace.c.B11.missing.globally.dead
I: creating makro_replace.c.B15.kconfig.globally.dead
I: creating makro_replace.c.B9.kconfig.globally.dead
I: loaded rsf model for x86
 * check-output-end
 */
