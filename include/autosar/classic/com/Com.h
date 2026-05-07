/*
 * Com.h
 * AUTOSAR COM Module - Public Interface
 * According to AUTOSAR SWS COM 4.4.0
 */

#ifndef COM_H
#define COM_H

/*==================[Includes]=============================================*/

#include "Com_Types.h"
#include "PduR.h"

/*==================[Version Information]==================================*/

#define COM_VENDOR_ID          0x0043u
#define COM_MODULE_ID          0x001Eu
#define COM_INSTANCE_ID        0x0000u

#define COM_SW_MAJOR_VERSION   0x01u
#define COM_SW_MINOR_VERSION   0x00u
#define COM_SW_PATCH_VERSION   0x00u

/*==================[Development Error Codes]==============================*/

#define COM_E_PARAM                             0x01u
#define COM_E_PARAM_POINTER                     0x02u
#define COM_E_UNINIT                            0x03u
#define COM_E_INIT_FAILED                       0x04u
#define COM_E_PARAM_SIGNALID                    0x05u
#define COM_E_PARAM_DATASERIESINDEX             0x06u
#define COM_E_PARAM_POINTER_TO_SIGNALGRP        0x07u
#define COM_E_ALREADY_INITIALIZED               0x08u

/*==================[API Services]=========================================*/

/**
 * @defgroup Com_Init_API Initialization and General Functions
 * @{
 */
/**
 * @brief Initialize COM module
 * @param config Pointer to configuration structure
 */
extern void Com_Init(const Com_ConfigType* config);

/**
 * @brief Deinitialize COM module
 */
extern void Com_DeInit(void);

/**
 * @brief Get COM module status
 * @return Current module status
 */
extern Com_StatusType Com_GetStatus(void);

/**
 * @brief Get version information
 * @param versioninfo Pointer to store version info
 */
extern void Com_GetVersionInfo(Std_VersionInfoType* versioninfo);
/** @} */

/**
 * @defgroup Com_IpduGroup_API IPdu Group Control
 * @{
 */
/**
 * @brief Start an I-PDU group
 * @param IpduGroupId I-PDU group identifier
 * @param Initialize TRUE to initialize signals, FALSE otherwise
 */
extern void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);

/**
 * @brief Stop an I-PDU group
 * @param IpduGroupId I-PDU group identifier
 */
extern void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);
/** @} */

/**
 * @defgroup Com_Signal_API Signal Operations
 * @{
 */
/**
 * @brief Send a signal
 * 
 * Copies signal data to I-PDU buffer and triggers transmission based on
 * transfer property configuration (TRIGGERED, TRIGGERED_ON_CHANGE, etc.)
 * 
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to signal data
 * @return E_OK if successful, COM_SERVICE_NOT_AVAILABLE if service not available
 * 
 * @req SWS_Com_00034
 * @req SWS_Com_00209 (ASIL-D: Input validation)
 */
extern uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Receive a signal
 * 
 * Extracts signal data from I-PDU buffer.
 * 
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to store signal data
 * @return E_OK if successful
 */
extern uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
/** @} */

/**
 * @defgroup Com_SignalGroup_API Signal Group Operations
 * @{
 */
/**
 * @brief Send a signal group
 * 
 * Copies shadow buffer to I-PDU and triggers transmission.
 * 
 * @param SignalGroupId Signal group identifier
 * @return E_OK if successful
 */
extern uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Receive a signal group
 * 
 * Copies I-PDU data to shadow buffer.
 * 
 * @param SignalGroupId Signal group identifier
 * @return E_OK if successful
 */
extern uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Update shadow signal
 * 
 * Updates a signal within a signal group's shadow buffer.
 * 
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to signal data
 * @return E_OK if successful
 */
extern uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Send signal group array
 * @param SignalGroupId Signal group identifier
 * @param SignalGroupArrayPtr Pointer to signal group data array
 * @return E_OK if successful
 */
extern uint8 Com_SendSignalGroupArray(Com_SignalGroupIdType SignalGroupId, const uint8* SignalGroupArrayPtr);

/**
 * @brief Receive signal group array
 * @param SignalGroupId Signal group identifier
 * @param SignalGroupArrayPtr Pointer to store signal group data
 * @return E_OK if successful
 */
extern uint8 Com_ReceiveSignalGroupArray(Com_SignalGroupIdType SignalGroupId, uint8* SignalGroupArrayPtr);
/** @} */

