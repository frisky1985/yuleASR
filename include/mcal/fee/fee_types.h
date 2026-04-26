/**
 * @file fee_types.h
 * @brief Fee (Flash EEPROM Emulation) Types Definition
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fee Module
 * Compliant with AUTOSAR R22-11 MCAL Specification
 */

#ifndef FEE_TYPES_H
#define FEE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Include Fls types */
#include "mcal/fls/fls_types.h"

/*============================================================================*
 * Version Information
 *============================================================================*/
#define FEE_MAJOR_VERSION           1u
#define FEE_MINOR_VERSION           0u
#define FEE_PATCH_VERSION           0u

#define FEE_MODULE_NAME             "Fee"
#define FEE_VENDOR_ID               0x00u
#define FEE_AR_MAJOR_VERSION        4u
#define FEE_AR_MINOR_VERSION        4u
#define FEE_AR_PATCH_VERSION        0u

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define FEE_MAX_BLOCKS              256u    /* Maximum number of logical blocks */
#define FEE_MAX_WRITE_QUEUE         32u     /* Maximum write queue size */
#define FEE_MAX_BLOCK_SIZE          4096u   /* Maximum block size in bytes */
#define FEE_MIN_BLOCK_SIZE          2u      /* Minimum block size in bytes */

/* Block header magic numbers */
#define FEE_BLOCK_HEADER_MAGIC      0xFEE0u  /* Valid block marker */
#define FEE_BLOCK_HEADER_ERASED     0xFFFFu /* Erased block marker */

/* Garbage collection thresholds */
#define FEE_GC_THRESHOLD_PERCENT    80u     /* Start GC when usage > 80% */
#define FEE_GC_EMERGENCY_PERCENT    95u     /* Emergency GC when usage > 95% */

/* Wear leveling thresholds */
#define FEE_WL_THRESHOLD            1000u   /* Max erase count difference */

/*============================================================================*
 * Error Codes (AUTOSAR Standard)
 *============================================================================*/
typedef enum {
    FEE_OK                      = 0x00u,    /* Operation successful */
    FEE_E_NOT_OK                = 0x01u,    /* General error */
    FEE_E_BUSY                  = 0x02u,    /* Fee busy */
    FEE_E_BUSY_INTERNAL         = 0x03u,    /* Internal operation ongoing */
    FEE_E_INVALID_BLOCK_NO      = 0x04u,    /* Invalid block number */
    FEE_E_INVALID_BLOCK_LEN     = 0x05u,    /* Invalid block length */
    FEE_E_PARAM_POINTER         = 0x06u,    /* NULL pointer parameter */
    FEE_E_PARAM_CONFIG          = 0x07u,    /* Invalid configuration */
    FEE_E_UNINIT                = 0x08u,    /* Module not initialized */
    FEE_E_ALREADY_INITIALIZED   = 0x09u,    /* Already initialized */
    FEE_E_GC_BUSY               = 0x0Au,    /* Garbage collection busy */
    FEE_E_WRITE_PROTECTED       = 0x0Bu,    /* Block write protected */
    FEE_E_READ_FAILED           = 0x0Cu,    /* Read operation failed */
    FEE_E_WRITE_FAILED          = 0x0Du,    /* Write operation failed */
    FEE_E_ERASE_FAILED          = 0x0Eu,    /* Erase operation failed */
    FEE_E_VERIFY_FAILED         = 0x0Fu,    /* Verify operation failed */
    FEE_E_INVALID_CLUSTER       = 0x10u,    /* Invalid cluster */
    FEE_E_GC_FAILED             = 0x11u,    /* Garbage collection failed */
    FEE_E_OUT_OF_MEMORY         = 0x12u,    /* Out of memory */
    FEE_E_TIMEOUT               = 0x13u,    /* Operation timeout */
    FEE_E_INCONSISTENT          = 0x14u     /* Data inconsistent */
} Fee_ErrorCode_t;

/*============================================================================*
 * Job Types
 *============================================================================*/
typedef enum {
    FEE_JOB_NONE                = 0x00u,
    FEE_JOB_READ                = 0x01u,
    FEE_JOB_WRITE               = 0x02u,
    FEE_JOB_ERASE               = 0x04u,
    FEE_JOB_GC                  = 0x08u,    /* Garbage collection */
    FEE_JOB_SWAP                = 0x10u,    /* Cluster swap */
    FEE_JOB_VERIFY              = 0x20u
} Fee_JobType_t;

