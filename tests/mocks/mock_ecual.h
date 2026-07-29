/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : ECUAL Mock Header
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef MOCK_ECUAL_H
#define MOCK_ECUAL_H

#include "Std_Types.h"

/*==================================================================================================
*                                      CANIF MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint32 TxConfirmations;
    uint32 RxIndications;
    uint32 BusOffNotifications;
    uint8 LastPduId;
} CanIf_MockStateType;

extern CanIf_MockStateType CanIf_MockState;

void CanIf_Mock_Reset(void);
void CanIf_Mock_SetPduMode(uint8 pduId, uint8 mode);

/*==================================================================================================
*                                      CANTP MOCK
==================================================================================================*/
#define CANTP_MOCK_MAX_CHANNELS     16

typedef struct {
    boolean Active;
    uint8 State;
    uint16 TxLength;
    uint16 RxLength;
} CanTp_MockChannelType;

extern CanTp_MockChannelType CanTp_MockChannels[CANTP_MOCK_MAX_CHANNELS];

void CanTp_Mock_Reset(void);
void CanTp_Mock_SetChannelState(uint8 channel, uint8 state);

/*==================================================================================================
*                                      IOHWAB MOCK
==================================================================================================*/
#define IOHWAB_MOCK_MAX_PINS        128

typedef struct {
    uint16 AdcValue;
    uint8 DIOValue;
    uint8 PWMValue;
    boolean Initialized;
} IoHwAb_MockPinType;

extern IoHwAb_MockPinType IoHwAb_MockPins[IOHWAB_MOCK_MAX_PINS];

void IoHwAb_Mock_Reset(void);
void IoHwAb_Mock_SetPinValue(uint8 pin, uint16 value);
uint16 IoHwAb_Mock_GetPinValue(uint8 pin);

/*==================================================================================================
*                                      MEMIF MOCK
==================================================================================================*/
typedef enum {
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY
} MemIf_MockStateType;

extern MemIf_MockStateType MemIf_MockState;
extern uint32 MemIf_MockReadCount;
extern uint32 MemIf_MockWriteCount;

void MemIf_Mock_Reset(void);
void MemIf_Mock_SetJobResult(uint8 device, uint8 result);

/*==================================================================================================
*                                      FEE MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint16 BlockOffset;
    uint16 BlockLength;
    uint8 JobResult;
} Fee_MockBlockType;

#define FEE_MOCK_MAX_BLOCKS     64
extern Fee_MockBlockType Fee_MockBlocks[FEE_MOCK_MAX_BLOCKS];

void Fee_Mock_Reset(void);
void Fee_Mock_SetBlockData(uint16 blockNumber, uint8* data, uint16 length);

/*==================================================================================================
*                                      EA MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint8 JobResult;
    uint8* DataPtr;
    uint16 Length;
} Ea_MockStateType;

extern Ea_MockStateType Ea_MockState;

void Ea_Mock_Reset(void);
void Ea_Mock_SetReadData(uint8* data, uint16 length);

/*==================================================================================================
*                                      ETHIF MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint32 TxFrames;
    uint32 RxFrames;
    uint8 LinkState;
} EthIf_MockStateType;

extern EthIf_MockStateType EthIf_MockState;

void EthIf_Mock_Reset(void);
void EthIf_Mock_SetLinkState(uint8 state);

/*==================================================================================================
*                                      FRIF MOCK
==================================================================================================*/
typedef struct {
    boolean Initialized;
    uint8 ClusterState;
    uint32 TxFrames;
    uint32 RxFrames;
} FrIf_MockStateType;

extern FrIf_MockStateType FrIf_MockState;

void FrIf_Mock_Reset(void);
void FrIf_Mock_SetClusterState(uint8 cluster, uint8 state);

/*==================================================================================================
*                                      LINIF MOCK
==================================================================================================*/
#define LINIF_MOCK_MAX_CHANNELS     8

typedef struct {
    boolean Active;
    uint8 State;
    uint8 Schedule;
    uint8 FrameIndex;
} LinIf_MockChannelType;

extern LinIf_MockChannelType LinIf_MockChannels[LINIF_MOCK_MAX_CHANNELS];

void LinIf_Mock_Reset(void);
void LinIf_Mock_SetChannelSchedule(uint8 channel, uint8 schedule);

#endif /* MOCK_ECUAL_H */
