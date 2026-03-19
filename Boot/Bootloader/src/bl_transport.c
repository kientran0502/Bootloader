#include "bl_transport.h"
#include "Dev/M2_BSP/UART/uart_irq.h"

void BlTransport_Init(void)
{
    UART_Driver_Init();
}

BlStatus_t BlTransport_Write(uint8_t *data, uint16_t len)
{
    if(UART_Driver_Write(UART3_REGS, data, len) == 0)
    {
        return BL_STATUS_ERROR;
    }
    return BL_STATUS_OK;
}

BlStatus_t BlTransport_ReadByte(uint8_t *byte)
{
    int ret = UART_Driver_ReadByte(UART3_REGS);

    if(ret == -1)
    {
        return BL_STATUS_ERROR;
    }

    *byte = (uint8_t)ret;
    return BL_STATUS_OK;
}

