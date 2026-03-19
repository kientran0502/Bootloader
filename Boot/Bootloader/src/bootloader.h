#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "bl_types.h"
#include "bl_protocol.h"
#include "bl_command.h"

/* Bootloader config */
typedef struct
{
    uint32_t timeoutMs;
} BootloaderConfig_t;

/* Bootloader context */
typedef struct
{
    BootloaderConfig_t config;

    BlProtocolContext_t protocolCtx;
    BlCommandContext_t  commandCtx;

    uint32_t startTime;
    bool updateMode;
} BootloaderContext_t;

/* Init bootloader */
void Bootloader_Init(BootloaderContext_t *ctx, BootloaderConfig_t *cfg);

/* Main loop process */
void Bootloader_Run(BootloaderContext_t *ctx);

/* Jump to application */
void Bootloader_JumpToApp(void);

#endif