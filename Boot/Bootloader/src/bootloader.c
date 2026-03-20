#include "bootloader.h"
#include "bl_transport.h"
#include "definitions.h"                // SYS function prototypes

typedef void (*fpJumpHandler)(void);

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
                    BlTransport_Write("1234\n",5);
                    Bootloader_JumpToApp();
                    flag = 0;
                }
            // }
        // }
    // }
}


void Bootloader_JumpToApp(void)
{
    fpJumpHandler fpApplicationResetHandler;
    uint32_t StackPointerValue;
    uint32_t str[64] = {0};
StackPointerValue = (uint32_t)(*(uint32_t *)(BL_APP_START_ADDR));
sprintf(str, "StackPointerValue: %X\n", StackPointerValue, 64);
BlTransport_Write(str, 64);
 delay(16);
 
StackPointerValue = (uint32_t)(*(uint32_t *)(BL_APP_START_ADDR + 4));
sprintf(str, "ResetHandler: %X\n", StackPointerValue, 64);
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