/*============================================================================*
 * Job Status
 *============================================================================*/
typedef enum {
    FEE_JOB_IDLE                = 0x00u,
    FEE_JOB_PENDING             = 0x01u,
    FEE_JOB_IN_PROGRESS         = 0x02u,
    FEE_JOB_COMPLETED           = 0x03u,
    FEE_JOB_FAILED              = 0x04u,
    FEE_JOB_CANCELLED           = 0x05u
} Fee_JobStatus_t;

/*============================================================================*
 * Module Status
 *============================================================================*/
typedef enum {
    FEE_UNINIT                  = 0x00u,    /* Module not initialized */
    FEE_IDLE                    = 0x01u,    /* Module idle */
    FEE_BUSY                    = 0x02u,    /* Module busy */
    FEE_BUSY_INTERNAL           = 0x03u     /* Internal operation busy */
} Fee_ModuleStatus_t;

/*============================================================================*
 * Block Status
 *============================================================================*/
typedef enum {
    FEE_BLOCK_EMPTY             = 0x00u,    /* Block never written */
    FEE_BLOCK_VALID             = 0x01u,    /* Block contains valid data */
    FEE_BLOCK_INVALID           = 0x02u,    /* Block invalidated (old version) */
    FEE_BLOCK_CORRUPTED         = 0x03u,    /* Block data corrupted */
    FEE_BLOCK_INCONSISTENT      = 0x04u     /* Block inconsistent */
} Fee_BlockStatus_t;

/*============================================================================*
 * Cluster Status
 *============================================================================*/
typedef enum {
    FEE_CLUSTER_EMPTY           = 0x00u,    /* Cluster empty */
    FEE_CLUSTER_ACTIVE          = 0x01u,    /* Cluster in use */
    FEE_CLUSTER_FULL            = 0x02u,    /* Cluster full */
    FEE_CLUSTER_ERASING         = 0x03u,    /* Cluster being erased */
    FEE_CLUSTER_CORRUPTED       = 0x04u     /* Cluster corrupted */
} Fee_ClusterStatus_t;

/*============================================================================*
 * Block Header (Stored in Flash)
 *============================================================================*/
typedef struct {
    uint16_t magic;                         /* Magic number: FEE_BLOCK_HEADER_MAGIC */
    uint16_t blockNumber;                   /* Logical block number */
    uint16_t dataLength;                    /* Data length */
    uint16_t headerCrc;                     /* Header CRC */
    uint32_t sequence;                      /* Write sequence number */
    uint32_t dataCrc;                       /* Data CRC32 */
    uint32_t timestamp;                     /* Write timestamp */
} Fee_BlockHeader_t;

#define FEE_BLOCK_HEADER_SIZE       (sizeof(Fee_BlockHeader_t))
#define FEE_BLOCK_OVERHEAD          (FEE_BLOCK_HEADER_SIZE + 4u) /* Header + CRC */

/*============================================================================*
 * Logical Block Configuration
 *============================================================================*/
typedef struct {
    uint16_t blockNumber;                   /* Logical block number */
    uint16_t blockSize;                     /* Block size in bytes */
    bool immediateData;                     /* Immediate write required */
    uint32_t writeCycleCount;               /* Expected write cycles (wear level) */
    uint8_t deviceIndex;                    /* Flash device index */
} Fee_BlockConfigType;

/*============================================================================*
 * Cluster Configuration
 *============================================================================*/
typedef struct {
    uint32_t startAddress;                  /* Cluster start address in flash */
    uint32_t size;                          /* Cluster size */
    uint8_t deviceIndex;                    /* Flash device index */
    Fee_ClusterStatus_t status;             /* Cluster status */
    uint32_t usedSpace;                     /* Used space in bytes */
    uint32_t eraseCount;                    /* Erase cycle count */
} Fee_ClusterConfigType;

/*============================================================================*
 * Block Runtime Info
 *============================================================================*/
typedef struct {
    uint16_t blockNumber;                   /* Logical block number */
    uint16_t dataLength;                    /* Current data length */
    uint32_t flashAddress;                  /* Current flash address */
    uint32_t sequence;                      /* Current sequence number */
    Fee_BlockStatus_t status;               /* Block status */
    uint32_t writeCount;                    /* Write operation count */
    uint32_t readCount;                     /* Read operation count */
    bool writeProtected;                    /* Write protection flag */
} Fee_BlockInfoType;

