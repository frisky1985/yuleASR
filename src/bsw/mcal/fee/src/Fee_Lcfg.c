/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
* Project       : YuleASR - AUTOSAR Flash EEPROM Emulation (Fee) Module
* File          : Fee_Lcfg.c
* Description   : Fee module link-time configuration providing block/page configs and state tables
*================================================================================================*/

/*==================================================================================================
*                                      INCLUDE FILES
==================================================================================================*/
#include "Fee.h"
#include "Fee_Cfg.h"

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      BLOCK CONFIGURATION
*==================================================================================================
* Block configuration table - defines all EEPROM emulation blocks
* Each block has: number, size, immediate flag, device index, cycle count, alignment
==================================================================================================*/
static const Fee_BlockConfigType Fee_BlockConfig[FEE_NUMBER_OF_BLOCKS] =
{
    /* Block 0: NVM Configuration Block - 32 bytes, immediate write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_NVM_CONFIG_ID,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_32,
        /* FeeImmediateData */        TRUE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      50000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 1: NVM Admin Block - 16 bytes, immediate write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_NVM_ADMIN_ID,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_16,
        /* FeeImmediateData */        TRUE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      50000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 2: User Block 1 - 64 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_1,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_64,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 3: User Block 2 - 128 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_2,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_128,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 4: User Block 3 - 256 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_3,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_256,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 5: User Block 4 - 512 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_4,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_512,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 6: User Block 5 - 64 bytes, immediate write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_5,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_64,
        /* FeeImmediateData */        TRUE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      50000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 7: User Block 6 - 128 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_6,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_128,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 8: User Block 7 - 32 bytes, immediate write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_7,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_32,
        /* FeeImmediateData */        TRUE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      50000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    },
    
    /* Block 9: User Block 8 - 256 bytes, normal write */
    {
        /* FeeBlockNumber */          FEE_BLOCK_ID_8,
        /* FeeBlockSize */            FEE_BLOCK_SIZE_256,
        /* FeeImmediateData */        FALSE,
        /* FeeDeviceIndex */          0U,
        /* FeeBlockCycleCount */      100000U,
        /* FeeDataAlignment */        FEE_DATA_ALIGNMENT
    }
};

/*==================================================================================================
*                                      PAGE CONFIGURATION
*==================================================================================================
* Page configuration table - defines flash pages for EEPROM emulation
* Dual-page scheme: Page 0 and Page 1 for wear leveling and garbage collection
==================================================================================================*/
static const Fee_PageConfigType Fee_PageConfig[FEE_NUMBER_OF_PAGES] =
{
    /* Page 0: First Fee page */
    {
        /* PageStartAddress */        FEE_PAGE_0_START_ADDRESS,
        /* PageSize */                FEE_PAGE_SIZE,
        /* PageNumber */              0U
    },
    
    /* Page 1: Second Fee page (for GC and wear leveling) */
    {
        /* PageStartAddress */        FEE_PAGE_1_START_ADDRESS,
        /* PageSize */                FEE_PAGE_SIZE,
        /* PageNumber */              1U
    }
};

/*==================================================================================================
*                                      SECTOR CONFIGURATION
==================================================================================================*/
static const Fee_SectorType Fee_SectorConfig[FEE_NUMBER_OF_SECTORS] =
{
    {
        .sectorStartAddr   = FEE_SECTOR0_START_ADDR,
        .sectorSize        = FEE_SECTOR0_SIZE,
        .sectorPageSize    = FEE_VIRTUAL_PAGE_SIZE,
        .sectorEraseCycles = FEE_SECTOR0_ERASE_CYCLES,
        .sectorWritable    = FEE_SECTOR0_WRITABLE,
        .sectorErasable    = FEE_SECTOR0_ERASABLE
    },
    {
        .sectorStartAddr   = FEE_SECTOR1_START_ADDR,
        .sectorSize        = FEE_SECTOR1_SIZE,
        .sectorPageSize    = FEE_VIRTUAL_PAGE_SIZE,
        .sectorEraseCycles = FEE_SECTOR1_ERASE_CYCLES,
        .sectorWritable    = FEE_SECTOR1_WRITABLE,
        .sectorErasable    = FEE_SECTOR1_ERASABLE
    }
};

/*==================================================================================================
*                                      GENERAL CONFIGURATION
==================================================================================================*/
static const Fee_ConfigType Fee_GeneralConfig =
{
    /* Core driver configuration */
    .sectorList                       = Fee_SectorConfig,
    .blockList                        = NULL_PTR,
    .sectorCount                      = FEE_NUMBER_OF_SECTORS,
    .blockCount                       = FEE_NUMBER_OF_BLOCKS,
    .defaultMode                      = FEE_MODE_NORMAL,
    .virtualPageSize                  = FEE_VIRTUAL_PAGE_SIZE,
    .maxReadNormalMode                = FEE_MAX_READ_NORMAL_MODE,
    .maxReadFastMode                  = FEE_MAX_READ_FAST_MODE,
    .maxWriteNormalMode               = FEE_MAX_WRITE_NORMAL_MODE,
    .maxWriteFastMode                 = FEE_MAX_WRITE_FAST_MODE,
    .eraseSuspendSupport              = FEE_USE_ERASE_SUSPEND,
    /* Link-time config */
    .FeePageConfig                    = &Fee_PageConfig[0],
    .FeeNumberOfPages                 = FEE_NUMBER_OF_PAGES,
    .FeeBlockConfig                   = &Fee_BlockConfig[0],
    .FeeNumberOfBlocks                = FEE_NUMBER_OF_BLOCKS,
    .FeeGarbageCollectThreshold       = FEE_GC_THRESHOLD_PERCENT,
    .FeeGcRepetitions                 = FEE_GC_REPETITIONS,
    .FeeNvmJobEndNotificationEnabled  = TRUE,
    .FeeUseEraseSuspend               = FEE_USE_ERASE_SUSPEND
};

