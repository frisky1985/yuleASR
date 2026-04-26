/**
 * @file fls_types.h
 * @brief Fls (Flash Driver) Types Definition
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fls Module - Flash EEPROM Emulation Support
 * Compliant with AUTOSAR R22-11 MCAL Specification
 */

#ifndef FLS_TYPES_H
#define FLS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*
 * Version Information
 *============================================================================*/
#define FLS_MAJOR_VERSION           1u
#define FLS_MINOR_VERSION           0u
#define FLS_PATCH_VERSION           0u

#define FLS_MODULE_NAME             "Fls"
#define FLS_VENDOR_ID               0x00u
#define FLS_AR_MAJOR_VERSION        4u
#define FLS_AR_MINOR_VERSION        4u
#define FLS_AR_PATCH_VERSION        0u

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define FLS_MAX_SECTORS             256u    /* Maximum flash sectors */
#define FLS_MAX_PAGES               4096u   /* Maximum flash pages */
#define FLS_MAX_PAGE_SIZE           256u    /* Maximum page size in bytes */
#define FLS_MAX_SECTOR_SIZE         65536u  /* Maximum sector size in bytes */
#define FLS_MAX_JOB_QUEUE           16u     /* Maximum async job queue size */

/* Flash programming/erase timing */
#define FLS_PROG_TIME_US            100u    /* Typical page program time (us) */
#define FLS_ERASE_TIME_MS           50u     /* Typical sector erase time (ms) */
#define FLS_MAX_ERASE_TIME_MS       500u    /* Maximum erase time (ms) */

/*============================================================================*
 * Error Codes (AUTOSAR Standard)
 *============================================================================*/
typedef enum {
    FLS_OK                      = 0x00u,    /* Operation successful */
    FLS_E_NOT_OK                = 0x01u,    /* General error */
    FLS_E_BUSY                  = 0x02u,    /* Flash busy */
    FLS_E_TIMEOUT               = 0x03u,    /* Operation timeout */
    FLS_E_VERIFY                = 0x04u,    /* Verify error */
    FLS_E_WRITE_PROTECTED       = 0x05u,    /* Write protection error */
    FLS_E_INVALID_ADDRESS       = 0x06u,    /* Invalid address */
    FLS_E_INVALID_LENGTH        = 0x07u,    /* Invalid length */
    FLS_E_INVALID_DATA          = 0x08u,    /* Invalid data */
    FLS_E_UNINIT                = 0x09u,    /* Module not initialized */
    FLS_E_ALREADY_INITIALIZED   = 0x0Au,    /* Already initialized */
    FLS_E_PARAM_POINTER         = 0x0Bu,    /* NULL pointer parameter */
    FLS_E_PARAM_CONFIG          = 0x0Cu,    /* Invalid configuration */
    FLS_E_ERASE_FAILED          = 0x0Du,    /* Erase operation failed */
    FLS_E_WRITE_FAILED          = 0x0Eu,    /* Write operation failed */
    FLS_E_READ_FAILED           = 0x0Fu,    /* Read operation failed */
    FLS_E_COMPARE_FAILED        = 0x10u,    /* Compare operation failed */
    FLS_E_JOB_PENDING           = 0x11u,    /* Job still pending */
    FLS_E_JOB_FAILED            = 0x12u,    /* Job failed */
    FLS_E_ECC_ERROR             = 0x13u,    /* ECC error detected */
    FLS_E_HW_ERROR              = 0x14u     /* Hardware error */
} Fls_ErrorCode_t;

/*============================================================================*
 * Job Types
 *============================================================================*/
typedef enum {
    FLS_JOB_NONE                = 0x00u,
    FLS_JOB_READ                = 0x01u,
    FLS_JOB_WRITE               = 0x02u,
    FLS_JOB_ERASE               = 0x04u,
    FLS_JOB_COMPARE             = 0x08u,
    FLS_JOB_BLANK_CHECK         = 0x10u,
    FLS_JOB_VERIFY              = 0x20u
} Fls_JobType_t;

