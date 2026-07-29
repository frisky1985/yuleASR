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
 * @file        MemIf_Cfg.h
 * @brief       Memory Interface configuration header
 * @module      SERVICES
 * @author      AutoSAR Generator
 * @version     1.0.0
 * @date        2026-05-06
 * @copyright   上海予乐电子科技有限公司
 * 
 * @description
 * This file contains the configuration parameters for the MemIf module.
 * These settings can be adjusted based on the target hardware and application
 * requirements.
 ******************************************************************************/

#ifndef MEMIF_CFG_H
#define MEMIF_CFG_H

/*******************************************************************************
 *                              Includes
 ******************************************************************************/
#include "Std_Types.h"

/*******************************************************************************
 *                          Pre-compile Configurations
 ******************************************************************************/

/** @brief Enable/disable development error detection */
#ifndef MEMIF_DEV_ERROR_DETECT
    #define MEMIF_DEV_ERROR_DETECT              (STD_ON)
#endif

/** @brief Enable/disable version info API */
#ifndef MEMIF_VERSION_INFO_API
    #define MEMIF_VERSION_INFO_API              (STD_ON)
#endif

/*******************************************************************************
 *                          Device Configuration
 ******************************************************************************/

/** @brief Number of memory devices supported */
#define MEMIF_NUMBER_OF_DEVICES             (2u)

/** @brief Maximum number of Fee devices */
#define MEMIF_NUMBER_OF_FEE_DEVICES         (1u)

/** @brief Maximum number of Ea devices */
#define MEMIF_NUMBER_OF_EA_DEVICES          (1u)

/*******************************************************************************
 *                          Device Index Constants
 ******************************************************************************/

/** @brief Fee device index */
#define MEMIF_FEE_DEVICE_INDEX              (0u)

/** @brief Ea device index */
#define MEMIF_EA_DEVICE_INDEX               (1u)

/*******************************************************************************
 *                          Operation Mode
 ******************************************************************************/

/** @brief Enable polling mode for status checking */
#define MEMIF_POLLING_MODE                  (STD_ON)

/** @brief Enable interrupt mode (if not using polling) */
#define MEMIF_INTERRUPT_MODE                (STD_OFF)

/*******************************************************************************
 *                          Block Configuration
 ******************************************************************************/

/** @brief Maximum number of blocks per device */
#define MEMIF_MAX_BLOCK_NUMBER              (256u)

/** @brief Maximum block size in bytes */
#define MEMIF_MAX_BLOCK_SIZE                (4096u)

/*******************************************************************************
 *                          Timeout Configuration
 ******************************************************************************/

/** @brief Read timeout in ms (0 = no timeout) */
#define MEMIF_READ_TIMEOUT_MS               (100u)

/** @brief Write timeout in ms (0 = no timeout) */
#define MEMIF_WRITE_TIMEOUT_MS              (1000u)

/** @brief Erase timeout in ms (0 = no timeout) */
#define MEMIF_ERASE_TIMEOUT_MS              (5000u)

/*******************************************************************************
 *                          Fee Integration
 ******************************************************************************/

/** @brief Include Fee header if Fee is used */
#define MEMIF_FEE_USED                      (STD_ON)

#if (MEMIF_FEE_USED == STD_ON)
    #include "Fee.h"
#endif

/*******************************************************************************
 *                          Ea Integration
 ******************************************************************************/

/** @brief Include Ea header if Ea is used */
#define MEMIF_EA_USED                       (STD_ON)

#if (MEMIF_EA_USED == STD_ON)
    #include "Ea.h"
#endif

/*******************************************************************************
 *                          RTE Integration
 ******************************************************************************/

/** @brief Enable RTE interface for service layer integration */
#define MEMIF_USE_RTE                       (STD_OFF)

/*******************************************************************************
 *                          API Configuration
 ******************************************************************************/

