#include "bootloader.h"

void Bootloader_Init(BootloaderContext_t *ctx, BootloaderConfig_t *cfg)
{
    ctx->config = *cfg;
    BlProtocol_Init(&ctx->protocolCtx);
    BlCommand_Init(&ctx->commandCtx);
    ctx->startTime = 0; // Set to current time (not implemented)
    ctx->updateMode = false;
}

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
//                    BlTransport_Write((uint8_t[]){BL_ACK}, 1);
                }
                else
                {
//                    BlTransport_Write((uint8_t[]){BL_NACK}, 1);
                }
            }
            else
            {
//                BlTransport_Write((uint8_t[]){BL_NACK}, 1);
            }

            BlProtocol_Reset(&ctx->protocolCtx);
        }
    }

    // 2. Boot timeout logic
    // if (!ctx->updateMode)
    // {
    //     if (GetTick() - ctx->startTime > ctx->config.timeoutMs)
    //     {
    //         if (BlFlash_IsAppValid())
    //         {
    //             Bootloader_JumpToApp();
    //         }
    //     }
    // }
}


void Bootloader_JumpToApp(void)
{
    // In a real bootloader, this would set the MSP and jump to the application reset vector
    // For simulation, we just print a message
//    printf("Jumping to application...\n");
}