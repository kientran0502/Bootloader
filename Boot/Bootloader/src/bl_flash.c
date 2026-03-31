#include "bl_flash.h"
#include "definitions.h"

void BlFlash_Init(void)
{
    /* Initialize flash hardware (stub) */
    EFC_Initialize();
}

// BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors)  // adrress pháº£i lÃ  bá»™i cá»§a 16 
// {
//     if( addr % 16 != 0 || numberOfSectors == 0)
//     {
//         return BL_STATUS_INVALID;
//     }
//     /* Erase flash region (stub) */
//     SCB_DisableICache();
//     SCB_DisableDCache();

//     for(uint32_t i = 0; i < numberOfSectors; i++)
//     {
//         while(EFC_IsBusy());
//         EFC_RegionUnlock(addr + (i * EFC_SECTORSIZE));

//         while(EFC_IsBusy());
//         EFC_SectorErase(addr + (i * EFC_SECTORSIZE)); // Function to erase a sector (16 pages at a time)

//         while(EFC_IsBusy());
//         EFC_RegionLock(addr + (i * EFC_SECTORSIZE));
//     }

//     SCB_InvalidateICache();
//     SCB_InvalidateDCache();

//     SCB_EnableICache();
//     SCB_EnableDCache();

//     return BL_STATUS_OK;
// }

BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors)
{
    for (uint32_t i = 0; i < numberOfSectors; i++)
    {
        if (!EFC_SectorErase(addr))
        {
            return BL_STATUS_ERROR;
        }

        while(EFC_IsBusy());
        addr += EFC_SECTORSIZE;
    }

    return BL_STATUS_OK;
}
//volatile uint32_t memory[252] = {0};
//BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
//{
////    uint32_t qw[4];
////
////    while(len)
////    {
////        uint32_t chunk = (len >= 16) ? 16 : len;
////
////        memset(qw, 0xFF, sizeof(qw));
////        memcpy(qw, data, chunk);
////
////        if(!EFC_QuadWordWrite(qw, addr))
////            return BL_STATUS_ERROR;
////
////        while(EFC_IsBusy());
////        
////        EFC_Read(memory, sizeof(memory), 0x0040c000);
////        addr += 16;
////        data += chunk;
////        len  -= chunk;
////    }
//    while (len > 0)
//    {
//        uint32_t alignedAddr = addr & ~0xF;      // align xu?ng 16 byte
//        uint32_t offset      = addr & 0xF;       // offset trong quadword
//
////        uint8_t buf[16];
//        __attribute__((aligned(16))) uint8_t buf[16];
//        
//        SCB_CleanInvalidateDCache();
//        SCB_InvalidateICache();
//        __DSB();
//        __ISB();
//        
//        memcpy(buf, (const void *)alignedAddr, 16);  // ??c quadword hi?n t?i
//
//        uint32_t writeLen = 16 - offset;
//        if (writeLen > len)
//            writeLen = len;
//
//        memcpy(&buf[offset], data, writeLen);
//
//        if (!EFC_QuadWordWrite((uint32_t *)buf, alignedAddr))
//            return BL_STATUS_ERROR;
//
//        while (EFC_IsBusy());
//        
//        SCB_InvalidateDCache();
//        SCB_InvalidateICache();
//        __DSB();
//        __ISB();
//
//        addr += writeLen;
//        data += writeLen;
//        len  -= writeLen;
//    }
//
//
//    return BL_STATUS_OK;
//}

#include "device.h" // Ho?c th? vi?n ch?a ??nh ngh?a SCB (CMSIS)

BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)   // ?amt b?o chia h?t 16
{
    static uint8_t page_buffer[16]; // Buffer t?m 16 byte (QuadWord)
    
    while (len > 0) 
    {
        uint32_t aligned_addr = addr & ~0xF; // ??a ch? b?t ??u c?a QuadWord (chia h?t cho 16)
        uint32_t offset = addr & 0xF;        // V? trí l?ch trong QuadWord
        uint32_t chunk = (16 - offset) > len ? len : (16 - offset);

        // 1. ??C: L?y 16 byte hi?n t?i t? Flash (bypass Cache)
        SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, 16);
        uint32_t *src = (uint32_t *)aligned_addr;
        uint32_t *dest = (uint32_t *)page_buffer;
        dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2]; dest[3] = src[3];

        // 2. S?A: Ch? ?è ph?n d? li?u m?i nh?n t? UART vào ?úng v? trí
        memcpy(&page_buffer[offset], data, chunk);

        // 3. GHI: Ghi c? c?m 16 byte ?ã hoàn ch?nh vào Flash
        if (!EFC_QuadWordWrite((uint32_t *)page_buffer, aligned_addr)) {
            return BL_STATUS_ERROR;
        }
        while (EFC_IsBusy());

        // C?p nh?t con tr?
        addr += chunk;
        data += chunk;
        len -= chunk;
    }
    return BL_STATUS_OK;
}

//BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
//{
//    // 1. T?t Cache toàn c?c (N?u b?n không t?t ? main thì ?? ? ?ây)
//    SCB_DisableDCache(); 
//    SCB_DisableICache();
//    __DSB();
//    __ISB();
//
//    __attribute__((aligned(16))) uint8_t buf[16];
//
//    while (len > 0)
//    {
//        uint32_t alignedAddr = addr & ~0xF;
//        uint32_t offset      = addr & 0xF;
//        uint32_t writeLen    = (16 - offset) > len ? len : (16 - offset);
//
//        // 2. READ-MODIFY-WRITE (Lúc này memcpy s? ??c TR?C TI?P t? Flash)
//        memcpy(buf, (const void *)alignedAddr, 16);
//        memcpy(&buf[offset], data, writeLen);
//
//        // 3. GHI VÀO FLASH
//        if (!EFC_QuadWordWrite((uint32_t *)buf, alignedAddr)) {
//            return BL_STATUS_ERROR;
//        }
//
//        // 4. ??NG B? C?NG (R?t quan tr?ng cho file l?n)
//        while (EFC_IsBusy());
//        
//        // ??c thanh ghi ?? ép Bus ??ng b? d? li?u th?c t? vào ô nh?
//        volatile uint32_t dummy = EFC_REGS->EEFC_FSR; 
//        (void)dummy;
//
//        // C?p nh?t con tr?
//        addr += writeLen;
//        data += writeLen;
//        len  -= writeLen;
//    }

    // 5. B?t l?i Cache sau khi n?p xong hoàn toàn (N?u c?n thi?t)
    // SCB_EnableICache();
    // SCB_EnableDCache();

//    return BL_STATUS_OK;
//}

//BlStatus_t BlFlash_Read(uint32_t addr, uint8_t *data, uint32_t len)
//{
////    if (!EFC_Read(data, len, addr))
////    {
////        return BL_STATUS_ERROR;
////    }
//    memcpy(data, (const void *)addr, len);
//
//    return BL_STATUS_OK;
//}

BlStatus_t BlFlash_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    // 1. Vô hi?u hóa vùng Cache t??ng ?ng v?i ??a ch? c?n ??c
    // ??m b?o CPU ph?i l?y d? li?u th?c t? Flash Memory
    SCB_InvalidateDCache_by_Addr((uint32_t *)addr, len);
    __DSB();
    __ISB();

    // 2. Bây gi? memcpy m?i an toàn
    memcpy(data, (const void *)addr, len);

    return BL_STATUS_OK;
}

bool BlFlash_IsAppValid(void)
{
    // uint32_t stack;
    // uint32_t reset;

    // stack = *(uint32_t*)(BL_FLASH_APP_START + 0);
    // reset = *(uint32_t*)(BL_FLASH_APP_START + 4);

    // if ((stack < SRAM_START) || (stack > SRAM_END))
    //     return false;

    // if ((reset < BL_FLASH_APP_START) || (reset > BL_FLASH_APP_END))
    //     return false;

    return true;
}

uint32_t BlFlash_GetAppEntry(void)
{
    /* Get application entry point (stub) */
        // return *(uint32_t*)(BL_FLASH_APP_START + 4);
    return BL_APP_START_ADDR;
}