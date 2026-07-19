/**
 * @file Eep.h
 * @brief EEPROM Driver - AUTOSAR MCAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details AUTOSAR EEPROM (Eep) module interface for emulated EEPROM
 *          using Flash or RAM backing store. Supports asynchronous read/write/erase
 *          operations with DET error reporting.
 *
 * @implements AUTOSAR_SWS_EEPROMDriver.pdf
 */

#ifndef EEP_H
#define EEP_H

/*==================================================================================================
 *                                          INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Eep_Cfg.h"

/*==================================================================================================
 *                                      VERSION INFORMATION
 *==================================================================================================*/
/** @brief Eep Vendor ID (YuleTech) */
#define EEP_VENDOR_ID                       0x0055U

/** @brief Eep Module ID */
#define EEP_MODULE_ID                       0x5FU

/** @brief Eep Software Version */
#define EEP_SW_MAJOR_VERSION                2U
#define EEP_SW_MINOR_VERSION                0U
#define EEP_SW_PATCH_VERSION                0U

/*==================================================================================================
 *                                      SERVICE IDs
 *==================================================================================================*/
#define EEP_SID_INIT                        0x01U
#define EEP_SID_DEINIT                      0x02U
#define EEP_SID_READ                        0x03U
#define EEP_SID_WRITE                       0x04U
#define EEP_SID_ERASE                       0x05U
#define EEP_SID_CANCEL                      0x06U
#define EEP_SID_GET_STATUS                  0x07U
#define EEP_SID_GET_JOB_RESULT              0x08U
#define EEP_SID_MAIN_FUNCTION               0x09U
#define EEP_SID_GET_VERSION_INFO            0x0AU
#define EEP_SID_SET_MODE                    0x0BU
#define EEP_SID_READ_EXTENDED               0x0CU
#define EEP_SID_WRITE_EXTENDED              0x0DU
#define EEP_SID_ERASE_IMMEDIATE             0x0EU

/*==================================================================================================
 *                                      ERROR CODES
 *==================================================================================================*/
/** @brief No error */
#define EEP_E_NO_ERROR                      0x00U
/** @brief Null pointer parameter */
#define EEP_E_PARAM_POINTER                 0x01U
/** @brief Invalid address parameter */
#define EEP_E_PARAM_ADDRESS                 0x02U
/** @brief Invalid length parameter */
#define EEP_E_PARAM_LENGTH                  0x03U
/** @brief Module not initialized */
#define EEP_E_UNINIT                        0x04U
/** @brief Module busy */
#define EEP_E_BUSY                          0x05U
/** @brief Write protection violation */
#define EEP_E_WRITE_PROTECTED               0x06U
/** @brief Compare failed */
#define EEP_E_COMPARE_FAILED                0x07U
/** @brief Erase failed */
#define EEP_E_ERASE_FAILED                  0x08U
/** @brief Timeout */
#define EEP_E_TIMEOUT                       0x09U
/** @brief Configuration invalid */
#define EEP_E_PARAM_CONFIG                  0x0AU

/*==================================================================================================
 *                                      TYPE DEFINITIONS
 *==================================================================================================*/

/** @brief EEPROM address type */
typedef uint32 Eep_AddressType;

/** @brief EEPROM length type */
typedef uint32 Eep_LengthType;

/** @brief EEPROM job result */
typedef enum {
    EEP_JOB_OK       = 0x00U,    /**< Job completed successfully */
    EEP_JOB_PENDING  = 0x01U,    /**< Job in progress */
    EEP_JOB_FAILED   = 0x02U,    /**< Job failed */
    EEP_JOB_CANCELED = 0x03U     /**< Job cancelled */
} Eep_JobResultType;

/** @brief EEPROM module status */
typedef enum {
    EEP_UNINIT = 0x00U,          /**< Module not initialized */
    EEP_IDLE   = 0x01U,          /**< Module idle, ready for operations */
    EEP_BUSY   = 0x02U           /**< Module busy with an operation */
} Eep_StatusType;

/** @brief EEPROM mode type */
typedef uint8 Eep_ModeType;

/** @brief EEPROM configuration structure */
typedef struct {
    Eep_AddressType BaseAddress;  /**< Base address of EEPROM region */
    Eep_LengthType  Size;         /**< Total EEPROM size in bytes */
    uint32          JobCallCycle; /**< MainFunction call cycle in ms */
    uint8           PageSize;     /**< Page size in bytes for write/erase */
    uint32          WriteCycleTimeMs; /**< Write cycle time in ms */
    uint32          EraseCycleTimeMs; /**< Erase cycle time in ms */
    boolean         PollingMode;  /**< TRUE = polling, FALSE = interrupt */
} Eep_ConfigType;

/*==================================================================================================
 *                                   FUNCTION PROTOTYPES
 *==================================================================================================*/
#define EEP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the EEPROM Driver module
 * @param ConfigPtr Pointer to configuration structure
 * @requirement Eep-100: Initialize to IDLE state
 * @requirement Eep-110: NULL pointer check with DET
 */
void Eep_Init(const Eep_ConfigType* ConfigPtr);

/**
 * @brief De-initializes the EEPROM Driver module
 * @requirement Eep-200: Reset to UNINIT state
 */
void Eep_DeInit(void);

/**
 * @brief Reads data from EEPROM (asynchronous)
 * @param Address Start address for read
 * @param DataPtr Pointer to data buffer
 * @param Length Number of bytes to read
 * @return E_OK if accepted, E_NOT_OK on error
 * @requirement Eep-300: Asynchronous read operation
 */
Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length);

/**
 * @brief Writes data to EEPROM (asynchronous)
 * @param Address Start address for write
 * @param DataPtr Pointer to data buffer
 * @param Length Number of bytes to write
 * @return E_OK if accepted, E_NOT_OK on error
 * @requirement Eep-400: Asynchronous write operation
 */
Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length);

/**
 * @brief Erases EEPROM region (asynchronous)
 * @param Address Start address for erase
 * @param Length Number of bytes to erase
 * @return E_OK if accepted, E_NOT_OK on error
 * @requirement Eep-500: Asynchronous erase operation
 */
Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length);

/**
 * @brief Cancels current operation
 * @requirement Eep-600: Cancel current job
 */
#if (EEP_CANCEL_API == STD_ON)
void Eep_Cancel(void);
#endif

/**
 * @brief Gets module status
 * @return Current status
 * @requirement Eep-700: Return module state
 */
Eep_StatusType Eep_GetStatus(void);

/**
 * @brief Gets last job result
 * @return Last job result
 * @requirement Eep-800: Return job completion status
 */
Eep_JobResultType Eep_GetJobResult(void);

/**
 * @brief Periodic main function
 * @requirement Eep-900: Process pending operations
 */
void Eep_MainFunction(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @requirement Eep-1000: Version info API
 */
#if (EEP_VERSION_INFO_API == STD_ON)
void Eep_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define EEP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* EEP_H */
