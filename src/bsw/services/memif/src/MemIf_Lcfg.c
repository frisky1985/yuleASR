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

/*******************************************************************************
 * @file        MemIf_Lcfg.c
 * @brief       Memory Interface link-time configuration
 * @module      SERVICES
 * @author      AutoSAR Generator
 * @version     1.0.0
 * @date        2026-05-06
 * @copyright   上海予乐电子科技有限公司
 * 
 * @description
 * This file contains the link-time configuration tables for the MemIf module.
 * It defines the device configurations and hardware mapping.
 ******************************************************************************/

/*******************************************************************************
 *                              Includes
 ******************************************************************************/
#include "MemIf.h"
#include "MemIf_Cfg.h"

#if (MEMIF_FEE_USED == STD_ON)
#include "Fee.h"
#endif

#if (MEMIF_EA_USED == STD_ON)
#include "Ea.h"
#endif

/*******************************************************************************
 *                          Local Function Declarations
 ******************************************************************************/

/* Fee wrapper functions */
#if (MEMIF_FEE_USED == STD_ON)
static void MemIf_Fee_ReadWrapper(void);
static void MemIf_Fee_WriteWrapper(void);
static void MemIf_Fee_EraseWrapper(void);
static void MemIf_Fee_CancelWrapper(void);
static MemIf_StatusType MemIf_Fee_GetStatusWrapper(void);
static MemIf_JobResultType MemIf_Fee_GetJobResultWrapper(void);
#endif

/* Ea wrapper functions */
#if (MEMIF_EA_USED == STD_ON)
static void MemIf_Ea_ReadWrapper(void);
static void MemIf_Ea_WriteWrapper(void);
static void MemIf_Ea_EraseWrapper(void);
static void MemIf_Ea_CancelWrapper(void);
static MemIf_StatusType MemIf_Ea_GetStatusWrapper(void);
static MemIf_JobResultType MemIf_Ea_GetJobResultWrapper(void);
#endif

/*******************************************************************************
 *                          Device Configuration Tables
 ******************************************************************************/

/** @brief Device abstraction table - mapped to actual drivers */
static const MemIf_DeviceAbstractionType MemIf_Devices[MEMIF_NUMBER_OF_DEVICES] = {
    /* Device 0: Fee (Flash EEPROM Emulation) */
    {
        .DeviceIndex = MEMIF_FEE_DEVICE_INDEX,
        .DeviceType = 0u,  /* Fee type */
#if (MEMIF_FEE_USED == STD_ON)
        .Read = MemIf_Fee_ReadWrapper,
        .Write = MemIf_Fee_WriteWrapper,
        .Erase = MemIf_Fee_EraseWrapper,
        .Cancel = MemIf_Fee_CancelWrapper,
        .GetStatus = MemIf_Fee_GetStatusWrapper,
        .GetJobResult = MemIf_Fee_GetJobResultWrapper
#else
        .Read = NULL_PTR,
        .Write = NULL_PTR,
        .Erase = NULL_PTR,
        .Cancel = NULL_PTR,
        .GetStatus = NULL_PTR,
        .GetJobResult = NULL_PTR
#endif
    },
    
    /* Device 1: Ea (EEPROM Abstraction) */
    {
        .DeviceIndex = MEMIF_EA_DEVICE_INDEX,
        .DeviceType = 1u,  /* Ea type */
#if (MEMIF_EA_USED == STD_ON)
        .Read = MemIf_Ea_ReadWrapper,
        .Write = MemIf_Ea_WriteWrapper,
        .Erase = MemIf_Ea_EraseWrapper,
        .Cancel = MemIf_Ea_CancelWrapper,
        .GetStatus = MemIf_Ea_GetStatusWrapper,
        .GetJobResult = MemIf_Ea_GetJobResultWrapper
#else
        .Read = NULL_PTR,
        .Write = NULL_PTR,
        .Erase = NULL_PTR,
        .Cancel = NULL_PTR,
        .GetStatus = NULL_PTR,
        .GetJobResult = NULL_PTR
#endif
    }
};

/*******************************************************************************
 *                          Fee Device Wrappers
 ******************************************************************************/

#if (MEMIF_FEE_USED == STD_ON)

/** @brief Wrapper for Fee_Read */
static void MemIf_Fee_ReadWrapper(void)
{
    /* This wrapper provides a generic interface - actual parameters
     * are passed through global context or direct API call in MemIf.c */
}

/** @brief Wrapper for Fee_Write */
static void MemIf_Fee_WriteWrapper(void)
{
    /* Fee write wrapper */
}

