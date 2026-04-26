/******************************************************************************
 * @file    dcm_routine.h
 * @brief   DCM Routine Control Service (0x31) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_ROUTINE_H
#define DCM_ROUTINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Routine Control Constants (ISO 14229-1:2020 Table 138)
 ******************************************************************************/
#define DCM_ROUTINE_CTRL_MIN_LENGTH             0x04U
#define DCM_ROUTINE_CTRL_RESPONSE_MIN_LENGTH    0x04U
#define DCM_ROUTINE_ID_LENGTH                   0x02U
#define DCM_MAX_ROUTINE_STATUS_RECORD           0xFFU
#define DCM_MAX_ROUTINE_INFO_RECORD             0xFFU

/******************************************************************************
 * Routine Control Types (ISO 14229-1:2020 Table 137)
 ******************************************************************************/
#define DCM_ROUTINE_CTRL_START                  0x01U
#define DCM_ROUTINE_CTRL_STOP                   0x02U
#define DCM_ROUTINE_CTRL_REQUEST_RESULTS        0x03U

/******************************************************************************
 * Predefined Routine Identifiers (ISO 14229-1:2020 Annex G)
 ******************************************************************************/
#define DCM_ROUTINE_ID_ERASE_MEMORY             0xFF00U
#define DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES  0xFF01U
#define DCM_ROUTINE_ID_ERASE_MIRROR_MEMORY      0xFF02U
#define DCM_ROUTINE_ID_SELF_TEST                0x0203U
#define DCM_ROUTINE_ID_LEARN_VIN                0xF100U
#define DCM_ROUTINE_ID_RESET_PARAM              0xF101U
#define DCM_ROUTINE_ID_ADJUST_PARAM             0xF102U

/******************************************************************************
 * Routine Execution States
 ******************************************************************************/
typedef enum {
    DCM_ROUTINE_STATE_IDLE = 0,             /* Routine not started */
    DCM_ROUTINE_STATE_RUNNING,              /* Routine executing */
    DCM_ROUTINE_STATE_COMPLETED,            /* Routine completed successfully */
    DCM_ROUTINE_STATE_STOPPED,              /* Routine stopped by request */
    DCM_ROUTINE_STATE_FAILED                /* Routine failed */
} Dcm_RoutineStateType;

/******************************************************************************
 * Routine Execution Results
 ******************************************************************************/
typedef enum {
    DCM_ROUTINE_RESULT_OK = 0,              /* Success */
    DCM_ROUTINE_RESULT_GENERAL_ERROR,       /* General error */
    DCM_ROUTINE_RESULT_NOT_SUPPORTED,       /* Routine not supported */
    DCM_ROUTINE_RESULT_IN_PROGRESS,         /* Already in progress */
    DCM_ROUTINE_RESULT_INVALID_SEQUENCE,    /* Invalid sequence */
    DCM_ROUTINE_RESULT_FAILED               /* Execution failed */
} Dcm_RoutineResultType;

/******************************************************************************
 * Routine Info Types
 ******************************************************************************/
typedef enum {
    DCM_ROUTINE_INFO_NONE = 0,              /* No additional info */
    DCM_ROUTINE_INFO_PERCENTAGE,            /* Percentage complete */
    DCM_ROUTINE_INFO_STATUS,                /* Status information */
    DCM_ROUTINE_INFO_RESULT                 /* Result data */
} Dcm_RoutineInfoType;

/******************************************************************************
 * Routine Callback Function Types
 ******************************************************************************/

/* Start routine callback */
typedef Dcm_ReturnType (*Dcm_RoutineStartFunc)(
    uint16_t routineId,
    const uint8_t *optionRecord,
    uint16_t optionLength,
    uint8_t *statusRecord,
    uint16_t *statusLength
);

/* Stop routine callback */
typedef Dcm_ReturnType (*Dcm_RoutineStopFunc)(
    uint16_t routineId,
    uint8_t *statusRecord,
    uint16_t *statusLength
);

/* Request results callback */
typedef Dcm_ReturnType (*Dcm_RoutineResultsFunc)(
    uint16_t routineId,
    uint8_t *statusRecord,
    uint16_t *statusLength,
    uint8_t *dataRecord,
    uint16_t *dataLength
);

/******************************************************************************
 * Routine Configuration
 ******************************************************************************/
