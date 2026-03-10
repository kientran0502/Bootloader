/************************************************
 *  @file     : uart_irq.c
 *  @date     : December, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    UART driver with RX/TX interrupt support
 *    - RX: Interrupt-driven with ring buffer
 *    - TX: Interrupt-driven with ring buffer
 ************************************************/
#include "uart_irq.h"
#include <string.h>

/*************************************************
 *                  BUFFERS                      *
 *************************************************/
static RingBufElement uart2_rx_data[UART2_RX_BUFFER_SIZE];
static RingBufElement uart2_tx_data[UART2_TX_BUFFER_SIZE];

static UART_Driver_t uart_drivers[UART_DRIVER_COUNT] = {
    {
        .uart              = UART2_REGS,
        .rx_buffer          = {0},
        .tx_buffer          = {0},
        .rx_data            = uart2_rx_data,
        .tx_data            = uart2_tx_data,
        .rx_buffer_size     = UART2_RX_BUFFER_SIZE,
        .tx_buffer_size     = UART2_TX_BUFFER_SIZE,
        .tx_busy            = false,
        .rx_error           = false,
        .rx_overflow_count  = 0,
        .rx_error_count     = 0
    }
};

/*************************************************
 *              FORWARD DECLARATIONS             *
 *************************************************/
static UART_Driver_t* UART_Driver_Get(uart_registers_t* uart);
static void UART_Driver_StartTx(UART_Driver_t* d);


static void UART_RX_Callback(uintptr_t context)
{
    UART_Driver_t* d = (UART_Driver_t*)context;
    
    uint32_t status = d->uart->UART_SR;
    
    if (status & (UART_SR_OVRE_Msk | UART_SR_FRAME_Msk | UART_SR_PARE_Msk))
    {
        d->rx_error = true;
        d->rx_error_count++;
        
        /* Clear errors */
        d->uart->UART_CR = UART_CR_RSTSTA_Msk;
        
        /* Flush error */
        while (d->uart->UART_SR & UART_SR_RXRDY_Msk)
        {
            volatile uint8_t dummy = (uint8_t)(d->uart->UART_RHR & UART_RHR_RXCHR_Msk);
            (void)dummy;
        }
        
        d->uart->UART_IDR = (UART_IDR_FRAME_Msk | UART_IDR_PARE_Msk | UART_IDR_OVRE_Msk);
        
        /* Re-enable */
        d->uart->UART_IER = (UART_IER_FRAME_Msk | UART_IER_PARE_Msk | UART_IER_OVRE_Msk);
        
        return;
    }
    
    while (status & UART_SR_RXRDY_Msk)
    {
        uint8_t data = (uint8_t)(d->uart->UART_RHR & UART_RHR_RXCHR_Msk);
        
        if (!RingBuffer_Put(&d->rx_buffer, data))
        {
            d->rx_overflow_count++;     // Buffer overflow -> FREERTOS Implement?
        }
        
        /* Re-check more data */
        status = d->uart->UART_SR;
    }
}

static void UART_TX_Callback(uintptr_t context)
{
    UART_Driver_t* d = (UART_Driver_t*)context;
    
    if (d->uart->UART_SR & UART_SR_TXRDY_Msk)
    {
        RingBufElement data;
        
        if (RingBuffer_Get(&d->tx_buffer, &data))
        {
            /* Send byte */
            d->uart->UART_THR = (uint32_t)data;
        }
        else
        {
            /* No more data - disable TX interrupt */
            d->uart->UART_IDR = UART_IDR_TXRDY_Msk;
            d->tx_busy = false;
        }
    }
}

/*************************************************
 *              INITIALIZATION                   *
 *************************************************/
/* Warning 
--------------- WARNING!!!!!!!!!!!!!! ----------------
You must edit plib_uartx.c like blow:
Because the default handler will not call the registered callback functions.
--------------- Patch in plib_uartx.c ----------------
if(UART_SR_RXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_RXRDY_Msk))
{
    UART2_ISR_RX_Handler();
    uintptr_t rxContext = uart2Obj.rxContext;
    uart2Obj.rxCallback(rxContext);
}
if(UART_SR_TXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_TXRDY_Msk))
{
    UART2_ISR_TX_Handler();
    uintptr_t txContext = uart2Obj.txContext;
    uart2Obj.txCallback(txContext);
}
*/
 
