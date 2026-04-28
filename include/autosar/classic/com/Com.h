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

/* Initialization and General Functions */
extern void Com_Init(const Com_ConfigType* config);
extern void Com_DeInit(void);
extern Com_StatusType Com_GetStatus(void);
extern void Com_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* IPdu Group Control */
extern void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);
extern void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);

/* Signal Operations */
extern uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
extern uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);

/* Signal Group Operations */
extern uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);
extern uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);
extern uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
extern uint8 Com_SendSignalGroupArray(Com_SignalGroupIdType SignalGroupId, const uint8* SignalGroupArrayPtr);
extern uint8 Com_ReceiveSignalGroupArray(Com_SignalGroupIdType SignalGroupId, uint8* SignalGroupArrayPtr);

/* Main Functions */
extern void Com_MainFunctionRx(void);
extern void Com_MainFunctionTx(void);
extern void Com_MainFunctionRouteSignals(void);

/* Triggered Send */
extern Std_ReturnType Com_TriggerIPDUSend(Com_IPduIdType PduId);
extern void Com_TriggerIPDUSendWithMetaData(Com_IPduIdType PduId, const uint8* MetaData);

/* Reception Deadline Monitoring */
extern void Com_InvalidateSignal(Com_SignalIdType SignalId);
extern void Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId);

/* Tx Mode Selection */
extern void Com_SwitchIpduTxMode(Com_IPduIdType PduId, boolean Mode);

/*==================[Call-back Notifications]=============================*/

/* PduR to Com Interface */
extern void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

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