/*============================================================================*
 * Job Status
 *============================================================================*/
typedef enum {
    FLS_JOB_IDLE                = 0x00u,    /* No job running */
    FLS_JOB_PENDING             = 0x01u,    /* Job pending */
    FLS_JOB_IN_PROGRESS         = 0x02u,    /* Job in progress */
    FLS_JOB_COMPLETED           = 0x03u,    /* Job completed successfully */
    FLS_JOB_FAILED              = 0x04u,    /* Job failed */
    FLS_JOB_CANCELLED           = 0x05u     /* Job cancelled */
} Fls_JobStatus_t;

/*============================================================================*
 * Module Status
 *============================================================================*/
typedef enum {
    FLS_UNINIT                  = 0x00u,    /* Module not initialized */
    FLS_IDLE                    = 0x01u,    /* Module initialized, idle */
    FLS_BUSY                    = 0x02u     /* Module busy with operation */
} Fls_ModuleStatus_t;

/*============================================================================*
 * Protection Type
 *============================================================================*/
typedef enum {
    FLS_PROTECTION_NONE         = 0x00u,    /* No protection */
    FLS_PROTECTION_WRITE        = 0x01u,    /* Write protected */
    FLS_PROTECTION_ERASE        = 0x02u,    /* Erase protected */
    FLS_PROTECTION_READ         = 0x04u,    /* Read protected */
    FLS_PROTECTION_ALL          = 0x07u     /* Full protection */
} Fls_ProtectionType_t;

/*============================================================================*
 * Hardware Type
 *============================================================================*/
typedef enum {
    FLS_HW_GENERIC              = 0x00u,    /* Generic/Emulator */
    FLS_HW_AURIX_TC3XX          = 0x01u,    /* Infineon Aurix TC3xx */
    FLS_HW_S32G3                = 0x02u,    /* NXP S32G3 */
    FLS_HW_STM32H7              = 0x03u,    /* STM32 H7 series */
    FLS_HW_CUSTOM               = 0xFFu     /* Custom hardware */
} Fls_HardwareType_t;

/*============================================================================*
 * Sector Configuration
 *============================================================================*/
typedef struct {
    uint32_t sectorIndex;                   /* Sector index */
    uint32_t startAddress;                  /* Sector start address */
    uint32_t size;                          /* Sector size */
    uint32_t pageSize;                      /* Page size within sector */
    Fls_ProtectionType_t protection;        /* Protection type */
    bool eraseAuto;                         /* Auto erase before write */
    uint32_t eraseCount;                    /* Erase cycle count (wear level) */
} Fls_SectorConfigType;

/*============================================================================*
 * General Configuration
 *============================================================================*/
typedef struct {
    uint32_t baseAddress;                   /* Flash base address */
    uint32_t totalSize;                     /* Total flash size */
    uint32_t defaultSectorSize;             /* Default sector size */
    uint32_t defaultPageSize;               /* Default page size */
    uint32_t maxReadMode;                   /* Max read fast mode */
    uint32_t maxWriteMode;                  /* Max write fast mode */
    uint32_t jobCallCycle;                  /* Job processing cycle (ms) */
    bool loadPageOnDemand;                  /* Load page on demand */
    bool jobEndNotificationEnabled;         /* Job end notification enabled */
    bool jobErrorNotificationEnabled;       /* Job error notification enabled */
} Fls_GeneralConfigType;

/*============================================================================*
 * Runtime Configuration (PC Variant)
 *============================================================================*/
typedef struct {
    const Fls_GeneralConfigType* general;   /* General configuration */
    const Fls_SectorConfigType* sectors;    /* Sector configuration array */
    uint32_t sectorCount;                   /* Number of sectors */
    Fls_HardwareType_t hwType;              /* Hardware type */
    void* hwConfig;                         /* Hardware-specific config */
} Fls_ConfigType;

/*============================================================================*
 * Job Structure
 *============================================================================*/