Std_ReturnType UART_Driver_Init(void)
{
    /* Initialize UART hardware (already done by UARTx_Initialize) */
    // UARTx_Initialize();
    
    /* [0] UART in list: Init following all step below: ----------- */
    /* Initialize ring buffers */
    RingBuffer_Create(&uart_drivers[0].rx_buffer, 1,
                     "UART2_RX", uart_drivers[0].rx_data, 
                     uart_drivers[0].rx_buffer_size);
    
    RingBuffer_Create(&uart_drivers[0].tx_buffer, 2,
                     "UART2_TX", uart_drivers[0].tx_data, 
                     uart_drivers[0].tx_buffer_size);
    
    uart_drivers[0].tx_busy = false;
    uart_drivers[0].rx_error = false;
    uart_drivers[0].rx_overflow_count = 0;
    uart_drivers[0].rx_error_count = 0;
    
    UART2_ReadCallbackRegister(UART_RX_Callback, (uintptr_t)&uart_drivers[0]);
    UART2_WriteCallbackRegister(UART_TX_Callback, (uintptr_t)&uart_drivers[0]);

    UART2_REGS->UART_IER = UART_IER_RXRDY_Msk | 
                           UART_IER_OVRE_Msk | 
                           UART_IER_FRAME_Msk | 
                           UART_IER_PARE_Msk;
    /* [0] UART in list: Done one --------------------------------- */
    /* ... */

    return E_OK;
}

/*************************************************
 *              HELPER FUNCTIONS                 *
 *************************************************/
static UART_Driver_t* UART_Driver_Get(uart_registers_t* uart)
{
    for (uint32_t i = 0; i < UART_DRIVER_COUNT; i++)
    {
        if (uart_drivers[i].uart == uart)
            return &uart_drivers[i];
    }
    return NULL;
}

/* Trigger TX */
static void UART_Driver_StartTx(UART_Driver_t* d)
{
    if (d->tx_busy) {
        return; 
    }
    
    RingBufElement data;
    if (RingBuffer_Get(&d->tx_buffer, &data))
    {
        d->tx_busy = true;
        
        if (d->uart->UART_SR & UART_SR_TXRDY_Msk)
        {
            d->uart->UART_THR = (uint32_t)data;
        }
        
        d->uart->UART_IER = UART_IER_TXRDY_Msk;
    }
}

/*************************************************
 *              PUBLIC API - WRITE               *
 *************************************************/
bool UART_Driver_WriteByte(uart_registers_t* uart, uint8_t data)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return false;
    }
    
    if (!RingBuffer_Put(&driver->tx_buffer, data)) {
        return false;  // Buffer full
    }
    
    UART_Driver_StartTx(driver);
    
    return true;
}

size_t UART_Driver_Write(uart_registers_t* uart, const uint8_t* data, size_t length)
{
    if (data == NULL || length == 0) {
        return 0;
    }
    
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    size_t written = 0;
    
    for (size_t i = 0; i < length; i++)
    {
        if (!RingBuffer_Put(&driver->tx_buffer, data[i])) {
            break;  // Buffer full
        }
        written++;
    }
    
    if (written > 0) {
        UART_Driver_StartTx(driver);
    }
    
    return written;
}

size_t UART_Driver_SendString(uart_registers_t* uart, const char* str)
{
    if (str == NULL) {
        return 0;
    }
    
    size_t len = strlen(str);
    return UART_Driver_Write(uart, (const uint8_t*)str, len);
}

/*************************************************
 *              PUBLIC API - READ                *
 *************************************************/
int UART_Driver_ReadByte(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return -1;
    }
    
    RingBufElement data;
    if (RingBuffer_Get(&driver->rx_buffer, &data)) {
        return (int)data;
    }
    
    return -1;
}

size_t UART_Driver_Read(uart_registers_t* uart, uint8_t* buffer, size_t length)
{
    if (buffer == NULL || length == 0) {
        return 0;
    }
    
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    size_t read = 0;
    RingBufElement data;
    
    while (read < length && RingBuffer_Get(&driver->rx_buffer, &data))
    {
        buffer[read++] = data;
    }
    
    return read;
}

bool UART_Driver_IsDataAvailable(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return false;
    }
    
    return RingBuffer_IsDataAvailable(&driver->rx_buffer);
}