/**
 * @defgroup Com_MainFunction_API Main Functions
 * @{
 */
/**
 * @brief Main function for reception processing
 * 
 * Called cyclically to process received data and timeout monitoring.
 */
extern void Com_MainFunctionRx(void);

/**
 * @brief Main function for transmission processing
 * 
 * Called cyclically to process transmission requests and send I-PDUs.
 * Handles periodic transmission, triggered transmission, and retry logic.
 * 
 * @req SWS_Com_00016
 * @req SWS_Com_00583 (ASIL-D: Timeout detection)
 */
extern void Com_MainFunctionTx(void);

/**
 * @brief Main function for signal routing (gateway)
 * 
 * Called cyclically to route signals between I-PDUs.
 */
extern void Com_MainFunctionRouteSignals(void);
/** @} */

/**
 * @defgroup Com_Trigger_API Triggered Send
 * @{
 */
/**
 * @brief Trigger immediate I-PDU transmission
 * 
 * Schedules an I-PDU for immediate transmission regardless of its 
 * configured transmission mode. The actual transmission occurs in
 * the next call to Com_MainFunctionTx.
 * 
 * @param PduId I-PDU identifier
 * @return E_OK if triggered successfully, E_NOT_OK otherwise
 * 
 * @req SWS_Com_00024
 * @req SWS_Com_00190 (ASIL-D: Input validation)
 */
extern Std_ReturnType Com_TriggerIPDUSend(Com_IPduIdType PduId);

/**
 * @brief Trigger I-PDU transmission with metadata
 * 
 * Similar to Com_TriggerIPDUSend but includes metadata.
 * 
 * @param PduId I-PDU identifier
 * @param MetaData Pointer to metadata
 */
extern void Com_TriggerIPDUSendWithMetaData(Com_IPduIdType PduId, const uint8* MetaData);
/** @} */

/**
 * @defgroup Com_Invalidation_API Signal Invalidation
 * @{
 */
/**
 * @brief Invalidate a signal
 * 
 * Sets signal to its configured invalid value and triggers transmission
 * if configured.
 * 
 * @param SignalId Signal identifier
 * 
 * @req SWS_Com_00118
 */
extern void Com_InvalidateSignal(Com_SignalIdType SignalId);

/**
 * @brief Invalidate a signal group
 * 
 * Sets all signals in group to their configured invalid values.
 * 
 * @param SignalGroupId Signal group identifier
 */
extern void Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId);
/** @} */

/**
 * @defgroup Com_TxMode_API Transmission Mode Control
 * @{
 */
/**
 * @brief Switch I-PDU transmission mode
 * 
 * Switches between TRUE/FALSE mode (if configured).
 * 
 * @param PduId I-PDU identifier
 * @param Mode TRUE or FALSE mode
 */
extern void Com_SwitchIpduTxMode(Com_IPduIdType PduId, boolean Mode);
/** @} */

/**
 * @defgroup Com_TransmitQueue_API Send Request Queue (T009 Extension)
 * @{
 */
/**
 * @brief Get current send request queue fill level
 * 
 * Returns the number of pending send requests in the queue.
 * Useful for monitoring and diagnostics.
 * 
 * @return Number of pending requests
 */
extern uint8 Com_GetTxQueueFillLevel(void);

/**
 * @brief Clear all pending transmission requests for an I-PDU
 * 
 * Removes all pending send requests for the specified I-PDU.
 * Used when stopping an I-PDU group or on error recovery.
 * 
 * @param PduId I-PDU identifier
 */
extern void Com_ClearTxQueueForPdu(Com_IPduIdType PduId);
/** @} */

/*==================[Call-back Notifications]=============================*/

/* PduR to Com Interface */
extern void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/* Com Confirmation Interface */
extern void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/*==================[Scheduled Functions]=================================*/

#ifndef COM_MAIN_FUNCTION_RX_PERIOD
#define COM_MAIN_FUNCTION_RX_PERIOD 10u /* ms */
#endif

#ifndef COM_MAIN_FUNCTION_TX_PERIOD
#define COM_MAIN_FUNCTION_TX_PERIOD 10u /* ms */
#endif

#ifndef COM_MAIN_FUNCTION_SIGNAL_PERIOD
#define COM_MAIN_FUNCTION_SIGNAL_PERIOD 10u /* ms */
#endif

#endif /* COM_H */