typedef struct {
    uint16_t routineId;                     /* Routine identifier */
    bool startSupported;                    /* Start routine supported */
    bool stopSupported;                     /* Stop routine supported */
    bool resultsSupported;                  /* Request results supported */
    uint8_t requiredSecurityLevel;          /* Required security level */
    Dcm_SessionType requiredSession;        /* Required session type */
    Dcm_RoutineStartFunc startFunc;         /* Start function */
    Dcm_RoutineStopFunc stopFunc;           /* Stop function */
    Dcm_RoutineResultsFunc resultsFunc;     /* Results function */
    const char *description;                /* Routine description */
} Dcm_RoutineConfigType;

/******************************************************************************
 * Routine Status
 ******************************************************************************/
typedef struct {
    uint16_t routineId;                     /* Active routine ID */
    Dcm_RoutineStateType state;             /* Current state */
    uint64_t startTime;                     /* Start timestamp */
    uint64_t endTime;                       /* End timestamp */
    uint8_t progress;                       /* Progress percentage (0-100) */
    uint16_t executionCount;                /* Number of executions */
    bool resultsAvailable;                  /* Results available flag */
    uint8_t lastStatusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
    uint16_t lastStatusLength;
} Dcm_RoutineStatusType;

/******************************************************************************
 * Routine Control Functions
 ******************************************************************************/

/**
 * @brief Initialize routine control service
 *
 * @param routines Array of routine configurations
 * @param numRoutines Number of routines
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using routine control service
 * @requirement ISO 14229-1:2020 10.14
 */
Dcm_ReturnType Dcm_RoutineControlInit(const Dcm_RoutineConfigType *routines,
                                      uint8_t numRoutines);

/**
 * @brief Process RoutineControl (0x31) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x31 for routine control
 * @requirement ISO 14229-1:2020 10.14
 */
Dcm_ReturnType Dcm_RoutineControl(const Dcm_RequestType *request,
                                  Dcm_ResponseType *response);

/**
 * @brief Start routine execution
 *
 * @param routineId Routine identifier
 * @param optionRecord Optional routine option record
 * @param optionLength Option record length
 * @param statusRecord Output: status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_StartRoutine(uint16_t routineId,
                                const uint8_t *optionRecord,
                                uint16_t optionLength,
                                uint8_t *statusRecord,
                                uint16_t *statusLength);

/**
 * @brief Stop routine execution
 *
 * @param routineId Routine identifier
 * @param statusRecord Output: status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_StopRoutine(uint16_t routineId,
                               uint8_t *statusRecord,
                               uint16_t *statusLength);

/**
 * @brief Request routine results
 *
 * @param routineId Routine identifier
 * @param statusRecord Output: status record
 * @param statusLength Output: status record length
 * @param dataRecord Output: data record
 * @param dataLength Output: data record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_RequestRoutineResults(uint16_t routineId,
                                         uint8_t *statusRecord,
                                         uint16_t *statusLength,
                                         uint8_t *dataRecord,
                                         uint16_t *dataLength);

/**
 * @brief Check if routine is supported
 *
 * @param routineId Routine identifier
 * @return bool True if supported
 */
bool Dcm_IsRoutineSupported(uint16_t routineId);

/**
 * @brief Get routine configuration
 *
 * @param routineId Routine identifier
 * @return const Dcm_RoutineConfigType* Configuration or NULL
 */
const Dcm_RoutineConfigType* Dcm_GetRoutineConfig(uint16_t routineId);

/**
 * @brief Get routine status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetRoutineStatus(Dcm_RoutineStatusType *status);

/**
 * @brief Check if routine control type is valid
 *
 * @param controlType Control type to check
 * @return bool True if valid
 */
bool Dcm_IsRoutineControlTypeValid(uint8_t controlType);

/**
 * @brief Get routine state
 *
 * @return Dcm_RoutineStateType Current routine state
 */
Dcm_RoutineStateType Dcm_GetRoutineState(void);

/**
 * @brief Check if routine is running
 *
 * @return bool True if running
 */
bool Dcm_IsRoutineRunning(void);

/******************************************************************************
 * Predefined Routine Implementations
 ******************************************************************************/

/**
 * @brief Erase memory routine
 *
 * @param memoryAddress Memory address to erase
 * @param memorySize Size to erase
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_Routine_EraseMemory(uint32_t memoryAddress, uint32_t memorySize);

/**
 * @brief Check programming dependencies routine
 *
 * @param result Output: check result
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_Routine_CheckProgrammingDependencies(bool *result);

/**
 * @brief Self test routine
 *
 * @param testId Test identifier
 * @param result Output: test result
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_Routine_SelfTest(uint8_t testId, bool *result);

#ifdef __cplusplus
}
#endif

#endif /* DCM_ROUTINE_H */
