/**
 * \file
 *
 * \brief Created to add Application definitions
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef APP_DEF_H
#define APP_DEF_H

/*
 * This file includes Application definitions.
 * 
 */
#include <stdint.h>

typedef void (*fpJumpHandler)(void);

#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MAJOR_VERSION		((uint32_t)0x1)
#define U32_USB_CDC_BOOTLOADER_SW_RELEASE_MINOR_VERSION		((uint32_t)0x1)
#define U32_BOOT_LOADER_APPLICATION_START_ADDRESS			((uint32_t)0x00400000)
#define U32_BOOT_LOADER_APPLICATION_ALLOCATED_SIZE			((uint32_t)0x0000C000)
#define U32_USER_APPLICATION_START_ADDRESS					((uint32_t)0x0040C000)
#define U32_USER_APPLICATION_ALLOCATED_SIZE					((uint32_t)0x001F4000)
#define CUSTOM_DEBUG(x)										
#define U16_USB_CDC_MAX_DATA_SIZE							((uint16_t)540)
#define U16_USB_DEVICE_CDC_MAX_DATA_SIZE					((uint16_t)20)
#define U8_HASH_TAG_DATA_SIZE								((uint8_t)5)
#define NO_OF_ELEMENTS_IN_ARRAY(array)						sizeof(array)/sizeof(array[0])


#define U16_UART_MAX_DATA_SIZE								((uint16_t)256)


typedef struct
{
	uint8_t u8FrameIdentifier;
	uint8_t u8Cmd;
	uint8_t au8Data[U16_UART_MAX_DATA_SIZE];
}TS_UART_FRAME;

typedef struct
{
	uint8_t u8FrameIdentifier;
	uint8_t u8Cmd;
	uint8_t au8Data[U16_USB_DEVICE_CDC_MAX_DATA_SIZE];
}TS_USB_DEVICE_CDC_FRAME;

typedef struct
{
	uint32_t u32ApplicationVersion_Major;
	uint32_t u32ApplicationVersion_Minor;
	fpJumpHandler fpApplicationJumpHandler;
	uint32_t u32BootApplicationStartAddress;
	uint32_t u32BootApplicationAllocationSize;
	uint32_t u32UserApplicationStartAddress;
	uint32_t u32UserApplicationAllocationSize;
}TS_ApplicationData;

typedef union
{
	uint32_t au32ApplicationData[32];
	TS_ApplicationData sApplicationData;
}TU_ApplicationData;

typedef struct
{
	uint32_t u32SHA_1PaddingBytes[16];
}TS_SHA_Padding;

typedef struct
{
	uint32_t au32ApplicationHashTag[16];
}TS_SHA_Digest;

typedef struct
{
	TU_ApplicationData uApplicationData;
	TS_SHA_Padding sSHA_Padding;
	TS_SHA_Digest sSHA_Digest;
}TS_ApplicationFooter;

typedef struct
{
	uint32_t u32FlashID;
	uint32_t u32FlashSize;
	uint32_t u32FlashPageSize;
	uint32_t u32FlashPlaneCount;
	uint32_t u32FlashBytesInPlane;
	uint32_t u32FlashNoOfLockBits;
	uint32_t u32FlashNoOfBytesInLockRegion;
}TS_FlashDescriptor;

typedef struct  
{
	uint32_t u32StartAddress;
	uint32_t u32NextAddress;
}TS_EraseLocations;


enum
{
	eBOOTLOADER_ENTER_BOOT_MODE = 0xB0,
	eBOOTLOADER_RESET_DEVICE,
	eBOOTLOADER_QUERY_DEVICE_MODE,
	eBOOTLOADER_CMD_AES_KEY_VECTOR_INDEX,
	eBOOTLOADER_CMD_ERASE_FLASH,
	eBOOTLOADER_CMD_WRITE_FLASH_UPPER_256,
	eBOOTLOADER_CMD_WRITE_FLASH_LOWER_256,
};

enum
{
	eBOOTLOADER_DATA_START_INDEX
};

enum
{
	eWRITE_FLASH_MSG_ADDRESS_INDEX = eBOOTLOADER_DATA_START_INDEX,
	eWRITE_FLASH_ENCRYPTION_STATUS_INDEX = eWRITE_FLASH_MSG_ADDRESS_INDEX + 4,
	eWRITE_FLASH_MSG_DATA_INDEX,
};

enum
{
	eAES_KEY_INDEX = eBOOTLOADER_DATA_START_INDEX,
	eAES_VECTOR_INDEX,
};

/* Function Prototypes main.c*/
// TS_ApplicationData* P_GetApplicationData(void);
// void ResetDevice(void);

// /* Function Prototypes AppCDC.c*/
// void USB_RecvHandler(void);
// void ProcessCDC_Msg(void);
// void PlaceRequestToXmitOnCDC(TS_USB_DEVICE_CDC_FRAME* psUSB_DeviceCDC_Data, U16 u16DataSize);
// Bool B_EnableUSB_CDC_Interface(U8 u8Port);
// void DisableUSB_CDC_Interface(U8 u8Port);

// /* Function Prototypes from AppFlash.c */
// void InitFlashWaitStates(void);
// uint8_t U8_EraseUserApplication(void);
// uint8_t U8_WriteToFlash(uint32_t u32Address, uint8_t* pDataBuffer, uint16_t u16DataSize);

// /* Function Prototypes from AppAES_ICM.c */
// void InitICM_Module(void);
// void CalculateICM(uint32_t u32StartAddress, uint32_t u32Size, uint32_t* pu32Result);
// void UpdateAES_Key(uint8_t u8KeyIndex);
// void UpdateAES_Vector(uint8_t u8IV_Index);
// uint8_t* P_DecryptReceivedPageData(void* pData, uint16_t u16DataSize);

#endif // APP_DEF_H
