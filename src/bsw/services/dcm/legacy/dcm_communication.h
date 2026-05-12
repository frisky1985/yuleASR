/******************************************************************************
 * @file    dcm_communication.h
 * @brief   DCM Communication Control Service (0x28) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_COMMUNICATION_H
#define DCM_COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Communication Control Constants (ISO 14229-1:2020 Table 60)
 ******************************************************************************/
#define DCM_COMM_CTRL_MIN_LENGTH                0x02U
#define DCM_COMM_CTRL_MAX_LENGTH                0x03U
#define DCM_COMM_CTRL_RESPONSE_LENGTH           0x02U

/******************************************************************************
 * Control Type Subfunctions (ISO 14229-1:2020 Table 59)
 ******************************************************************************/
#define DCM_COMM_CTRL_ENABLE_RX_TX              0x00U
#define DCM_COMM_CTRL_ENABLE_RX_DISABLE_TX      0x01U
#define DCM_COMM_CTRL_DISABLE_RX_ENABLE_TX      0x02U
#define DCM_COMM_CTRL_DISABLE_RX_TX             0x03U

/******************************************************************************
 * Communication Types (ISO 14229-1:2020 Table 61)
 ******************************************************************************/
#define DCM_COMM_TYPE_NORMAL                    0x01U
#define DCM_COMM_TYPE_NM                        0x02U
#define DCM_COMM_TYPE_NORMAL_AND_NM             0x03U

/******************************************************************************
 * Node Identification
 ******************************************************************************/
#define DCM_COMM_NODE_ECU                       0x00U  /* All networks */
#define DCM_COMM_NODE_SPECIFIC_MIN              0x01U
#define DCM_COMM_NODE_SPECIFIC_MAX              0xFEU

/******************************************************************************
 * Communication Control States
 ******************************************************************************/
typedef enum {
    DCM_COMM_STATE_ENABLED = 0,             /* Communication enabled */
    DCM_COMM_STATE_RX_DISABLED,             /* Receive disabled */
    DCM_COMM_STATE_TX_DISABLED,             /* Transmit disabled */
    DCM_COMM_STATE_RX_TX_DISABLED           /* Both disabled */
} Dcm_CommunicationStateType;

/******************************************************************************
 * Communication Type Definition
 ******************************************************************************/
typedef enum {
    DCM_COMM_NORMAL_MESSAGES = 0,           /* Normal application messages */
    DCM_COMM_NM_MESSAGES,                   /* Network management messages */
    DCM_COMM_BOTH_MESSAGES                  /* Both normal and NM */
} Dcm_CommunicationTypeEnum;

/******************************************************************************
 * Communication Control Result Types
 ******************************************************************************/
typedef enum {
    DCM_COMM_OK = 0,                        /* Operation successful */
    DCM_COMM_ERR_NOT_SUPPORTED,             /* Control type not supported */
    DCM_COMM_ERR_INVALID_TYPE,              /* Invalid communication type */
    DCM_COMM_ERR_INVALID_NODE,              /* Invalid node identification */
    DCM_COMM_ERR_CONDITIONS_NOT_CORRECT,    /* Conditions not correct */
    DCM_COMM_ERR_IN_PROGRESS                /* Operation in progress */
} Dcm_CommunicationResultType;

/******************************************************************************
 * Communication State Change Callback Type
 ******************************************************************************/
typedef void (*Dcm_CommunicationChangeCallback)(
    Dcm_CommunicationTypeEnum commType,
    bool rxEnabled,
    bool txEnabled,
    uint8_t subnetId
);

/******************************************************************************
 * DDS Topic Control Callback Type
 ******************************************************************************/
typedef void (*Dcm_DdsTopicControlCallback)(
    bool enable,
    uint8_t topicMask
);

/******************************************************************************
 * Network Control Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_NetworkControlCallback)(
    uint8_t subnetId,
    bool enableRx,
    bool enableTx
);

/******************************************************************************
 * Communication Control Configuration
 ******************************************************************************/
typedef struct {
    bool enableNormalCommControl;           /* Enable normal message control */
    bool enableNmCommControl;               /* Enable NM message control */
    bool enableDdsControl;                  /* Enable DDS topic control */
    bool requireExtendedSession;            /* Require extended session */
    bool requireProgrammingSession;         /* Require programming session */
    Dcm_CommunicationChangeCallback changeCallback; /* State change callback */
    Dcm_DdsTopicControlCallback ddsCallback;        /* DDS control callback */
    Dcm_NetworkControlCallback networkCallback;     /* Network control callback */
} Dcm_CommunicationControlConfigType;

