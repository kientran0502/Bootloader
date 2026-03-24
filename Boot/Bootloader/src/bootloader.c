#include "bootloader.h"
#include "bl_transport.h"
#include "definitions.h"                // SYS function prototypes

typedef void (*fpJumpHandler)(void);

// ****************************** DEFINE ***********************************************
#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MAJOR_VERSION		((uint32_t)0x1)
#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MINOR_VERSION		((uint32_t)0x1)
#define U32_BOOT_LOADER_APPLICATION_START_ADDRESS			((uint32_t)0x00400000)
#define U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE			((uint32_t)0x0000C000)
#define U32_USER_APPLICATION_START_ADDRESS					((uint32_t)0x0040C000)
#define U32_USER_APPLICATION_ALLOCATED_SIZE					((uint32_t)0x001F4000)

// ***************************** STRUCT ************************************************

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


void Bootloader_Init(BootloaderContext_t *ctx, BootloaderConfig_t *cfg)
{
    ctx->config = *cfg;
    BlProtocol_Init(&ctx->protocolCtx);
    BlCommand_Init(&ctx->commandCtx);
    ctx->startTime = 0; // Set to current time (not implemented)
    ctx->updateMode = false;
}

extern volatile uint8_t flag;
void Bootloader_Run(BootloaderContext_t *ctx)
{
    BlPacket_t *pkt;

    // 1. Process protocol
    if (BlProtocol_Process(&ctx->protocolCtx) == BL_STATUS_OK)
    {
        if (BlProtocol_GetPacket(&ctx->protocolCtx, &pkt) == BL_STATUS_OK)
        {
            ctx->updateMode = true;

            if (BlProtocol_VerifyCrc(pkt))
            {
                if (BlCommand_Execute(&ctx->commandCtx, pkt) == BL_STATUS_OK)
                {
                   BlTransport_Write((uint8_t[]){BL_ACK}, 1);
                }
                else
                {
                   BlTransport_Write((uint8_t[]){BL_NACK}, 1);
                }
            }
            else
            {
               BlTransport_Write((uint8_t[]){BL_NACK}, 1);
            }

            BlProtocol_Reset(&ctx->protocolCtx);
        }
    }

    // 2. Boot timeout logic
    // if (!ctx->updateMode)
    // {
        // if (GetTick() - ctx->startTime > ctx->config.timeoutMs)
        // {
            // if (BlFlash_IsAppValid())
            // {
                if(flag) 
                {
//                    BlTransport_Write("1234\n",5);
//                    Bootloader_JumpToApp();
                    JumpToUserApplication();
                    flag = 0;
                }
            // }
        // }
    // }
}



__attribute__ ((section(".JumpSignatureData")))
static uint8_t mau8JumpSignature[32];

	TS_ApplicationData* pCurrentApplicationData;
	TS_ApplicationData* pUserApplicationData;
	uint32_t u32StackPointerValue;
	fpJumpHandler fpApplicationResetHandler;
    uint32_t str[64] = {0};

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
    
    pCurrentApplicationData = P_GetApplicationData();
    sprintf(str, "pCurrentApplicationData: %X\n", pCurrentApplicationData, 64);
    UART3_Write(str, 64);
    delay(16);
    
    pUserApplicationData = (TS_ApplicationData*)(pCurrentApplicationData->u32UserApplicationStartAddress +
		pCurrentApplicationData->u32UserApplicationAllocationSize - sizeof(TS_ApplicationFooter));
    sprintf(str, "pCurrentApplicationData: %X\n", pUserApplicationData, 64);
    UART3_Write(str, 64);
    delay(16);
    
	sprintf(str, "SCB->VTOR: %X\n", pUserApplicationData->u32UserApplicationStartAddress, 64);
    UART3_Write(str, 64);
    delay(16);
    
    	/** Update stack pointer */
	u32StackPointerValue = (uint32_t)(*(uint32_t *)(pUserApplicationData->u32UserApplicationStartAddress));
    sprintf(str, "u32StackPointerValue: %X\n", u32StackPointerValue, 64);
    UART3_Write(str, 64);
    delay(16);
	
	/** Call Application reset handler */
	fpApplicationResetHandler = (fpJumpHandler)(*((uint32_t*)(pUserApplicationData->u32UserApplicationStartAddress + 4)));
    sprintf(str, "fpApplicationResetHandler: %X\n", fpApplicationResetHandler, 64);
    UART3_Write(str, 64);
    delay(16);
    
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

void Bootloader_JumpToApp(void)
{
    fpJumpHandler fpApplicationResetHandler;
    uint32_t StackPointerValue;
    
StackPointerValue = (uint32_t)(*(uint32_t *)(BL_APP_START_ADDR));
uint32_t *vt = (uint32_t*)BL_APP_START_ADDR;

uint32_t sp    = vt[0];
uint32_t reset = vt[1];
sprintf(str, "StackPointerValue: %X\n", sp, 64);
BlTransport_Write(str, 64);
 delay(16);
 
StackPointerValue = (uint32_t)(*(uint32_t *)(BL_APP_START_ADDR + 4));
sprintf(str, "ResetHandler: %X\n", reset, 64);
BlTransport_Write(str, 64);
 delay(16);
    	/** Disable interrupts */
	__disable_irq();
	
	/** Barriers */
	__DSB();
	__ISB();

	/** Update vector table */
	SCB->VTOR = BL_APP_START_ADDR & SCB_VTOR_TBLOFF_Msk;
	
	/** Barriers */
	__DSB();
	__ISB();

	/** Enable interrupts */
	__enable_irq();

	/** Update stack pointer */
	StackPointerValue = (uint32_t)(*(uint32_t *)(BL_APP_START_ADDR));
	__set_MSP(StackPointerValue);
	
	/** Call Application reset handler */
	fpApplicationResetHandler = (fpJumpHandler)(*((uint32_t*)(BL_APP_START_ADDR + 4)));
	(*fpApplicationResetHandler)();
}


TS_ApplicationData* P_GetApplicationData(void)
{
	return(&(sBootLoaderFooter.uApplicationData.sApplicationData));
}

static void BootloaderJumpHandler(void)
{
//	/** Update Signature to indicate Boot loader */
////	strcpy(mau8JumpSignature, "StayInBootLoader");
}