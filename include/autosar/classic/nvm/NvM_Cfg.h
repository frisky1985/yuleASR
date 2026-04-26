/**
 * @file NvM_Cfg.h
 * @brief AUTOSAR NvM Module Configuration Header
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - NvM Configuration (Module ID: 0x0E)
 * 
 * This file contains the configuration parameters for the NvM module.
 * These parameters should be adjusted based on the specific ECU requirements.
 * 
 * Key Configurations:
 * - Maximum 32 NV Blocks
 * - Write retry count: 3
 * - CRC type: CRC-32
 * - Write protection window: 5000ms
 * - Job queue size: 16
 * 
 * Copyright (c) 2025
 */

#ifndef NVM_CFG_H
#define NVM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 * Module Enable/Disable
 *============================================================================*/
#define NVM_DEV_ERROR_DETECT            STD_ON
#define NVM_VERSION_INFO_API            STD_ON
#define NVM_API_CONFIG_CLASS            NVM_API_CONFIG_CLASS_3
#define NVM_SET_RAM_BLOCK_STATUS_API    STD_ON
#define NVM_COMPILE_BLOCK_ID_CHECK      STD_ON

/*============================================================================*
 * Block Configuration
 *============================================================================*/
/**
 * @brief Maximum number of NV blocks
 * @range 1..65535
 * @default 32
 */
#define NVM_MAX_NUMBER_OF_BLOCKS        32u

/**
 * @brief Number of configured blocks (excluding reserved Block 0)
 * @note Block 0 is reserved for multi-block requests
 */
#define NVM_NUM_OF_CONFIGURED_BLOCKS    31u

/**
 * @brief Reserved block ID for multi-block operations
 */
#define NVM_MULTI_BLOCK_REQUEST_ID      0u

/*============================================================================*
 * CRC Configuration
 *============================================================================*/
/**
 * @brief Default CRC type for block validation
 * @options: NVM_CRC_NONE, NVM_CRC_8, NVM_CRC_16, NVM_CRC_32
 */
#define NVM_DEFAULT_CRC_TYPE            NVM_CRC_32

/**
 * @brief CRC-8 Polynomial (SAE J1850)
 */
#define NVM_CRC_8_POLYNOMIAL            0x1Du
#define NVM_CRC_8_INITIAL_VALUE         0xFFu

/**
 * @brief CRC-16 Polynomial (CCITT-FALSE)
 */
#define NVM_CRC_16_POLYNOMIAL           0x1021u
#define NVM_CRC_16_INITIAL_VALUE        0xFFFFu

/**
 * @brief CRC-32 Polynomial (IEEE 802.3)
 */
#define NVM_CRC_32_POLYNOMIAL           0xEDB88320u
#define NVM_CRC_32_INITIAL_VALUE        0xFFFFFFFFu
#define NVM_CRC_32_FINAL_XOR            0xFFFFFFFFu

/*============================================================================*
 * Write Retry Configuration
 *============================================================================*/
/**
 * @brief Maximum number of write retry attempts
 * @range 1..255
 * @default 3
 */
#define NVM_MAX_NUM_OF_WRITE_RETRIES    3u

/**
 * @brief Maximum number of read retry attempts
 * @range 1..255
 * @default 2
 */
#define NVM_MAX_NUM_OF_READ_RETRIES     2u

/*============================================================================*
 * Write Protection Configuration
 *============================================================================*/
/**
 * @brief Enable write protection API
 */
#define NVM_ENABLE_WRITE_PROTECTION     STD_ON

/**
 * @brief Write protection window in milliseconds
 * @details Time window during which write protection is enforced
 * @default 5000 (5 seconds)
 */
#define NVM_WRITE_PROTECTION_WINDOW_MS  5000u

/**
 * @brief Enable automatic write protection after write
 */
#define NVM_AUTO_WRITE_PROTECT          STD_OFF

/*============================================================================*
 * Queue Configuration
 *============================================================================*/
/**
 * @brief Size of the job queue
 * @range 1..255
 * @default 16
 */
#define NVM_SIZE_OF_JOB_QUEUE           16u

/**
 * @brief Maximum number of pending jobs per block
 */
