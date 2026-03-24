#ifndef BL_FLASH_H
#define BL_FLASH_H

#include "bl_types.h"
#include "peripheral/efc/plib_efc.h"

/* Flash address config */


/* Init flash driver */
void BlFlash_Init(void);

/* Erase region */
BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors);

/* Write data */
BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len);

BlStatus_t BlFlash_Read(uint32_t addr, uint8_t *data, uint32_t len);

/* Validate application */
bool BlFlash_IsAppValid(void);

/* Get app reset handler */
uint32_t BlFlash_GetAppEntry(void);

#endif