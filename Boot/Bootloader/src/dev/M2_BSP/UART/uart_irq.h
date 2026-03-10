/************************************************
 *  @file     : uart_irq.h
 *  @date     : December, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    UART driver with RX/TX interrupt support
 *    - RX: Interrupt-driven with ring buffer
 *    - TX: Interrupt-driven with ring buffer
 *    - Efficient ISR handling
 ************************************************/
#ifndef UART_IRQ_H
#define UART_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
 *                  INCLUDES                     *
 *************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "peripheral/uart/plib_uart2.h"
#include "M5_Utils/RingBuffer/ring_buffer.h"
#include "M5_Utils/Define/define.h"

/*************************************************
 *              CONFIGURATION                    *
 *************************************************/
#define UART_DRIVER_COUNT              (1U)
#define UART2_RX_BUFFER_SIZE           (1024U)
#define UART2_TX_BUFFER_SIZE           (1024U)

/*************************************************
 *              TYPE DEFINITIONS                 *
 *************************************************/
typedef uint8_t RingBufElement;

typedef struct {
    /* Hardware */
    uart_registers_t*       uart;
    
    /* Ring buffers */
    s_RingBufferType        rx_buffer;
    s_RingBufferType        tx_buffer;
    
    /* Ring buffer data storage */
    RingBufElement*         rx_data;
    RingBufElement*         tx_data;
    
    /* Buffer sizes */
    size_t                  rx_buffer_size;
    size_t                  tx_buffer_size;
    
    /* Status flags */
    volatile bool           tx_busy;
    volatile bool           rx_error;
    
    /* Statistics (optional) */
    volatile uint32_t       rx_overflow_count;
    volatile uint32_t       rx_error_count;
    
} UART_Driver_t;

/*************************************************
 *              PUBLIC API                       *
 *************************************************/

Std_ReturnType UART_Driver_Init(void);

/* ---- WRITE ----*/

bool UART_Driver_WriteByte(uart_registers_t* uart, uint8_t data);
size_t UART_Driver_Write(uart_registers_t* uart, const uint8_t* data, size_t length);
size_t UART_Driver_SendString(uart_registers_t* uart, const char* str);

/* ---- READ ----*/
/**
 * @brief Read single byte from RX buffer
 * @param uart UART peripheral instance
 * @return Received byte, or -1 if no data available
 */
int UART_Driver_ReadByte(uart_registers_t* uart);

/**
 * @brief Read multiple bytes from RX buffer
 * @param uart UART peripheral instance
 * @param buffer Pointer to destination buffer
 * @param length Maximum number of bytes to read
 * @return Number of bytes actually read
 */
size_t UART_Driver_Read(uart_registers_t* uart, uint8_t* buffer, size_t length);

/* ---- STATUS ----*/
/**
 * @brief Check if RX data is available
 * @param uart UART peripheral instance
 * @return true if data available, false otherwise
 */
bool UART_Driver_IsDataAvailable(uart_registers_t* uart);

/**
 * @brief Get number of bytes available in RX buffer
 * @param uart UART peripheral instance
 * @return Number of bytes available
 */
size_t UART_Driver_RXDataAvailable(uart_registers_t* uart);

/**
 * @brief Get number of free slots in TX buffer
 * @param uart UART peripheral instance
 * @return Number of free slots
 */
size_t UART_Driver_TXFreeSpace(uart_registers_t* uart);

/**
 * @brief Get number of free slots in RX buffer
 * @param uart UART peripheral instance
 * @return Number of free slots
 */
size_t UART_Driver_RXFreeSpace(uart_registers_t* uart);

/**
 * @brief Check if TX is busy (data pending)
 * @param uart UART peripheral instance
 * @return true if TX busy, false if idle
 */
bool UART_Driver_IsTxBusy(uart_registers_t* uart);

/**
 * @brief Wait for TX to complete
 * @param uart UART peripheral instance
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return true if completed, false if timeout
 */
bool UART_Driver_WaitTxComplete(uart_registers_t* uart, uint32_t timeout_ms);

/* ---- CLEANING ----*/
/**
 * @brief Flush TX buffer (discard pending data)
 * @param uart UART peripheral instance
 */
void UART_Driver_FlushTx(uart_registers_t* uart);

/**
 * @brief Flush RX buffer (discard received data)
 * @param uart UART peripheral instance
 */
void UART_Driver_FlushRx(uart_registers_t* uart);

/**
 * @brief Flush both TX and RX buffers
 * @param uart UART peripheral instance
 */
void UART_Driver_Flush(uart_registers_t* uart);

/**
 * @brief Get RX error status
 * @param uart UART peripheral instance
 * @return true if error occurred since last check
 */
bool UART_Driver_HasRxError(uart_registers_t* uart);

/**
 * @brief Clear RX error flag
 * @param uart UART peripheral instance
 */
void UART_Driver_ClearRxError(uart_registers_t* uart);

/**
 * @brief Get RX overflow count
 * @param uart UART peripheral instance
 * @return Number of RX overflows
 */
uint32_t UART_Driver_GetRxOverflowCount(uart_registers_t* uart);

/**
 * @brief Get RX error count
 * @param uart UART peripheral instance
 * @return Number of RX errors (framing, parity, etc.)
 */
uint32_t UART_Driver_GetRxErrorCount(uart_registers_t* uart);

/* ---- POLLING MODE ---- */
/**
 * @brief Write single byte in polling mode (blocking)
 * @param uart UART peripheral instance
 * @param data Byte to send
 * @note Use for debugging or early boot when interrupts not available
 */
void UART_Driver_Polling_WriteByte(uart_registers_t* uart, uint8_t data);

/**
 * @brief Write string in polling mode (blocking)
 * @param uart UART peripheral instance
 * @param str Null-terminated string to send
 * @note Use for debugging or early boot when interrupts not available
 */
void UART_Driver_Polling_SendString(uart_registers_t* uart, const char* str);

/*************************************************
 *              CONVENIENCE MACROS               *
 *************************************************/
#define UART2_WriteByte(data)          UART_Driver_WriteByte(UART2_REGS, data)
#define UART2_Write(data, len)         UART_Driver_Write(UART2_REGS, data, len)
#define UART2_SendString(str)          UART_Driver_SendString(UART2_REGS, str)
#define UART2_ReadByte()               UART_Driver_ReadByte(UART2_REGS)
#define UART2_Read(buf, len)           UART_Driver_Read(UART2_REGS, buf, len)
#define UART2_IsDataAvailable()        UART_Driver_IsDataAvailable(UART2_REGS)
#define UART2_RXDataAvailable()        UART_Driver_RXDataAvailable(UART2_REGS)
#define UART2_TXFreeSpace()            UART_Driver_TXFreeSpace(UART2_REGS)
#define UART2_IsTxBusy()               UART_Driver_IsTxBusy(UART2_REGS)
#define UART2_Flush()                  UART_Driver_Flush(UART2_REGS)
#define UART2_FlushTx()                UART_Driver_FlushTx(UART2_REGS)
#define UART2_FlushRx()                UART_Driver_FlushRx(UART2_REGS)

#ifdef __cplusplus
}
#endif

#endif /* UART_IRQ_H */