#define NVM_MAX_PENDING_JOBS_PER_BLOCK  1u

/*============================================================================*
 * Main Function Timing
 *============================================================================*/
/**
 * @brief Main function cycle time in milliseconds
 * @default 10ms
 */
#define NVM_MAIN_FUNCTION_PERIOD_MS     10u

/**
 * @brief Timeout for MemIf operations in main function cycles
 * @default 100 cycles (1 second at 10ms period)
 */
#define NVM_MEMIF_TIMEOUT_CYCLES        100u

/*============================================================================*
 * Block ID Configuration
 *============================================================================*/
/**
 * @brief Module ID for block identification
 */
#define NVM_BLOCK_MODULE_ID             0x0Eu

/**
 * @brief Block ID offset for validation
 */
#define NVM_BLOCK_ID_OFFSET             0x1000u

/**
 * @brief Enable static block ID checking
 */
#define NVM_STATIC_BLOCK_ID_CHECK       STD_ON

/**
 * @brief Block ID magic number for validation
 */
#define NVM_BLOCK_ID_MAGIC              0xA5B6C7D8u

/*============================================================================*
 * Dataset Configuration
 *============================================================================*/
/**
 * @brief Maximum number of datasets per block
 */
#define NVM_MAX_NUMBER_OF_DATASETS      8u

/*============================================================================*
 * Memory Device Configuration
 *============================================================================*/
/**
 * @brief Number of configured memory devices
 */
#define NVM_NUM_OF_MEMORY_DEVICES       2u

/**
 * @brief Default memory device ID
 */
#define NVM_DEFAULT_MEMORY_DEVICE_ID    0u

/**
 * @brief Primary memory device ID
 */
#define NVM_PRIMARY_MEMORY_DEVICE       0u

/**
 * @brief Secondary/redundant memory device ID
 */
#define NVM_SECONDARY_MEMORY_DEVICE     1u

/*============================================================================*
 * Validation and Verification
 *============================================================================*/
/**
 * @brief Enable write verification (read-after-write)
 */
#define NVM_WRITE_VERIFICATION          STD_ON

/**
 * @brief Enable automatic data validation after read
 */
#define NVM_AUTO_VALIDATION             STD_ON

/**
 * @brief Enable ROM block restoration on read failure
 */
#define NVM_RESTORE_ROM_ON_FAILURE      STD_ON

/*============================================================================*
 * Callback Configuration
 *============================================================================*/
/**
 * @brief Enable multi-block job callback
 */
#define NVM_MULTI_BLOCK_CALLBACK_API    STD_ON

/**
 * @brief Enable block-specific callbacks
 */
#define NVM_BLOCK_CALLBACK_API          STD_ON

/*============================================================================*
 * Debug and Tracing
 *============================================================================*/
/**
 * @brief Enable job queue tracing
 */
#define NVM_TRACE_JOB_QUEUE             STD_OFF

/**
 * @brief Enable state machine tracing
 */
#define NVM_TRACE_STATE_MACHINE         STD_OFF

/*============================================================================*
 * Pre-compile Configuration Classes
 *============================================================================*/
#define NVM_API_CONFIG_CLASS_1          1u
#define NVM_API_CONFIG_CLASS_2          2u
#define NVM_API_CONFIG_CLASS_3          3u

/*============================================================================*
 * Type Definitions (based on configuration)
 *============================================================================*/
/**
 * @brief NvM Block ID Type
 */
typedef uint16_t NvM_BlockIdType;

/**
 * @brief Standard Return Type (from Std_Types.h)
 */
typedef uint8_t Std_ReturnType;

#ifndef E_OK
#define E_OK                            0x00u
#endif

#ifndef E_NOT_OK
#define E_NOT_OK                        0x01u
#endif

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

#ifndef boolean
typedef unsigned char boolean;
#endif

/**
 * @brief Standard Version Info Type
 */
typedef struct {
    uint16_t vendorID;
    uint16_t moduleID;
    uint8_t  sw_major_version;
    uint8_t  sw_minor_version;
    uint8_t  sw_patch_version;
} Std_VersionInfoType;

#ifdef __cplusplus
}
#endif

#endif /* NVM_CFG_H */
