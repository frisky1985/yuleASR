/**
 * @file RamTst.h
 * @brief RAM Test Driver - AUTOSAR MCAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details AUTOSAR RAM Test (RamTst) module interface.
 *          Provides memory test services including March-C/GALPAT/Walkpath
 *          algorithms for SRAM testing with DET error reporting.
 *
 * @implements AUTOSAR_SWS_RAMTest.pdf
 */

#ifndef RAMTST_H
#define RAMTST_H

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "RamTst_Cfg.h"

/*==================================================================================================
 *                                      VERSION INFORMATION
 *==================================================================================================*/
/** @brief RamTst Vendor ID (YuleTech) */
#define RAMTST_VENDOR_ID                   0x0055U

/** @brief RamTst Module ID */
#define RAMTST_MODULE_ID                   0x64U

/** @brief RamTst Software Version */
#define RAMTST_SW_MAJOR_VERSION            2U
#define RAMTST_SW_MINOR_VERSION            0U
#define RAMTST_SW_PATCH_VERSION            0U

/*==================================================================================================
 *                                      SERVICE IDs
 *==================================================================================================*/
#define RAMTST_SID_INIT                    0x01U
#define RAMTST_SID_DEINIT                  0x02U
#define RAMTST_SID_RUN                     0x03U
#define RAMTST_SID_STOP                    0x04U
#define RAMTST_SID_GET_RESULT              0x05U
#define RAMTST_SID_GET_STATUS              0x06U
#define RAMTST_SID_MAINFUNCTION            0x07U
#define RAMTST_SID_GET_VERSION_INFO        0x08U
#define RAMTST_SID_SET_MODE                0x09U
#define RAMTST_SID_GET_MODE                0x0AU
#define RAMTST_SID_CANCEL                  0x0BU

/*==================================================================================================
 *                                      ERROR CODES
 *==================================================================================================*/
/** @brief No error */
#define RAMTST_E_NO_ERROR                  0x00U
/** @brief Null pointer parameter */
#define RAMTST_E_PARAM_POINTER             0x01U
/** @brief Module not initialized */
#define RAMTST_E_UNINIT                    0x02U
/** @brief Test already running */
#define RAMTST_E_BUSY                      0x03U
/** @brief Invalid parameter */
#define RAMTST_E_PARAM_CONFIG              0x04U
/** @brief Invalid address range */
#define RAMTST_E_PARAM_ADDRESS             0x05U
/** @brief Invalid algorithm */
#define RAMTST_E_PARAM_ALGORITHM           0x06U
/** @brief Timeout during test */
#define RAMTST_E_TIMEOUT                   0x07U

/*==================================================================================================
 *                                      TYPE DEFINITIONS
 *==================================================================================================*/

/** @brief Supported RAM test algorithms */
typedef enum {
    RAMTST_ALGORITHM_MARCH_C   = 0x00U,   /**< March-C: detects stuck-at, transition, coupling faults */
    RAMTST_ALGORITHM_MARCH_C_MINUS = 0x01U,/**< March C-: optimized March-C */
    RAMTST_ALGORITHM_GALPAT    = 0x02U,   /**< GALPAT: galloping pattern test */
    RAMTST_ALGORITHM_WALKPATH  = 0x03U,   /**< Walkpath: walking 1s/0s pattern */
    RAMTST_ALGORITHM_CHECKERBOARD = 0x04U,/**< Checkerboard: alternating pattern */
    RAMTST_ALGORITHM_MARCH_13N = 0x05U    /**< March 13N: 13N March algorithm */
} RamTst_AlgType;

/** @brief Test result enumeration */
typedef enum {
    RAMTST_RESULT_NOT_TESTED   = 0x00U,   /**< No test performed */
    RAMTST_RESULT_OK           = 0x01U,   /**< Test passed, no errors */
    RAMTST_RESULT_FAILED       = 0x02U,   /**< Test failed, errors detected */
    RAMTST_RESULT_ABORTED      = 0x03U,   /**< Test was aborted */
    RAMTST_RESULT_TIMEOUT      = 0x04U    /**< Test timed out */
} RamTst_TestResultType;

/** @brief Test status enumeration */
typedef enum {
    RAMTST_STATUS_UNINIT       = 0x00U,   /**< Module not initialized */
    RAMTST_STATUS_IDLE         = 0x01U,   /**< Module idle, ready for test */
    RAMTST_STATUS_RUNNING      = 0x02U,   /**< Test in progress */
    RAMTST_STATUS_COMPLETED    = 0x03U,   /**< Test completed, result available */
    RAMTST_STATUS_ERROR        = 0x04U    /**< Module in error state */
} RamTst_StatusType;

