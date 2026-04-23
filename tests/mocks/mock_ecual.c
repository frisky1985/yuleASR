/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : ECUAL Mock Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "mock_ecual.h"
#include <string.h>

/*==================================================================================================
*                                      CANIF MOCK VARIABLES
==================================================================================================*/
CanIf_MockStateType CanIf_MockState = {0};

/*==================================================================================================
*                                      CANTP MOCK VARIABLES
==================================================================================================*/
CanTp_MockChannelType CanTp_MockChannels[CANTP_MOCK_MAX_CHANNELS];

/*==================================================================================================
*                                      IOHWAB MOCK VARIABLES
==================================================================================================*/
IoHwAb_MockPinType IoHwAb_MockPins[IOHWAB_MOCK_MAX_PINS];

/*==================================================================================================
*                                      MEMIF MOCK VARIABLES
==================================================================================================*/
MemIf_MockStateType MemIf_MockState = MEMIF_UNINIT;
uint32 MemIf_MockReadCount = 0;
uint32 MemIf_MockWriteCount = 0;

/*==================================================================================================
*                                      FEE MOCK VARIABLES
==================================================================================================*/
Fee_MockBlockType Fee_MockBlocks[FEE_MOCK_MAX_BLOCKS];

/*==================================================================================================
*                                      EA MOCK VARIABLES
==================================================================================================*/
Ea_MockStateType Ea_MockState = {0};

/*==================================================================================================
*                                      ETHIF MOCK VARIABLES
==================================================================================================*/
EthIf_MockStateType EthIf_MockState = {0};

/*==================================================================================================
*                                      FRIF MOCK VARIABLES
==================================================================================================*/
FrIf_MockStateType FrIf_MockState = {0};

/*==================================================================================================
*                                      LINIF MOCK VARIABLES
==================================================================================================*/
LinIf_MockChannelType LinIf_MockChannels[LINIF_MOCK_MAX_CHANNELS];

/*==================================================================================================
*                                      CANIF MOCK FUNCTIONS
==================================================================================================*/
void CanIf_Mock_Reset(void)
{
    memset(&CanIf_MockState, 0, sizeof(CanIf_MockState));
}

void CanIf_Mock_SetPduMode(uint8 pduId, uint8 mode)
{
    (void)pduId;
    (void)mode;
}

/*==================================================================================================
*                                      CANTP MOCK FUNCTIONS
==================================================================================================*/
void CanTp_Mock_Reset(void)
{
    memset(CanTp_MockChannels, 0, sizeof(CanTp_MockChannels));
}

void CanTp_Mock_SetChannelState(uint8 channel, uint8 state)
{
    if (channel < CANTP_MOCK_MAX_CHANNELS)
    {
        CanTp_MockChannels[channel].State = state;
    }
}

/*==================================================================================================
*                                      IOHWAB MOCK FUNCTIONS
==================================================================================================*/
void IoHwAb_Mock_Reset(void)
{
    memset(IoHwAb_MockPins, 0, sizeof(IoHwAb_MockPins));
}

void IoHwAb_Mock_SetPinValue(uint8 pin, uint16 value)
{
    if (pin < IOHWAB_MOCK_MAX_PINS)
    {
        IoHwAb_MockPins[pin].AdcValue = value;
        IoHwAb_MockPins[pin].DIOValue = (value > 0) ? 1 : 0;
    }
}

uint16 IoHwAb_Mock_GetPinValue(uint8 pin)
{
    if (pin < IOHWAB_MOCK_MAX_PINS)
    {
        return IoHwAb_MockPins[pin].AdcValue;
    }
    return 0;
}

/*==================================================================================================
*                                      MEMIF MOCK FUNCTIONS
==================================================================================================*/
void MemIf_Mock_Reset(void)
{
    MemIf_MockState = MEMIF_UNINIT;
    MemIf_MockReadCount = 0;
    MemIf_MockWriteCount = 0;
}

void MemIf_Mock_SetJobResult(uint8 device, uint8 result)
{
    (void)device;
    (void)result;
}

/*==================================================================================================
*                                      FEE MOCK FUNCTIONS
==================================================================================================*/
void Fee_Mock_Reset(void)
{
    memset(Fee_MockBlocks, 0, sizeof(Fee_MockBlocks));
}

void Fee_Mock_SetBlockData(uint16 blockNumber, uint8* data, uint16 length)
{
    if (blockNumber < FEE_MOCK_MAX_BLOCKS)
    {
        /* Mock implementation - store data reference */
        Fee_MockBlocks[blockNumber].BlockLength = length;
        (void)data;
    }
}

/*==================================================================================================
*                                      EA MOCK FUNCTIONS
==================================================================================================*/
void Ea_Mock_Reset(void)
{
    memset(&Ea_MockState, 0, sizeof(Ea_MockState));
}

void Ea_Mock_SetReadData(uint8* data, uint16 length)
{
    Ea_MockState.DataPtr = data;
    Ea_MockState.Length = length;
}

/*==================================================================================================
*                                      ETHIF MOCK FUNCTIONS
==================================================================================================*/
void EthIf_Mock_Reset(void)
{
    memset(&EthIf_MockState, 0, sizeof(EthIf_MockState));
}

void EthIf_Mock_SetLinkState(uint8 state)
{
    EthIf_MockState.LinkState = state;
}

/*==================================================================================================
*                                      FRIF MOCK FUNCTIONS
==================================================================================================*/
void FrIf_Mock_Reset(void)
{
    memset(&FrIf_MockState, 0, sizeof(FrIf_MockState));
}

void FrIf_Mock_SetClusterState(uint8 cluster, uint8 state)
{
    (void)cluster;
    FrIf_MockState.ClusterState = state;
}

/*==================================================================================================
*                                      LINIF MOCK FUNCTIONS
==================================================================================================*/
void LinIf_Mock_Reset(void)
{
    memset(LinIf_MockChannels, 0, sizeof(LinIf_MockChannels));
}

void LinIf_Mock_SetChannelSchedule(uint8 channel, uint8 schedule)
{
    if (channel < LINIF_MOCK_MAX_CHANNELS)
    {
        LinIf_MockChannels[channel].Schedule = schedule;
    }
}
