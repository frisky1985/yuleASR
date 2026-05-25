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

/******************************************************************************
 * @file    dcm_io_control.h
 * @brief   DCM Input Output Control By Identifier Service (0x2F) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant (Section 10.7)
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_IO_CONTROL_H
#define DCM_IO_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Input Output Control Constants (ISO 14229-1:2020 Table 119)
 ******************************************************************************/
#define DCM_IO_CONTROL_MIN_LENGTH               0x03U  /* SID + DID (2 bytes) */
#define DCM_IO_CONTROL_RESPONSE_MIN_LENGTH      0x03U  /* SID + DID (2 bytes) */
#define DCM_IO_CONTROL_DID_LENGTH               0x02U  /* Data Identifier length */
#define DCM_IO_CONTROL_OPTION_RECORD_MIN_LENGTH 0x01U  /* Minimum controlOptionRecord */
#define DCM_MAX_CONTROL_STATUS_RECORD           0xFFU  /* Max controlStatusRecord length */
#define DCM_MAX_CONTROL_ENABLE_MASK             0x20U  /* Max controlEnableMaskRecord length */

/******************************************************************************
 * Input Output Control Option Records (ISO 14229-1:2020 Table 120)
 ******************************************************************************/
#define DCM_IO_CTRL_RETURN_TO_ECU               0x00U  /* Return control to ECU */
#define DCM_IO_CTRL_RESET_TO_DEFAULT            0x01U  /* Reset to default */
#define DCM_IO_CTRL_FREEZE_CURRENT_STATE        0x02U  /* Freeze current state */
#define DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT       0x03U  /* Short term adjustment */

/******************************************************************************
 * Predefined IO Control Data Identifiers (ISO 14229-1:2020 Annex G)
 ******************************************************************************/
#define DCM_IO_CTRL_DID_ECU_VOLTAGE             0xF101U  /* ECU supply voltage */
#define DCM_IO_CTRL_DID_ENGINE_RPM              0xF202U  /* Engine RPM control */
#define DCM_IO_CTRL_DID_VEHICLE_SPEED           0xF203U  /* Vehicle speed */
#define DCM_IO_CTRL_DIM_FAN_CONTROL             0xF301U  /* Cooling fan control */
#define DCM_IO_CTRL_DID_OIL_PRESSURE            0xF401U  /* Oil pressure */
#define DCM_IO_CTRL_DID_FUEL_PUMP               0xF501U  /* Fuel pump control */
#define DCM_IO_CTRL_DID_THROTTLE_POSITION       0xF601U  /* Throttle position */
#define DCM_IO_CTRL_DID_BRAKE_LIGHT             0xF701U  /* Brake light control */

/******************************************************************************
 * IO Control States
 ******************************************************************************/
typedef enum {
    DCM_IO_STATE_ECU_CONTROL = 0,           /* Controlled by ECU */
    DCM_IO_STATE_UNDER_DIAGNOSTIC_CONTROL,  /* Controlled by diagnostic tool */
    DCM_IO_STATE_FROZEN,                    /* State frozen */
    DCM_IO_STATE_DEFAULT,                   /* Default value */
    DCM_IO_STATE_ERROR                      /* Error state */
} Dcm_IoControlStateType;

/******************************************************************************
 * IO Control Results
 ******************************************************************************/
typedef enum {
    DCM_IO_RESULT_OK = 0,                   /* Success */
    DCM_IO_RESULT_GENERAL_ERROR,            /* General error */
    DCM_IO_RESULT_NOT_SUPPORTED,            /* IO control not supported */
    DCM_IO_RESULT_INVALID_CONTROL_TYPE,     /* Invalid control option record */
    DCM_IO_RESULT_FAILED                    /* Execution failed */
} Dcm_IoControlResultType;

/******************************************************************************
 * IO Control Callback Function Types
 ******************************************************************************/

/* Return control to ECU callback */
typedef Dcm_ReturnType (*Dcm_IoCtrlReturnToEcuFunc)(
    uint16_t dataIdentifier,
    uint8_t *controlStatusRecord,
    uint16_t *statusLength
);

