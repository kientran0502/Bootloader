//********************************************************************************************************
// PRNF Implementation
//********************************************************************************************************
/*
If extensions are not used, this file need only be 2 lines:
    #define PRNF_IMPLEMENTATION
    #include "prnf.h"

Otherwise before including "prnf.h" you have the options to provide a memory allocator, assert macro, and warning handler.

Other build options such as PRNF_SUPPORT_FLOAT, PRNF_SUPPORT_LONG_LONG etc.. (see prnf.h) may also be defined here
 if you do not want to define them in your build configuration.

*/


/*	If you have a runtime warning handler, include it here and define PRNF_WARN to be your handler.
 *  A 'true' argument is expected to generate a warning.
 *****************************************************************************************/
//	#include "my_warning_handler.h"
//	#define PRNF_WARN(arg) my_warning_handler(arg)
