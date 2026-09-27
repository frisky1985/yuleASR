/**
 * @file LinIf.h
 * @brief LIN Interface - AUTOSAR ECUAL Module
 * @version 2.1.0
 * @date 2026-09-26
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_LINInterface.pdf
 */

#ifndef LINIF_H
#define LINIF_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "LinIf_Cfg.h"

#define LINIF_AR_RELEASE_MAJOR_VERSION   4U
#define LINIF_AR_RELEASE_MINOR_VERSION   4U
#define LINIF_AR_RELEASE_REVISION_VERSION 0U
#define LINIF_SW_MAJOR_VERSION           1U
#define LINIF_SW_MINOR_VERSION           1U
#define LINIF_SW_PATCH_VERSION           0U
#define LINIF_MODULE_ID             0x27U
#define LINIF_VENDOR_ID             0x0055U

#define LINIF_UNCONDITIONAL_FRAME   0x00U
#define LINIF_EVENT_TRIGGERED_FRAME 0x01U
#define LINIF_SPORADIC_FRAME        0x02U
#define LINIF_DIAGNOSTIC_FRAME      0x03U

#define LINIF_NULL_SCHEDULE         0x00U
#define LINIF_Normal                0x01U

/* Channel State Machine */
typedef uint8 LinIf_ChannelStateType;
#define LINIF_CHANNEL_UNINIT        0x00U
#define LINIF_CHANNEL_INIT          0x01U
#define LINIF_CHANNEL_ONLINE        0x02U
#define LINIF_CHANNEL_SLEEP         0x03U

typedef uint8 LinIf_ScheduleTableType;

/* Frame Type */
typedef struct {
    uint8    FrameIdx;
    uint8    Pid;
    uint8    Dlc;
    uint8    FrameType;
    boolean  IsPublish;
} LinIf_FrameConfigType;

/* Schedule Entry */
typedef struct {
    uint16   DelayMs;
    uint8    FrameIdx;
} LinIf_ScheduleEntryType;

/* Schedule Table Config */
typedef struct {
    uint8    Schedule;
    uint8    EntryCount;
    const LinIf_ScheduleEntryType* Entries;
} LinIf_ScheduleTableConfigType;

/* Channel Config */
typedef struct {
    uint8    ChannelId;
    uint8    NumFrames;
    uint8    NumSchedules;
    const LinIf_FrameConfigType* Frames;
    const LinIf_ScheduleTableConfigType* Schedules;
} LinIf_ChannelConfigType;

/* PDU Type */
typedef struct {
    uint8 Id;
    uint8 Dlc;
    const uint8* DataPtr;
} LinIf_PduType;

/* TX PDU to frame mapping */
typedef struct {
    uint8 Channel;
    uint8 FrameIdx;
} LinIf_TxPduMapType;

/* Top Config */
typedef struct {
    uint8    NumChannels;
    const LinIf_ChannelConfigType* Channels;
    uint8    NumTxPdus;
    const LinIf_TxPduMapType* TxPduMap;
} LinIf_ConfigType;

extern const LinIf_ConfigType LinIf_Config;

void LinIf_Init(const LinIf_ConfigType* ConfigPtr);
void LinIf_DeInit(void);
Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType LinIf_SetSchedule(uint8 ScheduleTableId);
Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, LinIf_ScheduleTableType ScheduleTable);
Std_ReturnType LinIf_WakeUp(uint8 Channel);
Std_ReturnType LinIf_GotoSleep(uint8 Channel);
void LinIf_RxIndication(uint8 LinChannel, const LinIf_PduType* PduInfoPtr);
void LinIf_MainFunction(void);
void LinIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Upper layer callbacks. LinIf.c provides weak no-op defaults; the
 * integrating ECU overrides them (LinNm defines TxConfirmation and
 * ScheduleRequestConfirmation). */
void LinIf_TxConfirmation(uint8 Channel, uint8 LinTxPduId);
void LinIf_ScheduleRequestConfirmation(uint8 Channel, uint8 ScheduleIndex);
void LinIf_WakeUpConfirmation(uint8 Channel);
void LinIf_GotoSleepConfirmation(uint8 Channel);
void LinIf_RxCallback(uint8 Channel, const LinIf_PduType* PduInfoPtr);

#endif /* LINIF_H */