/* Reset to default callback */
typedef Dcm_ReturnType (*Dcm_IoCtrlResetToDefaultFunc)(
    uint16_t dataIdentifier,
    uint8_t *controlStatusRecord,
    uint16_t *statusLength
);

/* Freeze current state callback */
typedef Dcm_ReturnType (*Dcm_IoCtrlFreezeStateFunc)(
    uint16_t dataIdentifier,
    uint8_t *controlStatusRecord,
    uint16_t *statusLength
);

/* Short term adjustment callback */
typedef Dcm_ReturnType (*Dcm_IoCtrlShortTermAdjustmentFunc)(
    uint16_t dataIdentifier,
    const uint8_t *controlState,
    uint16_t controlStateLength,
    const uint8_t *controlEnableMask,
    uint16_t maskLength,
    uint8_t *controlStatusRecord,
    uint16_t *statusLength
);

/* Condition check callback */
typedef Dcm_ReturnType (*Dcm_IoCtrlConditionCheckFunc)(
    uint16_t dataIdentifier,
    uint8_t controlType,
    bool *conditionsOk
);

/******************************************************************************
 * IO Control Configuration
 ******************************************************************************/
typedef struct {
    uint16_t dataIdentifier;                /* Data identifier */
    bool returnToEcuSupported;              /* Return control to ECU supported */
    bool resetToDefaultSupported;           /* Reset to default supported */
    bool freezeStateSupported;              /* Freeze current state supported */
    bool shortTermAdjustmentSupported;      /* Short term adjustment supported */
    bool controlEnableMaskSupported;        /* Control enable mask supported */
    uint8_t requiredSecurityLevel;          /* Required security level */
    Dcm_SessionType requiredSession;        /* Required session type */
    Dcm_IoCtrlReturnToEcuFunc returnToEcuFunc;      /* Return to ECU function */
    Dcm_IoCtrlResetToDefaultFunc resetToDefaultFunc; /* Reset function */
    Dcm_IoCtrlFreezeStateFunc freezeStateFunc;      /* Freeze function */
    Dcm_IoCtrlShortTermAdjustmentFunc shortTermFunc; /* Short term adjustment function */
    Dcm_IoCtrlConditionCheckFunc conditionCheckFunc; /* Condition check function */
    uint16_t controlStateSize;              /* Size of control state */
    const char *description;                /* IO description */
} Dcm_IoControlConfigType;

/******************************************************************************
 * IO Control Status
 ******************************************************************************/
typedef struct {
    uint16_t dataIdentifier;                /* Current controlled DID */
    Dcm_IoControlStateType state;           /* Current state */
    uint8_t controlType;                    /* Current control type */
    uint64_t controlStartTime;              /* Control start timestamp */
    bool isActive;                          /* Control active flag */
    uint8_t lastControlStatus[DCM_MAX_CONTROL_STATUS_RECORD];
    uint16_t lastStatusLength;
} Dcm_IoControlStatusType;

/******************************************************************************
 * IO Control Functions
 ******************************************************************************/

/**
 * @brief Initialize IO control service
 *
 * @param ioControls Array of IO control configurations
 * @param numIoControls Number of IO controls
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using IO control service
 * @requirement ISO 14229-1:2020 10.7
 */
Dcm_ReturnType Dcm_IoControlInit(const Dcm_IoControlConfigType *ioControls,
                                 uint8_t numIoControls);

/**
 * @brief Process InputOutputControlByIdentifier (0x2F) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x2F for IO control
 * @requirement ISO 14229-1:2020 10.7
 */
Dcm_ReturnType Dcm_InputOutputControlByIdentifier(const Dcm_RequestType *request,
                                                  Dcm_ResponseType *response);