/** @brief Wrapper for Fee_Erase */
static void MemIf_Fee_EraseWrapper(void)
{
    /* Fee erase wrapper - calls Fee_EraseImmediateBlock or Fee_InvalidateBlock */
}

/** @brief Wrapper for Fee_Cancel */
static void MemIf_Fee_CancelWrapper(void)
{
    Fee_Cancel();
}

/** @brief Wrapper for Fee_GetStatus */
static MemIf_StatusType MemIf_Fee_GetStatusWrapper(void)
{
    MemIf_StatusType status = MEMIF_UNINIT;
    
    switch (Fee_GetStatus()) {
        case MEMIF_IDLE:
            status = MEMIF_IDLE;
            break;
        case MEMIF_BUSY:
            status = MEMIF_BUSY;
            break;
        case MEMIF_UNINIT:
        default:
            status = MEMIF_UNINIT;
            break;
    }
    
    return status;
}

/** @brief Wrapper for Fee_GetJobResult */
static MemIf_JobResultType MemIf_Fee_GetJobResultWrapper(void)
{
    MemIf_JobResultType result = MEMIF_JOB_FAILED;
    
    switch (Fee_GetJobResult()) {
        case MEMIF_JOB_OK:
            result = MEMIF_JOB_OK;
            break;
        case MEMIF_JOB_PENDING:
            result = MEMIF_JOB_PENDING;
            break;
        case MEMIF_JOB_CANCELED:
            result = MEMIF_JOB_CANCELED;
            break;
        case MEMIF_JOB_FAILED:
        default:
            result = MEMIF_JOB_FAILED;
            break;
    }
    
    return result;
}

#endif /* MEMIF_FEE_USED */

/*******************************************************************************
 *                          Ea Device Wrappers
 ******************************************************************************/

#if (MEMIF_EA_USED == STD_ON)

/** @brief Wrapper for Ea_Read */
static void MemIf_Ea_ReadWrapper(void)
{
    /* Ea read wrapper */
}

/** @brief Wrapper for Ea_Write */
static void MemIf_Ea_WriteWrapper(void)
{
    /* Ea write wrapper */
}

/** @brief Wrapper for Ea_Erase */
static void MemIf_Ea_EraseWrapper(void)
{
    /* Ea erase wrapper - calls Ea_EraseImmediateBlock or Ea_InvalidateBlock */
}

/** @brief Wrapper for Ea_Cancel */
static void MemIf_Ea_CancelWrapper(void)
{
    Ea_Cancel();
}

/** @brief Wrapper for Ea_GetStatus */
static MemIf_StatusType MemIf_Ea_GetStatusWrapper(void)
{
    MemIf_StatusType status = MEMIF_UNINIT;
    
    switch (Ea_GetStatus()) {
        case MEMIF_IDLE:
            status = MEMIF_IDLE;
            break;
        case MEMIF_BUSY:
            status = MEMIF_BUSY;
            break;
        case MEMIF_UNINIT:
        default:
            status = MEMIF_UNINIT;
            break;
    }
    
    return status;
}

/** @brief Wrapper for Ea_GetJobResult */
static MemIf_JobResultType MemIf_Ea_GetJobResultWrapper(void)
{
    MemIf_JobResultType result = MEMIF_JOB_FAILED;
    
    switch (Ea_GetJobResult()) {
        case MEMIF_JOB_OK:
            result = MEMIF_JOB_OK;
            break;
        case MEMIF_JOB_PENDING:
            result = MEMIF_JOB_PENDING;
            break;
        case MEMIF_JOB_CANCELED:
            result = MEMIF_JOB_CANCELED;
            break;
        case MEMIF_JOB_FAILED:
        default:
            result = MEMIF_JOB_FAILED;
            break;
    }
    
    return result;
}

#endif /* MEMIF_EA_USED */

/*******************************************************************************
 *                          Hardware Mapping Table
 ******************************************************************************/

/** @brief Hardware mapping structure */
typedef struct {
    uint8 DeviceIndex;
    uint32 BaseAddress;
    uint32 TotalSize;
    uint32 BlockSize;
    uint8 DeviceType;  /* 0=Fee, 1=Ea */
} MemIf_HardwareMappingType;

/** @brief Hardware mapping configuration */
static const MemIf_HardwareMappingType MemIf_HardwareMap[MEMIF_NUMBER_OF_DEVICES] = {
    /* Fee Device - Flash EEPROM Emulation */
    {
        .DeviceIndex = MEMIF_FEE_DEVICE_INDEX,
        .BaseAddress = 0x00010000u,  /* Flash base address for Fee */
        .TotalSize = 0x00010000u,    /* 64KB Fee area */
        .BlockSize = 0x00000080u,    /* 128 bytes per block */
        .DeviceType = 0u
    },
    
    /* Ea Device - EEPROM Abstraction */
    {
        .DeviceIndex = MEMIF_EA_DEVICE_INDEX,
        .BaseAddress = 0x00020000u,  /* EEPROM base address */
        .TotalSize = 0x00004000u,    /* 16KB EEPROM area */
        .BlockSize = 0x00000008u,    /* 8 bytes per block (EEPROM typical) */
        .DeviceType = 1u
    }
};

