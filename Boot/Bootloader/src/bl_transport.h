#ifndef BL_TRANSPORT_H
#define BL_TRANSPORT_H

#include "bl_types.h"

/* Init UART hardware */
void BlTransport_Init(void);

/* Read 1 byte from RX queue (non-blocking) */
BlStatus_t BlTransport_ReadByte(uint8_t *byte);

/* Write buffer */
BlStatus_t BlTransport_Write(uint8_t *data, uint16_t len);

/* Flush RX buffer */
void BlTransport_Flush(void);

#endif