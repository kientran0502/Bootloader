#ifndef BL_PROTOCOL_H
#define BL_PROTOCOL_H

#include "bl_types.h"

#define BL_ADDR_DATA_LEN                    4
#define BL_PAYLOAD_MAX_LEN                  248
#define BL_MAX_DATA_LEN                     (BL_ADDR_DATA_LEN + BL_PAYLOAD_MAX_LEN) 
#define BL_PACKET_START_BYTE                0xAA
#define BL_PACKET_SIZE                     (1 + 1 + BL_MAX_DATA_LEN + 4) // cmd + len + data + crc

typedef struct
{
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  data[BL_MAX_DATA_LEN];
    uint32_t crc;
} BlPacket_t;

/* Parser state machine */
typedef enum
{
    BL_PROTO_IDLE,
    BL_PROTO_CMD,
    BL_PROTO_LEN,
    BL_PROTO_DATA,
    BL_PROTO_CRC,
    BL_PROTO_COMPLETE,
    BL_PROTO_ERROR
} BlProtoState_t;

/* Protocol context (internal state machine) */
typedef struct
{
    BlProtoState_t state; // Current state of the parser
    BlPacket_t packet;  // Buffer for incoming packet
    uint16_t index;     // Current index DATA byte in packet 
    uint32_t lastRxTime;   // Timestamp of last received byte
} BlProtocolContext_t;

/* Init protocol */
void BlProtocol_Init(BlProtocolContext_t *ctx);

/* Process incoming bytes → build packet */
BlStatus_t BlProtocol_Process(BlProtocolContext_t *ctx);

/* Get completed packet */
BlStatus_t BlProtocol_GetPacket(BlProtocolContext_t *ctx, BlPacket_t **pkt);

/* Reset parser state */
void BlProtocol_Reset(BlProtocolContext_t *ctx);

/* CRC check */
bool BlProtocol_VerifyCrc(BlPacket_t *pkt);

/* CRC calculate */
uint32_t BlProtocol_CalcCrc(uint8_t *data, uint16_t len);

#endif