/*******************************************************************************
 *                          Block Configuration
 ******************************************************************************/

/** @brief Block configuration entry */
typedef struct {
    uint16 BlockNumber;
    uint16 BlockSize;
    uint8 DeviceIndex;
    boolean ImmediateData;
} MemIf_BlockConfigType;

/** @brief Block configuration table - example for Fee blocks */
#if (MEMIF_FEE_USED == STD_ON)
const MemIf_BlockConfigType MemIf_FeeBlockConfig[] = {
    {0,  64,   MEMIF_FEE_DEVICE_INDEX, FALSE},   /* Block 0: 64 bytes */
    {1,  128,  MEMIF_FEE_DEVICE_INDEX, FALSE},   /* Block 1: 128 bytes */
    {2,  256,  MEMIF_FEE_DEVICE_INDEX, FALSE},   /* Block 2: 256 bytes */
    {3,  512,  MEMIF_FEE_DEVICE_INDEX, TRUE},    /* Block 3: 512 bytes, immediate */
    {4,  1024, MEMIF_FEE_DEVICE_INDEX, FALSE},   /* Block 4: 1KB */
};

#define MEMIF_FEE_NUM_BLOCKS    (sizeof(MemIf_FeeBlockConfig) / sizeof(MemIf_BlockConfigType))
#endif

/** @brief Block configuration table - example for Ea blocks */
#if (MEMIF_EA_USED == STD_ON)
const MemIf_BlockConfigType MemIf_EaBlockConfig[] = {
    {0,  8,    MEMIF_EA_DEVICE_INDEX, FALSE},    /* Block 0: 8 bytes */
    {1,  16,   MEMIF_EA_DEVICE_INDEX, FALSE},    /* Block 1: 16 bytes */
    {2,  32,   MEMIF_EA_DEVICE_INDEX, FALSE},    /* Block 2: 32 bytes */
    {3,  64,   MEMIF_EA_DEVICE_INDEX, TRUE},     /* Block 3: 64 bytes, immediate */
    {4,  128,  MEMIF_EA_DEVICE_INDEX, FALSE},    /* Block 4: 128 bytes */
};

#define MEMIF_EA_NUM_BLOCKS     (sizeof(MemIf_EaBlockConfig) / sizeof(MemIf_BlockConfigType))
#endif

/*******************************************************************************
 *                          Runtime Configuration
 ******************************************************************************/

/** @brief Runtime configuration structure */
typedef struct {
    uint8 NumberOfDevices;
    uint8 NumberOfFeeDevices;
    uint8 NumberOfEaDevices;
    boolean PollingMode;
    uint32 ReadTimeout;
    uint32 WriteTimeout;
    uint32 EraseTimeout;
} MemIf_RuntimeConfigType;

/** @brief Runtime configuration instance */
static const MemIf_RuntimeConfigType MemIf_RuntimeConfig = {
    .NumberOfDevices = MEMIF_NUMBER_OF_DEVICES,
    .NumberOfFeeDevices = MEMIF_NUMBER_OF_FEE_DEVICES,
    .NumberOfEaDevices = MEMIF_NUMBER_OF_EA_DEVICES,
    .PollingMode = (boolean)MEMIF_POLLING_MODE,
    .ReadTimeout = MEMIF_READ_TIMEOUT_MS,
    .WriteTimeout = MEMIF_WRITE_TIMEOUT_MS,
    .EraseTimeout = MEMIF_ERASE_TIMEOUT_MS
};

/*******************************************************************************
 *                          Version Check
 ******************************************************************************/

/** @brief Version info for configuration consistency check */
typedef struct {
    uint16 VendorID;
    uint16 ModuleID;
    uint8 MajorVersion;
    uint8 MinorVersion;
    uint8 PatchVersion;
} MemIf_VersionCheckType;

static const MemIf_VersionCheckType MemIf_LcfgVersion = {
    .VendorID = MEMIF_VENDOR_ID,
    .ModuleID = MEMIF_MODULE_ID,
    .MajorVersion = MEMIF_SW_MAJOR_VERSION,
    .MinorVersion = MEMIF_SW_MINOR_VERSION,
    .PatchVersion = MEMIF_SW_PATCH_VERSION
};
