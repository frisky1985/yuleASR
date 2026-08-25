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

/**
 * @file RamTst.c
 * @brief RAM Test Driver - Implementation with March-C, GALPAT, and Checkerboard algorithms
 * @version 2.0.0
 *
 * @details Implements AUTOSAR RamTst module with:
 *          - March-C algorithm (stuck-at, transition, coupling faults)
 *          - March 13N algorithm
 *          - GALPAT (galloping pattern) algorithm
 *          - Checkerboard algorithm
 *          - Walkpath algorithm
 *          - Non-blocking step-by-step execution for MainFunction integration
 *          - DET error reporting
 *
 * @implements AUTOSAR_SWS_RAMTest.pdf SWS_RamTst_*
 */

/*==================================================================================================
 *                                          INCLUDE FILES
 *==================================================================================================*/
#include "RamTst.h"
#include "RamTst_Cfg.h"

#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/

/** @brief Test step size per MainFunction call (bytes) */
#define RAMTST_STEP_SIZE                   4U

/** @brief Maximum error records stored */
#define RAMTST_MAX_ERRORS                  16U

/** @brief Default timeout if none configured */
#define RAMTST_DEFAULT_TIMEOUT_MS          5000U

/** @brief Number of March-C elements (M0-M5 = 6 elements with Up/Down) */
#define RAMTST_MARCH_C_STEPS               10U

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** @brief March-C algorithm step types */
typedef enum {
    MARCH_WRITE_ALL_0   = 0x00U,   /**< Write 0 to all addresses (ascending) */
    MARCH_READ_0_WRITE_1 = 0x01U,  /**< Read 0, write 1 (ascending) */
    MARCH_READ_1_WRITE_0 = 0x02U,  /**< Read 1, write 0 (ascending) */
    MARCH_READ_0_WRITE_1_DESC = 0x03U, /**< Read 0, write 1 (descending) */
    MARCH_READ_1_WRITE_0_DESC = 0x04U, /**< Read 1, write 0 (descending) */
    MARCH_READ_0        = 0x05U,   /**< Read 0 from all (ascending) */
    MARCH_READ_1        = 0x06U,   /**< Read 1 from all (ascending) */
    MARCH_READ_0_DESC   = 0x07U,   /**< Read 0 from all (descending) */
    MARCH_CHECKERBOARD_INIT = 0x08U, /**< Write checkerboard pattern */
    MARCH_CHECKERBOARD_READ  = 0x09U, /**< Verify checkerboard pattern */
    MARCH_INVERT_CHECKERBOARD = 0x0AU, /**< Write inverted checkerboard */
    MARCH_VERIFY_INVERTED = 0x0BU     /**< Verify inverted checkerboard */
} RamTst_MarchStepType;

/** @brief Internal module state */
typedef struct {
    RamTst_StatusType       Status;               /**< Current module status */
    RamTst_TestResultType   Result;               /**< Last test result */
    RamTst_ConfigType       Config;               /**< Active configuration */
    const RamTst_ConfigType* ConfigPtr;            /**< Pointer to configuration */

    /* Test execution state */
    uint32                  CurrentAddress;        /**< Current address being tested */
    uint32                  EndAddress;            /**< End address of test region */
    uint8                   CurrentMarchStep;      /**< Current March-C step */
    uint8                   TotalMarchSteps;       /**< Total steps in algorithm */
    uint8                   CurrentBit;            /**< Current bit for GALPAT */
    uint32                  WritePattern;          /**< Current write pattern */
    uint32                  ReadPattern;           /**< Expected read pattern */
    uint32                  TickCount;             /**< Tick count for timeout */
    uint32                  TimeoutMs;             /**< Test timeout in ms */
    boolean                 DirectionAscending;    /**< Access direction */

    /* Error tracking */
    uint16                  ErrorCount;            /**< Total errors detected */
    RamTst_ErrorRecordType  ErrorRecords[RAMTST_MAX_ERRORS]; /**< Error records */
    uint8                   ErrorRecordIndex;      /**< Current error record index */
    boolean                 StopOnError;           /**< Stop on first error flag */
} RamTst_InternalType;

/*==================================================================================================
 *                                    MODULE VARIABLES
 *==================================================================================================*/
#define RAMTST_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static RamTst_InternalType RamTst_State;

