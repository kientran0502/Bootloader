#ifndef BL_TYPES_H
#define BL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
/* Command definitions */
// typedef enum
// {
//     BL_CMD_PING  = 0x01,
//     BL_CMD_READ  = 0x02,
//     BL_CMD_ERASE = 0x03,
//     BL_CMD_WRITE = 0x04,
//     BL_CMD_JUMP  = 0x05
// } BlCommand_t;

/* ACK/NACK */
#define BL_ACK   0x79
#define BL_NACK  0x1F

#define BL_APP_START_ADDR   0x0040C000U /// address where user app starts  (after 48KB bootloader)
#define BL_APP_SIZE         (512 * 1024)  // 512KB max app size
#define BL_APP_MAX_SIZE     (2000 * 1024)    // 2000KB max app size

/* Generic status */
typedef enum
{
    BL_STATUS_OK = 0,
    BL_STATUS_ERROR,
    BL_STATUS_TIMEOUT,
    BL_STATUS_INVALID,
    BL_STATUS_BUSY
} BlStatus_t;

#endif