/** @brief Enable MemIf_Cancel API */
#define MEMIF_CANCEL_API                    (STD_ON)

/** @brief Enable MemIf_SetMode API */
#define MEMIF_SETMODE_API                   (STD_ON)

/** @brief Enable MemIf_GetStatus API */
#define MEMIF_GETSTATUS_API                 (STD_ON)

/** @brief Enable MemIf_GetJobResult API */
#define MEMIF_GETJOBRESULT_API              (STD_ON)

/** @brief Enable MemIf_InvalidateBlock API */
#define MEMIF_INVALIDATEBLOCK_API           (STD_ON)

/** @brief Enable MemIf_EraseImmediateBlock API */
#define MEMIF_ERASEIMMEDIATEBLOCK_API       (STD_ON)

/*******************************************************************************
 *                          Multi-Core Configuration
 ******************************************************************************/

/** @brief Multi-core support enable/disable */
#define MEMIF_MULTI_CORE_SUPPORT            (STD_OFF)

#if (MEMIF_MULTI_CORE_SUPPORT == STD_ON)
    /** @brief Core ID for Fee device */
    #define MEMIF_FEE_CORE_ID               (0u)
    
    /** @brief Core ID for Ea device */
    #define MEMIF_EA_CORE_ID                (0u)
#endif

/*******************************************************************************
 *                          Memory Mapping
 ******************************************************************************/

#ifdef MEMIF_START_SEC_CODE
    #define MEMIF_START_SEC_CODE
#endif

#ifdef MEMIF_STOP_SEC_CODE
    #define MEMIF_STOP_SEC_CODE
#endif

#ifdef MEMIF_START_SEC_CONFIG_DATA_8
    #define MEMIF_START_SEC_CONFIG_DATA_8
#endif

#ifdef MEMIF_STOP_SEC_CONFIG_DATA_8
    #define MEMIF_STOP_SEC_CONFIG_DATA_8
#endif

#ifdef MEMIF_START_SEC_VAR_CLEARED_8
    #define MEMIF_START_SEC_VAR_CLEARED_8
#endif

#ifdef MEMIF_STOP_SEC_VAR_CLEARED_8
    #define MEMIF_STOP_SEC_VAR_CLEARED_8
#endif

/*******************************************************************************
 *                          Default Configuration
 ******************************************************************************/

/* If using Fee, define default Fee device configuration */
#if (MEMIF_FEE_USED == STD_ON)
    #ifndef Fee_Read
        #define Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length) \
            Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length)
    #endif
    
    #ifndef Fee_Write
        #define Fee_Write(BlockNumber, DataBufferPtr) \
            Fee_Write(BlockNumber, DataBufferPtr)
    #endif
    
    #ifndef Fee_Cancel
        #define Fee_Cancel() Fee_Cancel()
    #endif
    
    #ifndef Fee_GetStatus
        #define Fee_GetStatus() Fee_GetStatus()
    #endif
    
    #ifndef Fee_GetJobResult
        #define Fee_GetJobResult() Fee_GetJobResult()
    #endif
#endif

/* If using Ea, define default Ea device configuration */
#if (MEMIF_EA_USED == STD_ON)
    #ifndef Ea_Read
        #define Ea_Read(BlockNumber, DataBufferPtr) \
            Ea_Read(BlockNumber, DataBufferPtr)
    #endif
    
    #ifndef Ea_Write
        #define Ea_Write(BlockNumber, DataBufferPtr) \
            Ea_Write(BlockNumber, DataBufferPtr)
    #endif
    
    #ifndef Ea_Cancel
        #define Ea_Cancel() Ea_Cancel()
    #endif
    
    #ifndef Ea_GetStatus
        #define Ea_GetStatus() Ea_GetStatus()
    #endif
    
    #ifndef Ea_GetJobResult
        #define Ea_GetJobResult() Ea_GetJobResult()
    #endif
#endif

#endif /* MEMIF_CFG_H */