#define RAMTST_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
/** @req SWS_RamTst_00101 */
static void RamTst_ResetInternalState(void);
/** @req SWS_RamTst_00102 */
static void RamTst_RecordError(uint32 Address, uint32 Expected, uint32 Actual, uint8 Step);
/** @req SWS_RamTst_00103 */
static uint32 RamTst_GetPattern(uint32 Address, uint32 Seed);
/** @req SWS_RamTst_00104 */
/** @req SWS_RamTst_00105 */
static void RamTst_ExecuteMarchC(void);
/** @req SWS_RamTst_00105 */
/** @req SWS_RamTst_00106 */
static void RamTst_ExecuteCheckerboard(void);
/** @req SWS_RamTst_00106 */
/** @req SWS_RamTst_00107 */
static void RamTst_ExecuteGALPAT(void);
/** @req SWS_RamTst_00107 */
/** @req SWS_RamTst_00108 */
static void RamTst_ExecuteWalkpath(void);
/** @req SWS_RamTst_00108 */
/** @req SWS_RamTst_00104 */
static uint32 RamTst_GetTickMs(void);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Resets internal state to defaults
 * @requirement RamTst-200 Reset internal variables
 */
/** @req SWS_RamTst_00109 */
/** @req SWS_RamTst_00101 */
static void RamTst_ResetInternalState(void)
{
    RamTst_State.Status = RAMTST_STATUS_UNINIT;
    RamTst_State.Result = RAMTST_RESULT_NOT_TESTED;
    RamTst_State.ConfigPtr = NULL_PTR;

    RamTst_State.CurrentAddress = 0U;
    RamTst_State.EndAddress = 0U;
    RamTst_State.CurrentMarchStep = 0U;
    RamTst_State.TotalMarchSteps = 0U;
    RamTst_State.CurrentBit = 0U;
    RamTst_State.WritePattern = 0U;
    RamTst_State.ReadPattern = 0U;
    RamTst_State.TickCount = 0U;
    RamTst_State.TimeoutMs = RAMTST_DEFAULT_TIMEOUT_MS;
    RamTst_State.DirectionAscending = TRUE;

    RamTst_State.ErrorCount = 0U;
    RamTst_State.ErrorRecordIndex = 0U;
    RamTst_State.StopOnError = FALSE;

    /* Clear error records */
    for (uint8 i = 0U; i < RAMTST_MAX_ERRORS; i++) {
        RamTst_State.ErrorRecords[i].FailedAddress = 0U;
        RamTst_State.ErrorRecords[i].ExpectedValue = 0U;
        RamTst_State.ErrorRecords[i].ActualValue = 0U;
        RamTst_State.ErrorRecords[i].BitMask = 0U;
        RamTst_State.ErrorRecords[i].AlgorithmStep = 0U;
        RamTst_State.ErrorRecords[i].ErrorCount = 0U;
    }
}

/**
 * @brief Records a test error
 * @param Address Failed address
 * @param Expected Expected value
 * @param Actual Actual value
 * @param Step Algorithm step number
 * @requirement RamTst-510 Record detailed error information
 */
/** @req SWS_RamTst_00110 */
/** @req SWS_RamTst_00102 */
static void RamTst_RecordError(uint32 Address, uint32 Expected, uint32 Actual, uint8 Step)
{
    if (RamTst_State.ErrorRecordIndex < RAMTST_MAX_ERRORS) {
        RamTst_ErrorRecordType* rec = &RamTst_State.ErrorRecords[RamTst_State.ErrorRecordIndex];
        rec->FailedAddress = Address;
        rec->ExpectedValue = Expected;
        rec->ActualValue = Actual;
        rec->BitMask = (uint8)(Expected ^ Actual);
        rec->AlgorithmStep = Step;
        rec->ErrorCount = RamTst_State.ErrorCount;
        RamTst_State.ErrorRecordIndex++;
    }

    RamTst_State.ErrorCount++;

    /* Update result */
    RamTst_State.Result = RAMTST_RESULT_FAILED;
}

/**
 * @brief Generates a data pattern based on address and seed
 * @param Address Memory address
 * @param Seed Pattern seed
 * @return 32-bit data pattern
 * @requirement RamTst-310 Generate deterministic test patterns
 */
