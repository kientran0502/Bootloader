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
#include "peripheral/efc/plib_efc.h"

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
	uint32_t u32SHA_1PaddingBytes[16];
}TS_SHA_Padding;

typedef struct
{
	uint32_t au32ApplicationHashTag[16];
}TS_SHA_Digest;

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
	TS_SHA_Padding sSHA_Padding;
	TS_SHA_Digest sSHA_Digest;
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
	{0},
	{0},
};

// *****************************************************************************

__attribute__ ((section(".JumpSignatureData")))
static uint8_t mau8JumpSignature[32];

//COMPILER_ALIGNED(128

char str[64] = {0};
uint32_t arr[128] = {132, 124, 38, 54, 5, 6};

void delay(uint8_t time)
{
    for(int i = 0; i <= 1000000*time ; i++)
    {
        __NOP();
    }
}

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
//		GPIO_PA23_Toggle();

//        if(PIO_PortRead(PIO_PIN_PA9) == 0)
//        {
//        sprintf(str, "Jump To App\n", 13);
        UART3_Write("Jump To App\n", 13);
//        delay(16);
            // JumpToUserApplication();
//        }
		// InitFlashWaitStates();
        uint32_t addr = U32_USER_APPLICATION_START_ADDRESS;

        SCB_DisableICache();
        SCB_DisableDCache();

        while(EFC_IsBusy());
        EFC_RegionUnlock(addr);

        while(EFC_IsBusy());
        EFC_SectorErase(addr);

        while(EFC_IsBusy());
        EFC_PageWrite(arr, addr);

        while(EFC_IsBusy());
        
        uint32_t *p = (uint32_t*)U32_USER_APPLICATION_START_ADDRESS;

        sprintf(str, "%d\n",  p[0], 3);
        UART3_Write(str, 3);
        delay(32);
        
        sprintf(str, "%d\n",  p[1], 3);
        UART3_Write(str, 3);
        delay(32);
        
        sprintf(str, "%d\n",  p[2], 3);
        UART3_Write(str, 3);
        delay(32);
        
        sprintf(str, "%d\n",  p[3], 3);
        UART3_Write(str, 3);
        delay(32);
        
      
        /* invalidate cache */
        SCB_InvalidateICache();
        SCB_InvalidateDCache();

        SCB_EnableICache();
        SCB_EnableDCache();
          
            delay(64);
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

//	TS_ApplicationData* pCurrentApplicationData;
//	TS_ApplicationData* pUserApplicationData;
//	uint32_t u32StackPointerValue;
//	fpJumpHandler fpApplicationResetHandler;
////		
//
//static void JumpToUserApplication(void)
//{
////	TS_ApplicationData* pCurrentApplicationData;
////	TS_ApplicationData* pUserApplicationData;
////	uint32_t u32StackPointerValue;
////	fpJumpHandler fpApplicationResetHandler;
////		
//	/** Release Resources before jumping to User Application */
////	udc_stop();
////	sysclk_disable_peripheral_clock(CONSOLE_UART_ID);
//    
//    pCurrentApplicationData = P_GetApplicationData();
//    sprintf(str, "pCurrentApplicationData: %X\n", pCurrentApplicationData, 64);
//    UART3_Write(str, 64);
//    delay(16);
//    
//    pUserApplicationData = (TS_ApplicationData*)(pCurrentApplicationData->u32UserApplicationStartAddress +
//		pCurrentApplicationData->u32UserApplicationAllocationSize - sizeof(TS_ApplicationFooter));
//    sprintf(str, "pCurrentApplicationData: %X\n", pUserApplicationData, 64);
//    UART3_Write(str, 64);
//    delay(16);
//    
//	sprintf(str, "SCB->VTOR: %X\n", pUserApplicationData->u32UserApplicationStartAddress, 64);
//    UART3_Write(str, 64);
//    delay(16);
//    
//    	/** Update stack pointer */
//	u32StackPointerValue = (uint32_t)(*(uint32_t *)(pUserApplicationData->u32UserApplicationStartAddress));
//    sprintf(str, "u32StackPointerValue: %X\n", u32StackPointerValue, 64);
//    UART3_Write(str, 64);
//    delay(16);
//	
//	/** Call Application reset handler */
//	fpApplicationResetHandler = (fpJumpHandler)(*((uint32_t*)(pUserApplicationData->u32UserApplicationStartAddress + 4)));
//    sprintf(str, "fpApplicationResetHandler: %X\n", fpApplicationResetHandler, 64);
//    UART3_Write(str, 64);
//    delay(16);
//    
//	/** Disable interrupts */
//	__disable_irq();
//	
//	/** Get Application header data*/
//	pCurrentApplicationData = P_GetApplicationData();
//	
//	pUserApplicationData = (TS_ApplicationData*)(pCurrentApplicationData->u32UserApplicationStartAddress +
//		pCurrentApplicationData->u32UserApplicationAllocationSize - sizeof(TS_ApplicationFooter));
//    
//	
//	/** Barriers */
//	__DSB();
//	__ISB();
//
//	/** Update vector table */
//	SCB->VTOR = pUserApplicationData->u32UserApplicationStartAddress & SCB_VTOR_TBLOFF_Msk;
//	
//	/** Barriers */
//	__DSB();
//	__ISB();
//
//	/** Enable interrupts */
//	__enable_irq();
//
//	/** Update stack pointer */
//	u32StackPointerValue = (uint32_t)(*(uint32_t *)(pUserApplicationData->u32UserApplicationStartAddress));
//	__set_MSP(u32StackPointerValue);
//	
//	/** Call Application reset handler */
//	fpApplicationResetHandler = (fpJumpHandler)(*((uint32_t*)(pUserApplicationData->u32UserApplicationStartAddress + 4)));
//	(*fpApplicationResetHandler)();
//    
//}
//
//
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

