/**
 * @file Com.h
 * @brief Communication Services - AutoSAR Service Layer
 * @version 1.0.0
 * @date 2026-04-14
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details Provides an AUTOSAR COM module offering services for
 *          sending and receiving signals and signal groups.
 */

#ifndef COM_H
#define COM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Com_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief COM Module Vendor ID */
#define COM_VENDOR_ID                           0x0055U  /* YuleTech */

/** @brief COM Module ID */
#define COM_MODULE_ID                           0x001EU

/** @brief COM Instance ID */
#define COM_INSTANCE_ID                         0x00U

/* API Service IDs */
#define COM_API_ID_INIT                         0x01U
#define COM_API_ID_DEINIT                       0x02U
#define COM_API_ID_IPDU_GROUP_CONTROL           0x03U
#define COM_API_ID_RECEPTION_DM_CONTROL         0x04U
#define COM_API_ID_GET_STATUS                   0x05U
#define COM_API_ID_GET_CONFIGURATION_ID         0x06U
#define COM_API_ID_GET_VERSION_INFO             0x07U
#define COM_API_ID_SEND_SIGNAL                  0x08U
#define COM_API_ID_RECEIVE_SIGNAL               0x09U
#define COM_API_ID_SEND_SIGNAL_GROUP            0x0AU
#define COM_API_ID_RECEIVE_SIGNAL_GROUP         0x0BU
#define COM_API_ID_RECEIVE_SIGNAL_GROUP_ARRAY   0x0CU
#define COM_API_ID_SEND_SIGNAL_GROUP_ARRAY      0x0DU
#define COM_API_ID_INVALIDATE_SIGNAL            0x10U
#define COM_API_ID_INVALIDATE_SIGNAL_GROUP      0x11U
#define COM_API_ID_TRIGGER_IPDU_SEND            0x13U
#define COM_API_ID_TRIGGER_IPDU_SEND_WITH_META  0x14U
#define COM_API_ID_SWITCH_IPDU_TX_MODE          0x15U
#define COM_API_ID_GET_IPDU_STATUS              0x18U
#define COM_API_ID_GET_SIGNAL_STATUS            0x19U
#define COM_API_ID_GET_SIGNAL_GROUP_STATUS      0x1AU
#define COM_API_ID_GET_TX_IPDU_COUNTER          0x1BU
#define COM_API_ID_GET_RX_IPDU_COUNTER          0x1CU
#define COM_API_ID_CLEAR_IPDU_GROUP_VOTER       0x1DU
#define COM_API_ID_MAIN_FUNCTION_RX             0x1EU
#define COM_API_ID_MAIN_FUNCTION_TX             0x1FU
#define COM_API_ID_MAIN_FUNCTION_ROUTE_SIGNALS  0x20U
#define COM_API_ID_ENABLE_RECEPTION_DM          0x21U
#define COM_API_ID_DISABLE_RECEPTION_DM         0x22U

/* Error Codes */
#define COM_E_PARAM                             0x01U
#define COM_E_UNINIT                            0x02U
#define COM_E_PARAM_POINTER                     0x03U
#define COM_E_PARAM_INIT                        0x04U
#define COM_E_INIT_FAILED                       0x05U
#define COM_E_INVALID_SIGNAL_ID                 0x06U
#define COM_E_INVALID_SIGNAL_GROUP_ID           0x07U
#define COM_E_INVALID_IPDU_ID                   0x08U

/* Signal Status */
#define COM_E_OK                                0x00U
#define COM_E_SKIPPED_TRANSMISSION              0x01U
#define COM_E_UPDATE_DETECTED                   0x02U

/* I-PDU Status */
#define COM_BUSY                                0x01U
#define COM_BUSY_WITH_REPETITION                0x02U
#define COM_NOT_STARTED                         0x04U
#define COM_STARTED                             0x08U

/* Signal Status */
#define COM_FALSE                               0x00U
#define COM_TRUE                                0x01U
#define COM_SIG_ERROR                           0x02U
#define COM_SIG_WAITING                         0x04U
#define COM_SIG_TIMEOUT                         0x08U
#define COM_SIG_VALID                           0x10U
#define COM_SIG_INVALID                         0x20U

/* Transmission Modes */
typedef uint8 ComTxModeModeType;
#define COM_DIRECT                              0x00U
#define COM_MIXED                               0x01U
#define COM_NONE                                0x02U
#define COM_PERIODIC                            0x03U

/*==================================================================================================
*                                          TYPE DEFINITIONS
==================================================================================================*/
/** @brief Type for the signal ID */
typedef uint16 Com_SignalIdType;

