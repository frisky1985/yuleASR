/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Services Mock Header
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef MOCK_SERVICES_H
#define MOCK_SERVICES_H

#include "Std_Types.h"

/*==================================================================================================
*                                      COM MOCK
==================================================================================================*/
#define COM_MOCK_MAX_SIGNALS    256
#define COM_MOCK_MAX_IPDUS      128

typedef struct {
    boolean Updated;
    uint8 Data[64];
    uint8 Length;
    uint32 Timestamp;
} Com_MockSignalType;

extern Com_MockSignalType Com_MockSignals[COM_MOCK_MAX_SIGNALS];

typedef struct {
    boolean Active;
    uint8 Data[256];
    uint16 Length;
} Com_MockIpduType;

extern Com_MockIpduType Com_MockIpdus[COM_MOCK_MAX_IPDUS];

void Com_Mock_Reset(void);
void Com_Mock_SetSignalData(uint16 signalId, uint8* data, uint8 length);
void Com_Mock_GetSignalData(uint16 signalId, uint8* data, uint8* length);

/*==================================================================================================
*                                      PDUR MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint32 TxConfirmations;
    uint32 RxIndications;
    uint32 TriggerTransmitCount;
    uint8 LastPduId;
} PduR_MockStateType;

extern PduR_MockStateType PduR_MockState;

void PduR_Mock_Reset(void);
void PduR_Mock_SetRoutingResult(uint8 pduId, uint8 result);

/*==================================================================================================
*                                      NVM MOCK
==================================================================================================*/
#define NVM_MOCK_MAX_BLOCKS     256

typedef struct {
    boolean Valid;
    uint8 Data[1024];
    uint16 Length;
    uint16 WriteCounter;
    uint16 ReadCounter;
    uint8 JobResult;
} NvM_MockBlockType;

extern NvM_MockBlockType NvM_MockBlocks[NVM_MOCK_MAX_BLOCKS];

typedef struct {
    boolean Initialized;
    uint8 State;
    uint32 PendingJobs;
} NvM_MockStateType;

extern NvM_MockStateType NvM_MockState;

void NvM_Mock_Reset(void);
void NvM_Mock_SetBlockData(uint16 blockId, uint8* data, uint16 length);
Std_ReturnType NvM_Mock_GetBlockData(uint16 blockId, uint8* data, uint16* length);

/*==================================================================================================
*                                      DCM MOCK
==================================================================================================*/
#define DCM_MOCK_MAX_PROTOCOLS  4
#define DCM_MOCK_MAX_CHANNELS   4
#define DCM_MOCK_MAX_DIDS       256
#define DCM_MOCK_MAX_RIDS       256

typedef struct {
    uint8 ProtocolId;
    boolean Active;
    uint8 SessionLevel;
    uint8 SecurityLevel;
} Dcm_MockProtocolType;

extern Dcm_MockProtocolType Dcm_MockProtocols[DCM_MOCK_MAX_PROTOCOLS];

typedef struct {
    boolean Valid;
    uint16 DataLength;
    uint8 Data[256];
} Dcm_MockDidType;

extern Dcm_MockDidType Dcm_MockDids[DCM_MOCK_MAX_DIDS];

void Dcm_Mock_Reset(void);
void Dcm_Mock_SetProtocolSession(uint8 protocol, uint8 session);
void Dcm_Mock_SetDidData(uint16 did, uint8* data, uint16 length);

/*==================================================================================================
*                                      DEM MOCK
==================================================================================================*/
#define DEM_MOCK_MAX_EVENTS     512
#define DEM_MOCK_MAX_DTC        256

typedef enum {
    DEM_MOCK_EVENT_STATUS_PASSED = 0,
    DEM_MOCK_EVENT_STATUS_FAILED,
    DEM_MOCK_EVENT_STATUS_PENDING,
    DEM_MOCK_EVENT_STATUS_CONFIRMED
} Dem_MockEventStatusType;

typedef struct {
    boolean Valid;
    uint8 Status;
    uint8 FaultCounter;
    uint32 OccurrenceCounter;
    uint32 LastFailedTime;
} Dem_MockEventType;

extern Dem_MockEventType Dem_MockEvents[DEM_MOCK_MAX_EVENTS];

typedef struct {
    boolean Valid;
    uint32 DtcNumber;
    uint8 StatusByte;
    uint16 EventCount;
} Dem_MockDtcType;

extern Dem_MockDtcType Dem_MockDtcs[DEM_MOCK_MAX_DTC];

void Dem_Mock_Reset(void);
void Dem_Mock_SetEventStatus(uint16 eventId, uint8 status);
uint8 Dem_Mock_GetEventStatus(uint16 eventId);
void Dem_Mock_SetDtcStatus(uint32 dtcNumber, uint8 status);

#endif /* MOCK_SERVICES_H */