/*============================================================================*
 * Job Structure
 *============================================================================*/
typedef struct {
    Fee_JobType_t jobType;                  /* Job type */
    uint16_t blockNumber;                   /* Target block number */
    uint16_t blockOffset;                   /* Offset within block */
    uint8_t* dataBuffer;                    /* Data buffer */
    uint16_t dataLength;                    /* Data length */
    Fee_JobStatus_t status;                 /* Job status */
    uint32_t priority;                      /* Job priority */
} Fee_JobType;

/*============================================================================*
 * Write Queue Entry
 *============================================================================*/
typedef struct {
    uint16_t blockNumber;
    uint8_t* data;
    uint16_t length;
    bool pending;
} Fee_WriteQueueEntry_t;

/*============================================================================*
 * General Configuration
 *============================================================================*/
typedef struct {
    uint32_t workingAddress;                /* Working area start address */
    uint32_t workingSize;                   /* Working area size */
    uint32_t gcThreshold;                   /* GC threshold percentage */
    uint32_t emergencyThreshold;            /* Emergency GC threshold */
    uint32_t maxGcDuration;                 /* Max GC duration (ms) */
    bool swapOnDemand;                      /* Swap cluster only when needed */
    bool gcDuringWrite;                     /* Allow GC during write */
    uint32_t jobCallCycle;                  /* Main function call cycle (ms) */
} Fee_GeneralConfigType;

/*============================================================================*
 * Runtime Configuration (PC Variant)
 *============================================================================*/
typedef struct {
    const Fee_GeneralConfigType* general;   /* General configuration */
    const Fee_BlockConfigType* blocks;      /* Block configuration array */
    const Fee_ClusterConfigType* clusters;  /* Cluster configuration array */
    uint32_t blockCount;                    /* Number of blocks */
    uint32_t clusterCount;                  /* Number of clusters */
} Fee_ConfigType;

/*============================================================================*
 * Cluster Runtime State
 *============================================================================*/
typedef struct {
    Fee_ClusterStatus_t status;             /* Current status */
    uint32_t usedSpace;                     /* Used space in bytes */
    uint32_t eraseCount;                    /* Erase cycle count */
} Fee_ClusterRuntimeStateType;

/*============================================================================*
 * Runtime State
 *============================================================================*/
typedef struct {
    Fee_ModuleStatus_t status;              /* Module status */
    const Fee_ConfigType* config;           /* Current configuration */
    Fee_BlockInfoType blockInfo[FEE_MAX_BLOCKS]; /* Block runtime info */
    Fee_ClusterRuntimeStateType clusterState[FEE_MAX_CLUSTERS]; /* Cluster runtime state */
    uint32_t activeCluster;                 /* Currently active cluster */
    uint32_t writePointer;                  /* Next write position */
    uint32_t sequenceCounter;               /* Global sequence counter */
    Fee_JobType currentJob;                 /* Current job */
    Fee_WriteQueueEntry_t writeQueue[FEE_MAX_WRITE_QUEUE]; /* Write queue */
    uint32_t writeQueueHead;                /* Queue head */
    uint32_t writeQueueTail;                /* Queue tail */
    uint32_t gcState;                       /* GC state machine */
    uint32_t gcSourceCluster;               /* GC source cluster */
    uint32_t gcTargetCluster;               /* GC target cluster */
    uint32_t gcBlockIndex;                  /* Current GC block index */
    uint32_t totalWrites;                   /* Total write count */
    uint32_t totalReads;                    /* Total read count */
    uint32_t totalErases;                   /* Total erase count */
    uint32_t gcCount;                       /* GC execution count */
    uint32_t errorCount;                    /* Total error count */
    bool initialized;                       /* Module initialized */
} Fee_RuntimeStateType;

/*============================================================================*
 * Notification Callbacks
 *============================================================================*/
typedef void (*Fee_JobEndNotification_t)(void);
typedef void (*Fee_JobErrorNotification_t)(void);
typedef void (*Fee_NotificationCallback_t)(
    Fee_JobType_t jobType,
    Fee_ErrorCode_t result,
    uint16_t blockNumber
);

#ifdef __cplusplus
}
#endif

#endif /* FEE_TYPES_H */
