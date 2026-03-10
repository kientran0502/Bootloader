#ifdef EX_UART_DMA
/************************************************
 *  @file     : usart_rxdma_txirq.c
 *  @date     : December, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    USART driver with RX DMA and TX DMA support
 *    - RX: DMA circular mode with polling check
 *    - TX: DMA single transfer mode
 *    - Ring buffer for both RX and TX
 ************************************************/
#include "uart_dma.h"
#include <string.h>

/*************************************************
 *                  BUFFERS                      *
 *************************************************/
static uint8_t usart_dma_rx_buffer[USART_DMA_RX_BUFFER_SIZE];
static uint8_t usart_dma_tx_buffer[USART_DMA_TX_BUFFER_SIZE];
static RingBufElement usart_rx_data[USART_BUFFER_SIZE];
static RingBufElement usart_tx_data[USART_BUFFER_SIZE];

static USART_Driver_t usart_drivers[USART_DRIVER_COUNT] = {
    {
        .usart               = UART_REGS,
        .rx_buffer           = {0},
        .tx_buffer           = {0},
        .dma_rx_buffer       = usart_dma_rx_buffer,
        .dma_tx_buffer       = usart_dma_tx_buffer,
        .dma_rx_buffer_size  = USART_DMA_RX_BUFFER_SIZE,
        .dma_tx_buffer_size  = USART_DMA_TX_BUFFER_SIZE,
        .rx_buffer_size      = USART_BUFFER_SIZE,
        .tx_buffer_size      = USART_BUFFER_SIZE,
        .xdmac_ch_rx         = XDMAC_CHANNEL_1,
        .xdmac_ch_tx         = XDMAC_CHANNEL_0,
        .old_dma_pos         = 0,
        .tx_dma_busy         = false
    }
};

/*************************************************
 *              FORWARD DECLARATIONS             *
 *************************************************/
static USART_Driver_t* USART_Driver_Get(uart_registers_t* usart);
static size_t usart_dma_rx_received_bytes(USART_Driver_t* d);
static void usart_dma_rx_flush_delta(USART_Driver_t* d, size_t pos);
static void usart_dma_rxdma_cb(XDMAC_TRANSFER_EVENT event, uintptr_t context);
static void usart_dma_txdma_cb(XDMAC_TRANSFER_EVENT event, uintptr_t context);
static void usart_dma_tx_kick_if_idle(USART_Driver_t* d);

/*************************************************
 *              INITIALIZATION                   *
 *************************************************/
Std_ReturnType USART_Driver_Init(void)
{
    /* Initialize ring buffers */
    RingBuffer_Create(&usart_drivers[0].rx_buffer, 1,
                     "USART_RX", usart_rx_data, 
                     usart_drivers[0].rx_buffer_size);
    
    RingBuffer_Create(&usart_drivers[0].tx_buffer, 2,
                     "USART_TX", usart_tx_data, 
                     usart_drivers[0].tx_buffer_size);
    
    XDMAC_ChannelCallbackRegister(usart_drivers[0].xdmac_ch_rx,
                                   usart_dma_rxdma_cb,
                                   (uintptr_t)&usart_drivers[0]);
    
    XDMAC_ChannelCallbackRegister(usart_drivers[0].xdmac_ch_tx,
                                   usart_dma_txdma_cb,
                                   (uintptr_t)&usart_drivers[0]);
    
    bool ret = XDMAC_ChannelTransfer(usart_drivers[0].xdmac_ch_rx,
                                      (const void*)&UART2_REGS->UART_RHR,
                                      usart_drivers[0].dma_rx_buffer,
                                      usart_drivers[0].dma_rx_buffer_size);
    
    if (!ret) {
        return E_ERROR;
    }
    
    usart_drivers[0].old_dma_pos = 0;
    usart_drivers[0].tx_dma_busy = false;
    
    return E_OK;
}

/*************************************************
 *              HELPER FUNCTIONS                 *
 *************************************************/
