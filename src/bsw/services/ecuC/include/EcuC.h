/**
 * @file EcuC.h
 * @brief EcuC - ECU Configuration Module
 * @version 1.0.0
 * @date 2024-05-05
 */

#ifndef ECUC_H
#define ECUC_H

/*==================[Includes]==============================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"

/*==================[Macros]================================================*/
#define ECUC_MODULE_ID                     150U
#define ECUC_VENDOR_ID                     0x0001U

#define ECUC_SW_MAJOR_VERSION              1U
#define ECUC_SW_MINOR_VERSION              0U
#define ECUC_SW_PATCH_VERSION              0U

/* Error Codes */
#define ECUC_E_NO_ERROR                    0x00U
#define ECUC_E_PARAM_POINTER               0x01U
#define ECUC_E_PARAM_CONFIG                0x02U
#define ECUC_E_UNINIT                      0x03U
#define ECUC_E_INIT_FAILED                 0x04U
#define ECUC_E_INVALID_PDU_ID              0x05U
#define ECUC_E_INVALID_SIGNAL_ID           0x06U

/* Service IDs */
#define ECUC_SID_INIT                      0x01U
#define ECUC_SID_DEINIT                    0x02U
#define ECUC_SID_GET_VERSION_INFO          0x03U
#define ECUC_SID_TRANSMIT_SIGNAL           0x04U
#define ECUC_SID_RECEIVE_SIGNAL            0x05U
#define ECUC_SID_UPDATE_SHADOW_SIGNAL      0x06U
#define ECUC_SID_RECEIVE_SHADOW_SIGNAL     0x07U
#define ECUC_SID_SEND_SIGNAL               0x08U
#define ECUC_SID_SEND_SHADOW_SIGNAL        0x09U

/*==================[Type Definitions]======================================*/

/* EcuC Initialization Status */
typedef enum {
    ECUC_STATE_UNINIT = 0,
    ECUC_STATE_INIT
} EcuC_StateType;

/* Signal Transfer Property */
typedef enum {
    ECUC_SIGNAL_PENDING = 0,
    ECUC_SIGNAL_TRIGGERED,
    ECUC_SIGNAL_TRIGGERED_ON_CHANGE,
    ECUC_SIGNAL_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION
} EcuC_SignalTransferPropertyType;

/* Signal Direction */
typedef enum {
    ECUC_SEND = 0,
    ECUC_RECEIVE
} EcuC_SignalDirectionType;

/* Signal Configuration */
typedef struct {
    uint16 SignalId;
    uint16 SignalSize;
    uint16 SignalStartBit;
    uint16 SignalBitOrder;
    EcuC_SignalTransferPropertyType TransferProperty;
    EcuC_SignalDirectionType Direction;
    PduIdType RelatedPduId;
} EcuC_SignalConfigType;

/* PDU Configuration */
typedef struct {
    PduIdType PduId;
    uint16 PduLength;
    uint16 SignalCount;
    const EcuC_SignalConfigType* Signals;
} EcuC_PduConfigType;

/* Gateway Routing Configuration */
typedef struct {
    PduIdType SourcePduId;
    PduIdType DestinationPduId;
    uint16 SignalCount;
    const uint16* SignalMapping;
} EcuC_RoutingPathType;

/* EcuC Configuration */
typedef struct {
    uint16 PduCount;
    uint16 SignalCount;
    uint16 RoutingPathCount;
    const EcuC_PduConfigType* Pdus;
    const EcuC_SignalConfigType* Signals;
    const EcuC_RoutingPathType* RoutingPaths;
} EcuC_ConfigType;

/*==================[Function Prototypes]===================================*/
void EcuC_Init(const EcuC_ConfigType* ConfigPtr);
void EcuC_DeInit(void);
#if (ECUC_VERSION_INFO_API == STD_ON)
void EcuC_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

Std_ReturnType EcuC_TransmitSignal(uint16 SignalId, const void* SignalDataPtr);
Std_ReturnType EcuC_ReceiveSignal(uint16 SignalId, void* SignalDataPtr);
Std_ReturnType EcuC_UpdateShadowSignal(uint16 SignalId, const void* SignalDataPtr);
Std_ReturnType EcuC_ReceiveShadowSignal(uint16 SignalId, void* SignalDataPtr);
Std_ReturnType EcuC_SendSignal(uint16 SignalId, const void* SignalDataPtr);
Std_ReturnType EcuC_SendShadowSignal(uint16 SignalId);

/* Rx/Tx Confirmation Callbacks */
void EcuC_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void EcuC_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result);
void EcuC_TpRxIndication(PduIdType RxPduId, Std_ReturnType Result);
void EcuC_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

/* Main Function */
void EcuC_MainFunction(void);

/*==================[External Declarations]=================================*/
extern const EcuC_ConfigType EcuC_Config;

#endif /* ECUC_H */
