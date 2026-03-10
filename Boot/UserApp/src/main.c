/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes


// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
typedef void (*fpJumpHandler)(void);

#define U32_USB_CDC_USER_APP_SW_RELEASE_MAJOR_VERSION		((uint32_t)0x2)
#define U32_USB_CDC_USER_APP_SW_RELEASE_MINOR_VERSION		((uint32_t)0x1)
#define U32_BOOT_LOADER_APPLICATION_START_ADDRESS			((uint32_t)0x00400000)
#define U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE			((uint32_t)0x0000C000)
#define U32_USER_APPLICATION_START_ADDRESS					((uint32_t)0x0040C000)
#define U32_USER_APPLICATION_ALLOCATED_SIZE					((uint32_t)0x001F4000)
#define CUSTOM_DEBUG(x)										
#define U16_USB_CDC_MAX_DATA_SIZE							((uint16_t)540)
#define U16_USB_DEVICE_CDC_MAX_DATA_SIZE					((uint16_t)20)
#define U8_HASH_TAG_DATA_SIZE								((uint8_t)5)
#define NO_OF_ELEMENTS_IN_ARRAY(array)						sizeof(array)/sizeof(array[0])							
#define U32_TIME_BEFORE_WDT_RESET_MSEC						((uint32_t)3000)		/* Watchdog reset time */

typedef struct
{
	uint32_t u32ApplicationVersion_Major;
	uint32_t u32ApplicationVersion_Minor;
	fpJumpHandler fpApplicationJumpHandler;
	uint32_t u32BootApplicationStartAddress;
	uint32_t u32BootApplicationAllocationSize;
	uint32_t u32UserApplicationStartAddress;
	uint32_t u32UserApplicationAllocationSize;
}TS_ApplicationData;


typedef union
{
	uint32_t au32ApplicationData[32];
	TS_ApplicationData sApplicationData;
}TU_ApplicationData;


typedef struct
{
	TU_ApplicationData uApplicationData;
//	TS_SHA_Padding sSHA_Padding;
//	TS_SHA_Digest sSHA_Digest;
}TS_ApplicationFooter;


__attribute__ ((section(".ApplicationFooterData")))
const TS_ApplicationFooter sUserApplicationFooter =
{
	{
		U32_USB_CDC_USER_APP_SW_RELEASE_MAJOR_VERSION,
		U32_USB_CDC_USER_APP_SW_RELEASE_MINOR_VERSION,
		NULL,
		U32_BOOT_LOADER_APPLICATION_START_ADDRESS,
		U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE,
		U32_USER_APPLICATION_START_ADDRESS,
		U32_USER_APPLICATION_ALLOCATED_SIZE,
	},
//	{0},
//	{0},
};


int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
//        SYS_Tasks ( );
        GPIO_PA23_Set();
//        for(int i = 0; i <= 8000000; i++)
//        {
//            __NOP();
//        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