static USART_Driver_t* USART_Driver_Get(uart_registers_t* usart)
{
    for (uint32_t i = 0; i < USART_DRIVER_COUNT; i++)
    {
        if (usart_drivers[i].usart == usart)
            return &usart_drivers[i];
    }
    return NULL;
}

static size_t usart_dma_rx_received_bytes(USART_Driver_t* d)
{
    
    uint32_t cubc = XDMAC_REGS->XDMAC_CHID[d->xdmac_ch_rx].XDMAC_CUBC;
    
    /* Extract UBLEN field (bits 23:0) */
    uint32_t remaining = (cubc & XDMAC_CUBC_UBLEN_Msk) >> XDMAC_CUBC_UBLEN_Pos;
    
    if (remaining > d->dma_rx_buffer_size) {
        remaining = (uint32_t)d->dma_rx_buffer_size;
    }
    
    return d->dma_rx_buffer_size - (size_t)remaining;
}

/* Flush delta [old_pos .. pos) from DMA buffer into RX ringbuffer */
static void usart_dma_rx_flush_delta(USART_Driver_t* d, size_t pos)
{
    if (pos == d->old_dma_pos) {
        return;
    }

    /* Handle circular buffer wrap-around */
    if (pos > d->old_dma_pos) {
        /* Normal case: copy from old_pos to pos */
        for (size_t i = d->old_dma_pos; i < pos; i++) {
            (void)RingBuffer_Put(&d->rx_buffer, d->dma_rx_buffer[i]);
        }
    } else {
        /* Wrap-around case: copy from old_pos to end, then from 0 to pos */
        for (size_t i = d->old_dma_pos; i < d->dma_rx_buffer_size; i++) {
            (void)RingBuffer_Put(&d->rx_buffer, d->dma_rx_buffer[i]);
        }
        for (size_t i = 0; i < pos; i++) {
            (void)RingBuffer_Put(&d->rx_buffer, d->dma_rx_buffer[i]);
        }
    }
    
    d->old_dma_pos = pos;
}

static void usart_dma_rxdma_cb(XDMAC_TRANSFER_EVENT event, uintptr_t context)
{
    USART_Driver_t* d = (USART_Driver_t*)context;
    
    if (event == XDMAC_TRANSFER_COMPLETE)
    {
        usart_dma_rx_flush_delta(d, d->dma_rx_buffer_size);
        d->old_dma_pos = 0;
        /* Restart RX DMA */
        (void)XDMAC_ChannelTransfer(
            d->xdmac_ch_rx,
            (const void*)&d->usart->UART_RHR,
            (const void*)d->dma_rx_buffer,
            d->dma_rx_buffer_size
        );
    }
    else if (event == XDMAC_TRANSFER_ERROR)
    {
        /* Error: reset and restart */
        XDMAC_ChannelDisable(d->xdmac_ch_rx);
        d->old_dma_pos = 0;
        
        (void)XDMAC_ChannelTransfer(
            d->xdmac_ch_rx,
            (const void*)&d->usart->UART_RHR,
            (const void*)d->dma_rx_buffer,
            d->dma_rx_buffer_size
        );
    }
}

/* TX DMA callback - called when transfer completes */
static void usart_dma_txdma_cb(XDMAC_TRANSFER_EVENT event, uintptr_t context)
{
    USART_Driver_t* d = (USART_Driver_t*)context;
    
    if (event == XDMAC_TRANSFER_COMPLETE)
    {
        d->tx_dma_busy = false;
        
        /* Check if more data to send */
        usart_dma_tx_kick_if_idle(d);
    }
    else if (event == XDMAC_TRANSFER_ERROR)
    {
        d->tx_dma_busy = false;
        usart_dma_tx_kick_if_idle(d);
    }
}