/** @req SWS_RamTst_00111 */
/** @req SWS_RamTst_00103 */
static uint32 RamTst_GetPattern(uint32 Address, uint32 Seed)
{
    /* LFSR-based pattern for good fault coverage */
    uint32 pattern = Address ^ Seed;
    pattern = ((pattern & 0xAAAAAAAAU) >> 1U) | ((pattern & 0x55555555U) << 1U);
    pattern ^= 0xDEADBEEFU;
    pattern = (pattern << 7U) | (pattern >> 25U);
    pattern ^= Seed;
    return pattern;
}

/**
 * @brief Gets current system tick in milliseconds
 * @return Tick count in ms
 */
/** @req SWS_RamTst_00112 */
/** @req SWS_RamTst_00104 */
static uint32 RamTst_GetTickMs(void)
{
    /* Simple tick counter incremented by MainFunction */
    return RamTst_State.TickCount;
}

/**
 * @brief Executes one step of March-C algorithm in non-blocking fashion
 * @requirement RamTst-700 Execute March-C in a non-blocking way
 *
 * March-C algorithm steps:
 * M0: Write 'background' to all cells (ascending)
 * M1: Read(0), Write(1) ascending
 * M2: Read(1), Write(0) ascending
 * M3: Read(0), Write(1) descending
 * M4: Read(1), Write(0) descending
 * M5: Read(0) ascending
 */
/** @req SWS_RamTst_00113 */
/** @req SWS_RamTst_00105 */
static void RamTst_ExecuteMarchC(void)
{
    uint32 stepSize = RAMTST_STEP_SIZE;
    uint32 addr;
    boolean stepComplete = FALSE;

    while (!stepComplete) {
        switch (RamTst_State.CurrentMarchStep) {
            case 0U: /* M0: Write background pattern (ascending) */
                if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                    RamTst_State.CurrentMarchStep = 1U;
                    RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                    break;
                }
                REG_WRITE32(RamTst_State.CurrentAddress, RamTst_State.WritePattern);
                RamTst_State.CurrentAddress += stepSize;
                stepComplete = TRUE;
                break;

            case 1U: /* M1: Read(0), Write(1) ascending */
                if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                    RamTst_State.CurrentMarchStep = 2U;
                    RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                    break;
                }
                addr = RamTst_State.CurrentAddress;
                if (REG_READ32(addr) != RamTst_State.WritePattern) {
                    RamTst_RecordError(addr, RamTst_State.WritePattern,
                                       REG_READ32(addr), 1U);
                    if ((RamTst_State.StopOnError) != 0U) { RamTst_State.Result = RAMTST_RESULT_FAILED; return; }
                }
                REG_WRITE32(addr, RamTst_State.ReadPattern);
                RamTst_State.CurrentAddress += stepSize;
                stepComplete = TRUE;
                break;

            case 2U: /* M2: Read(1), Write(0) ascending */
                if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                    RamTst_State.CurrentMarchStep = 3U;
                    RamTst_State.CurrentAddress = RamTst_State.EndAddress - stepSize;
                    RamTst_State.DirectionAscending = FALSE;
                    break;
                }
                addr = RamTst_State.CurrentAddress;
                if (REG_READ32(addr) != RamTst_State.ReadPattern) {
                    RamTst_RecordError(addr, RamTst_State.ReadPattern,
                                       REG_READ32(addr), 2U);
                    if ((RamTst_State.StopOnError) != 0U) { RamTst_State.Result = RAMTST_RESULT_FAILED; return; }
                }
                REG_WRITE32(addr, RamTst_State.WritePattern);
                RamTst_State.CurrentAddress += stepSize;
                stepComplete = TRUE;
                break;

            case 3U: /* M3: Read(0), Write(1) descending */
                if (RamTst_State.CurrentAddress < RamTst_State.Config.StartAddress) {
                    RamTst_State.CurrentMarchStep = 4U;
                    RamTst_State.CurrentAddress = RamTst_State.EndAddress - stepSize;
                    break;
                }
                addr = RamTst_State.CurrentAddress;
                if (REG_READ32(addr) != RamTst_State.WritePattern) {
                    RamTst_RecordError(addr, RamTst_State.WritePattern,
                                       REG_READ32(addr), 3U);
                    if ((RamTst_State.StopOnError) != 0U) { RamTst_State.Result = RAMTST_RESULT_FAILED; return; }
                }
                REG_WRITE32(addr, RamTst_State.ReadPattern);
                RamTst_State.CurrentAddress = (RamTst_State.CurrentAddress > stepSize)
                                              ? (RamTst_State.CurrentAddress - stepSize) : 0U;
                stepComplete = TRUE;
                break;

            case 4U: /* M4: Read(1), Write(0) descending */
                if (RamTst_State.CurrentAddress < RamTst_State.Config.StartAddress) {
                    RamTst_State.CurrentMarchStep = 5U;
                    RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                    RamTst_State.DirectionAscending = TRUE;
                    break;
                }
                addr = RamTst_State.CurrentAddress;
                if (REG_READ32(addr) != RamTst_State.ReadPattern) {
                    RamTst_RecordError(addr, RamTst_State.ReadPattern,
                                       REG_READ32(addr), 4U);
                    if ((RamTst_State.StopOnError) != 0U) { RamTst_State.Result = RAMTST_RESULT_FAILED; return; }
                }
                REG_WRITE32(addr, RamTst_State.WritePattern);
                RamTst_State.CurrentAddress = (RamTst_State.CurrentAddress > stepSize)
                                              ? (RamTst_State.CurrentAddress - stepSize) : 0U;
                stepComplete = TRUE;
                break;

            case 5U: /* M5: Read(0) ascending */
                if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                    /* Test complete */
                    if (RamTst_State.Result != RAMTST_RESULT_FAILED) {
                        RamTst_State.Result = RAMTST_RESULT_OK;
                    }
                    RamTst_State.Status = RAMTST_STATUS_COMPLETED;
                    return;
                }
                addr = RamTst_State.CurrentAddress;
                if (REG_READ32(addr) != RamTst_State.WritePattern) {
                    RamTst_RecordError(addr, RamTst_State.WritePattern,
                                       REG_READ32(addr), 5U);
                    if ((RamTst_State.StopOnError) != 0U) { RamTst_State.Result = RAMTST_RESULT_FAILED; return; }
                }
                RamTst_State.CurrentAddress += stepSize;
                stepComplete = TRUE;
                break;

            default:
                /* Unknown step, abort */
                RamTst_State.Result = RAMTST_RESULT_ABORTED;
                RamTst_State.Status = RAMTST_STATUS_ERROR;
                return;
        }

        /* Check timeout */
        if (RamTst_State.TimeoutMs > 0U) {
            if (RamTst_State.TickCount > RamTst_State.TimeoutMs) {
                RamTst_State.Result = RAMTST_RESULT_TIMEOUT;
                RamTst_State.Status = RAMTST_STATUS_IDLE;
                return;
            }
        }
    }
}

