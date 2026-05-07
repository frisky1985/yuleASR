/**
 * @file Com.h
 * @brief Communication Module
 * @version 1.0.0
 */

#ifndef COM_H
#define COM_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define COM_MODULE_ID           50U
#define COM_VENDOR_ID           0x0001U

/* Error Codes */
#define COM_E_NO_ERROR          0x00U
#define COM_E_PARAM_POINTER     0x01U
#define COM_E_UNINIT            0x02U
#define COM_E_PARAM_LENGTH      0x03U
#define COM_E_PARAM_SIGNALID    0x04U

/* Service IDs */
#define COM_SID_INIT                    0x01U
#define COM_SID_DEINIT                  0x02U
#define COM_SID_GET_VERSION_INFO        0x03U
#define COM_SID_SEND_SIGNAL             0x04U
#define COM_SID_RECEIVE_SIGNAL          0x05U
#define COM_SID_SEND_SIGNAL_GROUP       0x06U
#define COM_SID_RECEIVE_SIGNAL_GROUP    0x07U
#define COM_SID_IPDU_GROUP_CONTROL      0x08U
#define COM_SID_MAIN_FUNCTION           0x09U

/* Transmission Modes */
typedef enum {
    COM_DIRECT = 0,
    COM_PERIODIC,
    COM_MIXED
} Com_TransferPropertyType;

/* Signal Type */
typedef struct {
    uint16 SignalId;
    uint16 BitPosition;
    uint8 BitSize;
    uint8 ByteOrder; /* 0=Little, 1=Big */
    Com_TransferPropertyType TransferProperty;
    uint16 UpdateBitPosition;
    boolean HasUpdateBit;
} Com_SignalConfigType;

/* I-PDU Configuration */
typedef struct {
    PduIdType PduId;
    uint16 Length;
    uint8 NumSignals;
    const Com_SignalConfigType* Signals;
    Com_TransferPropertyType TransferProperty;
    uint16 Period;
} Com_IPduConfigType;

/* I-PDU Group */
typedef struct {
    uint8 GroupId;
    uint8 NumIPdus;
    const PduIdType* IPdus;
    boolean IsStarted;
} Com_IPduGroupType;

/* Configuration */
typedef struct {
    uint16 NumSignals;
    uint16 NumIPdus;
    uint8 NumGroups;
    const Com_SignalConfigType* Signals;
    const Com_IPduConfigType* IPdus;
    const Com_IPduGroupType* Groups;
} Com_ConfigType;

/* Functions */
void Com_Init(const Com_ConfigType* ConfigPtr);
void Com_DeInit(void);
#if (COM_VERSION_INFO_API == STD_ON)
void Com_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif
uint8 Com_SendSignal(uint16 SignalId, const void* SignalDataPtr);
uint8 Com_ReceiveSignal(uint16 SignalId, void* SignalDataPtr);
void Com_IpduGroupControl(uint8 GroupId, boolean Start);
void Com_MainFunction(void);

/* Callbacks */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result);

#endif
