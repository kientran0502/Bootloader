#ifndef BL_FLASH_H
#define BL_FLASH_H

#include "bl_types.h"

/* Flash address config */
#define BL_APP_START_ADDR   0x00410000U
#define BL_APP_MAX_SIZE     (512 * 1024)

/* Init flash driver */
void BlFlash_Init(void);

/* Erase region */
BlStatus_t BlFlash_Erase(uint32_t addr, uint32_t numberOfSectors);

/* Write data */
BlStatus_t BlFlash_Write(uint32_t addr, uint8_t *data, uint32_t len);

/* Validate application */
bool BlFlash_IsAppValid(void);

/* Get app reset handler */
uint32_t BlFlash_GetAppEntry(void);

#endif