//size_t UART_Driver_RXDataAvailable(uart_registers_t* uart)
//{
//    UART_Driver_t* driver = UART_Driver_Get(uart);
//    if (driver == NULL) {
//        return 0;
//    }
//    
//    return RingBuffer_NumDataAvailable(&driver->rx_buffer);
//}

/*************************************************
 *              PUBLIC API - STATUS              *
 *************************************************/
size_t UART_Driver_TXFreeSpace(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    return RingBuffer_NumFreeSlots(&driver->tx_buffer);
}

size_t UART_Driver_RXFreeSpace(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    return RingBuffer_NumFreeSlots(&driver->rx_buffer);
}

bool UART_Driver_IsTxBusy(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return false;
    }
    
    return driver->tx_busy || RingBuffer_IsDataAvailable(&driver->tx_buffer);
}

bool UART_Driver_WaitTxComplete(uart_registers_t* uart, uint32_t timeout_ms)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return false;
    }
    
    uint32_t timeout_count = timeout_ms * 1000; 
    
    while (UART_Driver_IsTxBusy(uart))
    {
        if (timeout_ms > 0 && timeout_count-- == 0) {
            return false;  // Timeout 
        }
    }
    
    while ((uart->UART_SR & UART_SR_TXEMPTY_Msk) == 0U)
    {
        if (timeout_ms > 0 && timeout_count-- == 0) {
            return false;  // Timeout
        }
    }
    
    return true;
}

/*************************************************
 *              PUBLIC API - FLUSH               *
 *************************************************/


 
void UART_Driver_FlushTx(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return;
    }

    uart->UART_IDR = UART_IDR_TXRDY_Msk;

    memset(driver->tx_buffer.buffer, 0, driver->tx_buffer.max_size);

    driver->tx_buffer.head = 0U;
    driver->tx_buffer.tail = 0U;


    driver->tx_busy = false;
}


void UART_Driver_FlushRx(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return;
    }

    uart->UART_IDR = UART_IDR_RXRDY_Msk |
                     UART_IDR_OVRE_Msk |
                     UART_IDR_FRAME_Msk |
                     UART_IDR_PARE_Msk;

    memset(driver->rx_buffer.buffer, 0, driver->rx_buffer.max_size);


    driver->rx_buffer.head = 0U;
    driver->rx_buffer.tail = 0U;

    while (uart->UART_SR & UART_SR_RXRDY_Msk)
    {
        volatile uint32_t dummy = uart->UART_RHR;
        (void)dummy;
    }

    uart->UART_CR = UART_CR_RSTSTA_Msk;
    driver->rx_error = false;

    uart->UART_IER = UART_IER_RXRDY_Msk |
                     UART_IER_OVRE_Msk |
                     UART_IER_FRAME_Msk |
                     UART_IER_PARE_Msk;
}

void UART_Driver_Flush(uart_registers_t* uart)
{
    UART_Driver_FlushRx(uart);
    UART_Driver_FlushTx(uart);
}

/*************************************************
 *              PUBLIC API - ERROR HANDLING      *
 *************************************************/
bool UART_Driver_HasRxError(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return false;
    }
    
    return driver->rx_error;
}

void UART_Driver_ClearRxError(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return;
    }
    
    driver->rx_error = false;
}

uint32_t UART_Driver_GetRxOverflowCount(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    return driver->rx_overflow_count;
}

uint32_t UART_Driver_GetRxErrorCount(uart_registers_t* uart)
{
    UART_Driver_t* driver = UART_Driver_Get(uart);
    if (driver == NULL) {
        return 0;
    }
    
    return driver->rx_error_count;
}

/*************************************************
 *              POLLING MODE (BACKUP)            *
 *************************************************/
void UART_Driver_Polling_WriteByte(uart_registers_t* uart, uint8_t data)
{
    while ((uart->UART_SR & UART_SR_TXRDY_Msk) == 0U)
    {
    }
    
    uart->UART_THR = (uint32_t)data;
}

void UART_Driver_Polling_SendString(uart_registers_t* uart, const char* str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str)
    {
        UART_Driver_Polling_WriteByte(uart, (uint8_t)(*str));
        str++;
    }
    
    while ((uart->UART_SR & UART_SR_TXEMPTY_Msk) == 0U)
    {
    }
}