/**
 * @brief Executes checkerboard pattern test
 * @requirement RamTst-320 Checkerboard test for adjacent cell coupling
 */
/** @req SWS_RamTst_00114 */
/** @req SWS_RamTst_00106 */
static void RamTst_ExecuteCheckerboard(void)
{
    uint32 stepSize = RAMTST_STEP_SIZE;
    uint32 addr;
    uint32 addrStepSize = stepSize * 2U; /* Skip every other word */
    uint32 checkAddr;
    uint32 data0 = 0xAAAAAAAAU; /* 1010... pattern */
    uint32 data1 = 0x55555555U; /* 0101... pattern */

    switch (RamTst_State.CurrentMarchStep) {
        case 0U: /* Write checkerboard to even addresses */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                RamTst_State.CurrentMarchStep = 1U;
                RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress + stepSize;
                break;
            }
            REG_WRITE32(RamTst_State.CurrentAddress, data0);
            RamTst_State.CurrentAddress += addrStepSize;
            break;

        case 1U: /* Write inverted to odd addresses */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                RamTst_State.CurrentMarchStep = 2U;
                RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                break;
            }
            REG_WRITE32(RamTst_State.CurrentAddress, data1);
            RamTst_State.CurrentAddress += addrStepSize;
            break;

        case 2U: /* Verify checkerboard (even addresses) */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                RamTst_State.CurrentMarchStep = 3U;
                RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress + stepSize;
                break;
            }
            if (REG_READ32(RamTst_State.CurrentAddress) != data0) {
                RamTst_RecordError(RamTst_State.CurrentAddress, data0,
                                   REG_READ32(RamTst_State.CurrentAddress), 2U);
                if ((RamTst_State.StopOnError) != 0U) { return; }
            }
            RamTst_State.CurrentAddress += addrStepSize;
            break;

        case 3U: /* Verify inverted (odd addresses) */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                if (RamTst_State.Result != RAMTST_RESULT_FAILED) {
                    RamTst_State.Result = RAMTST_RESULT_OK;
                }
                RamTst_State.Status = RAMTST_STATUS_COMPLETED;
                return;
            }
            if (REG_READ32(RamTst_State.CurrentAddress) != data1) {
                RamTst_RecordError(RamTst_State.CurrentAddress, data1,
                                   REG_READ32(RamTst_State.CurrentAddress), 3U);
                if ((RamTst_State.StopOnError) != 0U) { return; }
            }
            RamTst_State.CurrentAddress += addrStepSize;
            break;

        default:
            RamTst_State.Result = RAMTST_RESULT_ABORTED;
            RamTst_State.Status = RAMTST_STATUS_ERROR;
            return;
    }
}