typedef struct {
    Fls_JobType_t jobType;                  /* Job type */
    uint32_t address;                       /* Target address */
    uint8_t* dataBuffer;                    /* Data buffer */
    uint32_t length;                        /* Data length */
    uint32_t processed;                     /* Bytes processed */
    Fls_JobStatus_t status;                 /* Job status */
    uint32_t timestamp;                     /* Job start timestamp */
    uint32_t timeout;                       /* Job timeout */
} Fls_JobType;

/*============================================================================*
 * Callback Types
 *============================================================================*/
typedef void (*Fls_JobEndNotification_t)(void);
typedef void (*Fls_JobErrorNotification_t)(void);
typedef void (*Fls_NotificationCallback_t)(
    Fls_JobType_t jobType,
    Fls_ErrorCode_t result,
    uint32_t address,
    uint32_t length
);

/*============================================================================*
 * Hardware Interface (Abstract Layer)
 *============================================================================*/
typedef struct {
    /* Initialize hardware */
    Fls_ErrorCode_t (*Init)(const Fls_ConfigType* config);

    /* Deinitialize hardware */
    Fls_ErrorCode_t (*Deinit)(void);

    /* Read data from flash */
    Fls_ErrorCode_t (*Read)(
        uint32_t address,
        uint8_t* data,
        uint32_t length
    );

    /* Write data to flash */
    Fls_ErrorCode_t (*Write)(
        uint32_t address,
        const uint8_t* data,
        uint32_t length
    );

    /* Erase sector */
    Fls_ErrorCode_t (*Erase)(
        uint32_t sectorIndex,
        uint32_t sectorCount
    );

    /* Compare flash data with buffer */
    Fls_ErrorCode_t (*Compare)(
        uint32_t address,
        const uint8_t* data,
        uint32_t length
    );

    /* Blank check */
    Fls_ErrorCode_t (*BlankCheck)(
        uint32_t address,
        uint32_t length
    );

    /* Get hardware status */
    Fls_ModuleStatus_t (*GetStatus)(void);

    /* Check if flash is busy */
    bool (*IsBusy)(void);

    /* Get sector index from address */
    uint32_t (*GetSectorIndex)(uint32_t address);

    /* Get sector size */
    uint32_t (*GetSectorSize)(uint32_t sectorIndex);

    /* Get page size */
    uint32_t (*GetPageSize)(uint32_t sectorIndex);

    /* Set protection */
    Fls_ErrorCode_t (*SetProtection)(
        uint32_t sectorIndex,
        Fls_ProtectionType_t protection
    );

    /* Get erase count (wear level) */
    uint32_t (*GetEraseCount)(uint32_t sectorIndex);

    /* Lock/Unlock sector for writing */
    Fls_ErrorCode_t (*Lock)(uint32_t sectorIndex);
    Fls_ErrorCode_t (*Unlock)(uint32_t sectorIndex);
} Fls_HwInterfaceType;

/*============================================================================*
 * Runtime State
 *============================================================================*/
typedef struct {
    Fls_ModuleStatus_t status;              /* Module status */
    const Fls_ConfigType* config;           /* Current configuration */
    const Fls_HwInterfaceType* hwInterface; /* Hardware interface */
    Fls_JobType currentJob;                 /* Current job */
    uint32_t jobQueueHead;                  /* Job queue head */
    uint32_t jobQueueTail;                  /* Job queue tail */
    uint32_t jobQueueCount;                 /* Job queue count */
    Fls_JobType jobQueue[FLS_MAX_JOB_QUEUE]; /* Job queue */
    uint32_t operationStartTime;            /* Operation start timestamp */
    uint32_t totalReadCount;                /* Total read operations */
    uint32_t totalWriteCount;               /* Total write operations */
    uint32_t totalEraseCount;               /* Total erase operations */
    uint32_t errorCount;                    /* Total error count */
    bool initialized;                       /* Module initialized flag */
    Fls_NotificationCallback_t notificationCb; /* Notification callback */
} Fls_RuntimeStateType;

#ifdef __cplusplus
}
#endif

#endif /* FLS_TYPES_H */