static void usart_dma_tx_kick_if_idle(USART_Driver_t* d)
{
    if (d->tx_dma_busy) {
        return;
    }

    size_t count = 0;
    RingBufElement data;
    
    
    while (count < d->dma_tx_buffer_size && 
           RingBuffer_Get(&d->tx_buffer, &data))
    {
        d->dma_tx_buffer[count++] = data;
    }
    
    if (count > 0) {
        d->tx_dma_busy = true;
        SCB_CleanInvalidateDCache();
        (void)XDMAC_ChannelTransfer(
            d->xdmac_ch_tx,
            (const void*)d->dma_tx_buffer,
            (const void*)&d->usart->UART_THR,
            count
        );
    }
}

/*************************************************
 *              RX POLLING CHECK                 *
 *************************************************/
void USART_DMA_Rx_Check(uart_registers_t* usart)
{
    USART_Driver_t* d = USART_Driver_Get(usart);
    if (d == NULL) {
        return;
    }
    SCB_CleanInvalidateDCache();
    XDMAC_ChannelSuspend(d->xdmac_ch_rx);
    
    size_t pos = usart_dma_rx_received_bytes(d);
    usart_dma_rx_flush_delta(d, pos);
    
    XDMAC_ChannelResume(d->xdmac_ch_rx);
}

/*************************************************
 *              PUBLIC API - WRITE               *
 *************************************************/
void USART_Driver_Write(uart_registers_t* usart, uint8_t data)
{

    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return;
    }
    
    uint32_t timeout = 500000;
    
    while (!RingBuffer_Put(&driver->tx_buffer, data))
    {
        if (--timeout == 0) {
            return;
        }
    }
    
    /* Kick TX DMA if idle */
    usart_dma_tx_kick_if_idle(driver);
}

void USART_Driver_SendString(uart_registers_t* usart, const char* str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str)
    {
        USART_Driver_Write(usart, (uint8_t)(*str));
        str++;
    }
}

/*************************************************
 *              PUBLIC API - READ                *
 *************************************************/
int USART_Driver_Read(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return -1;
    }
    
    RingBufElement data;
    if (RingBuffer_Get(&driver->rx_buffer, &data)) {
        return (int)data;
    }
    
    return -1;
}

bool USART_Driver_IsDataAvailable(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return false;
    }
    
    return RingBuffer_IsDataAvailable(&driver->rx_buffer);
}

/*************************************************
 *              PUBLIC API - STATUS              *
 *************************************************/
uint16_t USART_Driver_TXNumFreeSlots(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return 0;
    }
    
    return (uint16_t)RingBuffer_NumFreeSlots(&driver->tx_buffer);
}

uint16_t USART_Driver_RXNumFreeSlots(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return 0;
    }
    
    return (uint16_t)RingBuffer_NumFreeSlots(&driver->rx_buffer);
}

/*************************************************
 *              PUBLIC API - FLUSH               *
 *************************************************/
void USART_Driver_FlushTx(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return;
    }
    
    RingBuffer_Flush(&driver->tx_buffer);
}

void USART_Driver_FlushRx(uart_registers_t* usart)
{
    USART_Driver_t* driver = USART_Driver_Get(usart);
    if (driver == NULL) {
        return;
    }
    
    RingBuffer_Flush(&driver->rx_buffer);
}

void USART_Driver_Flush(uart_registers_t* usart)
{
    USART_Driver_FlushRx(usart);
    USART_Driver_FlushTx(usart);
}

/*************************************************
 *              POLLING MODE (BACKUP)            *
 *************************************************/
void USART_Driver_Polling_Write(uart_registers_t* usart, uint8_t data)
{
    while ((usart->UART_SR & UART_SR_TXEMPTY_Msk) == 0U)
    {
    }
    
    /* Send data */
    usart->UART_THR = data;
    
    while ((usart->UART_SR & UART_SR_TXEMPTY_Msk) == 0U)
    {
    }
}

void USART_Driver_Polling_SendString(uart_registers_t* usart, const char* str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str)
    {
        USART_Driver_Polling_Write(usart, (uint8_t)(*str));
        str++;
    }
}

#endif