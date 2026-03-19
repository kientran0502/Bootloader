#include "bl_protocol.h"
#include "bl_transport.h"

void BlProtocol_Init(BlProtocolContext_t *ctx)
{
    ctx->state = BL_PROTO_IDLE;
    ctx->index = 0;
    ctx->lastRxTime = 0;
}

BlStatus_t BlProtocol_Process(BlProtocolContext_t *ctx)
{
    uint8_t byte;

    while (BlTransport_ReadByte(&byte) == BL_STATUS_OK)
    {
//        ctx->lastRxTime = GetTick();

        switch (ctx->state)
        {
        case BL_PROTO_IDLE:

            if (byte == BL_PACKET_START_BYTE)
                ctx->state = BL_PROTO_CMD;
            break;

        case BL_PROTO_CMD:
            ctx->packet.cmd = byte;
            ctx->state = BL_PROTO_LEN;
            break;

        case BL_PROTO_LEN:
            if (byte > BL_MAX_DATA_LEN)
            {
                ctx->state = BL_PROTO_ERROR;
                break;
            }
            
            ctx->packet.len = byte;
            ctx->index = 0;
            ctx->state = (byte == 0) ? BL_PROTO_CRC : BL_PROTO_DATA;
            break;

        case BL_PROTO_DATA:
            ctx->packet.data[ctx->index++] = byte;
            if (ctx->index >= ctx->packet.len)
            {
                ctx->index = 0;
                ctx->state = BL_PROTO_CRC;
            }
            break;

        case BL_PROTO_CRC:
            ((uint8_t*)&ctx->packet.crc)[ctx->index++] = byte;
            if (ctx->index >= 4)
            {
                ctx->state = BL_PROTO_COMPLETE;
                return BL_STATUS_OK;
            }
            break;

        default:
            BlProtocol_Reset(ctx);
            return BL_STATUS_ERROR;
        }
    }

    return BL_STATUS_BUSY;
}

BlStatus_t BlProtocol_GetPacket(BlProtocolContext_t *ctx, BlPacket_t **pkt)
{
    if (ctx->state == BL_PROTO_COMPLETE) 
    {
        *pkt = &ctx->packet;
        return BL_STATUS_OK;
    }
    return BL_STATUS_ERROR;
}

void BlProtocol_Reset(BlProtocolContext_t *ctx)
{
    ctx->state = BL_PROTO_IDLE;
    ctx->index = 0;
    ctx->lastRxTime = 0;
}

bool BlProtocol_VerifyCrc(BlPacket_t *pkt)
{
//    uint32_t calcCrc = BlProtocol_CalcCrc((uint8_t*)&pkt->cmd, sizeof(pkt->cmd) + sizeof(pkt->len) + pkt->len);
//    return (calcCrc == pkt->crc);
    return 1;
}



