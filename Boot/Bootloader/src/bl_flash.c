#include "bl_flash.h"
#include "peripheral/efc/plib_efc.h"

void BlFlash_Init(void)
{
    /* Initialize flash hardware (stub) */
    EFC_Initialize();
}

BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors)  // adrress phải là bội của 16 
{
    if( addr % 16 != 0 || numberOfSectors == 0)
    {
        return BL_STATUS_INVALID;
    }
    /* Erase flash region (stub) */
    SCB_DisableICache();
    SCB_DisableDCache();

    for(uint32_t i = 0; i < numberOfSectors; i++)
    {
        while(EFC_IsBusy());
        EFC_RegionUnlock(addr + (i * EFC_SECTORSIZE));

        while(EFC_IsBusy());
        EFC_SectorErase(addr + (i * EFC_SECTORSIZE)); // Function to erase a sector (16 pages at a time)

        while(EFC_IsBusy());
        EFC_RegionLock(addr + (i * EFC_SECTORSIZE));
    }

    SCB_InvalidateICache();
    SCB_InvalidateDCache();

    SCB_EnableICache();
    SCB_EnableDCache();

    return BL_STATUS_OK;
}

BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    /* Write data to flash (stub) */
    SCB_DisableICache();
    SCB_DisableDCache();

    for(uint32_t i = 0; i < len; i += EFC_PAGESIZE)
    {
        while(EFC_IsBusy());
        EFC_RegionUnlock(addr + i);
        
        while(EFC_IsBusy());
        EFC_PageWrite((uint32_t*)(data + i), addr + i); // Function to write a page (512 bytes at a time)

        while(EFC_IsBusy());
        EFC_RegionLock(addr + i);
    }

    SCB_InvalidateICache();
    SCB_InvalidateDCache();

    SCB_EnableICache();
    SCB_EnableDCache();

    return BL_STATUS_OK;
}

bool BlFlash_IsAppValid(void)
{
    /* Check if application is valid (stub) */
    return true;
}

uint32_t BlFlash_GetAppEntry(void)
{
    /* Get application entry point (stub) */
    return BL_APP_START_ADDR;
}