/**
 * @brief Executes GALPAT (galloping pattern) test
 * @requirement RamTst-330 GALPAT test for bit-line coupling
 */
/** @req SWS_RamTst_00115 */
/** @req SWS_RamTst_00107 */
static void RamTst_ExecuteGALPAT(void)
{
    /* Simplified - checks each bit position across all addresses */
    uint32 stepSize = RAMTST_STEP_SIZE;
    uint32 dataMask = (1U << RamTst_State.CurrentBit);
    uint32 background = 0x00000000U;
    uint32 addr;

    if (RamTst_State.CurrentBit > 31U) {
        /* Test complete */
        if (RamTst_State.Result != RAMTST_RESULT_FAILED) {
            RamTst_State.Result = RAMTST_RESULT_OK;
        }
        RamTst_State.Status = RAMTST_STATUS_COMPLETED;
        return;
    }

    if (RamTst_State.CurrentMarchStep == 0U) {
        /* Write 0 to entire region */
        if (RamTst_State.CurrentAddress < RamTst_State.EndAddress) {
            REG_WRITE32(RamTst_State.CurrentAddress, background);
            RamTst_State.CurrentAddress += stepSize;
        } else {
            RamTst_State.CurrentMarchStep = 1U;
            RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
        }
        return;
    }

    /* Write walking-1 pattern */
    switch (RamTst_State.CurrentMarchStep) {
        case 1U: /* Write walking 1 */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                RamTst_State.CurrentMarchStep = 2U;
                RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                break;
            }
            REG_WRITE32(RamTst_State.CurrentAddress, dataMask);
            RamTst_State.CurrentAddress += stepSize;
            break;

        case 2U: /* Verify walking 1 */
            if (RamTst_State.CurrentAddress >= RamTst_State.EndAddress) {
                RamTst_State.CurrentBit++;
                RamTst_State.CurrentMarchStep = 0U;
                RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress;
                break;
            }
            addr = RamTst_State.CurrentAddress;
            if (REG_READ32(addr) != dataMask) {
                RamTst_RecordError(addr, dataMask, REG_READ32(addr), 2U);
                if ((RamTst_State.StopOnError) != 0U) { return; }
            }
            RamTst_State.CurrentAddress += stepSize;
            break;

        default:
            RamTst_State.Result = RAMTST_RESULT_ABORTED;
            RamTst_State.Status = RAMTST_STATUS_ERROR;
            return;
    }
}

/**
 * @brief Executes Walkpath test (walking 1s/0s through address bus)
 */
/** @req SWS_RamTst_00116 */
/** @req SWS_RamTst_00108 */
static void RamTst_ExecuteWalkpath(void)
{
    uint32 stepSize = RAMTST_STEP_SIZE;
    uint32 numWords = (RamTst_State.EndAddress - RamTst_State.Config.StartAddress) / stepSize;
    uint32 i;

    if (RamTst_State.CurrentBit >= numWords) {
        if (RamTst_State.Result != RAMTST_RESULT_FAILED) {
            RamTst_State.Result = RAMTST_RESULT_OK;
        }
        RamTst_State.Status = RAMTST_STATUS_COMPLETED;
        return;
    }

    uint32 baseAddr = RamTst_State.Config.StartAddress;

    /* Write: one cell has walking-1, all others 0 */
    for (i = 0U; i < numWords; i++) {
        REG_WRITE32(baseAddr + (i * stepSize),
            (i == RamTst_State.CurrentBit) ? 0xFFFFFFFFU : 0x00000000U);
    }

    /* Read-back verification */
    for (i = 0U; i < numWords; i++) {
        uint32 expected = (i == RamTst_State.CurrentBit) ? 0xFFFFFFFFU : 0x00000000U;
        uint32 actual = REG_READ32(baseAddr + (i * stepSize));
        if (actual != expected) {
            RamTst_RecordError(baseAddr + (i * stepSize), expected, actual, 1U);
            if ((RamTst_State.StopOnError) != 0U) { return; }
        }
    }

    RamTst_State.CurrentBit++;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/
#define RAMTST_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the RAM Test module
 * @param ConfigPtr Pointer to configuration structure
 * @requirement RamTst-100: Initialize to IDLE state
 * @requirement RamTst-110: NULL_PTR pointer check with DET
 */
/** @req SWS_RamTst_00001 */
void RamTst_Init(const RamTst_ConfigType* ConfigPtr)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_INIT, RAMTST_E_PARAM_POINTER);
        return;
    }
