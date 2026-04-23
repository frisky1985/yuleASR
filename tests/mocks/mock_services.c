/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Services Mock Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "mock_services.h"
#include <string.h>

/*==================================================================================================
*                                      COM MOCK VARIABLES
==================================================================================================*/
Com_MockSignalType Com_MockSignals[COM_MOCK_MAX_SIGNALS];
Com_MockIpduType Com_MockIpdus[COM_MOCK_MAX_IPDUS];

/*==================================================================================================
*                                      PDUR MOCK VARIABLES
==================================================================================================*/
PduR_MockStateType PduR_MockState = {0};

/*==================================================================================================
*                                      NVM MOCK VARIABLES
==================================================================================================*/
NvM_MockBlockType NvM_MockBlocks[NVM_MOCK_MAX_BLOCKS];
NvM_MockStateType NvM_MockState = {0};

/*==================================================================================================
*                                      DCM MOCK VARIABLES
==================================================================================================*/
Dcm_MockProtocolType Dcm_MockProtocols[DCM_MOCK_MAX_PROTOCOLS];
Dcm_MockDidType Dcm_MockDids[DCM_MOCK_MAX_DIDS];

/*==================================================================================================
*                                      DEM MOCK VARIABLES
==================================================================================================*/
Dem_MockEventType Dem_MockEvents[DEM_MOCK_MAX_EVENTS];
Dem_MockDtcType Dem_MockDtcs[DEM_MOCK_MAX_DTC];

/*==================================================================================================
*                                      COM MOCK FUNCTIONS
==================================================================================================*/
void Com_Mock_Reset(void)
{
    memset(Com_MockSignals, 0, sizeof(Com_MockSignals));
    memset(Com_MockIpdus, 0, sizeof(Com_MockIpdus));
}

void Com_Mock_SetSignalData(uint16 signalId, uint8* data, uint8 length)
{
    if (signalId < COM_MOCK_MAX_SIGNALS && length <= 64)
    {
        memcpy(Com_MockSignals[signalId].Data, data, length);
        Com_MockSignals[signalId].Length = length;
        Com_MockSignals[signalId].Updated = TRUE;
    }
}

void Com_Mock_GetSignalData(uint16 signalId, uint8* data, uint8* length)
{
    if (signalId < COM_MOCK_MAX_SIGNALS && data != NULL && length != NULL)
    {
        uint8 copyLen = (*length < Com_MockSignals[signalId].Length) ? 
                        *length : Com_MockSignals[signalId].Length;
        memcpy(data, Com_MockSignals[signalId].Data, copyLen);
        *length = copyLen;
    }
}

/*==================================================================================================
*                                      PDUR MOCK FUNCTIONS
==================================================================================================*/
void PduR_Mock_Reset(void)
{
    memset(&PduR_MockState, 0, sizeof(PduR_MockState));
}

void PduR_Mock_SetRoutingResult(uint8 pduId, uint8 result)
{
    (void)pduId;
    (void)result;
}

/*==================================================================================================
*                                      NVM MOCK FUNCTIONS
==================================================================================================*/
void NvM_Mock_Reset(void)
{
    memset(NvM_MockBlocks, 0, sizeof(NvM_MockBlocks));
    memset(&NvM_MockState, 0, sizeof(NvM_MockState));
}

void NvM_Mock_SetBlockData(uint16 blockId, uint8* data, uint16 length)
{
    if (blockId < NVM_MOCK_MAX_BLOCKS && length <= 1024)
    {
        memcpy(NvM_MockBlocks[blockId].Data, data, length);
        NvM_MockBlocks[blockId].Length = length;
        NvM_MockBlocks[blockId].Valid = TRUE;
        NvM_MockBlocks[blockId].WriteCounter++;
    }
}

Std_ReturnType NvM_Mock_GetBlockData(uint16 blockId, uint8* data, uint16* length)
{
    if (blockId < NVM_MOCK_MAX_BLOCKS && NvM_MockBlocks[blockId].Valid && 
        data != NULL && length != NULL)
    {
        uint16 copyLen = (*length < NvM_MockBlocks[blockId].Length) ? 
                         *length : NvM_MockBlocks[blockId].Length;
        memcpy(data, NvM_MockBlocks[blockId].Data, copyLen);
        *length = copyLen;
        NvM_MockBlocks[blockId].ReadCounter++;
        return E_OK;
    }
    return E_NOT_OK;
}

/*==================================================================================================
*                                      DCM MOCK FUNCTIONS
==================================================================================================*/
void Dcm_Mock_Reset(void)
{
    memset(Dcm_MockProtocols, 0, sizeof(Dcm_MockProtocols));
    memset(Dcm_MockDids, 0, sizeof(Dcm_MockDids));
}

void Dcm_Mock_SetProtocolSession(uint8 protocol, uint8 session)
{
    if (protocol < DCM_MOCK_MAX_PROTOCOLS)
    {
        Dcm_MockProtocols[protocol].SessionLevel = session;
        Dcm_MockProtocols[protocol].Active = TRUE;
    }
}

void Dcm_Mock_SetDidData(uint16 did, uint8* data, uint16 length)
{
    if (did < DCM_MOCK_MAX_DIDS && length <= 256)
    {
        memcpy(Dcm_MockDids[did].Data, data, length);
        Dcm_MockDids[did].DataLength = length;
        Dcm_MockDids[did].Valid = TRUE;
    }
}

/*==================================================================================================
*                                      DEM MOCK FUNCTIONS
==================================================================================================*/
void Dem_Mock_Reset(void)
{
    memset(Dem_MockEvents, 0, sizeof(Dem_MockEvents));
    memset(Dem_MockDtcs, 0, sizeof(Dem_MockDtcs));
}

void Dem_Mock_SetEventStatus(uint16 eventId, uint8 status)
{
    if (eventId < DEM_MOCK_MAX_EVENTS)
    {
        Dem_MockEvents[eventId].Valid = TRUE;
        Dem_MockEvents[eventId].Status = status;
        if (status == DEM_MOCK_EVENT_STATUS_FAILED)
        {
            Dem_MockEvents[eventId].FaultCounter++;
            Dem_MockEvents[eventId].OccurrenceCounter++;
        }
    }
}

uint8 Dem_Mock_GetEventStatus(uint16 eventId)
{
    if (eventId < DEM_MOCK_MAX_EVENTS)
    {
        return Dem_MockEvents[eventId].Status;
    }
    return DEM_MOCK_EVENT_STATUS_PASSED;
}

void Dem_Mock_SetDtcStatus(uint32 dtcNumber, uint8 status)
{
    for (uint16 i = 0; i < DEM_MOCK_MAX_DTC; i++)
    {
        if (!Dem_MockDtcs[i].Valid)
        {
            Dem_MockDtcs[i].Valid = TRUE;
            Dem_MockDtcs[i].DtcNumber = dtcNumber;
            Dem_MockDtcs[i].StatusByte = status;
            return;
        }
    }
}
