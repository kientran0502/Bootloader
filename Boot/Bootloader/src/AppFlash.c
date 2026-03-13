///**
// *
// * \file
// *
// * \brief This module takes care of Flash handlers associated with this Application
// *
// */
//
///*
// * Include header files for all drivers that have been imported from
// * Atmel Software Framework (ASF).
// */
///*
// * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
// */
//// #include <asf.h>
//#include <string.h>
//#include <stdint.h>
//#include "AppDef.h"
//#include "peripheral/efc/plib_efc.h"
//
///** Global/Static Variables*/
//static TS_FlashDescriptor msDeviceFlashDescriptor;
//static uint32_t mau32DeviceSectorsStartAddress[] = 
//{
//	0x00400000, 0x00402000, 0x00404000,
//	0x00420000, 0x00440000, 0x00460000, 0x00480000, 0x004A0000, 0x004C0000, 0x004E0000, 0x00500000, 
//	0x00520000, 0x00540000, 0x00560000, 0x00580000, 0x005A0000, 0x005C0000, 0x005E0000, 0x00600000
//};
///** Static function Prototypes */
//static uint32_t U32_GetNextAddressToErase(uint32_t u32CurrentAddressToErase);
//
//
///**
// * \brief This handler increments an index to indicate there is a message received on USB and requires processing.
// * \param
// */
//void InitFlashWaitStates(void)
//{
//	/** Initialize Flash: 6 wait states for flash writing. */
//	// (void)flash_init(FLASH_ACCESS_MODE_128, 6);
//	
//	/** Get Flash Descriptors from Device */
//	// (void)flash_get_descriptor(U32_BOOT_LOADER_APPLICATION_START_ADDRESS, (uint32_t*)&msDeviceFlashDescriptor,
//	// 	sizeof(TS_FlashDescriptor));
//	
//	// EFC_Read(uint32_t*)U32_BOOT_LOADER_APPLICATION_START_ADDRESS, sizeof(TS_FlashDescriptor), U32_BOOT_LOADER_APPLICATION_START_ADDRESS);
//		
//	// printf("Display Flash Descriptor\r\n");
//	// printf("Flash ID : 0x%08X\r\n", msDeviceFlashDescriptor.u32FlashID);
//	// printf("Flash Size : 0x%08X\r\n", msDeviceFlashDescriptor.u32FlashSize);
//	// printf("Flash Page Size : %d\r\n", msDeviceFlashDescriptor.u32FlashPageSize);
//	// printf("Flash Plane Count : %d\r\n", msDeviceFlashDescriptor.u32FlashPlaneCount);
//	// printf("Flash Bytes in Plane : 0x%08X\r\n", msDeviceFlashDescriptor.u32FlashBytesInPlane);
//	// printf("Flash No Of Lock Bits : %d\r\n", msDeviceFlashDescriptor.u32FlashNoOfLockBits);
//	// printf("Flash No Of Bytes in Lock region: %d\r\n", msDeviceFlashDescriptor.u32FlashNoOfBytesInLockRegion);
//}
//
///**
// * \brief This function takes care of reading received message from USB buffers to Application buffers and then triggers processing of those messages.
// * \param[out] U8 u8ErrorCode, 0 - Success, others - fail
// */
//uint8_t U8_EraseUserApplication(void)
//{
//	uint32_t u32UserApplicationLength;
//	uint32_t u32CurrentAddressToErase;
//	uint32_t u32NextAddressToErase;
//	uint32_t u32CurrentEraseLength;
//	uint8_t u8NoOfPagesToErase;
//	uint8_t u8ErrorCode = 0;
//	
//	TS_ApplicationData* pApplicationFooter = P_GetApplicationData();
//	
//	/** Check Address is with in User Application area */
//	if(pApplicationFooter->u32UserApplicationStartAddress < 
//		(pApplicationFooter->u32BootApplicationStartAddress + pApplicationFooter->u32BootApplicationAllocationSize))
//	{
//		CUSTOM_DEBUG(printf("User Application Address Overlaps with Bootloader Address\r\n"));
//		return u8ErrorCode;
//	}
//	
//	/** Ensure User Application start address is on page boundary */
//	if(pApplicationFooter->u32UserApplicationStartAddress % msDeviceFlashDescriptor.u32FlashPageSize)
//	{
//		CUSTOM_DEBUG(printf("User Application start address is not on page boundary\r\n"));
//		u8ErrorCode = 0xFF;
//		return u8ErrorCode;
//	}
//	
//	/** Identify Pages & Sectors to Erase */
//	u32UserApplicationLength = pApplicationFooter->u32UserApplicationAllocationSize;
//	u32CurrentAddressToErase = pApplicationFooter->u32UserApplicationStartAddress;
//	
//	while(u32UserApplicationLength)
//	{
//		u32NextAddressToErase  = U32_GetNextAddressToErase(u32CurrentAddressToErase);
//		u32CurrentEraseLength = u32NextAddressToErase  - u32CurrentAddressToErase;
//		
//		if(!u32CurrentEraseLength)
//		{
//			CUSTOM_DEBUG(printf("Erase length should not be 0\r\n"));
//			u8ErrorCode = 0xFF;
//			break;
//		}
//
//		/** Get No Of pages to Erase */
//		u8NoOfPagesToErase = (u32NextAddressToErase - u32CurrentAddressToErase) / msDeviceFlashDescriptor.u32FlashPageSize;
//		
//		CUSTOM_DEBUG(printf("Current Address :%08X, Next Address : %08X, Erase Length : %08X\r\n", 
//			u32CurrentAddressToErase, u32NextAddressToErase, u32CurrentEraseLength));
//		CUSTOM_DEBUG(printf("No Of pages to Erase :%d\r\n", u8NoOfPagesToErase));
//		
//		if(u32CurrentAddressToErase % (128*1024))
//		{
//			/** Erase Pages */
//			CUSTOM_DEBUG(printf("Page Erase :%08X, %d\r\n", u32CurrentAddressToErase, u8NoOfPagesToErase));
//			if(u8NoOfPagesToErase == 16)
//			{
//				if(flash_erase_page(u32CurrentAddressToErase, IFLASH_ERASE_PAGES_16))
//				{
//					CUSTOM_DEBUG(printf("Flash 16 Pages Erase failed\r\n"));
//					u8ErrorCode = 0xFF;
//					break;
//				}
//			}
//			else if (u8NoOfPagesToErase == 32)
//			{
//				if(flash_erase_page(u32CurrentAddressToErase, IFLASH_ERASE_PAGES_32))
//				{
//					CUSTOM_DEBUG(printf("Flash 32 Pages Erase failed\r\n"));
//					u8ErrorCode = 0xFF;
//					break;
//				}
//			}
//			else
//			{
//				CUSTOM_DEBUG(printf("Invalid No of Pages to Erase\r\n"));
//				u8ErrorCode = 0xFF;
//				break;
//			}
//		}
//		else
//		{
//			/** Erase Sector */
//			CUSTOM_DEBUG(printf("Sector Erase :%08X,\r\n", u32CurrentAddressToErase));
//			if(flash_erase_sector(u32CurrentAddressToErase))
//			{
//				CUSTOM_DEBUG(printf("Flash Sector Erase failed\r\n"));
//				u8ErrorCode = 0xFF;
//				break;
//			}						
//		}
//		
//		u32UserApplicationLength -= u32CurrentEraseLength;
//		u32CurrentAddressToErase = u32NextAddressToErase;
//	}
//	
//	return u8ErrorCode;
//}
//
///**
// * \brief This function checks whether address is on Sector boundary or not. Based on that it populates next address to Erase by adding 
// either page(s) size or sector size
// * \param[in] U32 u32CurrentAddressToErase
// * \param[out] U32 u32NextAddressToErase
// */
//static uint32_t U32_GetNextAddressToErase(uint32_t u32CurrentAddressToErase)
//{
//	uint32_t u32NextAddressToErase;
//	uint8_t u8SectorIndex;
//	uint8_t u8CurrentSector=0;
//
//	u32NextAddressToErase = u32CurrentAddressToErase;
//	
//	for(u8SectorIndex=0; u8SectorIndex < NO_OF_ELEMENTS_IN_ARRAY(mau32DeviceSectorsStartAddress); u8SectorIndex++)
//	{
//		if(mau32DeviceSectorsStartAddress[u8SectorIndex] < u32CurrentAddressToErase)
//		{
//			u8CurrentSector = u8SectorIndex;
//		}
//		else if (mau32DeviceSectorsStartAddress[u8SectorIndex] == u32CurrentAddressToErase)
//		{
//			break;
//		}
//	}
//	
//	/** Check if address is on Sector Boundary or not */
//	if(u8SectorIndex < NO_OF_ELEMENTS_IN_ARRAY(mau32DeviceSectorsStartAddress))
//	{
//		/** Address is on sector boundary... Go for sector Erase */
//		/** Increment sector Index to get next address*/
//		u8SectorIndex++;
//		
//		/** Bound check for Sector Index */
//		if(u8SectorIndex < NO_OF_ELEMENTS_IN_ARRAY(mau32DeviceSectorsStartAddress))
//		{
//			/** Update next Erase address with next sector address */
//			u32NextAddressToErase = mau32DeviceSectorsStartAddress[u8SectorIndex];	
//		}
//	}
//	else
//	{
//		/** Address is not on sector boundary... Go for Page Erase */
//		
//		/** Check for maximum pages to erase */
//		if((u8CurrentSector > 2) && (u32CurrentAddressToErase + (32 * msDeviceFlashDescriptor.u32FlashPageSize) <= 
//			mau32DeviceSectorsStartAddress[u8CurrentSector+1]))
//		{
//			u32NextAddressToErase += (32 * msDeviceFlashDescriptor.u32FlashPageSize);
//		}
//		else if(u32CurrentAddressToErase + (16 * msDeviceFlashDescriptor.u32FlashPageSize) <=
//			mau32DeviceSectorsStartAddress[u8CurrentSector+1])
//		{
//			/** Address is not on sector boundary... Go for Page Erase */
//			u32NextAddressToErase += (16 * msDeviceFlashDescriptor.u32FlashPageSize);
//		}
//		else
//		{
//			CUSTOM_DEBUG(printf("Unable to find matching page count\r\n"));
//		}		
//	}
//	
//	return(u32NextAddressToErase);
//}
//
///**
// * \brief This function checks whether address is on Sector boundary or not. Based on that it populates next address to Erase by adding 
// either page(s) size or sector size
// * \param[in] U32 u32Address
// * \param[in] U8* pDataBuffer
// * \param[out] U8 u8ErrorCode, 0 - Success, others - fail
// */
//uint8_t U8_WriteToFlash(uint32_t u32Address, uint8_t* pDataBuffer, uint16_t u16DataSize)
//{
//	uint8_t u8ErrorCode = 0xFF;
//	TS_ApplicationData* pApplicationFooter = P_GetApplicationData();
//
//	/** Check Address is with in User Application area */
//	if((u32Address < pApplicationFooter->u32UserApplicationStartAddress) || (u32Address >= 
//		(pApplicationFooter->u32UserApplicationStartAddress + pApplicationFooter->u32UserApplicationAllocationSize)))
//	{
//		CUSTOM_DEBUG(printf("Address is not in User Application boundary\r\n", u16DataSize));
//		return u8ErrorCode;
//	}
//	
//	/** Check if Address in Page aligned or not */
//	if(u32Address % msDeviceFlashDescriptor.u32FlashPageSize)
//	{
//		CUSTOM_DEBUG(printf("Address - %08X is not Page aligned\r\n", u32Address));
//		return u8ErrorCode;
//	}
//	
//	/** Check if Data size is matching with Flash Page size */
//	if(u16DataSize != msDeviceFlashDescriptor.u32FlashPageSize)
//	{
//		CUSTOM_DEBUG(printf("Data size is not matching with Flash size\r\n", u16DataSize));
//		return u8ErrorCode;
//	}
//	
//	/** Trigger Write request for entire page */
//	if(flash_write(u32Address, (void*)pDataBuffer, msDeviceFlashDescriptor.u32FlashPageSize, 0))
//	{
//		CUSTOM_DEBUG(printf("Error while writing to Flash %08X\r\n", u32Address));
//	}
//	else
//	{
//		u8ErrorCode =0;
//	}
//	
//	return u8ErrorCode;
//}
//
//
