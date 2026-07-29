/*==================================================================================================
 *                                  MOCK FLASH HARDWARE LAYER
 *==================================================================================================
 * FILENAME: mock_flshw.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Mock implementation of Flash Hardware layer for integration testing
 *==================================================================================================
 */

#ifndef MOCK_FLSHW_H
#define MOCK_FLSHW_H

#include "Std_Types.h"

/*==================================================================================================
 *                                      CONSTANTS
 *==================================================================================================*/
#define MOCK_FLASH_SIZE         0x100000u    /* 1MB simulated flash */
#define MOCK_FLASH_SECTOR_SIZE  0x10000u     /* 64KB sectors */
#define MOCK_FLASH_PAGE_SIZE    0x100u       /* 256 bytes per page */
#define MOCK_FLASH_NUM_SECTORS  16u

/*==================================================================================================
 *                                      TYPES
 *==================================================================================================*/
typedef enum {
    MOCK_FLSHW_OK = 0,
    MOCK_FLSHW_NOT_OK,
    MOCK_FLSHW_BUSY,
    MOCK_FLSHW_TIMEOUT
} Mock_FlsHw_StatusType;

typedef struct {
    uint32 Address;
    uint32 Size;
    uint32 EraseCount;
    boolean IsErased;
    boolean IsValid;
} Mock_FlsHw_SectorInfoType;

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 *==================================================================================================*/
extern uint8 Mock_FlashMemory[MOCK_FLASH_SIZE];
extern boolean Mock_FlsHw_WriteFailInjected;
extern boolean Mock_FlsHw_ReadFailInjected;
extern boolean Mock_FlsHw_EraseFailInjected;

/*==================================================================================================
 *                                      FUNCTION PROTOTYPES
 *==================================================================================================*/
void Mock_FlsHw_Init(void);
void Mock_FlsHw_DeInit(void);

Mock_FlsHw_StatusType Mock_FlsHw_Read(uint32 Address, uint8* DataPtr, uint32 Length);
Mock_FlsHw_StatusType Mock_FlsHw_Write(uint32 Address, const uint8* DataPtr, uint32 Length);
Mock_FlsHw_StatusType Mock_FlsHw_Erase(uint32 Address, uint32 Size);

void Mock_FlsHw_InjectWriteFail(boolean enable);
void Mock_FlsHw_InjectReadFail(boolean enable);
void Mock_FlsHw_InjectEraseFail(boolean enable);

void Mock_FlsHw_GetSectorInfo(uint32 Address, Mock_FlsHw_SectorInfoType* InfoPtr);
void Mock_FlsHw_Clear(void);

#endif /* MOCK_FLSHW_H */
