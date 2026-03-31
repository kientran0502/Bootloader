/*******************************************************************************
  UART2 PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_uart2.c

  Summary:
    UART2 PLIB Implementation File

  Description:
    None

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#include "device.h"
#include "plib_uart2.h"
#include "interrupts.h"

// *****************************************************************************
// *****************************************************************************
// Section: UART2 Implementation
// *****************************************************************************
// *****************************************************************************

static volatile UART_OBJECT uart2Obj;

static void __attribute__((used)) UART2_ISR_RX_Handler( void )
{
    if(uart2Obj.rxBusyStatus == true)
    {
        size_t rxSize = uart2Obj.rxSize;
        size_t rxProcessedSize = uart2Obj.rxProcessedSize;

        while((UART_SR_RXRDY_Msk == (UART2_REGS->UART_SR& UART_SR_RXRDY_Msk)) && (rxSize > rxProcessedSize) )
        {
            uart2Obj.rxBuffer[rxProcessedSize] = (uint8_t)(UART2_REGS->UART_RHR& UART_RHR_RXCHR_Msk);
            rxProcessedSize++;
        }

        uart2Obj.rxProcessedSize = rxProcessedSize;

        /* Check if the buffer is done */
        if(uart2Obj.rxProcessedSize >= rxSize)
        {
            uart2Obj.rxBusyStatus = false;

            /* Disable Read, Overrun, Parity and Framing error interrupts */
            UART2_REGS->UART_IDR = (UART_IDR_RXRDY_Msk | UART_IDR_FRAME_Msk | UART_IDR_PARE_Msk | UART_IDR_OVRE_Msk);

            if(uart2Obj.rxCallback != NULL)
            {
                uintptr_t rxContext = uart2Obj.rxContext;

                uart2Obj.rxCallback(rxContext);
            }
        }
    }
    else
    {
        /* Nothing to process */
        ;
    }
}

static void __attribute__((used)) UART2_ISR_TX_Handler( void )
{
    if(uart2Obj.txBusyStatus == true)
    {
        size_t txSize = uart2Obj.txSize;
        size_t txProcessedSize = uart2Obj.txProcessedSize;

        while((UART_SR_TXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_TXRDY_Msk)) && (txSize > txProcessedSize) )
        {
            UART2_REGS->UART_THR|= uart2Obj.txBuffer[txProcessedSize];
            txProcessedSize++;
        }

        uart2Obj.txProcessedSize = txProcessedSize;

        /* Check if the buffer is done */
        if(uart2Obj.txProcessedSize >= txSize)
        {
            uart2Obj.txBusyStatus = false;
            UART2_REGS->UART_IDR = UART_IDR_TXEMPTY_Msk;

            if(uart2Obj.txCallback != NULL)
            {
                uintptr_t txContext = uart2Obj.txContext;

                uart2Obj.txCallback(txContext);
            }
        }
    }
    else
    {
        /* Nothing to process */
        ;
    }
}

void __attribute__((used)) UART2_InterruptHandler( void )
{
    /* Error status */
    uint32_t errorStatusx = (UART2_REGS->UART_SR & (UART_SR_OVRE_Msk | UART_SR_FRAME_Msk | UART_SR_PARE_Msk));

    if(errorStatusx != 0U)
    {
        /* Client must call UARTx_ErrorGet() function to clear the errors */

        /* Disable Read, Overrun, Parity and Framing error interrupts */
        UART2_REGS->UART_IDR = (UART_IDR_RXRDY_Msk | UART_IDR_FRAME_Msk | UART_IDR_PARE_Msk | UART_IDR_OVRE_Msk);

        uart2Obj.rxBusyStatus = false;

        /* UART errors are normally associated with the receiver, hence calling
         * receiver callback */
        if( uart2Obj.rxCallback != NULL )
        {
            uintptr_t rxContext = uart2Obj.rxContext;

            uart2Obj.rxCallback(rxContext);
        }
    }

    /* Receiver status */
    if(UART_SR_RXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_RXRDY_Msk))
    {
        UART2_ISR_RX_Handler();
    }

    /* Transmitter status */
    if(UART_SR_TXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_TXRDY_Msk))
    {
        UART2_ISR_TX_Handler();
    }
}

static void UART2_ErrorClear( void )
{
    uint8_t dummyData = 0u;

    UART2_REGS->UART_CR = UART_CR_RSTSTA_Msk;

    /* Flush existing error bytes from the RX FIFO */
    while( UART_SR_RXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_RXRDY_Msk) )
    {
        dummyData = (uint8_t)(UART2_REGS->UART_RHR & UART_RHR_RXCHR_Msk);
    }

    /* Ignore the warning */
    (void)dummyData;
}

void UART2_Initialize( void )
{
    /* Reset UART2 */
    UART2_REGS->UART_CR = (UART_CR_RSTRX_Msk | UART_CR_RSTTX_Msk | UART_CR_RSTSTA_Msk);

    /* Enable UART2 */
    UART2_REGS->UART_CR = (UART_CR_TXEN_Msk | UART_CR_RXEN_Msk);

    /* Configure UART2 mode */
    UART2_REGS->UART_MR = ((UART_MR_BRSRCCK_PERIPH_CLK) | (UART_MR_PAR_NO) | (0U << UART_MR_FILTER_Pos));

    /* Configure UART2 Baud Rate */
    UART2_REGS->UART_BRGR = UART_BRGR_CD(81);

    /* Initialize instance object */
    uart2Obj.rxBuffer = NULL;
    uart2Obj.rxSize = 0;
    uart2Obj.rxProcessedSize = 0;
    uart2Obj.rxBusyStatus = false;
    uart2Obj.rxCallback = NULL;
    uart2Obj.txBuffer = NULL;
    uart2Obj.txSize = 0;
    uart2Obj.txProcessedSize = 0;
    uart2Obj.txBusyStatus = false;
    uart2Obj.txCallback = NULL;
}

