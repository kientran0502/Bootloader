#include "bl_command.h"
#include "bl_flash.h"
#include "bl_transport.h"



void BlCommand_Init(BlCommandContext_t *ctx)
{
    ctx->state = BL_STATE_IDLE;
    // ctx->currentAddr = BL_APP_START_ADDR;  // 
    ctx->totalWritten = 0;
    ctx->lastActivityTime = 0;
}

uint32_t test_count = 0;
volatile uint8_t flag = 0;
BlStatus_t BlCommand_Execute(BlCommandContext_t *ctx, BlPacket_t *pkt)
{
    if (ctx == NULL || pkt == NULL)
        return BL_STATUS_INVALID;

    switch (pkt->cmd)
    {
        case CMD_PING:
            // Blink an LED or just return ACK
            return BL_STATUS_OK;

        case CMD_ERASE:
            if (BlFlash_Erase(BL_APP_START_ADDR, BL_APP_SIZE / EFC_SECTORSIZE) != BL_STATUS_OK)
                return BL_STATUS_ERROR;

            ctx->currentAddr = BL_APP_START_ADDR;
            ctx->totalWritten = 0;
            ctx->state = BL_STATE_READY;
            return BL_STATUS_OK;

        case CMD_WRITE:
            if (ctx->state != BL_STATE_READY && ctx->state != BL_STATE_WRITING)
                return BL_STATUS_ERROR;
            test_count++;
            if (BlFlash_Write(ctx->currentAddr, pkt->data, pkt->len) != BL_STATUS_OK)
                return BL_STATUS_ERROR;

            ctx->currentAddr += pkt->len;
            ctx->totalWritten += pkt->len;
            ctx->state = BL_STATE_WRITING;
            return BL_STATUS_OK;

        case CMD_READ:
//            if (BlFlash_Read(BL_APP_START_ADDR, memory, 252) != BL_STATUS_OK)
//                return BL_STATUS_ERROR;
            BlTransport_Write(pkt->data, 252);
            return BL_STATUS_OK;

        case CMD_JUMP_APP:
//             return BL_STATUS_OK;
            
            flag = 1;
//            BlTransport_Write("JUMP\n",5);
            return BL_STATUS_OK;

        default:
            return BL_STATUS_ERROR;
    }
}

void BlCommand_Reset(BlCommandContext_t *ctx)
{
    ctx->state = BL_STATE_IDLE;
    ctx->currentAddr = BL_APP_START_ADDR;
    ctx->totalWritten = 0;
    ctx->lastActivityTime = 0;
}