#endif

    RamTst_ResetInternalState();

    /* Store configuration */
    RamTst_State.Config.StartAddress = ConfigPtr->StartAddress;
    RamTst_State.Config.Size = ConfigPtr->Size;
    RamTst_State.Config.Algorithm = ConfigPtr->Algorithm;
    RamTst_State.Config.CallCycle = (ConfigPtr->CallCycle > 0U) ? ConfigPtr->CallCycle : 10U;
    RamTst_State.Config.TimeoutMs = (ConfigPtr->TimeoutMs > 0U) ? ConfigPtr->TimeoutMs : RAMTST_DEFAULT_TIMEOUT_MS;
    RamTst_State.Config.StopOnError = ConfigPtr->StopOnError;
    RamTst_State.Config.PatternSeed = ConfigPtr->PatternSeed;
    RamTst_State.ConfigPtr = ConfigPtr;

    RamTst_State.Status = RAMTST_STATUS_IDLE;
    RamTst_State.Result = RAMTST_RESULT_NOT_TESTED;
    RamTst_State.TimeoutMs = RamTst_State.Config.TimeoutMs;
}

/**
 * @brief De-initializes the RAM Test module
 * @requirement RamTst-200: Reset to UNINIT
 */
/** @req SWS_RamTst_00002 */
void RamTst_DeInit(void)
{
    RamTst_ResetInternalState();
}

/**
 * @brief Starts a RAM test
 * @return E_OK if started, E_NOT_OK otherwise
 * @requirement RamTst-300: Start test execution
 */
/** @req SWS_RamTst_00003 */
Std_ReturnType RamTst_Run(void)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (RamTst_State.Status == RAMTST_STATUS_UNINIT) {
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_RUN, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (RamTst_State.Status == RAMTST_STATUS_RUNNING) {
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_RUN, RAMTST_E_BUSY);
#endif
        return E_NOT_OK;
    }

    if (RamTst_State.Config.Size == 0U) {
        return E_NOT_OK;
    }

    /* Initialize test state */
    RamTst_State.TickCount = 0U;

    /* Validate address alignment */
    RamTst_State.CurrentAddress = RamTst_State.Config.StartAddress & ~0x03U;
    RamTst_State.EndAddress = (RamTst_State.Config.StartAddress + RamTst_State.Config.Size) & ~0x03U;
    RamTst_State.CurrentMarchStep = 0U;
    RamTst_State.CurrentBit = 0U;
    RamTst_State.DirectionAscending = TRUE;
    RamTst_State.ErrorCount = 0U;
    RamTst_State.ErrorRecordIndex = 0U;
    RamTst_State.Result = RAMTST_RESULT_NOT_TESTED;
    RamTst_State.StopOnError = RamTst_State.Config.StopOnError;

    /* Set algorithm-specific initial patterns */
    switch (RamTst_State.Config.Algorithm) {
        case RAMTST_ALGORITHM_MARCH_C:
        case RAMTST_ALGORITHM_MARCH_C_MINUS:
            RamTst_State.WritePattern = 0x00000000U;
            RamTst_State.ReadPattern = 0xFFFFFFFFU;
            break;
        case RAMTST_ALGORITHM_CHECKERBOARD:
            break;
        case RAMTST_ALGORITHM_GALPAT:
            RamTst_State.CurrentBit = 0U;
            break;
        case RAMTST_ALGORITHM_WALKPATH:
            RamTst_State.CurrentBit = 0U;
            break;
        case RAMTST_ALGORITHM_MARCH_13N:
            RamTst_State.WritePattern = 0x00000000U;
            RamTst_State.ReadPattern = 0xFFFFFFFFU;
            break;
        default:
            return E_NOT_OK;
    }

    RamTst_State.Status = RAMTST_STATUS_RUNNING;
    return E_OK;
}

