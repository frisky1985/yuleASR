/**
 * @file LinIf.h
 * @brief LIN Interface - AUTOSAR ECUAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_LINInterface.pdf
 */

#ifndef LINIF_H
#define LINIF_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "LinIf_Cfg.h"

#define LINIF_MODULE_ID             0x27U
#define LINIF_VENDOR_ID             0x0055U

#define LINIF_UNCONDITIONAL_FRAME   0x00U
#define LINIF_EVENT_TRIGGERED_FRAME 0x01U
#define LINIF_SPORADIC_FRAME        0x02U
#define LINIF_DIAGNOSTIC_FRAME      0x03U

#define LINIF_NULL_SCHEDULE         0x00U
#define LINIF_Normal                0x01U

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

/* Callbacks */
typedef void (*LinIf_TransmitCallback)(PduIdType PduId);

/* Top Config */
typedef struct {
    uint8    NumChannels;
    const LinIf_ChannelConfigType* Channels;
} LinIf_ConfigType;

void LinIf_Init(const LinIf_ConfigType* ConfigPtr);
void LinIf_DeInit(void);
Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType LinIf_SetSchedule(uint8 ScheduleTableId);
void LinIf_RxIndication(uint8 LinChannel, const LinIf_PduType* PduInfoPtr);
void LinIf_MainFunction(void);
void LinIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* LINIF_H */