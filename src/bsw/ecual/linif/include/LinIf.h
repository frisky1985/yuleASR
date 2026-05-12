/**
 * @file LinIf.h
 * @brief LIN Interface Module
 * @version 1.0.0
 */

#ifndef LINIF_H
#define LINIF_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define LINIF_MODULE_ID         62U
#define LINIF_VENDOR_ID         0x0001U

/* Error Codes */
#define LINIF_E_NO_ERROR        0x00U
#define LINIF_E_PARAM_POINTER   0x01U
#define LINIF_E_UNINIT          0x02U
#define LINIF_E_INVALID_CHANNEL 0x03U
#define LINIF_E_INVALID_PDU     0x04U

/* Service IDs */
#define LINIF_SID_INIT                  0x01U
#define LINIF_SID_DEINIT                0x02U
#define LINIF_SID_GET_VERSION_INFO      0x03U
#define LINIF_SID_TRANSMIT              0x04U
#define LINIF_SID_SCHEDULE_REQUEST      0x05U
#define LINIF_SID_WAKEUP                0x06U
#define LINIF_SID_GOTOSLEEP             0x07U
#define LINIF_SID_MAIN_FUNCTION         0x08U

/* Schedule Types */
typedef enum {
    LINIF_RUN_CONTINUOUS = 0,
    LINIF_RUN_ONCE
} LinIf_ScheduleRunModeType;

typedef enum {
    LINIF_NULL_SCHEDULE = 0,
    LINIF_DIAGRequest,
    LINIF_DIAGResponse,
    LINIF_MASTERReqSchedule,
    LINIF_SlaveRespSchedule,
    LINIF_Normal,
    LINIF_Master,
    LINIF_Sporadic
} LinIf_ScheduleTableType;

/* Frame Type */
typedef enum {
    LINIF_UNCONDITIONAL_FRAME = 0,
    LINIF_EVENT_TRIGGERED_FRAME,
    LINIF_SPORADIC_FRAME
} LinIf_FrameType;

/* Frame Configuration */
typedef struct {
    uint16 FrameIdx;
    uint8 Pid;
    uint8 Dlc;
    LinIf_FrameType FrameType;
    boolean IsPublish;
} LinIf_FrameConfigType;

/* Schedule Entry */
typedef struct {
    uint16 Delay;
    uint16 FrameIdx;
} LinIf_ScheduleEntryType;

/* Schedule Table */
typedef struct {
    LinIf_ScheduleTableType Schedule;
    uint8 EntryCount;
    const LinIf_ScheduleEntryType* Entries;
} LinIf_ScheduleTableConfigType;

/* Channel Configuration */
typedef struct {
    uint8 ChannelId;
    uint8 NumFrames;
    uint8 NumSchedules;
    const LinIf_FrameConfigType* Frames;
    const LinIf_ScheduleTableConfigType* Schedules;
} LinIf_ChannelConfigType;

/* Configuration */
typedef struct {
    uint8 NumChannels;
    const LinIf_ChannelConfigType* Channels;
} LinIf_ConfigType;

/* Functions */
void LinIf_Init(const LinIf_ConfigType* ConfigPtr);
void LinIf_DeInit(void);
#if (LINIF_VERSION_INFO_API == STD_ON)
void LinIf_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif
Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, LinIf_ScheduleTableType Schedule);
Std_ReturnType LinIf_WakeUp(uint8 Channel);
Std_ReturnType LinIf_GotoSleep(uint8 Channel);
void LinIf_MainFunction(void);

/* Callbacks */
void LinIf_RxIndication(uint8 Channel, uint8* Data, uint8 Length);
void LinIf_TxConfirmation(uint8 Channel, Std_ReturnType Result);
void LinIf_WakeUpConfirmation(uint8 Channel, boolean Success);

#endif
