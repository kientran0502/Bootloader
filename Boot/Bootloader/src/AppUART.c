/**
 *
 * \file
 *
 * \brief This module takes care of CDC handlers associated with this Application
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
// #include <asf.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "AppDef.h"
#include "Dev/M2_BSP/UART/uart_irq.h"

/** Define*/
#define BOOTLOADER_UART_PORT		UART0_REGS

/** Global/Static Variables*/
static TS_USB_CDC_FRAME msUSB_CDC_Msg;
static TS_USB_DEVICE_CDC_FRAME msUSB_Device_CDC_Msg;

/** Static function Prototypes */
static void DecodeCDC_Msg(void);
static void LogMessageDataToConsole(uint8_t* pData, uint16_t u16DataSize);

/**
 * \brief This handler increments an index to indicate there is a message received on USB and requires processing.
 * \param
 */
// void UART_RecvHandler(void)
// {
// 	/** Increment number of messages received on USB... */
// 	u8RecvHandlerCount++;
// }

/**
 * \brief This function takes care of reading received message from USB buffers to Application buffers and then triggers processing of those messages.
 * Msg Format - 0xFF CMD LEN_H LEN_L DATA0... DATAn
 * \param
 */
void ProcessUART_Msg(void)
{
	uint32_t u32ReceivedData;
	uint32_t u32ReturnedValue;
	
	/** Check if UART data is enabled and there is pending message to Process */
	if(UART_Driver_IsDataAvailable(BOOTLOADER_UART_PORT))
	{
			u32ReceivedData = udi_cdc_read_no_polling((void*)&msUSB_CDC_Msg, sizeof(msUSB_CDC_Msg));
			u32ReceivedData = UART_Driver_Read(BOOTLOADER_UART_PORT, (uint8_t*)&msUSB_CDC_Msg, sizeof(msUSB_CDC_Msg));
			if(u32ReceivedData) 
			{
				CUSTOM_DEBUG(printf("%X, %X\r\n", u32ReceivedData, u32ReturnedValue));
				CUSTOM_DEBUG(printf("Hdr-%02X, Cmd-%02X, ", msUSB_CDC_Msg.u8FrameIdentifier, msUSB_CDC_Msg.u8Cmd));
				if(msUSB_CDC_Msg.u8FrameIdentifier == 0xFF)
				{
					DecodeCDC_Msg();
				}
			}
	}
	else
	{

	}
}

/**
 * \brief Indicates to Application on receiving a message on CDC.
 * \param
 */