UART_ERROR UART2_ErrorGet( void )
{
    UART_ERROR errors = UART_ERROR_NONE;
    uint32_t status = UART2_REGS->UART_SR;

    errors = (UART_ERROR)(status & (UART_SR_OVRE_Msk | UART_SR_PARE_Msk | UART_SR_FRAME_Msk));

    if(errors != UART_ERROR_NONE)
    {
        UART2_ErrorClear();
    }

    /* All errors are cleared, but send the previous error state */
    return errors;
}

bool UART2_SerialSetup( UART_SERIAL_SETUP *setup, uint32_t srcClkFreq )
{
    bool status = false;
    uint32_t baud;
    uint32_t brgVal = 0;
    uint32_t uartMode;

    if(uart2Obj.rxBusyStatus == true)
    {
        /* Transaction is in progress, so return without updating settings */
        return false;
    }
    if(uart2Obj.txBusyStatus == true)
    {
        /* Transaction is in progress, so return without updating settings */
        return false;
    }
    if (setup != NULL)
    {
        baud = setup->baudRate;
        if(srcClkFreq == 0U)
        {
            srcClkFreq = UART2_FrequencyGet();
        }

        /* Calculate BRG value */
        brgVal = srcClkFreq / (16U * baud);

        /* If the target baud rate is acheivable using this clock */
        if (brgVal <= 65535U)
        {
            /* Configure UART2 mode */
            uartMode = UART2_REGS->UART_MR;
            uartMode &= ~UART_MR_PAR_Msk;
            UART2_REGS->UART_MR = uartMode | setup->parity ;

            /* Configure UART2 Baud Rate */
            UART2_REGS->UART_BRGR = UART_BRGR_CD(brgVal);

            status = true;
        }
    }

    return status;
}

bool UART2_Read( void *buffer, const size_t size )
{
    bool status = false;
    UART_ERROR errorinfo;

    uint8_t * lBuffer = (uint8_t *)buffer;

    if(NULL != lBuffer)
    {
        /* Clear errors before submitting the request.
         * ErrorGet clears errors internally. */
         errorinfo = UART2_ErrorGet();

         if(errorinfo != 0U)
         {
             /* Nothing to do */
         }

        /* Check if receive request is in progress */
        if(uart2Obj.rxBusyStatus == false)
        {
            uart2Obj.rxBuffer = lBuffer;
            uart2Obj.rxSize = size;
            uart2Obj.rxProcessedSize = 0;
            uart2Obj.rxBusyStatus = true;
            status = true;

            /* Enable Read, Overrun, Parity and Framing error interrupts */
            UART2_REGS->UART_IER = (UART_IER_RXRDY_Msk | UART_IER_FRAME_Msk | UART_IER_PARE_Msk | UART_IER_OVRE_Msk);
        }
    }

    return status;
}

bool UART2_Write( void *buffer, const size_t size )
{
    bool status = false;
    uint8_t * lBuffer = (uint8_t *)buffer;

    if(NULL != lBuffer)
    {
        /* Check if transmit request is in progress */
        if(uart2Obj.txBusyStatus == false)
        {
            uart2Obj.txBuffer = lBuffer;
            uart2Obj.txSize = size;
            uart2Obj.txProcessedSize = 0;
            uart2Obj.txBusyStatus = true;
            status = true;

            /* Initiate the transfer by sending first byte */
            if(UART_SR_TXRDY_Msk == (UART2_REGS->UART_SR & UART_SR_TXRDY_Msk))
            {
                UART2_REGS->UART_THR = (UART_THR_TXCHR(*lBuffer) & UART_THR_TXCHR_Msk);
                uart2Obj.txProcessedSize++;
            }

            UART2_REGS->UART_IER = UART_IER_TXEMPTY_Msk;
        }
    }

    return status;
}

void UART2_WriteCallbackRegister( UART_CALLBACK callback, uintptr_t context )
{
    uart2Obj.txCallback = callback;

    uart2Obj.txContext = context;
}

void UART2_ReadCallbackRegister( UART_CALLBACK callback, uintptr_t context )
{
    uart2Obj.rxCallback = callback;

    uart2Obj.rxContext = context;
}

bool UART2_WriteIsBusy( void )
{
    return uart2Obj.txBusyStatus;
}

bool UART2_ReadIsBusy( void )
{
    return uart2Obj.rxBusyStatus;
}

size_t UART2_WriteCountGet( void )
{
    return uart2Obj.txProcessedSize;
}

size_t UART2_ReadCountGet( void )
{
    return uart2Obj.rxProcessedSize;
}

bool UART2_ReadAbort(void)
{
    if (uart2Obj.rxBusyStatus == true)
    {
        /* Disable Read, Overrun, Parity and Framing error interrupts */
        UART2_REGS->UART_IDR = (UART_IDR_RXRDY_Msk | UART_IDR_FRAME_Msk | UART_IDR_PARE_Msk | UART_IDR_OVRE_Msk);

        uart2Obj.rxBusyStatus = false;

        /* If required application should read the num bytes processed prior to calling the read abort API */
        uart2Obj.rxSize = 0;
        uart2Obj.rxProcessedSize = 0;
    }

    return true;
}


bool UART2_TransmitComplete( void )
{
    bool status = false;

    if(UART_SR_TXEMPTY_Msk == (UART2_REGS->UART_SR & UART_SR_TXEMPTY_Msk))
    {
        status = true;
    }

    return status;
}