/******************************************************************************
 * Communication State Per Subnet
 ******************************************************************************/
typedef struct {
    uint8_t subnetId;                       /* Subnet identifier */
    Dcm_CommunicationStateType state;       /* Current communication state */
    bool normalRxEnabled;                   /* Normal message RX enabled */
    bool normalTxEnabled;                   /* Normal message TX enabled */
    bool nmRxEnabled;                       /* NM message RX enabled */
    bool nmTxEnabled;                       /* NM message TX enabled */
    bool ddsEnabled;                        /* DDS topics enabled */
} Dcm_SubnetStateType;

/******************************************************************************
 * Communication Control Status
 ******************************************************************************/
typedef struct {
    Dcm_SubnetStateType subnetStates[8];    /* State per subnet (0 = all) */
    uint8_t numSubnets;                     /* Number of configured subnets */
    uint8_t activeControlType;              /* Last control type applied */
    uint8_t activeCommunicationType;        /* Last communication type */
    uint16_t controlRequestCount;           /* Total control requests */
} Dcm_CommunicationStatusType;

/******************************************************************************
 * Communication Control Functions
 ******************************************************************************/

/**
 * @brief Initialize communication control service
 *
 * @param config Pointer to communication control configuration
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using communication control service
 * @requirement ISO 14229-1:2020 10.7
 */
Dcm_ReturnType Dcm_CommunicationControlInit(const Dcm_CommunicationControlConfigType *config);

/**
 * @brief Process CommunicationControl (0x28) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x28 for communication control
 * @requirement ISO 14229-1:2020 10.7
 */
Dcm_ReturnType Dcm_CommunicationControl(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Enable all communication
 *
 * @param subnetId Subnet identifier (0 = all)
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_EnableCommunication(uint8_t subnetId);

/**
 * @brief Disable all communication
 *
 * @param subnetId Subnet identifier (0 = all)
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_DisableCommunication(uint8_t subnetId);

/**
 * @brief Get current communication state
 *
 * @param subnetId Subnet identifier
 * @param state Pointer to store state
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetCommunicationState(
    uint8_t subnetId,
    Dcm_CommunicationStateType *state
);

/**
 * @brief Check if normal communication is enabled
 *
 * @param subnetId Subnet identifier
 * @return bool True if normal communication enabled
 */
bool Dcm_IsNormalCommunicationEnabled(uint8_t subnetId);

/**
 * @brief Check if NM communication is enabled
 *
 * @param subnetId Subnet identifier
 * @return bool True if NM communication enabled
 */
bool Dcm_IsNmCommunicationEnabled(uint8_t subnetId);

/**
 * @brief Get communication control status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetCommunicationStatus(Dcm_CommunicationStatusType *status);

/**
 * @brief Reset communication state to default (all enabled)
 *
 * @return Dcm_ReturnType Result of operation
 *
 * @note Typically called on session transition to default
 */
Dcm_ReturnType Dcm_ResetCommunicationState(void);

/**
 * @brief Validate control type subfunction
 *
 * @param controlType Control type to validate
 * @return bool True if valid
 */
bool Dcm_IsControlTypeValid(uint8_t controlType);

/**
 * @brief Validate communication type
 *
 * @param commType Communication type to validate
 * @return bool True if valid
 */
bool Dcm_IsCommunicationTypeValid(uint8_t commType);

/**
 * @brief Validate node identification
 *
 * @param nodeId Node identification to validate
 * @return bool True if valid
 */
bool Dcm_IsNodeIdentificationValid(uint8_t nodeId);

/**
 * @brief Control DDS topic communication
 *
 * @param enable True to enable, false to disable
 * @param topicMask Bitmask of topics to control
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ControlDdsTopics(bool enable, uint8_t topicMask);

/**
 * @brief Check if DDS topics are enabled
 *
 * @param topicMask Topic mask to check
 * @return bool True if enabled
 */
bool Dcm_AreDdsTopicsEnabled(uint8_t topicMask);

#ifdef __cplusplus
}
#endif

#endif /* DCM_COMMUNICATION_H */