static void DecodeCDC_Msg(void)
{
	uint32_t u32Address;
	uint8_t u8ErrorCode = 0x00;
	uint8_t* pu8FlashData;
	
	switch(msUSB_CDC_Msg.u8Cmd)
	{
		case eBOOTLOADER_ENTER_BOOT_MODE:
		case eBOOTLOADER_QUERY_DEVICE_MODE:
			/** Log data to Console */
			LogMessageDataToConsole(&msUSB_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX], 0);
			
			/** Return with Response */
			msUSB_Device_CDC_Msg.u8FrameIdentifier = 0xFF;
			msUSB_Device_CDC_Msg.u8Cmd = msUSB_CDC_Msg.u8Cmd;
			msUSB_Device_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX] = 0x00;	/** 00 - Indicates Boot Application */
			PlaceRequestToXmitOnCDC(&msUSB_Device_CDC_Msg, 1);
			break;
		
		case eBOOTLOADER_CMD_AES_KEY_VECTOR_INDEX:
			/** Log data to Console */
			LogMessageDataToConsole(&msUSB_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX], 2);
			
			UpdateAES_Key(msUSB_CDC_Msg.au8Data[eAES_KEY_INDEX]);
			UpdateAES_Vector(msUSB_CDC_Msg.au8Data[eAES_VECTOR_INDEX]);

			/** Return with Response */
			msUSB_Device_CDC_Msg.u8FrameIdentifier = 0xFF;
			msUSB_Device_CDC_Msg.u8Cmd = msUSB_CDC_Msg.u8Cmd;
			msUSB_Device_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX] = 0x00;
			PlaceRequestToXmitOnCDC(&msUSB_Device_CDC_Msg, 1);
			break;
		
		case eBOOTLOADER_CMD_ERASE_FLASH:
			/** Erase User Application Area */			
			u8ErrorCode = U8_EraseUserApplication();
			
			/** Return with Response */
			msUSB_Device_CDC_Msg.u8FrameIdentifier = 0xFF;
			msUSB_Device_CDC_Msg.u8Cmd = msUSB_CDC_Msg.u8Cmd;
			msUSB_Device_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX] = u8ErrorCode;
			PlaceRequestToXmitOnCDC(&msUSB_Device_CDC_Msg, 1);
			break;
			
		case eBOOTLOADER_CMD_WRITE_FLASH_UPPER_256:
			/** Log data to Console */
			LogMessageDataToConsole(&msUSB_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX], 261);
			
			/** Copy data to second part of Buffer */
			memcpy(&msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_DATA_INDEX+256], &msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_DATA_INDEX], 256);

			/** Return with Response */
			msUSB_Device_CDC_Msg.u8FrameIdentifier = 0xFF;
			msUSB_Device_CDC_Msg.u8Cmd = msUSB_CDC_Msg.u8Cmd;
			msUSB_Device_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX] = u8ErrorCode;
			PlaceRequestToXmitOnCDC(&msUSB_Device_CDC_Msg, 1);
			break;

		case eBOOTLOADER_CMD_WRITE_FLASH_LOWER_256:
			/** Log data to Console */
			LogMessageDataToConsole(&msUSB_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX], 261);

			/** Check if Encryption is enabled for this data */
			if(msUSB_CDC_Msg.au8Data[eWRITE_FLASH_ENCRYPTION_STATUS_INDEX])
			{
				/** Decrypt data */
				pu8FlashData = P_DecryptReceivedPageData((void*)&msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_DATA_INDEX], 512);
			}
			else
			{
				/** No decryption is required .. Just pass data to Programming */
				pu8FlashData = &msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_DATA_INDEX];
			}
			
			/** Write Flash data at given address */
			u32Address = ((uint16_t)msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_ADDRESS_INDEX] << 24) +
				((uint16_t)msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_ADDRESS_INDEX+1] << 16) + 
				((uint16_t)msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_ADDRESS_INDEX+2] << 8) +
				((uint16_t)msUSB_CDC_Msg.au8Data[eWRITE_FLASH_MSG_ADDRESS_INDEX+3]);

			u8ErrorCode = U8_WriteToFlash(u32Address, pu8FlashData, 512);
			
			/** Return with Response */
			msUSB_Device_CDC_Msg.u8FrameIdentifier = 0xFF;
			msUSB_Device_CDC_Msg.u8Cmd = msUSB_CDC_Msg.u8Cmd;
			msUSB_Device_CDC_Msg.au8Data[eBOOTLOADER_DATA_START_INDEX] = u8ErrorCode;
			PlaceRequestToXmitOnCDC(&msUSB_Device_CDC_Msg, 1);
			break;

		case eBOOTLOADER_RESET_DEVICE:
			/** Reset Device */
			ResetDevice();
			break;

		default:
			break;
	}
	CUSTOM_DEBUG(printf ("\r\n"));
}

/**
 * \brief Indicates to Application on receiving a message on CDC.
 * \param[in] TS_USB_CDC_FRAME* psCDC_USB_Data
 * \param[in] U8 u8DataSize
 */
void PlaceRequestToXmitOnCDC(TS_USB_DEVICE_CDC_FRAME* psUSB_DeviceCDC_Data, uint16_t u16DataSize)
{
	uint16_t u16DataIndex;

	CUSTOM_DEBUG(printf("Hdr-%02X, Cmd-%02X, ", psUSB_DeviceCDC_Data->u8FrameIdentifier, psUSB_DeviceCDC_Data->u8Cmd));

	CUSTOM_DEBUG(printf ("Data-"));
	
	for(u16DataIndex = 0; u16DataIndex < u16DataSize; u16DataIndex++)
	{
		CUSTOM_DEBUG(printf ("%02X,", psUSB_DeviceCDC_Data->au8Data[u16DataIndex]));
	}
	CUSTOM_DEBUG(printf ("\r\n"));
				
	(void)udi_cdc_write_buf((void*)psUSB_DeviceCDC_Data, u16DataSize+2);
}

/**
 * \brief Call back function from USB module to indicate USB Host enabled this interface
 * \param[in] U8 u8Port Port to operate on
 * \param[out] true 
 */
bool B_Enable_UART_Interface(uint8_t u8Port)
{
	(void)u8Port;
	mu8UART_EnableStatus = true;
	
	return true;
}

/**
 * \brief Call back function from UART module to indicate UART interface is disabled
 * \param 
 */
void Disable_UART_Interface(uint8_t u8Port)
{
	(void)u8Port;
	mu8UART_EnableStatus = false;
}

/**
 * \brief This Function prints data on Console window
 * \param[in] U8* pData
 * \param[in] U16 u16DataSize
 */
static void LogMessageDataToConsole(uint8_t* pData, uint16_t u16DataSize)
{
	uint16_t u16DataIndex;
	
	CUSTOM_DEBUG(printf ("Data-"));
	for(u16DataIndex = 0; u16DataIndex < u16DataSize; u16DataIndex++)
	{
		CUSTOM_DEBUG(printf ("%02X,", *pData++));
	}
};