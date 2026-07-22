/*==================================================================================================
 *                                      FLASH DRIVER CONFIGURATION
 *==================================================================================================
 * FILENAME: Fls_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Pre-compile configuration for Flash Driver module
 *==================================================================================================
 */

#ifndef FLS_CFG_H
#define FLS_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    PRE-COMPILE OPTIONS
 *==================================================================================================*/

/**
 * @brief Enable/disable version info API
 */
#define FLS_VERSION_INFO_API            (STD_ON)

/**
 * @brief Enable/disable development error detection
 */
#define FLS_DEV_ERROR_DETECT            (STD_ON)

/**
 * @brief Enable/disable runtime error detection
 */
#define FLS_RUNTIME_ERROR_DETECT        (STD_ON)

/**
 * @brief Enable/disable job end notification
 */
#define FLS_JOB_END_NOTIFICATION        (STD_ON)

/**
 * @brief Enable/disable job error notification
 */
#define FLS_JOB_ERROR_NOTIFICATION      (STD_ON)

/**
 * @brief Enable/disable interrupt service routine usage
 * @note If STD_OFF, Fls_MainFunction must be called cyclically
 */
#define FLS_USE_ISR                     (STD_OFF)

/**
 * @brief Enable/disable synchronous read API
 */
#define FLS_READ_SYNC_API               (STD_OFF)

/**
 * @brief Enable/disable compare functionality
 */
#define FLS_COMPARE_API                 (STD_ON)

/**
 * @brief Enable/disable cancel functionality
 */
#define FLS_CANCEL_API                  (STD_ON)

/**
 * @brief Enable/disable set mode functionality
 */
#define FLS_SET_MODE_API                (STD_ON)

/**
 * @brief Total number of configured sectors
 */
#define FLS_NUM_OF_SECTORS              (4u)

/**
 * @brief Total flash size in bytes
 */
#define FLS_TOTAL_SIZE                  (0x00100000u)  /* 1 MB */

/**
 * @brief Flash base address
 */
#define FLS_BASE_ADDRESS                (0x08000000u)

/**
 * @brief Sector 0 configuration
 */
#define FLS_SECTOR_0_START_ADDR         (0x08000000u)
#define FLS_SECTOR_0_SIZE               (0x00010000u)  /* 64 KB */
#define FLS_SECTOR_0_PAGE_SIZE          (4u)           /* 4 bytes per write */

/**
 * @brief Sector 1 configuration
 */
#define FLS_SECTOR_1_START_ADDR         (0x08010000u)
#define FLS_SECTOR_1_SIZE               (0x00010000u)  /* 64 KB */
#define FLS_SECTOR_1_PAGE_SIZE          (4u)

/**
 * @brief Sector 2 configuration
 */
#define FLS_SECTOR_2_START_ADDR         (0x08020000u)
#define FLS_SECTOR_2_SIZE               (0x00020000u)  /* 128 KB */
#define FLS_SECTOR_2_PAGE_SIZE          (4u)

/**
 * @brief Sector 3 configuration
 */
#define FLS_SECTOR_3_START_ADDR         (0x08040000u)
#define FLS_SECTOR_3_SIZE               (0x000C0000u)  /* 768 KB */
#define FLS_SECTOR_3_PAGE_SIZE          (4u)

/**
 * @brief Maximum bytes to read in normal mode per main function cycle
 */
#define FLS_MAX_READ_NORMAL_MODE        (256u)

/**
 * @brief Maximum bytes to read in fast mode per main function cycle
 */
#define FLS_MAX_READ_FAST_MODE          (512u)

/**
 * @brief Maximum bytes to write in normal mode per main function cycle
 */
#define FLS_MAX_WRITE_NORMAL_MODE       (32u)

/**
 * @brief Maximum bytes to write in fast mode per main function cycle
 */
#define FLS_MAX_WRITE_FAST_MODE         (64u)

/**
 * @brief Call cycle of Fls_MainFunction in milliseconds
 */
#define FLS_MAIN_FUNCTION_PERIOD        (10u)

/**
 * @brief Timeout for flash operations in milliseconds
 */
#define FLS_TIMEOUT_VALUE               (1000u)

/*==================================================================================================
 *                                    CALLBACK DECLARATIONS
 *==================================================================================================*/
#if defined(FLS_JOB_END_NOTIFICATION) && (FLS_JOB_END_NOTIFICATION == STD_ON)
    extern void Fls_JobEndNotification(void);
#endif

#if defined(FLS_JOB_ERROR_NOTIFICATION) && (FLS_JOB_ERROR_NOTIFICATION == STD_ON)
    extern void Fls_JobErrorNotification(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLS_CFG_H */
