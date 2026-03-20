#include "bl_flash.h"

void BlFlash_Init(void)
{
    /* Initialize flash hardware (stub) */
    EFC_Initialize();
}

// BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors)  // adrress phải là bội của 16 
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
volatile uint32_t memory[252] = {0};
BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t qw[4];

    while(len)
    {
        uint32_t chunk = (len >= 16) ? 16 : len;

        memset(qw, 0xFF, sizeof(qw));
        memcpy(qw, data, chunk);

        if(!EFC_QuadWordWrite(qw, addr))
            return BL_STATUS_ERROR;

        while(EFC_IsBusy());
        
        EFC_Read(memory, sizeof(memory), 0x0040c000);
        addr += 16;
        data += chunk;
        len  -= chunk;
    }

    return BL_STATUS_OK;
}

BlStatus_t BlFlash_Read(uint32_t addr, uint32_t *data, uint32_t len)
{
    if (!EFC_Read(data, len, addr))
    {
        return BL_STATUS_ERROR;
    }

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