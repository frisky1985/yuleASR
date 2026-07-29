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
*                                      GENERAL CONFIGURATION
==================================================================================================*/
static const Fee_ConfigType Fee_GeneralConfig =
{
    /* FeePageConfig */                   &Fee_PageConfig[0],
    /* FeeNumberOfPages */                FEE_NUMBER_OF_PAGES,
    /* FeeBlockConfig */                  &Fee_BlockConfig[0],
    /* FeeNumberOfBlocks */               FEE_NUMBER_OF_BLOCKS,
    /* FeeGarbageCollectThreshold */      FEE_GC_THRESHOLD_PERCENT,
    /* FeeGcRepetitions */                FEE_GC_REPETITIONS,
    /* FeeNvmJobEndNotificationEnabled */ TRUE,
    /* FeeUseEraseSuspend */              FEE_USE_ERASE_SUSPEND
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
    /* FeePageConfig */                   &Fee_PageConfig[0],
    /* FeeNumberOfPages */                FEE_NUMBER_OF_PAGES,
    /* FeeBlockConfig */                  &Fee_BlockConfig[0],
    /* FeeNumberOfBlocks */               FEE_NUMBER_OF_BLOCKS,
    /* FeeGarbageCollectThreshold */      FEE_GC_THRESHOLD_PERCENT,
    /* FeeGcRepetitions */                FEE_GC_REPETITIONS,
    /* FeeNvmJobEndNotificationEnabled */ TRUE,
    /* FeeUseEraseSuspend */              FEE_USE_ERASE_SUSPEND
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

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/

/* Get state transition for current state and job */
Fee_StateType Fee_GetNextState(Fee_StateType CurrentState, Fee_JobType JobType)
{
    uint8 Index;
    
    for (Index = 0U; Index < FEE_STATE_TRANSITION_COUNT; Index++)
    {
        if ((Fee_StateTransitions[Index].CurrentState == CurrentState) &&
            (Fee_StateTransitions[Index].JobType == JobType))
        {
            return Fee_StateTransitions[Index].NextState;
        }
    }
    
    /* No valid transition found - stay in current state */
    return CurrentState;
}

/* Check if state transition is valid */
boolean Fee_IsStateTransitionValid(Fee_StateType CurrentState, Fee_JobType JobType)
{
    uint8 Index;
    
    for (Index = 0U; Index < FEE_STATE_TRANSITION_COUNT; Index++)
    {
        if ((Fee_StateTransitions[Index].CurrentState == CurrentState) &&
            (Fee_StateTransitions[Index].JobType == JobType))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/* Update wear leveling counters */
void Fee_UpdateWearLeveling(uint8 PageIndex, uint8 Operation)
{
    if (PageIndex < FEE_NUMBER_OF_PAGES)
    {
        switch (Operation)
        {
            case 0U:  /* Erase operation */
                Fee_WearLevelingData[PageIndex].EraseCycleCount++;
                break;
                
            case 1U:  /* Write operation */
                Fee_WearLevelingData[PageIndex].WriteCycleCount++;
                break;
                
            case 2U:  /* GC trigger */
                Fee_WearLevelingData[PageIndex].GcTriggerCount++;
                break;
                
            default:
                /* Unknown operation - ignore */
                break;
        }
    }
}

/* Get page with lowest erase count for wear leveling */
uint8 Fee_GetPreferredPageForGc(void)
{
    uint8 PageIndex;
    uint8 PreferredPage = 0U;
    uint32 MinEraseCount = 0xFFFFFFFFU;
    
    for (PageIndex = 0U; PageIndex < FEE_NUMBER_OF_PAGES; PageIndex++)
    {
        if (Fee_WearLevelingData[PageIndex].EraseCycleCount < MinEraseCount)
        {
            MinEraseCount = Fee_WearLevelingData[PageIndex].EraseCycleCount;
            PreferredPage = PageIndex;
        }
    }
    
    return PreferredPage;
}

/* Get block configuration by block number */
const Fee_BlockConfigType* Fee_GetBlockConfig(uint16 BlockNumber)
{
    uint16 Index;
    
    for (Index = 0U; Index < FEE_NUMBER_OF_BLOCKS; Index++)
    {
        if (Fee_BlockConfig[Index].FeeBlockNumber == BlockNumber)
        {
            return &Fee_BlockConfig[Index];
        }
    }
    
    return NULL_PTR;
}

/* Get page configuration by page number */
const Fee_PageConfigType* Fee_GetPageConfig(uint8 PageNumber)
{
    uint8 Index;
    
    for (Index = 0U; Index < FEE_NUMBER_OF_PAGES; Index++)
    {
        if (Fee_PageConfig[Index].PageNumber == PageNumber)
        {
            return &Fee_PageConfig[Index];
        }
    }
    
    return NULL_PTR;
}

#define FEE_STOP_SEC_CODE
#include "Fee_MemMap.h"
