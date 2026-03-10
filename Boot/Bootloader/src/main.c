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


// ***************************** INCLUDE ************************************************
#include <stdint.h>
#include <string.h>

// ****************************** DEFINE ***********************************************
#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MAJOR_VERSION		((uint32_t)0x1)
#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MINOR_VERSION		((uint32_t)0x1)
#define U32_BOOT_LOADER_APPLICATION_START_ADDRESS			((uint32_t)0x00400000)
#define U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE			((uint32_t)0x0000C000)
#define U32_USER_APPLICATION_START_ADDRESS					((uint32_t)0x0040C000)
#define U32_USER_APPLICATION_ALLOCATED_SIZE					((uint32_t)0x001F4000)

// ***************************** STRUCT ************************************************
typedef void (*fpJumpHandler)(void);

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


// ******************************* PROTOTYPE **********************************************
// Section: Main Entry Point
static void JumpToUserApplication(void);
TS_ApplicationData* P_GetApplicationData(void);
static void BootloaderJumpHandler(void);

// ********************************** VARIABLES *******************************************
/** Creating section to store Application Footer Data... This data is critical as Bootloader uses this before jumping here */
__attribute__ ((section(".ApplicationFooterData")))
const TS_ApplicationFooter sBootLoaderFooter = 
{
	{
		U32_USB_CDC_BOOTLOADER_SW_RELEASE_MAJOR_VERSION,
		U32_USB_CDC_BOOTLOADER_SW_RELEASE_MINOR_VERSION,
		BootloaderJumpHandler,
		U32_BOOT_LOADER_APPLICATION_START_ADDRESS,
		U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE,
		U32_USER_APPLICATION_START_ADDRESS,
		U32_USER_APPLICATION_ALLOCATED_SIZE,
	},
//	{0},
//	{0},
};

//__attribute__((section(".ApplicationFooterData"), used))
//const TS_ApplicationFooter sBootLoaderFooter =
//{
//    .uApplicationData =
//    {
//        .sApplicationData =
//        {
//            U32_USB_CDC_BOOTLOADER_SW_RELEASE_MAJOR_VERSION,
//            U32_USB_CDC_BOOTLOADER_SW_RELEASE_MINOR_VERSION,
//            BootloaderJumpHandler,
//            U32_BOOT_LOADER_APPLICATION_START_ADDRESS,
//            U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE,
//            U32_USER_APPLICATION_START_ADDRESS,
//            U32_USER_APPLICATION_ALLOCATED_SIZE
//        }
//    }
//};

// *****************************************************************************

__attribute__ ((section(".JumpSignatureData")))
static uint8_t mau8JumpSignature[32];

//COMPILER_ALIGNED(128)

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );

	/*Init*/
	// UART_Driver_Init(void);

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
//        SYS_Tasks ( );
		GPIO_PA23_Toggle();

//        if(PIO_PortRead(PIO_PIN_PA9) == 0)
//        {
                    // Unlock CTLPPBLOCK (clear bits 31:28 an to�n nh?t)
            *((volatile uint32_t *)0xE000E008) &= ~0xF0000000UL;  // Ho?c SCB->ACTLR n?u header c�
            JumpToUserApplication();
//        }
		for(int i = 0; i <= 64000000; i++)
		{
			__NOP();
		}
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

	TS_ApplicationData* pCurrentApplicationData;
	TS_ApplicationData* pUserApplicationData;
	uint32_t u32StackPointerValue;
	fpJumpHandler fpApplicationResetHandler;
//		

static void JumpToUserApplication(void)
{
//	TS_ApplicationData* pCurrentApplicationData;
//	TS_ApplicationData* pUserApplicationData;
//	uint32_t u32StackPointerValue;
//	fpJumpHandler fpApplicationResetHandler;
//		
	/** Release Resources before jumping to User Application */
//	udc_stop();
//	sysclk_disable_peripheral_clock(CONSOLE_UART_ID);
	
	/** Disable interrupts */
	__disable_irq();
	
	/** Get Application header data*/
	pCurrentApplicationData = P_GetApplicationData();
	
	pUserApplicationData = (TS_ApplicationData*)(pCurrentApplicationData->u32UserApplicationStartAddress +
		pCurrentApplicationData->u32UserApplicationAllocationSize - sizeof(TS_ApplicationFooter));
	
	/** Barriers */
	__DSB();
	__ISB();

	/** Update vector table */
	SCB->VTOR = pUserApplicationData->u32UserApplicationStartAddress & SCB_VTOR_TBLOFF_Msk;
//    SCB->VTOR = 0x00410000;
	
	/** Barriers */
	__DSB();
	__ISB();

	/** Enable interrupts */
	__enable_irq();

	/** Update stack pointer */
	u32StackPointerValue = (uint32_t)(*(uint32_t *)(pUserApplicationData->u32UserApplicationStartAddress));
	__set_MSP(u32StackPointerValue);
	
	/** Call Application reset handler */
	fpApplicationResetHandler = (fpJumpHandler)(*((uint32_t*)(pUserApplicationData->u32UserApplicationStartAddress + 4)));
	(*fpApplicationResetHandler)();
}


static void BootloaderJumpHandler(void)
{
	/** Update Signature to indicate Boot loader */
//	strcpy(mau8JumpSignature, "StayInBootLoader");
}

TS_ApplicationData* P_GetApplicationData(void)
{
	return(&(sBootLoaderFooter.uApplicationData.sApplicationData));
}

/*******************************************************************************
 End of File
*/

