/**
 * @file NvM.h
 * @brief NVRAM Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: NVRAM Manager (NVM)
 * Layer: Service Layer
 */

#ifndef NVM_H
#define NVM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "NvM_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define NVM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define NVM_MODULE_ID                   (0x14U) /* NVM Module ID */
#define NVM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define NVM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define NVM_AR_RELEASE_REVISION_VERSION (0x00U)
#define NVM_SW_MAJOR_VERSION            (0x01U)
#define NVM_SW_MINOR_VERSION            (0x00U)
#define NVM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define NVM_SID_INIT                    (0x00U)
#define NVM_SID_SETDATAINDEX            (0x01U)
#define NVM_SID_GETERRORSTATUS          (0x04U)
#define NVM_SID_SETRAMBLOCKSTATUS       (0x05U)
#define NVM_SID_READBLOCK               (0x06U)
#define NVM_SID_WRITEBLOCK              (0x07U)
#define NVM_SID_RESTOREBLOCKDEFAULTS    (0x08U)
#define NVM_SID_ERASEBLOCK              (0x09U)
#define NVM_SID_INVALIDATENVBLOCK       (0x0BU)
#define NVM_SID_READPRAMBLOCK           (0x16U)
#define NVM_SID_WRITEPRAMBLOCK          (0x17U)
#define NVM_SID_WRITEBLOCK_ONCE         (0x0FU)
#define NVM_SID_GETVERSIONINFO          (0x0FU)
#define NVM_SID_CANCELJOBS              (0x10U)
#define NVM_SID_SETBLOCKLOCKSTATUS      (0x13U)
#define NVM_SID_SETBLOCKPROTECTION      (0x14U)
#define NVM_SID_SETWRITEONCESTATUS      (0x15U)
#define NVM_SID_REPAIRDamagedBlocks     (0x18U)
#define NVM_SID_KILLWRITEALL            (0x19U)
#define NVM_SID_KILLREADALL             (0x1AU)
#define NVM_SID_MAINFUNCTION            (0x1CU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define NVM_E_NOT_INITIALIZED           (0x14U)
#define NVM_E_BLOCK_PENDING             (0x15U)
#define NVM_E_BLOCK_CONFIG              (0x16U)
#define NVM_E_PARAM_BLOCK_ID            (0x0AU)
#define NVM_E_PARAM_BLOCK_DATA_IDX      (0x0BU)
#define NVM_E_PARAM_BLOCK_TYPE          (0x0CU)
#define NVM_E_PARAM_DATA_IDX            (0x0DU)
#define NVM_E_PARAM_POINTER             (0x0EU)
#define NVM_E_BLOCK_LOCKED              (0x13U)
#define NVM_E_WRITE_PROTECTED           (0x12U)

/*==================================================================================================
*                                    NVM REQUEST RESULT TYPE
==================================================================================================*/
typedef enum {
    NVM_REQ_OK = 0,
    NVM_REQ_NOT_OK,
    NVM_REQ_PENDING,
    NVM_REQ_INTEGRITY_FAILED,
    NVM_REQ_BLOCK_SKIPPED,
    NVM_REQ_NV_INVALIDATED,
    NVM_REQ_CANCELED,
    NVM_REQ_REDUNDANCY_FAILED,
    NVM_REQ_RESTORED_FROM_ROM,
    NVM_REQ_RESTORED_DEFAULTS
} NvM_RequestResultType;

/*==================================================================================================
*                                    NVM BLOCK MANAGEMENT TYPE
==================================================================================================*/
typedef enum {
    NVM_BLOCK_NATIVE = 0,
    NVM_BLOCK_REDUNDANT,
    NVM_BLOCK_DATASET
} NvM_BlockManagementType;

/*==================================================================================================
*                                    NVM BLOCK CRC TYPE
==================================================================================================*/
typedef enum {
    NVM_CRC_NONE = 0,
    NVM_CRC_8,
    NVM_CRC_16,
    NVM_CRC_32
} NvM_BlockCrcType;

/*==================================================================================================
*                                    NVM BLOCK ID TYPE
==================================================================================================*/
typedef uint16 NvM_BlockIdType;

/*==================================================================================================
*                                    NVM BLOCK CONFIG TYPE
==================================================================================================*/
typedef struct {
    NvM_BlockIdType BlockId;
    uint16 BlockBaseNumber;
    NvM_BlockManagementType ManagementType;
    uint8 NumberOfNvBlocks;
    uint8 NumberOfDataSets;
    uint16 NvBlockLength;
    uint16 NvBlockNum;
    uint16 RomBlockNum;
    uint16 InitCallback;
    uint16 JobEndCallback;
    NvM_BlockCrcType CrcType;
    boolean BlockUseCrc;
    boolean BlockUseSetRamBlockStatus;
    boolean BlockWriteProt;
    boolean BlockWriteOnce;
    boolean BlockAutoValidation;
    boolean BlockUseMirror;
    boolean BlockUseCompression;
    const void* RomBlockData;
    void* RamBlockData;
    void* MirrorBlockData;
} NvM_BlockDescriptorType;

