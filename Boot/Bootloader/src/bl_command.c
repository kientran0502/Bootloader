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

volatile uint8_t flag = 0;
uint8_t buf[248] = {0};
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
            if (BlFlash_Erase(BL_APP_START_ADDR, BL_APP_MAX_SIZE / EFC_SECTORSIZE) != BL_STATUS_OK)  // BL_APP_SIZE để tạm
                return BL_STATUS_ERROR;

            // ctx->currentAddr = BL_APP_START_ADDR;
            ctx->totalWritten = 0;
            ctx->state = BL_STATE_READY;
            return BL_STATUS_OK;

        case CMD_WRITE:
        {
            if (ctx->state != BL_STATE_READY && ctx->state != BL_STATE_WRITING)
                return BL_STATUS_ERROR;

            if (pkt->len < 4)
                return BL_STATUS_INVALID;

            uint32_t addr;
            memcpy(&addr, pkt->data, 4);

            uint8_t *payload = pkt->data + 4;
            uint16_t payloadLen = pkt->len - 4;

            /* Check address range */
            if (addr < BL_APP_START_ADDR || addr + payloadLen > BL_APP_START_ADDR + BL_APP_MAX_SIZE)
            {
                return BL_STATUS_INVALID;
            }

            if (BlFlash_Write(addr, payload, payloadLen) != BL_STATUS_OK)
                return BL_STATUS_ERROR;

            ctx->totalWritten += payloadLen;
            ctx->state = BL_STATE_WRITING;
            return BL_STATUS_OK;
        }
        case CMD_READ:
        {
            if (pkt->len < 4)
                return BL_STATUS_INVALID;

            uint32_t addr;
            memcpy(&addr, pkt->data, 4);

            

            if (BlFlash_Read(addr, buf, sizeof(buf)) != BL_STATUS_OK)
                return BL_STATUS_ERROR;
            
            BlTransport_Write((uint8_t[]){BL_ACK}, 1);
            
            BlTransport_Write(buf, sizeof(buf));
            return BL_STATUS_OK;
        }

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
    // ctx->currentAddr = BL_APP_START_ADDR;
    ctx->totalWritten = 0;
    ctx->lastActivityTime = 0;
}