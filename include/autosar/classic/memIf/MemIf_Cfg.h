/**
 * @file MemIf_Cfg.h
 * @brief AUTOSAR MemIf Module Configuration Header
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - MemIf Configuration (Module ID: 0x0F)
 * 
 * This file contains the configuration parameters for the MemIf module.
 * 
 * Key Configurations:
 * - Support for 2 memory devices
 * - Device abstraction layer
 * - Asynchronous operation support
 * 
 * Copyright (c) 2025
 */

#ifndef MEMIF_CFG_H
#define MEMIF_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 * Module Enable/Disable
 *============================================================================*/
#define MEMIF_VERSION_INFO_API          STD_ON
#define MEMIF_DEV_ERROR_DETECT          STD_ON

/*============================================================================*
 * Number of Devices Configuration
 *============================================================================*/
/**
 * @brief Number of configured memory devices
 * @range 1..2
 * @default 2
 */
#define MEMIF_NUMBER_OF_DEVICES         2u

/**
 * @brief Maximum number of devices supported
 */
#define MEMIF_MAX_DEVICES               2u

/*============================================================================*
 * Device Configuration
 *============================================================================*/
/**
 * @brief Primary device index
 */
#define MEMIF_PRIMARY_DEVICE            0u

/**
 * @brief Secondary device index (for redundancy)
 */
#define MEMIF_SECONDARY_DEVICE          1u

/**
 * @brief Device 0 enabled
 */
#define MEMIF_DEVICE_0_ENABLED          STD_ON

/**
 * @brief Device 0 block count
 */
#define MEMIF_DEVICE_0_BLOCK_COUNT      256u

/**
 * @brief Device 0 block size
 */
#define MEMIF_DEVICE_0_BLOCK_SIZE       64u

/**
 * @brief Device 0 type (FEE/EA)
 */
#define MEMIF_DEVICE_0_TYPE_FEE         STD_ON
#define MEMIF_DEVICE_0_TYPE_EA          STD_OFF

/**
 * @brief Device 1 enabled
 */
#define MEMIF_DEVICE_1_ENABLED          STD_ON

/**
 * @brief Device 1 block count
 */
#define MEMIF_DEVICE_1_BLOCK_COUNT      256u

/**
 * @brief Device 1 block size
 */
#define MEMIF_DEVICE_1_BLOCK_SIZE       64u

/**
 * @brief Device 1 type (FEE/EA)
 */
#define MEMIF_DEVICE_1_TYPE_FEE         STD_ON
#define MEMIF_DEVICE_1_TYPE_EA          STD_OFF

/*============================================================================*
 * Address Mapping Configuration
 *============================================================================*/
/**
 * @brief Block number offset for device 0
 */
#define MEMIF_DEVICE_0_BLOCK_OFFSET     0u

/**
 * @brief Block number offset for device 1
 */
#define MEMIF_DEVICE_1_BLOCK_OFFSET     0u

/**
 * @brief Maximum block number supported
 */
#define MEMIF_MAX_BLOCK_NUMBER          65535u

/*============================================================================*
 * Operation Configuration
 *============================================================================*/
/**
 * @brief Enable erase operation
 */
#define MEMIF_ERASE_ENABLED             STD_ON

/**
 * @brief Enable invalidate operation
 */
#define MEMIF_INVALIDATE_ENABLED        STD_ON

/**
 * @brief Enable cancel operation
 */
#define MEMIF_CANCEL_ENABLED            STD_ON

/*============================================================================*
 * Timing Configuration
 *============================================================================*/
/**
 * @brief Main function period in milliseconds
 */
#define MEMIF_MAIN_FUNCTION_PERIOD_MS   10u

/**
 * @brief Maximum job processing time (timeout) in ms
 */
#define MEMIF_JOB_TIMEOUT_MS            1000u

/*============================================================================*
 * Standard Types (redefined for self-containment)
 *============================================================================*/
#ifndef STD_ON
#define STD_ON                          0x01u
#endif

#ifndef STD_OFF
#define STD_OFF                         0x00u
#endif

#ifndef TRUE
#define TRUE                            1u
#endif

#ifndef FALSE
#define FALSE                           0u
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

typedef unsigned char boolean;

typedef uint8_t Std_ReturnType;

/*============================================================================*
 * Device Driver Callback Types
 *============================================================================*/
/**
 * @brief Function pointer type for device initialization
 */
typedef void (*MemIf_InitFnPtrType)(void);

/**
 * @brief Function pointer type for setting device mode
 */
typedef void (*MemIf_SetModeFnPtrType)(uint8_t Mode);

/**
 * @brief Function pointer type for device read
 */
typedef Std_ReturnType (*MemIf_ReadFnPtrType)(
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataBufferPtr,
    uint16_t Length
);

/**
 * @brief Function pointer type for device write
 */
typedef Std_ReturnType (*MemIf_WriteFnPtrType)(
    uint16_t BlockNumber,
    uint8_t* DataBufferPtr
);

/**
 * @brief Function pointer type for cancel operation
 */
typedef void (*MemIf_CancelFnPtrType)(void);

/**
 * @brief Function pointer type for getting status
 */
typedef uint8_t (*MemIf_GetStatusFnPtrType)(void);

/**
 * @brief Function pointer type for getting job result
 */
typedef uint8_t (*MemIf_GetJobResultFnPtrType)(void);

/**
 * @brief Function pointer type for invalidate block
 */
typedef Std_ReturnType (*MemIf_InvalidateFnPtrType)(uint16_t BlockNumber);

/**
 * @brief Function pointer type for erase block
 */
typedef Std_ReturnType (*MemIf_EraseFnPtrType)(uint16_t BlockNumber);

/**
 * @brief Function pointer type for device main function
 */
typedef void (*MemIf_MainFunctionFnPtrType)(void);

/*============================================================================*
 * Device Driver Structure
 *============================================================================*/
/**
 * @brief Device driver function table
 */
typedef struct {
    MemIf_InitFnPtrType         Init;
    MemIf_SetModeFnPtrType      SetMode;
    MemIf_ReadFnPtrType         Read;
    MemIf_WriteFnPtrType        Write;
    MemIf_CancelFnPtrType       Cancel;
    MemIf_GetStatusFnPtrType    GetStatus;
    MemIf_GetJobResultFnPtrType GetJobResult;
    MemIf_InvalidateFnPtrType   Invalidate;
    MemIf_EraseFnPtrType        Erase;
    MemIf_MainFunctionFnPtrType MainFunction;
    boolean                     IsFee;          /*!< TRUE=FEE, FALSE=EA */
    uint16_t                    BlockOffset;    /*!< Block number offset */
    uint16_t                    NumOfBlocks;    /*!< Number of blocks */
    boolean                     Enabled;        /*!< Device enabled flag */
} MemIf_DeviceDriverType;

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const MemIf_DeviceDriverType MemIf_DeviceDrivers[MEMIF_NUMBER_OF_DEVICES];

#ifdef __cplusplus
}
#endif

#endif /* MEMIF_CFG_H */
