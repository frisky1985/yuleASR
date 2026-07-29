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

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define COM_SW_MAJOR_VERSION                    0x01U
#define COM_SW_MINOR_VERSION                    0x00U
#define COM_SW_PATCH_VERSION                    0x00U

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

/* Service ID aliases for backward compatibility */
#define COM_SERVICE_ID_INIT                     COM_API_ID_INIT
#define COM_SERVICE_ID_DEINIT                   COM_API_ID_DEINIT
#define COM_SERVICE_ID_SENDSIGNAL               COM_API_ID_SEND_SIGNAL
#define COM_SERVICE_ID_RECEIVESIGNAL            COM_API_ID_RECEIVE_SIGNAL
#define COM_SERVICE_ID_SENDSIGNALGROUP          COM_API_ID_SEND_SIGNAL_GROUP
#define COM_SERVICE_ID_RECEIVESIGNALGROUP       COM_API_ID_RECEIVE_SIGNAL_GROUP
#define COM_SERVICE_ID_TRIGGERIPDUSEND          COM_API_ID_TRIGGER_IPDU_SEND
#define COM_SERVICE_ID_TXCONFIRMATION           COM_API_ID_TRIGGER_IPDU_SEND
#define COM_SERVICE_ID_RXINDICATION             COM_API_ID_TRIGGER_IPDU_SEND
#define COM_SERVICE_ID_TRIGGERTRANSMIT          COM_API_ID_TRIGGER_IPDU_SEND_WITH_META
#define COM_SERVICE_ID_UPDATESHADOWSIGNAL       COM_API_ID_SEND_SIGNAL
#define COM_SERVICE_ID_RECEIVESHADOWSIGNAL      COM_API_ID_RECEIVE_SIGNAL
#define COM_SERVICE_ID_GETVERSIONINFO           COM_API_ID_GET_VERSION_INFO

/* Error Codes */
#define COM_E_PARAM                             0x01U
#define COM_E_UNINIT                            0x02U
#define COM_E_PARAM_POINTER                     0x03U
#define COM_E_PARAM_INIT                        0x04U
#define COM_E_INIT_FAILED                       0x05U
#define COM_E_INVALID_SIGNAL_ID                 0x06U
#define COM_E_INVALID_SIGNAL_GROUP_ID           0x07U
#define COM_E_INVALID_IPDU_ID                   0x08U

/* Error code aliases for backward compatibility */
#define COM_E_PARAM_SIGNAL                      COM_E_INVALID_SIGNAL_ID
#define COM_E_PARAM_SIGNALGROUP                 COM_E_INVALID_SIGNAL_GROUP_ID
#define COM_E_PARAM_IPDU                        COM_E_INVALID_IPDU_ID

/* Signal Status */
#define COM_E_OK                                0x00U
#define COM_E_SKIPPED_TRANSMISSION              0x01U
#define COM_E_UPDATE_DETECTED                   0x02U

/* I-PDU Status */
#define COM_BUSY                                0x01U
#define COM_BUSY_WITH_REPETITION                0x02U
#define COM_NOT_STARTED                         0x04U
#define COM_STARTED                             0x08U

/* Module Status */
#define COM_UNINIT                              0x00U
#define COM_INIT                                0x01U

/* Signal Status */
#define COM_FALSE                               0x00U
#define COM_TRUE                                0x01U
#define COM_SIG_ERROR                           0x02U
#define COM_SIG_WAITING                         0x04U
#define COM_SIG_TIMEOUT                         0x08U
#define COM_SIG_VALID                           0x10U
#define COM_SIG_INVALID                         0x20U

/* Service Return Codes */
#define COM_SERVICE_OK                          0x00U
#define COM_SERVICE_NOT_OK                      0x01U

/* Transmission Modes */
typedef uint8 ComTxModeModeType;
#define COM_DIRECT                              0x00U
#define COM_MIXED                               0x01U
#define COM_NONE                                0x02U
#define COM_PERIODIC                            0x03U

