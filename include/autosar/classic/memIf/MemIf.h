/**
 * @file MemIf.h
 * @brief AUTOSAR MemIf (Memory Interface) Module Header
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - MemIf Module (Module ID: 0x0F)
 * 
 * The Memory Interface (MemIf) module provides uniform access to 
 * memory devices (EEPROM, Flash EEPROM Emulation) for the NvM module.
 * 
 * Key Features:
 * - Device abstraction layer for 1-2 memory devices
 * - Read/Write/Erase interface forwarding
 * - Device selection logic based on block number
 * - Asynchronous operation support
 * - Status polling interface
 * 
 * Copyright (c) 2025
 */

#ifndef MEMIF_H
#define MEMIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 * Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "MemIf_Cfg.h"

/*============================================================================*
 * Module Version Information
 *============================================================================*/
#define MEMIF_MODULE_ID                 0x0Fu
#define MEMIF_VENDOR_ID                 0x00u
#define MEMIF_INSTANCE_ID               0x00u

#define MEMIF_AR_RELEASE_MAJOR_VERSION  4u
#define MEMIF_AR_RELEASE_MINOR_VERSION  4u
#define MEMIF_AR_RELEASE_REVISION_VERSION 0u

#define MEMIF_SW_MAJOR_VERSION          1u
#define MEMIF_SW_MINOR_VERSION          0u
#define MEMIF_SW_PATCH_VERSION          0u

/*============================================================================*
 * Service IDs for Error Reporting
 *============================================================================*/
#define MEMIF_SID_READ                  0x01u
#define MEMIF_SID_WRITE                 0x02u
#define MEMIF_SID_ERASE                 0x03u
#define MEMIF_SID_CANCEL                0x04u
#define MEMIF_SID_GET_STATUS            0x05u
#define MEMIF_SID_GET_JOB_RESULT        0x06u
#define MEMIF_SID_INVALIDATE            0x07u

/*============================================================================*
 * Error Codes
 *============================================================================*/
#define MEMIF_E_NO_ERROR                0x00u
#define MEMIF_E_PARAM_DEVICE            0x01u
#define MEMIF_E_PARAM_POINTER           0x02u
#define MEMIF_E_BUSY                    0x03u
#define MEMIF_E_INVALID_BLOCK           0x04u

/*============================================================================*
 * Type Definitions
 *============================================================================*/

/**
 * @brief MemIf Status Type
 * @details Represents the current status of a memory device
 */
typedef enum {
    MEMIF_UNINIT = 0,               /*!< Module uninitialized */
    MEMIF_IDLE = 1,                 /*!< Device idle, ready for operation */
    MEMIF_BUSY = 2,                 /*!< Device busy with operation */
    MEMIF_BUSY_INTERNAL = 3         /*!< Device busy with internal operation */
} MemIf_StatusType;

/**
 * @brief MemIf Job Result Type
 * @details Result of the last asynchronous job
 */
typedef enum {
    MEMIF_JOB_OK = 0,               /*!< Job completed successfully */
    MEMIF_JOB_FAILED = 1,           /*!< Job failed */
    MEMIF_JOB_PENDING = 2,          /*!< Job still pending */
    MEMIF_JOB_CANCELED = 3,         /*!< Job canceled */
    MEMIF_BLOCK_INCONSISTENT = 4,   /*!< Block inconsistent */
    MEMIF_BLOCK_INVALID = 5         /*!< Block invalid */
} MemIf_JobResultType;

/**
 * @brief MemIf Device Mode Type
 */
typedef enum {
    MEMIF_MODE_SLOW = 0,            /*!< Slow mode (low power) */
    MEMIF_MODE_FAST = 1             /*!< Fast mode (normal operation) */
} MemIf_ModeType;

/*============================================================================*
 * Public API Functions
 *============================================================================*/

/**
 * @brief Initializes the MemIf module
 * @details Initializes all configured memory device drivers
 * @pre None
 * @post Module and underlying devices initialized
 */
extern void MemIf_Init(void);

/**
 * @brief Sets the mode for the specified device
 * @param DeviceIndex Index of the memory device (0 or 1)
 * @param Mode Operating mode (SLOW or FAST)
 * @return None
 */
extern void MemIf_SetMode(
    uint8_t DeviceIndex,
    MemIf_ModeType Mode
);

/**
 * @brief Reads data from the memory device
 * @param DeviceIndex Index of the memory device
 * @param BlockNumber Block number to read from
 * @param BlockOffset Offset within the block
 * @param DataBufferPtr Pointer to destination buffer
 * @param Length Number of bytes to read
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 * @note Operation is asynchronous
 */
extern Std_ReturnType MemIf_Read(
    uint8_t DeviceIndex,
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataBufferPtr,
    uint16_t Length
);

/**
 * @brief Writes data to the memory device
 * @param DeviceIndex Index of the memory device
 * @param BlockNumber Block number to write to
 * @param DataBufferPtr Pointer to source buffer
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 * @note Operation is asynchronous
 */
extern Std_ReturnType MemIf_Write(
    uint8_t DeviceIndex,
    uint16_t BlockNumber,
    uint8_t* DataBufferPtr
);

/**
 * @brief Cancels an ongoing operation
 * @param DeviceIndex Index of the memory device
 * @return None
 */
extern void MemIf_Cancel(uint8_t DeviceIndex);

/**
 * @brief Gets the current status of a memory device
 * @param DeviceIndex Index of the memory device
 * @return Current status (UNINIT, IDLE, BUSY, BUSY_INTERNAL)
 */
extern MemIf_StatusType MemIf_GetStatus(uint8_t DeviceIndex);

/**
 * @brief Gets the result of the last job
 * @param DeviceIndex Index of the memory device
 * @return Job result (OK, FAILED, PENDING, etc.)
 */
extern MemIf_JobResultType MemIf_GetJobResult(uint8_t DeviceIndex);

/**
 * @brief Invalidates a block
 * @param DeviceIndex Index of the memory device
 * @param BlockNumber Block number to invalidate
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType MemIf_Invalidate(
    uint8_t DeviceIndex,
    uint16_t BlockNumber
);

/**
 * @brief Erases a block or sector
 * @param DeviceIndex Index of the memory device
 * @param BlockNumber Block number to erase
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType MemIf_Erase(
    uint8_t DeviceIndex,
    uint16_t BlockNumber
);

/**
 * @brief Main function for MemIf
 * @details Called cyclically to process pending operations
 * @note Call period should match the fastest underlying device
 */
extern void MemIf_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* MEMIF_H */