/** @brief RAM test error record */
typedef struct {
    uint32 FailedAddress;                  /**< Address where error was detected */
    uint32 ExpectedValue;                  /**< Expected value at failed address */
    uint32 ActualValue;                    /**< Actual value read at failed address */
    uint8  BitMask;                        /**< Bit mask of failing bits */
    uint8  AlgorithmStep;                  /**< Algorithm step number at failure */
    uint16 ErrorCount;                     /**< Total error count */
} RamTst_ErrorRecordType;

/** @brief RAM test configuration structure */
typedef struct {
    uint32            StartAddress;        /**< Start address of RAM region to test */
    uint32            Size;                /**< Size of RAM region in bytes */
    RamTst_AlgType    Algorithm;           /**< Test algorithm to use */
    uint32            CallCycle;           /**< MainFunction call cycle in ms */
    uint32            TimeoutMs;           /**< Max test duration in ms */
    boolean           StopOnError;         /**< Stop on first error */
    uint32            PatternSeed;         /**< Seed for data pattern generation */
} RamTst_ConfigType;

/** @brief RamTst mode type */
typedef uint8 RamTst_ModeType;

/*==================================================================================================
 *                                      GLOBAL CONFIG POINTER
 *==================================================================================================*/
#define RAMTST_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/** @brief External reference to the default configuration */
extern const RamTst_ConfigType RamTst_Config;

#define RAMTST_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                   FUNCTION PROTOTYPES
 *==================================================================================================*/
#define RAMTST_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the RAM Test module
 *
 * @param[in] ConfigPtr Pointer to configuration structure
 *
 * @requirement RamTst-100: Shall initialize module state to IDLE
 * @requirement RamTst-110: Shall report DET error if ConfigPtr is NULL
 */
void RamTst_Init(const RamTst_ConfigType* ConfigPtr);

/**
 * @brief De-initializes the RAM Test module
 *
 * @requirement RamTst-200: Shall reset module to UNINIT state
 */
void RamTst_DeInit(void);

/**
 * @brief Starts a RAM test execution
 *
 * @return Std_ReturnType E_OK if test started, E_NOT_OK otherwise
 *
 * @requirement RamTst-300: Shall validate module state before starting
 */
Std_ReturnType RamTst_Run(void);

/**
 * @brief Stops an ongoing RAM test
 *
 * @requirement RamTst-400: Shall abort test and return to IDLE
 */
void RamTst_Stop(void);

/**
 * @brief Gets the current test result
 *
 * @return RamTst_TestResultType Current test result
 *
 * @requirement RamTst-500: Shall return last completed test result
 */
RamTst_TestResultType RamTst_GetTestResult(void);

/**
 * @brief Gets detailed error information from last test
 *
 * @param[out] ErrorRecord Pointer to store error details
 * @return Std_ReturnType E_OK if successful, E_NOT_OK otherwise
 *
 * @requirement RamTst-510: Shall provide detailed error info
 */
Std_ReturnType RamTst_GetErrorRecord(RamTst_ErrorRecordType* ErrorRecord);

/**
 * @brief Gets the current test status
 *
 * @return RamTst_StatusType Current module status
 *
 * @requirement RamTst-600: Shall return current module state
 */
RamTst_StatusType RamTst_GetTestStatus(void);

/**
 * @brief Periodic main function called by the OS/SchM
 *
 * @requirement RamTst-700: Shall execute test algorithm in non-blocking steps
 */
void RamTst_MainFunction(void);

/**
 * @brief Gets the version information of this module
 *
 * @param[out] versioninfo Pointer to version info structure
 *
 * @requirement RamTst-800: Shall populate version info
 * @note Only compiled when RAMTST_VERSION_INFO_API == STD_ON
 */
#if (RAMTST_VERSION_INFO_API == STD_ON)
void RamTst_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Sets the module operation mode
 *
 * @param[in] Mode Operating mode to set
 * @return Std_ReturnType E_OK if successful
 */
#if (RAMTST_SET_MODE_API == STD_ON)
Std_ReturnType RamTst_SetMode(RamTst_ModeType Mode);
#endif

/**
 * @brief Gets the current operation mode
 *
 * @return RamTst_ModeType Current operating mode
 */
#if (RAMTST_GET_MODE_API == STD_ON)
RamTst_ModeType RamTst_GetMode(void);
#endif

#define RAMTST_STOP_SEC_CODE
#include "MemMap.h"

#endif /* RAMTST_H */