/**
 * @brief Return control to ECU
 *
 * @param dataIdentifier Data identifier
 * @param controlStatusRecord Output: control status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_IoControlReturnToEcu(uint16_t dataIdentifier,
                                        uint8_t *controlStatusRecord,
                                        uint16_t *statusLength);

/**
 * @brief Reset to default
 *
 * @param dataIdentifier Data identifier
 * @param controlStatusRecord Output: control status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_IoControlResetToDefault(uint16_t dataIdentifier,
                                           uint8_t *controlStatusRecord,
                                           uint16_t *statusLength);

/**
 * @brief Freeze current state
 *
 * @param dataIdentifier Data identifier
 * @param controlStatusRecord Output: control status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_IoControlFreezeCurrentState(uint16_t dataIdentifier,
                                               uint8_t *controlStatusRecord,
                                               uint16_t *statusLength);

/**
 * @brief Short term adjustment
 *
 * @param dataIdentifier Data identifier
 * @param controlState Control state value
 * @param controlStateLength Control state length
 * @param controlEnableMask Control enable mask (NULL if not used)
 * @param maskLength Mask length
 * @param controlStatusRecord Output: control status record
 * @param statusLength Output: status record length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_IoControlShortTermAdjustment(uint16_t dataIdentifier,
                                                const uint8_t *controlState,
                                                uint16_t controlStateLength,
                                                const uint8_t *controlEnableMask,
                                                uint16_t maskLength,
                                                uint8_t *controlStatusRecord,
                                                uint16_t *statusLength);

/**
 * @brief Check if IO control is supported
 *
 * @param dataIdentifier Data identifier
 * @return bool True if supported
 */
bool Dcm_IsIoControlSupported(uint16_t dataIdentifier);

/**
 * @brief Get IO control configuration
 *
 * @param dataIdentifier Data identifier
 * @return const Dcm_IoControlConfigType* Configuration or NULL
 */
const Dcm_IoControlConfigType* Dcm_GetIoControlConfig(uint16_t dataIdentifier);

/**
 * @brief Get IO control status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetIoControlStatus(Dcm_IoControlStatusType *status);

/**
 * @brief Check if control type is valid
 *
 * @param controlType Control type to check
 * @return bool True if valid
 */
bool Dcm_IsIoControlTypeValid(uint8_t controlType);

/**
 * @brief Get current IO control state
 *
 * @return Dcm_IoControlStateType Current IO control state
 */
Dcm_IoControlStateType Dcm_GetIoControlState(void);

/**
 * @brief Check if IO is under diagnostic control
 *
 * @return bool True if under diagnostic control
 */
bool Dcm_IsIoUnderDiagnosticControl(void);

/**
 * @brief Check if control type is supported for DID
 *
 * @param dataIdentifier Data identifier
 * @param controlType Control type to check
 * @return bool True if supported
 */
bool Dcm_IsIoControlTypeSupported(uint16_t dataIdentifier, uint8_t controlType);

/******************************************************************************
 * Predefined IO Control Implementations
 ******************************************************************************/

/**
 * @brief Default return control to ECU handler
 *
 * @param dataIdentifier Data identifier
 * @param statusRecord Output: status record
 * @param statusLength Output: status length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_IoCtrl_DefaultReturnToEcu(uint16_t dataIdentifier,
                                             uint8_t *statusRecord,
                                             uint16_t *statusLength);

/**
 * @brief Default reset to default handler
 *
 * @param dataIdentifier Data identifier
 * @param statusRecord Output: status record
 * @param statusLength Output: status length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_IoCtrl_DefaultResetToDefault(uint16_t dataIdentifier,
                                                uint8_t *statusRecord,
                                                uint16_t *statusLength);

/**
 * @brief Default freeze state handler
 *
 * @param dataIdentifier Data identifier
 * @param statusRecord Output: status record
 * @param statusLength Output: status length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_IoCtrl_DefaultFreezeState(uint16_t dataIdentifier,
                                             uint8_t *statusRecord,
                                             uint16_t *statusLength);

/**
 * @brief Default short term adjustment handler
 *
 * @param dataIdentifier Data identifier
 * @param controlState Control state
 * @param controlStateLength Control state length
 * @param controlEnableMask Control enable mask
 * @param maskLength Mask length
 * @param statusRecord Output: status record
 * @param statusLength Output: status length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_IoCtrl_DefaultShortTermAdjustment(uint16_t dataIdentifier,
                                                     const uint8_t *controlState,
                                                     uint16_t controlStateLength,
                                                     const uint8_t *controlEnableMask,
                                                     uint16_t maskLength,
                                                     uint8_t *statusRecord,
                                                     uint16_t *statusLength);

#ifdef __cplusplus
}
#endif

#endif /* DCM_IO_CONTROL_H */