/* Signal Transfer Properties */
#define COM_TRIGGERED                           0x00U
#define COM_TRIGGERED_ON_CHANGE                 0x01U
#define COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION 0x02U
#define COM_PENDING                             0x03U

/* Filter Algorithms */
#define COM_ALWAYS                              0x00U
#define COM_NEVER                               0x01U
#define COM_MASKED_NEW_EQUALS_X                 0x02U
#define COM_MASKED_NEW_DIFFERS_X                0x03U
#define COM_MASKED_NEW_DIFFERS_MASKED_OLD       0x04U
#define COM_NEW_IS_WITHIN                       0x05U
#define COM_NEW_IS_OUTSIDE                      0x06U
#define COM_ONE_EVERY_N                         0x07U

/* Endianness definitions are in Com_Cfg.h */

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

/** @brief Type for I-PDU group vector */
typedef uint8 Com_IpduGroupVector[(COM_NUM_IPDU_GROUPS + 7U) / 8U];

/** @brief Type for signal configuration */
typedef struct {
    Com_SignalIdType SignalId;
    uint16 BitPosition;
    uint8 BitSize;
    uint8 Endianness;
    uint8 TransferProperty;
    uint8 FilterAlgorithm;
    uint32 FilterMask;
    uint32 FilterX;
    uint16 SignalGroupRef;
} Com_SignalConfigType;

/** @brief Type for IPDU configuration */
typedef struct {
    PduIdType PduId;
    uint16 DataLength;
    boolean RepeatingEnabled;
    uint8 NumRepetitions;
    uint16 TimeBetweenRepetitions;
    uint16 TimePeriod;
} Com_IPduConfigType;

/** @brief Type for COM configuration */
typedef struct {
    const Com_SignalConfigType* Signals;
    uint16 NumSignals;
    const Com_IPduConfigType* IPdus;
    uint16 NumIPdus;
} Com_ConfigType;

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
 * @brief Enables deadline monitoring for I-PDU groups
 *
 * @param[in] IpduGroupVector I-PDU group vector
 *
 * @pre COM module initialized
 * @post Deadline monitoring enabled for specified groups
 */
void Com_EnableReceptionDM(Com_IpduGroupVector IpduGroupVector);

/**
 * @brief Disables deadline monitoring for I-PDU groups
 *
 * @param[in] IpduGroupVector I-PDU group vector
 *
 * @pre COM module initialized
 * @post Deadline monitoring disabled for specified groups
 */
void Com_DisableReceptionDM(Com_IpduGroupVector IpduGroupVector);

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
 * @brief Invalidates a signal
 *
 * @param[in] SignalId ID of the signal to be invalidated
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal set to invalid value
 */
uint8 Com_InvalidateSignal(Com_SignalIdType SignalId);

/**
 * @brief Invalidates a signal group
 *
 * @param[in] SignalGroupId ID of the signal group to be invalidated
 *
 * @return uint8
 *         - COM_SERVICE_NOT_AVAILABLE: Service not available
 *         - E_OK: Service accepted
 *
 * @pre COM module initialized
 * @post Signal group set to invalid values
 */
uint8 Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId);

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
 * @brief Trigger transmit callback from PduR
 *
 * @param[in] TxPduId PDU to trigger
 * @param[out] PduInfoPtr Pointer to PDU info
 *
 * @return Std_ReturnType
 *         - E_OK: Data provided
 *         - E_NOT_OK: Data not provided
 */
Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/**
 * @brief Receive indication callback from PduR
 *
 * @param[in] RxPduId PDU that was received
 * @param[in] PduInfoPtr Pointer to PDU info
 *
 * @pre COM module initialized
 * @post Received data copied to IPDU buffer
 */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Transmit confirmation callback from PduR
 *
 * @param[in] TxPduId PDU that was transmitted
 * @param[in] result Transmission result
 *
 * @pre COM module initialized
 * @post IPDU transmission state updated
 */
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

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
