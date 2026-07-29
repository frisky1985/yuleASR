/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Lin.h
 * @brief LIN Driver
 * @version 1.0.0
 * 
 * LIN (Local Interconnect Network) driver for AUTOSAR MCAL layer.
 * Supports LIN 1.x, 2.x protocols.
 */

#ifndef LIN_H
#define LIN_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define LIN_AR_RELEASE_MAJOR_VERSION       4
#define LIN_AR_RELEASE_MINOR_VERSION       0
#define LIN_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define LIN_SW_MAJOR_VERSION               1
#define LIN_SW_MINOR_VERSION               0
#define LIN_SW_PATCH_VERSION               0

/* Module ID */
#define LIN_MODULE_ID                      0x52

/* Service IDs */
#define LIN_INIT_SID                       0x00
#define LIN_DEINIT_SID                     0x01
#define LIN_GETVERSIONINFO_SID             0x02
#define LIN_SENDFRAME_SID                  0x03
#define LIN_SENDRESPONSE_SID               0x04
#define LIN_DISABLERESPONSE_SID            0x05
#define LIN_WAKEUP_SID                     0x06
#define LIN_GETSTATUS_SID                  0x07
#define LIN_GOTOSLEEP_SID                  0x08
#define LIN_WAKEUPINTERNAL_SID             0x09
#define LIN_CHECKWAKEUP_SID                0x0A

/* Error Codes */
#define LIN_E_UNINIT                       0x00
#define LIN_E_INVALID_CHANNEL              0x01
#define LIN_E_INVALID_POINTER              0x02
#define LIN_E_STATE_TRANSITION             0x03
#define LIN_E_PARAM_VALUE                  0x04
#define LIN_E_PARAM_POINTER                0x05

/* Status Types */
typedef uint8 Lin_StatusType;
#define LIN_NOT_OK                         0x00
#define LIN_TX_OK                          0x01
#define LIN_TX_BUSY                        0x02
#define LIN_TX_HEADER_ERROR                0x03
#define LIN_TX_ERROR                       0x04
#define LIN_RX_OK                          0x05
#define LIN_RX_BUSY                        0x06
#define LIN_RX_ERROR                       0x07
#define LIN_RX_NO_RESPONSE                 0x08
#define LIN_OPERATIONAL                    0x09
#define LIN_CH_SLEEP                       0x0A

/* Frame Types */
typedef uint8 Lin_FrameTypeType;
#define LIN_FRAMETYPE_UNCONDITIONAL        0x00
#define LIN_FRAMETYPE_EVENT_TRIGGERED      0x01
#define LIN_FRAMETYPE_SPORADIC             0x02
#define LIN_FRAMETYPE_DIAGNOSTIC           0x03
#define LIN_FRAMETYPE_USER_DEFINED         0x04

/* Frame Response Types */
typedef uint8 Lin_FrameResponseType;
#define LIN_MASTER_RESPONSE                0x00
#define LIN_SLAVE_RESPONSE                 0x01
#define LIN_SLAVE_TO_SLAVE                 0x02

/* Checksum Types */
typedef uint8 Lin_FrameCheckSumType;
#define LIN_CLASSIC_CS                     0x00
#define LIN_ENHANCED_CS                    0x01

/* PID Types */
typedef uint8 Lin_FramePidType;

/* Channel Type */
typedef uint8 Lin_ChannelType;

/* Frame Structure */
typedef struct {
    Lin_FramePidType Pid;
    Lin_FrameTypeType FrameType;
    Lin_FrameResponseType FrameResponse;
    uint8 Length;
    Lin_FrameCheckSumType ChecksumType;
    uint8* SduPtr;
} Lin_PduType;

/* Configuration Types */
typedef struct {
    uint32 LinChannelBaudRate;
    uint8 LinChannelId;
    boolean LinChannelWakeupSupport;
    uint32 LinChannelSleepMode;
} Lin_ChannelConfigType;

typedef struct {
    const Lin_ChannelConfigType* ChannelConfigPtr;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} Lin_ConfigType;

/* Function Prototypes */
extern void Lin_Init(const Lin_ConfigType* Config);
extern void Lin_DeInit(void);
extern void Lin_GetVersionInfo(Std_VersionInfoType* versioninfo);

extern Std_ReturnType Lin_SendFrame(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr);
extern Std_ReturnType Lin_SendResponse(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr);
extern Std_ReturnType Lin_DisableResponse(Lin_ChannelType Channel);

extern Std_ReturnType Lin_WakeUp(Lin_ChannelType Channel);
extern Std_ReturnType Lin_WakeUpInternal(Lin_ChannelType Channel);
extern Std_ReturnType Lin_CheckWakeup(Lin_ChannelType Channel);

extern Lin_StatusType Lin_GetStatus(Lin_ChannelType Channel, uint8** Lin_SduPtr);
extern Std_ReturnType Lin_GoToSleep(Lin_ChannelType Channel);
extern Std_ReturnType Lin_GoToSleepInternal(Lin_ChannelType Channel);

/* Callbacks */
extern void Lin_WakeUpConfirmation(Lin_ChannelType Channel);
extern void Lin_WakeUpFrameIndication(void);

#define LIN_START_SEC_CODE
#include "MemMap.h"

/* ISR declarations - to be defined by implementation */
extern void Lin_IsrTx(Lin_ChannelType Channel);
extern void Lin_IsrRx(Lin_ChannelType Channel);
extern void Lin_IsrErr(Lin_ChannelType Channel);

#define LIN_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LIN_H */
