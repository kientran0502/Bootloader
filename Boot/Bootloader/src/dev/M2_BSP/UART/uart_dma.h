/************************************************
 *  @file     : usart_rxdma_txirq.h
 *  @date     : December, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    USART driver with RX DMA and TX DMA support
 ************************************************/
#ifdef EX_UART_DMA

#ifndef USART_RXDMA_TXIRQ_H
#define USART_RXDMA_TXIRQ_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
 *                  INCLUDES                     *
 *************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
//#include "device.h"
//#include "peripheral/xdmac/plib_xdmac.h"
#include "M5_Utils/RingBuffer/ring_buffer.h"  /* Assume ring buffer API exists */
#include "M5_Utils/Define/define.h"
    
/*************************************************
 *              CONFIGURATION                    *
 *************************************************/
#define USART_DRIVER_COUNT              (1U)

#define USART_DMA_RX_BUFFER_SIZE       (512U)
#define USART_DMA_TX_BUFFER_SIZE       (128U)
#define USART_BUFFER_SIZE              (1024U)

/*************************************************
 *              TYPE DEFINITIONS                 *
 *************************************************/
typedef uint8_t RingBufElement;

typedef struct {
    /* Hardware */
    uart_registers_t*      usart;
    
    /* Ring buffers */
    s_RingBufferType            rx_buffer;
    s_RingBufferType            tx_buffer;
    
    /* DMA buffers */
    uint8_t*                dma_rx_buffer;
    uint8_t*                dma_tx_buffer;
    size_t                  dma_rx_buffer_size;
    size_t                  dma_tx_buffer_size;
    
    /* Ring buffer sizes */
    size_t                  rx_buffer_size;
    size_t                  tx_buffer_size;
    
    /* XDMAC channels */
    XDMAC_CHANNEL           xdmac_ch_rx;
    XDMAC_CHANNEL           xdmac_ch_tx;
    
    /* RX tracking */
    size_t                  old_dma_pos;
    
    /* TX status */
    volatile bool           tx_dma_busy;
    
} USART_Driver_t;

/*************************************************
 *              PUBLIC API                       *
 *************************************************/
Std_ReturnType USART_Driver_Init(void);
void USART_DMA_Rx_Check(uart_registers_t* usart);
void USART_Driver_Write(uart_registers_t* usart, uint8_t data);
void USART_Driver_SendString(uart_registers_t* usart, const char* str);
int USART_Driver_Read(uart_registers_t* usart);
bool USART_Driver_IsDataAvailable(uart_registers_t* usart);
uint16_t USART_Driver_TXNumFreeSlots(uart_registers_t* usart);
uint16_t USART_Driver_RXNumFreeSlots(uart_registers_t* usart);
uint16_t USART_Driver_RXNumDataAvailable(uart_registers_t* usart);
void USART_Driver_FlushTx(uart_registers_t* usart);
void USART_Driver_FlushRx(uart_registers_t* usart);
void USART_Driver_Flush(uart_registers_t* usart);
void USART_Driver_Polling_Write(uart_registers_t* usart, uint8_t data);
void USART_Driver_Polling_SendString(uart_registers_t* usart, const char* str);

/*************************************************
 *              CONVENIENCE MACROS               *
 *************************************************/
#define UART_REGS                       UART2_REGS
#define USART_Write(data)              USART_Driver_Write(UART_REGS, data)
#define USART_SendString(str)          USART_Driver_SendString(UART_REGS, str)
#define USART_Read()                   USART_Driver_Read(UART_REGS)
#define USART_IsDataAvailable()        USART_Driver_IsDataAvailable(UART_REGS)
#define USART_RxCheck()                USART_DMA_Rx_Check(UART_REGS)
#define USART_Flush()                  USART_Driver_Flush(UART_REGS)

#ifdef __cplusplus
}
#endif

#endif /* USART_RXDMA_TXDMA_H */
#endif