/** @brief Type for the signal group ID */
typedef uint16 Com_SignalGroupIdType;

/** @brief Type for the I-PDU ID */
typedef uint16 Com_IpduGroupIdType;

/** @brief Type for the I-PDU ID */
typedef uint16 Com_IpduIdType;

/** @brief Type for the signal size */
typedef uint8 Com_SignalSizeType;

/** @brief Type for the configuration ID */
typedef uint32 Com_ConfigIdType;

/** @brief Type for the signal status */
typedef uint8 Com_SignalStatusType;

/** @brief Type for the I-PDU status */
typedef uint8 Com_IpduStatusType;

/** @brief Type for the return status */
typedef uint8 Com_StatusType;

/** @brief Type for the transmission mode */
typedef struct {
    ComTxModeModeType Mode;
    uint8 NumberOfRepetitions;
    uint16 RepetitionPeriodFactor;
    uint16 TimePeriodFactor;
} ComTxModeType;

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
#define COM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the COM module
 *
 * @param[in] config Pointer to the COM configuration
 *
 * @pre None
 * @post COM module initialized
 */
void Com_Init(const Com_ConfigType* config);

/**
 * @brief De-initializes the COM module
 *
 * @pre COM module initialized
 * @post COM module de-initialized
 */
void Com_DeInit(void);

/**
 * @brief Starts I-PDU groups
 *
 * @param[in] IpduGroupVector I-PDU group vector
 * @param[in] Initialize Initialize the I-PDU groups
 *
 * @pre COM module initialized
 * @post I-PDU groups started
 */
void Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize);

/**
 * @brief Enables or disables deadline monitoring for I-PDU groups
 *
 * @param[in] IpduGroupVector I-PDU group vector
 * @param[in] Enable Enable or disable deadline monitoring
 *
 * @pre COM module initialized
 * @post Deadline monitoring enabled/disabled
 */
void Com_ReceptionDMControl(Com_IpduGroupVector IpduGroupVector, boolean Enable);

/**
 * @brief Returns the status of the COM module
 *
 * @return Com_StatusType Status of the COM module
 */
Com_StatusType Com_GetStatus(void);

/**
 * @brief Returns the configuration ID
 *
 * @return Com_ConfigIdType Configuration ID
 */
Com_ConfigIdType Com_GetConfigurationId(void);

/**
 * @brief Returns the version information
 *
 * @param[out] versioninfo Pointer to version information structure
 *
 * @pre None
 * @post Version information stored
 */
#if (COM_VERSION_INFO_API == STD_ON)
void Com_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Sends a signal
 *
 * @param[in] SignalId ID of the signal to be sent
 * @param[in] SignalDataPtr Pointer to the signal data
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - COM_BUSY: Transmission is currently ongoing
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal queued for transmission
 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Receives a signal
 *
 * @param[in] SignalId ID of the signal to be received
 * @param[out] SignalDataPtr Pointer to the signal data buffer
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal data stored in buffer
 */
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);

/**
 * @brief Sends a signal group
 *
 * @param[in] SignalGroupId ID of the signal group to be sent
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - COM_BUSY: Transmission is currently ongoing
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal group queued for transmission
 */
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Receives a signal group
 *
 * @param[in] SignalGroupId ID of the signal group to be received
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal group data stored
 */
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Triggers the transmission of an I-PDU
 *
 * @param[in] PduId ID of the I-PDU to be transmitted
 *
 * @return Std_ReturnType
 *         - E_OK: Transmission triggered
 *         - E_NOT_OK: Transmission not triggered
 *
 * @pre COM module initialized
 * @post I-PDU transmission triggered
 */
Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId);

/**
 * @brief Switches the transmission mode of an I-PDU
 *
 * @param[in] PduId ID of the I-PDU
 * @param[in] Mode New transmission mode
 *
 * @pre COM module initialized
 * @post Transmission mode switched
 */
void Com_SwitchIpduTxMode(PduIdType PduId, ComTxModeModeType Mode);

/**
 * @brief Main function for reception processing
 *
 * @pre COM module initialized
 * @post Reception processing done
 */
void Com_MainFunctionRx(void);

/**
 * @brief Main function for transmission processing
 *
 * @pre COM module initialized
 * @post Transmission processing done
 */
void Com_MainFunctionTx(void);

/**
 * @brief Main function for signal routing
 *
 * @pre COM module initialized
 * @post Signal routing done
 */
void Com_MainFunctionRouteSignals(void);

#define COM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                      INLINE FUNCTIONS
==================================================================================================*/

#endif /* COM_H */
