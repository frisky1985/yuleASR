/**
 * @file NvM_Lcfg.c
 * @brief NVRAM Manager Link-Time Configuration (Auto-Generated)
 * @version 1.0.0
 * @date 2026-04-28
 * @author NvM Configurator Tool
 * 
 * DO NOT EDIT MANUALLY - This file is auto-generated
 */

#include "NvM.h"
#include "NvM_Cfg.h"

/*==================================================================================================
*                                    ROM BLOCK DATA
==================================================================================================*/
static const uint8 NvM_RomBlock_Config[64] = {0}; /* Default values */
static const uint8 NvM_RomBlock_Calibration[256] = {0}; /* Default values */
static const uint8 NvM_RomBlock_VIN[17] = {0}; /* Default values */

/*==================================================================================================
*                                    BLOCK DESCRIPTOR TABLE
==================================================================================================*/
#define NVM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

STATIC const NvM_BlockDescriptorType NvM_BlockDescriptorTable[NVM_NUM_OF_NVRAM_BLOCKS] = {
    {
        .BlockId = 1,
        .BlockBaseNumber = 1,
        .ManagementType = NVM_BLOCK_NATIVE,
        .NumberOfNvBlocks = 1,
        .NumberOfDataSets = 1,
        .NvBlockLength = 64,
        .NvBlockNum = 1,
        .RomBlockNum = 1,
        .InitCallback = NULL_PTR,
        .JobEndCallback = NULL_PTR,
        .CrcType = NVM_CRC_16,
        .BlockUseCrc = TRUE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = TRUE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = NvM_RomBlock_Config,
        .RamBlockData = NULL_PTR,
        .MirrorBlockData = NULL_PTR
    },    {
        .BlockId = 2,
        .BlockBaseNumber = 2,
        .ManagementType = NVM_BLOCK_DATASET,
        .NumberOfNvBlocks = 4,
        .NumberOfDataSets = 4,
        .NvBlockLength = 256,
        .NvBlockNum = 4,
        .RomBlockNum = 4,
        .InitCallback = NULL_PTR,
        .JobEndCallback = NULL_PTR,
        .CrcType = NVM_CRC_16,
        .BlockUseCrc = TRUE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = TRUE,
        .BlockUseCompression = FALSE,
        .RomBlockData = NvM_RomBlock_Calibration,
        .RamBlockData = NULL_PTR,
        .MirrorBlockData = NULL_PTR
    },    {
        .BlockId = 3,
        .BlockBaseNumber = 3,
        .ManagementType = NVM_BLOCK_REDUNDANT,
        .NumberOfNvBlocks = 2,
        .NumberOfDataSets = 1,
        .NvBlockLength = 512,
        .NvBlockNum = 2,
        .RomBlockNum = 0,
        .InitCallback = NULL_PTR,
        .JobEndCallback = NULL_PTR,
        .CrcType = NVM_CRC_32,
        .BlockUseCrc = TRUE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = NULL_PTR,
        .RamBlockData = NULL_PTR,
        .MirrorBlockData = NULL_PTR
    },    {
        .BlockId = 4,
        .BlockBaseNumber = 4,
        .ManagementType = NVM_BLOCK_NATIVE,
        .NumberOfNvBlocks = 1,
        .NumberOfDataSets = 1,
        .NvBlockLength = 17,
        .NvBlockNum = 1,
        .RomBlockNum = 1,
        .InitCallback = NULL_PTR,
        .JobEndCallback = NULL_PTR,
        .CrcType = NVM_CRC_8,
        .BlockUseCrc = TRUE,
        .BlockUseSetRamBlockStatus = FALSE,
        .BlockWriteProt = TRUE,
        .BlockWriteOnce = TRUE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = NvM_RomBlock_VIN,
        .RamBlockData = NULL_PTR,
        .MirrorBlockData = NULL_PTR
    },    {
        .BlockId = 5,
        .BlockBaseNumber = 5,
        .ManagementType = NVM_BLOCK_REDUNDANT,
        .NumberOfNvBlocks = 2,
        .NumberOfDataSets = 1,
        .NvBlockLength = 8,
        .NvBlockNum = 2,
        .RomBlockNum = 0,
        .InitCallback = NULL_PTR,
        .JobEndCallback = NULL_PTR,
        .CrcType = NVM_CRC_16,
        .BlockUseCrc = TRUE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = TRUE,
        .BlockUseMirror = TRUE,
        .BlockUseCompression = FALSE,
        .RomBlockData = NULL_PTR,
        .RamBlockData = NULL_PTR,
        .MirrorBlockData = NULL_PTR
    }
};

#define NVM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL CONFIGURATION
==================================================================================================*/
#define NVM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

const NvM_ConfigType NvM_Config = {
    .BlockDescriptors = NvM_BlockDescriptorTable,
    .NumBlockDescriptors = NVM_NUM_OF_NVRAM_BLOCKS,
    .NumOfNvBlocks = NVM_NUM_OF_NVRAM_BLOCKS,
    .NumOfDataSets = NVM_NUM_OF_DATASETS,
    .NumOfRomBlocks = NVM_NUM_OF_ROM_BLOCKS,
    .MaxNumberOfWriteRetries = NVM_MAX_NUMBER_OF_WRITE_RETRIES,
    .MaxNumberOfReadRetries = NVM_MAX_NUMBER_OF_READ_RETRIES,
    .DevErrorDetect = NVM_DEV_ERROR_DETECT,
    .VersionInfoApi = NVM_VERSION_INFO_API,
    .SetRamBlockStatusApi = NVM_SET_RAM_BLOCK_STATUS_API,
    .GetErrorStatusApi = NVM_GET_ERROR_STATUS_API,
    .SetBlockProtectionApi = NVM_SET_BLOCK_PROTECTION_API,
    .GetBlockProtectionApi = NVM_GET_BLOCK_PROTECTION_API,
    .SetDataIndexApi = NVM_SET_DATA_INDEX_API,
    .GetDataIndexApi = NVM_GET_DATA_INDEX_API,
    .CancelJobApi = NVM_CANCEL_JOB_API,
    .KillWriteAllApi = NVM_KILL_WRITE_ALL_API,
    .KillReadAllApi = NVM_KILL_READ_ALL_API,
    .RepairDamagedBlocksApi = NVM_REPAIR_DAMAGED_BLOCKS_API,
    .CalcRamBlockCrc = NVM_CALC_RAM_BLOCK_CRC,
    .UseCrcCompMechanism = NVM_USE_CRC_COMP_MECHANISM,
    .MainFunctionPeriod = NVM_MAIN_FUNCTION_PERIOD_MS
};

#define NVM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"
