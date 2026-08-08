/******************************************************************************
 * @file    dcm_ecu_reset.h
 * @brief   DCM ECU Reset Service (0x11) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_ECU_RESET_H
#define DCM_ECU_RESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * ECU Reset Subfunctions (ISO 14229-1:2020 Table 42)
 ******************************************************************************/
#define DCM_RESET_SUBFUNC_HARD_RESET                0x01U
#define DCM_RESET_SUBFUNC_KEY_OFF_ON_RESET          0x02U
#define DCM_RESET_SUBFUNC_SOFT_RESET                0x03U
#define DCM_RESET_SUBFUNC_ENABLE_RAPID_SHUTDOWN     0x04U
#define DCM_RESET_SUBFUNC_DISABLE_RAPID_SHUTDOWN    0x05U

/******************************************************************************
 * ECU Reset Response Sizes
 ******************************************************************************/
#define DCM_ECU_RESET_RESPONSE_MIN_SIZE             0x02U
#define DCM_ECU_RESET_RESPONSE_FULL_SIZE            0x03U  /* With powerDownTime */

/******************************************************************************
 * ECU Reset Configuration Constants
 ******************************************************************************/
#define DCM_ECU_RESET_POWER_DOWN_TIME_MAX           0xFEU  /* Max 254 seconds */
#define DCM_ECU_RESET_POWER_DOWN_TIME_NOT_AVAILABLE 0xFFU

/******************************************************************************
 * ECU Reset Result Types
 ******************************************************************************/
typedef enum {
    DCM_ECU_RESET_OK = 0,                   /* Reset accepted */
    DCM_ECU_RESET_ERR_NOT_SUPPORTED,        /* Reset type not supported */
    DCM_ECU_RESET_ERR_SECURITY_DENIED,      /* Security access denied */
    DCM_ECU_RESET_ERR_CONDITIONS_NOT_CORRECT, /* Conditions not correct */
    DCM_ECU_RESET_ERR_IN_PROGRESS,          /* Reset already in progress */
    DCM_ECU_RESET_ERR_FAILED                /* Reset preparation failed */
} Dcm_EcuResetResultType;

/******************************************************************************
 * ECU Reset State Types
 ******************************************************************************/
typedef enum {
    DCM_ECU_RESET_STATE_IDLE = 0,           /* No reset pending */
    DCM_ECU_RESET_STATE_PENDING,            /* Reset pending (response sent) */
    DCM_ECU_RESET_STATE_EXECUTING           /* Reset executing */
} Dcm_EcuResetStateType;

/******************************************************************************
 * ECU Reset Configuration
 ******************************************************************************/

typedef struct {
    bool hardResetSupported;
    bool keyOffOnResetSupported;
    bool softResetSupported;
    bool rapidShutdownSupported;
    uint8_t powerDownTimeSeconds;           /* Estimated power down time */
    uint32_t resetDelayMs;                  /* Delay before reset execution */
    bool requireProgrammingSession;         /* Require programming session for hard reset */
    bool requireExtendedSession;            /* Require extended session for soft reset */
    Dcm_EcuResetCallback resetCallback;     /* Reset notification callback */
} Dcm_EcuResetConfigType;

/******************************************************************************
 * ECU Reset Status Types
 ******************************************************************************/
typedef struct {
    Dcm_EcuResetStateType state;            /* Current reset state */
    Dcm_ResetType pendingResetType;         /* Pending reset type */
    uint32_t resetTimer;                    /* Reset delay timer */
    uint8_t powerDownTime;                  /* Power down time estimate */
    uint16_t requestingTesterAddress;       /* Tester that requested reset */
    uint64_t resetRequestTime;              /* Time of reset request */
    uint32_t resetCount;                    /* Total reset count */
    uint32_t resetByType[6];                /* Reset count by type */
} Dcm_EcuResetStatusType;

/******************************************************************************
 * ECU Reset Functions
 ******************************************************************************/

/**
 * @brief Initialize ECU reset service
 * 
 * @param config Pointer to ECU reset configuration
 * @return Dcm_ReturnType Initialization result
 * 
 * @note Must be called before using ECU reset service
 * @requirement ISO 14229-1:2020 10.3
 */
Dcm_ReturnType Dcm_EcuResetInit(const Dcm_EcuResetConfigType *config);

/**
 * @brief Process ECUReset (0x11) service request
 * 
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 * 
 * @details Implements UDS service 0x11 for ECU reset control
 * @requirement ISO 14229-1:2020 10.3
 */
Dcm_ReturnType Dcm_EcuReset(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Check if ECU reset is supported
 * 
 * @param resetType Reset type to check
 * @return bool True if reset type is supported
 */
bool Dcm_IsEcuResetSupported(Dcm_ResetType resetType);

/**
 * @brief Check if reset conditions are valid
 * 
 * @param resetType Reset type to check
 * @return Dcm_ReturnType Result of condition check
 * 
 * @details Validates that current session and conditions allow reset
 */
Dcm_ReturnType Dcm_CheckResetConditions(Dcm_ResetType resetType);

/**
 * @brief Execute ECU reset (called after positive response is sent)
 * 
 * @return Dcm_ReturnType Result of reset execution
 * 
 * @warning This function should trigger the actual hardware reset
 * @requirement ISO 14229-1:2020 10.3.4
 */
Dcm_ReturnType Dcm_ExecuteEcuReset(void);

/**
 * @brief Get current ECU reset state
 * 
 * @return Dcm_EcuResetStateType Current reset state
 */
Dcm_EcuResetStateType Dcm_GetEcuResetState(void);

/**
 * @brief Get pending reset type
 * 
 * @return Dcm_ResetType Pending reset type, or 0 if none pending
 */
Dcm_ResetType Dcm_GetPendingResetType(void);

/**
 * @brief Cancel pending ECU reset
 * 
 * @return Dcm_ReturnType Result of cancellation
 * 
 * @note Can be used to cancel a reset before it executes
 */
Dcm_ReturnType Dcm_CancelEcuReset(void);

/**
 * @brief Get ECU reset status information
 * 
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetEcuResetStatus(Dcm_EcuResetStatusType *status);

/**
 * @brief Update ECU reset timer (call periodically from main function)
 * 
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 * @return Dcm_ReturnType Result of operation
 * 
 * @note Should be called from Dcm_MainFunction or equivalent
 */
Dcm_ReturnType Dcm_EcuResetTimerUpdate(uint32_t elapsedTimeMs);

/**
 * @brief Set power down time estimate
 * 
 * @param seconds Power down time in seconds (0-254, 255 = not available)
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_SetPowerDownTime(uint8_t seconds);

/**
 * @brief Get power down time estimate
 * 
 * @return uint8_t Power down time in seconds
 */
uint8_t Dcm_GetPowerDownTime(void);

/**
 * @brief Reset statistics
 * 
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ResetEcuResetStatistics(void);

/******************************************************************************
 * Platform-Specific Reset Functions (to be implemented by platform layer)
 ******************************************************************************/

/**
 * @brief Platform-specific hard reset implementation
 * 
 * @warning This function should not return
 */
void Dcm_Platform_HardReset(void);

/**
 * @brief Platform-specific soft reset implementation
 * 
 * @warning This function should not return
 */
void Dcm_Platform_SoftReset(void);

/**
 * @brief Platform-specific key off/on reset implementation
 * 
 * @warning This function should not return
 */
void Dcm_Platform_KeyOffOnReset(void);

/**
 * @brief Platform-specific rapid shutdown implementation
 * 
 * @param enable True to enable, false to disable
 */
void Dcm_Platform_SetRapidShutdown(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* DCM_ECU_RESET_H */