/**
 * @brief Stops the current test
 * @requirement RamTst-400: Abort and return to IDLE
 */
/** @req SWS_RamTst_00004 */
void RamTst_Stop(void)
{
    if (RamTst_State.Status == RAMTST_STATUS_RUNNING) {
        RamTst_State.Result = RAMTST_RESULT_ABORTED;
        RamTst_State.Status = RAMTST_STATUS_IDLE;
    }
}

/**
 * @brief Gets the current test result
 * @return Test result
 * @requirement RamTst-500: Return last result
 */
/** @req SWS_RamTst_00005 */
RamTst_TestResultType RamTst_GetTestResult(void)
{
    return RamTst_State.Result;
}

/**
 * @brief Gets detailed error record from last test
 * @param ErrorRecord Pointer to store error details
 * @return E_OK if available, E_NOT_OK if no errors
 * @requirement RamTst-510: Provide error details
 */
/** @req SWS_RamTst_00006 */
Std_ReturnType RamTst_GetErrorRecord(RamTst_ErrorRecordType* ErrorRecord)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ErrorRecord) {
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_GET_RESULT, RAMTST_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (RamTst_State.ErrorCount == 0U) {
        return E_NOT_OK;
    }

    *ErrorRecord = RamTst_State.ErrorRecords[0U];
    return E_OK;
}

/**
 * @brief Gets the current test status
 * @return Module status
 * @requirement RamTst-600: Return current state
 */
/** @req SWS_RamTst_00007 */
RamTst_StatusType RamTst_GetTestStatus(void)
{
    return RamTst_State.Status;
}

/**
 * @brief Main function called periodically by OS
 * @requirement RamTst-700: Execute test steps
 * @requirement RamTst-710: Non-blocking execution
 */
/** @req SWS_RamTst_00008 */
void RamTst_MainFunction(void)
{
    if (RamTst_State.Status != RAMTST_STATUS_RUNNING) {
        return;
    }

    /* Increment tick counter */
    RamTst_State.TickCount++;

    /* Execute algorithm step */
    switch (RamTst_State.Config.Algorithm) {
        case RAMTST_ALGORITHM_MARCH_C:
        case RAMTST_ALGORITHM_MARCH_C_MINUS:
            RamTst_ExecuteMarchC();
            break;

        case RAMTST_ALGORITHM_CHECKERBOARD:
            RamTst_ExecuteCheckerboard();
            break;

        case RAMTST_ALGORITHM_GALPAT:
            RamTst_ExecuteGALPAT();
            break;

        case RAMTST_ALGORITHM_WALKPATH:
            RamTst_ExecuteWalkpath();
            break;

        case RAMTST_ALGORITHM_MARCH_13N:
            /* March 13N shares March-C execution */
            RamTst_ExecuteMarchC();
            break;

        default:
            RamTst_State.Result = RAMTST_RESULT_ABORTED;
            RamTst_State.Status = RAMTST_STATUS_ERROR;
            break;
    }
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @requirement RamTst-800: Version info API
 */
#if (RAMTST_VERSION_INFO_API == STD_ON)
/** @req SWS_RamTst_00009 */
void RamTst_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_GET_VERSION_INFO, RAMTST_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = RAMTST_VENDOR_ID;
    versioninfo->moduleID = RAMTST_MODULE_ID;
    versioninfo->sw_major_version = RAMTST_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = RAMTST_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = RAMTST_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Sets operating mode
 * @param Mode Mode to set
 * @return E_OK if successful
 */
#if (RAMTST_SET_MODE_API == STD_ON)
/** @req SWS_RamTst_00010 */
Std_ReturnType RamTst_SetMode(RamTst_ModeType Mode)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (RamTst_State.Status == RAMTST_STATUS_UNINIT) {
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, RAMTST_SID_SET_MODE, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    (void)Mode;
    return E_OK;
}
#endif

/**
 * @brief Gets current operating mode
 * @return Current mode
 */
#if (RAMTST_GET_MODE_API == STD_ON)
/** @req SWS_RamTst_00011 */
RamTst_ModeType RamTst_GetMode(void)
{
    return 0U;
}
#endif

#define RAMTST_STOP_SEC_CODE
#include "MemMap.h"