/*==================================================================================================
*                                      STATE MACHINE TABLES
*==================================================================================================
/* State transition table for Fee module state machine */
typedef struct
{
    Fee_StateType CurrentState;
    Fee_JobType   JobType;
    Fee_StateType NextState;
} Fee_StateTransitionType;

/* State transition table - defines valid state transitions */
static const Fee_StateTransitionType Fee_StateTransitions[] =
{
    /* From IDLE state */
    { FEE_STATE_IDLE, FEE_JOB_READ,             FEE_STATE_READ_HEADER },
    { FEE_STATE_IDLE, FEE_JOB_WRITE,            FEE_STATE_WRITE_HEADER },
    { FEE_STATE_IDLE, FEE_JOB_ERASE_IMMEDIATE,  FEE_STATE_ERASE_IMMEDIATE },
    { FEE_STATE_IDLE, FEE_JOB_GC_PAGE,          FEE_STATE_GC_COPY },
    
    /* From READ states */
    { FEE_STATE_READ_HEADER, FEE_JOB_READ,      FEE_STATE_READ_DATA },
    { FEE_STATE_READ_DATA,   FEE_JOB_READ,      FEE_STATE_IDLE },
    
    /* From WRITE states */
    { FEE_STATE_WRITE_HEADER, FEE_JOB_WRITE,    FEE_STATE_WRITE_DATA },
    { FEE_STATE_WRITE_DATA,   FEE_JOB_WRITE,    FEE_STATE_IDLE },
    
    /* From ERASE states */
    { FEE_STATE_ERASE_IMMEDIATE, FEE_JOB_ERASE_IMMEDIATE, FEE_STATE_IDLE },
    
    /* From GC states */
    { FEE_STATE_GC_COPY,  FEE_JOB_GC_PAGE,      FEE_STATE_GC_ERASE },
    { FEE_STATE_GC_ERASE, FEE_JOB_GC_PAGE,      FEE_STATE_IDLE }
};

#define FEE_STATE_TRANSITION_COUNT    (sizeof(Fee_StateTransitions) / sizeof(Fee_StateTransitionType))

/*==================================================================================================
*                                      WEAR LEVELING DATA
==================================================================================================*/
/* Wear leveling tracking structure */
typedef struct
{
    uint32 EraseCycleCount;              /* Number of erase cycles for each page */
    uint32 WriteCycleCount;              /* Total write cycles */
    uint32 GcTriggerCount;               /* Number of GC triggers */
} Fee_WearLevelingType;

/* Wear leveling data (RAM mirror, initialized at startup) */
static Fee_WearLevelingType Fee_WearLevelingData[FEE_NUMBER_OF_PAGES] =
{
    { 0U, 0U, 0U },  /* Page 0 */
    { 0U, 0U, 0U }   /* Page 1 */
};

/*==================================================================================================
*                                      EXTERNAL REFERENCES
==================================================================================================*/
/* External reference to the main configuration structure */
#define FEE_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"

const Fee_ConfigType Fee_Config =
{
    .sectorList                       = Fee_SectorConfig,
    .blockList                        = NULL_PTR,
    .sectorCount                      = FEE_NUMBER_OF_SECTORS,
    .blockCount                       = FEE_NUMBER_OF_BLOCKS,
    .defaultMode                      = FEE_MODE_NORMAL,
    .virtualPageSize                  = FEE_VIRTUAL_PAGE_SIZE,
    .maxReadNormalMode                = FEE_MAX_READ_NORMAL_MODE,
    .maxReadFastMode                  = FEE_MAX_READ_FAST_MODE,
    .maxWriteNormalMode               = FEE_MAX_WRITE_NORMAL_MODE,
    .maxWriteFastMode                 = FEE_MAX_WRITE_FAST_MODE,
    .eraseSuspendSupport              = FEE_USE_ERASE_SUSPEND,
    .FeePageConfig                    = &Fee_PageConfig[0],
    .FeeNumberOfPages                 = FEE_NUMBER_OF_PAGES,
    .FeeBlockConfig                   = &Fee_BlockConfig[0],
    .FeeNumberOfBlocks                = FEE_NUMBER_OF_BLOCKS,
    .FeeGarbageCollectThreshold       = FEE_GC_THRESHOLD_PERCENT,
    .FeeGcRepetitions                 = FEE_GC_REPETITIONS,
    .FeeNvmJobEndNotificationEnabled  = TRUE,
    .FeeUseEraseSuspend               = FEE_USE_ERASE_SUSPEND
};

#define FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Fee_MemMap.h"

/*==================================================================================================
*                                      CALLBACK FUNCTIONS
==================================================================================================*/
#define FEE_START_SEC_CODE
#include "Fee_MemMap.h"

/* NVM Job End Notification - called when Fee job completes successfully */
void Fee_NvmJobEndNotification(void)
{
    /* This function should be implemented by the NvM module
     * It is called by Fee when a job completes successfully
     */
}

/* NVM Job Error Notification - called when Fee job fails */
void Fee_NvmJobErrorNotification(void)
{
    /* This function should be implemented by the NvM module
     * It is called by Fee when a job fails
     */
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
