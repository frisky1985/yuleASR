/**
 * @file NvM_Private.h
 * @brief NVRAM Manager Private Header - stub for compilation
 */
#ifndef NVM_PRIVATE_H
#define NVM_PRIVATE_H

#include "Std_Types.h"
#include "NvM.h"
#include "NvM_Cfg.h"

/* NvM internal status */
typedef uint8 NvM_InternalStatusType;
#define NVM_STATUS_UNINIT       0x00u
#define NVM_STATUS_IDLE         0x01u
#define NVM_STATUS_BUSY         0x02u
#define NVM_STATUS_WRITING      0x03u
#define NVM_STATUS_READING      0x04u
#define NVM_STATUS_ERASING      0x05u
#define NVM_STATUS_ERROR        0x06u

/* NvM block state */
typedef uint8 NvM_BlockStateType;
#define NVM_BLOCK_INVALID       0x00u
#define NVM_BLOCK_VALID         0x01u
#define NVM_BLOCK_CHANGED       0x02u
#define NVM_BLOCK_ERROR         0x03u

/* NvM internal block info */
typedef struct {
    NvM_BlockIdType BlockId;
    NvM_BlockStateType State;
    uint8* DataPtr;
    uint16 Length;
    uint16 Crc;
    uint32 WriteCounter;
} NvM_BlockInfoType;

#endif /* NVM_PRIVATE_H */