/*==================================================================================================
*                                    NVM CONFIG TYPE
==================================================================================================*/
typedef struct {
    const NvM_BlockDescriptorType* BlockDescriptors;
    uint16 NumBlockDescriptors;
    uint16 NumOfNvBlocks;
    uint16 NumOfDataSets;
    uint16 NumOfRomBlocks;
    uint16 MaxNumberOfWriteRetries;
    uint16 MaxNumberOfReadRetries;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean SetRamBlockStatusApi;
    boolean GetErrorStatusApi;
    boolean SetBlockProtectionApi;
    boolean GetBlockProtectionApi;
    boolean SetDataIndexApi;
    boolean GetDataIndexApi;
    boolean CancelJobApi;
    boolean KillWriteAllApi;
    boolean KillReadAllApi;
    boolean RepairDamagedBlocksApi;
    boolean CalcRamBlockCrc;
    boolean UseCrcCompMechanism;
    uint16 MainFunctionPeriod;
} NvM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define NVM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const NvM_ConfigType NvM_Config;

#define NVM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the NVRAM Manager
 * @param ConfigPtr Pointer to configuration structure
 */
void NvM_Init(const NvM_ConfigType* ConfigPtr);

/**
 * @brief Sets the data index for a dataset block
 * @param BlockId Block ID
 * @param DataIndex Data index to set
 * @return Result of operation
 */
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex);

/**
 * @brief Gets the error status of a block
 * @param BlockId Block ID
 * @param RequestResultPtr Pointer to store result
 * @return Result of operation
 */
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr);

/**
 * @brief Sets the RAM block status
 * @param BlockId Block ID
 * @param BlockChanged Status to set
 * @return Result of operation
 */
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged);

/**
 * @brief Reads a block from NV memory
 * @param BlockId Block ID to read
 * @param NvM_DstPtr Destination pointer
 * @return Result of operation
 */
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr);

/**
 * @brief Writes a block to NV memory
 * @param BlockId Block ID to write
 * @param NvM_SrcPtr Source pointer
 * @return Result of operation
 */
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);

/**
 * @brief Restores block defaults from ROM
 * @param BlockId Block ID to restore
 * @param NvM_DestPtr Destination pointer
 * @return Result of operation
 */
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr);

/**
 * @brief Erases a block in NV memory
 * @param BlockId Block ID to erase
 * @return Result of operation
 */
Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId);

/**
 * @brief Invalidates a block in NV memory
 * @param BlockId Block ID to invalidate
 * @return Result of operation
 */
Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId);

/**
 * @brief Reads a permanent RAM block
 * @param BlockId Block ID to read
 * @return Result of operation
 */
Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId);

/**
 * @brief Writes a permanent RAM block
 * @param BlockId Block ID to write
 * @return Result of operation
 */
Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId);

/**
 * @brief Writes a block once (write-once protection)
 * @param BlockId Block ID to write
 * @param NvM_SrcPtr Source pointer
 * @return Result of operation
 */
Std_ReturnType NvM_WriteBlockOnce(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void NvM_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Cancels all jobs for a block
 * @param BlockId Block ID
 * @return Result of operation
 */
Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId);

/**
 * @brief Sets block lock status
 * @param BlockId Block ID
 * @param BlockLocked Lock status
 * @return Result of operation
 */
Std_ReturnType NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked);

/**
 * @brief Sets block protection
 * @param BlockId Block ID
 * @param ProtectionEnabled Protection status
 * @return Result of operation
 */
Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled);

/**
 * @brief Sets write once status
 * @param BlockId Block ID
 * @param WriteOnce Write once status
 * @return Result of operation
 */
Std_ReturnType NvM_SetWriteOnceStatus(NvM_BlockIdType BlockId, boolean WriteOnce);

/**
 * @brief Repairs damaged blocks
 * @return Result of operation
 */
Std_ReturnType NvM_RepairDamagedBlocks(void);

/**
 * @brief Kills WriteAll operation
 */
void NvM_KillWriteAll(void);

/**
 * @brief Kills ReadAll operation
 */
void NvM_KillReadAll(void);

/**
 * @brief Main function for periodic processing
 */
void NvM_MainFunction(void);

#define NVM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* NVM_H */
