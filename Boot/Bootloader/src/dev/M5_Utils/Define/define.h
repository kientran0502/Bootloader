/************************************************
 *  @file     : define.h
 *  @date     : October 2025
 *  @author   : CAO HIEU
 *  @version  : 1.0.0
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#ifndef define_H
#define define_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    E_OK       = 0x00U,  /* Operation successful */
    E_ERROR    = 0x01U,  /* Operation failed */
    E_BUSY     = 0x02U,  /* Resource is busy */
    E_TIMEOUT  = 0x03U   /* Operation timed out */
} Std_ReturnType;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* define_H */