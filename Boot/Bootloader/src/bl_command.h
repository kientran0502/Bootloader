#ifndef BL_COMMAND_H
#define BL_COMMAND_H

#include "bl_types.h"
#include "bl_protocol.h"

// #define BL_APP_START_ADDR   0x0040C000

#define CMD_PING            0x01
#define CMD_READ            0x02
#define CMD_ERASE           0x03
#define CMD_WRITE           0x04
#define CMD_JUMP_APP        0x05

#define CMD_ACK             0x79
#define CMD_NACK            0x1F

/* Transaction state */
typedef enum
{
    BL_STATE_IDLE,
    BL_STATE_READY,
    BL_STATE_WRITING,
    BL_STATE_ERROR
} BlTransactionState_t;

/* Command context */
typedef struct
{
    BlTransactionState_t state;
    // uint32_t currentAddr;
    uint32_t totalWritten;
    uint32_t lastActivityTime;
} BlCommandContext_t;

/* Init command handler */
void BlCommand_Init(BlCommandContext_t *ctx);

/* Execute command */
BlStatus_t BlCommand_Execute(BlCommandContext_t *ctx, BlPacket_t *pkt);

/* Reset transaction */
void BlCommand_Reset(BlCommandContext_t *ctx);

#endif