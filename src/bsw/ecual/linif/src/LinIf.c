/** @file LinIf.c
 *  @brief LIN Interface implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_LINInterface.pdf
 */

#include "LinIf.h"
#include "LinIf_Cfg.h"
#include "Lin.h"
#include "LinTrcv.h"
#include "Det.h"
#include <string.h>

/* Version check */
#if defined(LINIF_AR_RELEASE_MAJOR_VERSION) && (LINIF_AR_RELEASE_MAJOR_VERSION != 4u)
#error "LinIf: AR major mismatch"
#endif
#if defined(LINIF_AR_RELEASE_MINOR_VERSION) && (LINIF_AR_RELEASE_MINOR_VERSION != 4u)
#error "LinIf: AR minor mismatch"
#endif

#define LINIF_SID_INIT              0x00U
#define LINIF_SID_DEINIT            0x01U
#define LINIF_SID_TRANSMIT          0x02U
#define LINIF_SID_RX_INDICATION     0x03U
#define LINIF_SID_MAINFUNCTION      0x04U
#define LINIF_SID_SCHEDULE          0x05U
#define LINIF_SID_GETVERSIONINFO    0x08U
#define LINIF_SID_WAKEUP            0x09U
#define LINIF_SID_GOTOSLEEP         0x0AU
#define LINIF_SID_SCHEDULEREQUEST   0x0BU

/* LINIF_E_PARAM_POINTER (0x10) and LINIF_E_PARAM_CHANNEL (0x50) are exported
 * in LinIf.h. */
#define LINIF_E_UNINIT              0x20U
#define LINIF_E_PARAM_PDU           0x30U
#define LINIF_E_PARAM_SCHEDULE      0x40U

/* Node configuration runtime limits (see AD2: NAD/PID live in the channel
 * runtime, not in static config tables). The per-channel PID table is capped
 * by the frame-array capacity of the configuration. */
#define LINIF_MAX_CHANNEL_FRAMES    LINIF_MAX_FRAMES
#define LINIF_DEFAULT_NAD           0x60U

/* Diagnostic frames use the classic checksum, all other identifiers the
 * enhanced checksum (LIN 2.x, SWS_LinIf_00period). */
#define LINIF_PID_DIAG_MASTER       0x3CU
#define LINIF_PID_DIAG_SLAVE        0x3DU

#if defined(__GNUC__)
#define LINIF_WEAK __attribute__((weak))
#else
#define LINIF_WEAK
#endif

/* Default upper layer hooks: no-ops unless the ECU overrides them. */
LINIF_WEAK void LinIf_TxConfirmation(uint8 Channel, uint8 LinTxPduId)
{
    (void)Channel; (void)LinTxPduId;
}
LINIF_WEAK void LinIf_ScheduleRequestConfirmation(uint8 Channel, uint8 ScheduleIndex)
{
    (void)Channel; (void)ScheduleIndex;
}
LINIF_WEAK void LinIf_WakeUpConfirmation(uint8 Channel)
{
    (void)Channel;
}
LINIF_WEAK void LinIf_GotoSleepConfirmation(uint8 Channel)
{
    (void)Channel;
}
LINIF_WEAK void LinIf_RxCallback(uint8 Channel, const LinIf_PduType* PduInfoPtr)
{
    (void)Channel; (void)PduInfoPtr;
}

typedef struct {
    LinIf_ChannelStateType state;
    uint8 currentSchedule;
    uint8 requestedSchedule;
    boolean scheduleSwitchPending;
    uint8 currentEntry;
    uint16 entryDelayCounter;
    uint8 txPduId[LINIF_MAX_FRAMES];
    uint8 txBuffer[LINIF_MAX_FRAMES][LINIF_MAX_FRAME_LENGTH];
    boolean txPending[LINIF_MAX_FRAMES];
    uint8 rxBuffer[LINIF_MAX_FRAMES][LINIF_MAX_FRAME_LENGTH];
    boolean rxValid[LINIF_MAX_FRAMES];
    /* Node configuration runtime state (AD2) */
    uint8 configuredNad;
    uint8 pidFrameCount;
    uint8 pidTable[LINIF_MAX_CHANNEL_FRAMES];
    /* Tracked because the LinTrcv driver exposes no SetWakeupMode API */
    LinIf_TrcvWakeupModeType trcvWakeupMode;
} LinIf_ChannelRuntimeType;

static struct {
    const LinIf_ConfigType* configPtr;
    LinIf_ChannelRuntimeType channels[LINIF_MAX_CHANNELS];
} LinIf_State;

/* (Re)establish the node configuration defaults for one channel: NAD 0x60,
 * wake-up enabled and the PID table copied from the configured frames. */
static void LinIf_ResetChannelNodeConfig(uint8 Channel, const LinIf_ChannelConfigType* ch)
{
    LinIf_ChannelRuntimeType* rt = &LinIf_State.channels[Channel];
    uint8 i;
    uint8 count = 0U;

    rt->configuredNad = LINIF_DEFAULT_NAD;
    rt->trcvWakeupMode = LINIF_TRCV_WU_ENABLE;
    rt->pidFrameCount = 0U;
    (void)memset(rt->pidTable, 0, sizeof(rt->pidTable));

    if ((ch != NULL_PTR) && (ch->Frames != NULL_PTR)) {
        count = (ch->NumFrames > LINIF_MAX_CHANNEL_FRAMES) ? LINIF_MAX_CHANNEL_FRAMES : ch->NumFrames;
        for (i = 0U; i < count; i++) {
            rt->pidTable[i] = ch->Frames[i].Pid;
        }
        rt->pidFrameCount = count;
    }
}

static const LinIf_ChannelConfigType* LinIf_GetChannelConfig(uint8 Channel)
{
    if ((LinIf_State.configPtr == NULL_PTR)
        || (Channel >= LinIf_State.configPtr->NumChannels)
        || (Channel >= LINIF_MAX_CHANNELS)
        || (LinIf_State.configPtr->Channels == NULL_PTR)) {
        return NULL_PTR;
    }
    return &LinIf_State.configPtr->Channels[Channel];
}

static const LinIf_ScheduleTableConfigType* LinIf_FindSchedule(const LinIf_ChannelConfigType* ch, uint8 ScheduleId)
{
    uint8 i;
    if ((ch == NULL_PTR) || (ch->Schedules == NULL_PTR)) { return NULL_PTR; }
    for (i = 0U; (i < ch->NumSchedules) && (i < LINIF_MAX_SCHEDULES); i++) {
        if (ch->Schedules[i].Schedule == ScheduleId) { return &ch->Schedules[i]; }
    }
    return NULL_PTR;
}

static uint8 LinIf_FindFrameIndex(const LinIf_ChannelConfigType* ch, uint8 Pid)
{
    uint8 i;
    if ((ch == NULL_PTR) || (ch->Frames == NULL_PTR)) { return 0xFFU; }
    for (i = 0U; (i < ch->NumFrames) && (i < LINIF_MAX_FRAMES); i++) {
        if (ch->Frames[i].Pid == Pid) { return i; }
    }
    return 0xFFU;
}

static void LinIf_StartSchedule(uint8 Channel, const LinIf_ScheduleTableConfigType* SchedulePtr)
{
    LinIf_ChannelRuntimeType* rt = &LinIf_State.channels[Channel];

    rt->currentSchedule = SchedulePtr->Schedule;
    rt->currentEntry = 0U;
    if ((SchedulePtr->Entries != NULL_PTR) && (SchedulePtr->EntryCount > 0U)) {
        rt->entryDelayCounter = (SchedulePtr->Entries[0U].DelayMs > 0U) ? SchedulePtr->Entries[0U].DelayMs : 1U;
    } else {
        rt->entryDelayCounter = 0U;
    }
}

static void LinIf_ExecuteEntry(uint8 Channel, const LinIf_ChannelConfigType* ch, const LinIf_ScheduleTableConfigType* SchedulePtr)
{
    LinIf_ChannelRuntimeType* rt = &LinIf_State.channels[Channel];
    const LinIf_ScheduleEntryType* entryPtr;
    const LinIf_FrameConfigType* framePtr;
    Lin_PduType pdu;
    uint8 frameIdx;

    entryPtr = &SchedulePtr->Entries[rt->currentEntry];
    frameIdx = entryPtr->FrameIdx;
    if ((frameIdx >= ch->NumFrames) || (ch->Frames == NULL_PTR)) { return; }
    framePtr = &ch->Frames[frameIdx];

    pdu.Pid = framePtr->Pid;
    pdu.FrameType = (framePtr->FrameType == LINIF_EVENT_TRIGGERED_FRAME)
                    ? LIN_FRAMETYPE_EVENT_TRIGGERED : LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = framePtr->IsPublish ? LIN_MASTER_RESPONSE : LIN_SLAVE_RESPONSE;
    pdu.Length = framePtr->Dlc;
    pdu.ChecksumType = ((framePtr->Pid == LINIF_PID_DIAG_MASTER) || (framePtr->Pid == LINIF_PID_DIAG_SLAVE))
                       ? LIN_CLASSIC_CS : LIN_ENHANCED_CS;
    pdu.SduPtr = framePtr->IsPublish ? rt->txBuffer[frameIdx] : rt->rxBuffer[frameIdx];

    if (Lin_SendFrame(Channel, &pdu) == E_OK) {
        /* Only application-requested transmissions are confirmed upwards,
         * otherwise the upper layer would see unsolicited confirmations */
        if (framePtr->IsPublish && rt->txPending[frameIdx]) {
            rt->txPending[frameIdx] = FALSE;
            LinIf_TxConfirmation(Channel, rt->txPduId[frameIdx]);
        }
    }
}

/** @req SWS_LinIf_00001 */
void LinIf_Init(const LinIf_ConfigType* ConfigPtr)
{
    uint8 ch;

#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_INIT, LINIF_E_PARAM_POINTER);
        return;
    }
#endif
    if (NULL_PTR == ConfigPtr) {
        ConfigPtr = &LinIf_Config;
    }

    (void)memset(&LinIf_State, 0, sizeof(LinIf_State));
    LinIf_State.configPtr = ConfigPtr;

    for (ch = 0U; (ch < LINIF_MAX_CHANNELS) && (ch < ConfigPtr->NumChannels); ch++) {
        LinIf_State.channels[ch].state = LINIF_CHANNEL_INIT;
        LinIf_State.channels[ch].currentSchedule = LINIF_NULL_SCHEDULE;
        LinIf_State.channels[ch].requestedSchedule = LINIF_NULL_SCHEDULE;
        if (ConfigPtr->Channels != NULL_PTR) {
            LinIf_ResetChannelNodeConfig(ch, &ConfigPtr->Channels[ch]);
        } else {
            LinIf_ResetChannelNodeConfig(ch, NULL_PTR);
        }
    }
}

/** @req SWS_LinIf_00002 */
void LinIf_DeInit(void)
{
    const LinIf_ConfigType* cfg = LinIf_State.configPtr;
    uint8 ch;

    (void)memset(&LinIf_State, 0, sizeof(LinIf_State));

    /* Restore the NAD/PID defaults so a subsequent Init starts from a clean
     * node configuration even if the caller passes a different config. */
    if (cfg != NULL_PTR) {
        for (ch = 0U; (ch < LINIF_MAX_CHANNELS) && (ch < cfg->NumChannels); ch++) {
            const LinIf_ChannelConfigType* chCfg =
                (cfg->Channels != NULL_PTR) ? &cfg->Channels[ch] : NULL_PTR;
            LinIf_ResetChannelNodeConfig(ch, chCfg);
        }
    }
}

/** @req SWS_LinIf_00003 */
Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    const LinIf_TxPduMapType* mapPtr;
    const LinIf_ChannelConfigType* ch;
    const LinIf_FrameConfigType* framePtr;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }
    if ((LinIf_State.configPtr->TxPduMap == NULL_PTR)
        || (TxPduId >= (PduIdType)LinIf_State.configPtr->NumTxPdus)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_PDU);
#endif
        return E_NOT_OK;
    }

    mapPtr = &LinIf_State.configPtr->TxPduMap[TxPduId];
    ch = LinIf_GetChannelConfig(mapPtr->Channel);
    if ((ch == NULL_PTR) || (ch->Frames == NULL_PTR) || (mapPtr->FrameIdx >= ch->NumFrames)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_PDU);
#endif
        return E_NOT_OK;
    }
    framePtr = &ch->Frames[mapPtr->FrameIdx];
    rt = &LinIf_State.channels[mapPtr->Channel];

    if (rt->state < LINIF_CHANNEL_INIT) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if ((PduInfoPtr->SduDataPtr == NULL_PTR)
        || (PduInfoPtr->SduLength != (PduLengthType)framePtr->Dlc)
        || (framePtr->Dlc > LINIF_MAX_FRAME_LENGTH)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_PDU);
#endif
        return E_NOT_OK;
    }

    (void)memcpy(rt->txBuffer[mapPtr->FrameIdx], PduInfoPtr->SduDataPtr, framePtr->Dlc);
    rt->txPending[mapPtr->FrameIdx] = TRUE;
    rt->txPduId[mapPtr->FrameIdx] = (uint8)TxPduId;
    return E_OK;
}

/** @req SWS_LinIf_00004 */
Std_ReturnType LinIf_SetSchedule(uint8 ScheduleTableId)
{
    const LinIf_ChannelConfigType* ch;
    const LinIf_ScheduleTableConfigType* schedulePtr;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    ch = LinIf_GetChannelConfig(0U);
    schedulePtr = LinIf_FindSchedule(ch, ScheduleTableId);
    if (schedulePtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULE, LINIF_E_PARAM_SCHEDULE);
#endif
        return E_NOT_OK;
    }

    rt = &LinIf_State.channels[0U];
    LinIf_StartSchedule(0U, schedulePtr);
    rt->requestedSchedule = ScheduleTableId;
    rt->scheduleSwitchPending = FALSE;
    if (rt->state == LINIF_CHANNEL_INIT) {
        rt->state = LINIF_CHANNEL_ONLINE;
    }
    return E_OK;
}

/** @req SWS_LinIf_00008 */
Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, LinIf_ScheduleTableType ScheduleTable)
{
    const LinIf_ChannelConfigType* ch;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULEREQUEST, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    ch = LinIf_GetChannelConfig(Channel);
    if (ch == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULEREQUEST, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (LinIf_FindSchedule(ch, (uint8)ScheduleTable) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULEREQUEST, LINIF_E_PARAM_SCHEDULE);
#endif
        return E_NOT_OK;
    }

    rt = &LinIf_State.channels[Channel];
    if (rt->state < LINIF_CHANNEL_INIT) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULEREQUEST, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }

    rt->requestedSchedule = (uint8)ScheduleTable;
    rt->scheduleSwitchPending = TRUE;
    return E_OK;
}

/** @req SWS_LinIf_00009 */
Std_ReturnType LinIf_WakeUp(uint8 Channel)
{
    const LinIf_ChannelConfigType* ch;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_WAKEUP, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    ch = LinIf_GetChannelConfig(Channel);
    if (ch == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_WAKEUP, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }

    if (Lin_WakeUp(Channel) != E_OK) {
        return E_NOT_OK;
    }
    rt = &LinIf_State.channels[Channel];
    rt->state = LINIF_CHANNEL_ONLINE;
    LinIf_WakeUpConfirmation(Channel);
    return E_OK;
}

/** @req SWS_LinIf_00010 */
Std_ReturnType LinIf_GotoSleep(uint8 Channel)
{
    const LinIf_ChannelConfigType* ch;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GOTOSLEEP, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    ch = LinIf_GetChannelConfig(Channel);
    if (ch == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GOTOSLEEP, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }

    if (Lin_GoToSleep(Channel) != E_OK) {
        return E_NOT_OK;
    }
    rt = &LinIf_State.channels[Channel];
    rt->state = LINIF_CHANNEL_SLEEP;
    LinIf_GotoSleepConfirmation(Channel);
    return E_OK;
}

/** @req SWS_LinIf_00005 */
void LinIf_RxIndication(uint8 LinChannel, const LinIf_PduType* PduInfoPtr)
{
    const LinIf_ChannelConfigType* ch;
    LinIf_ChannelRuntimeType* rt;
    uint8 frameIdx;

    if ((LinIf_State.configPtr == NULL_PTR) || (PduInfoPtr == NULL_PTR)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_RX_INDICATION, LINIF_E_PARAM_POINTER);
#endif
        return;
    }
    ch = LinIf_GetChannelConfig(LinChannel);
    if (ch == NULL_PTR) {
        return;
    }
    frameIdx = LinIf_FindFrameIndex(ch, PduInfoPtr->Id);
    if (frameIdx == 0xFFU) {
        return;
    }

    rt = &LinIf_State.channels[LinChannel];
    if ((PduInfoPtr->DataPtr != NULL_PTR) && (PduInfoPtr->Dlc <= LINIF_MAX_FRAME_LENGTH)) {
        (void)memcpy(rt->rxBuffer[frameIdx], PduInfoPtr->DataPtr, PduInfoPtr->Dlc);
        rt->rxValid[frameIdx] = TRUE;
    }
    LinIf_RxCallback(LinChannel, PduInfoPtr);
}

/** @req SWS_LinIf_00006 */
void LinIf_MainFunction(void)
{
    uint8 chIdx;

    if (LinIf_State.configPtr == NULL_PTR) { return; }

    for (chIdx = 0U; (chIdx < LinIf_State.configPtr->NumChannels) && (chIdx < LINIF_MAX_CHANNELS); chIdx++) {
        const LinIf_ChannelConfigType* ch = &LinIf_State.configPtr->Channels[chIdx];
        LinIf_ChannelRuntimeType* rt = &LinIf_State.channels[chIdx];
        const LinIf_ScheduleTableConfigType* schedulePtr;

        if (rt->state < LINIF_CHANNEL_INIT) { continue; }

        /* Schedule switches take effect immediately and restart at entry 0.
         * The switch tick only arms the new schedule, the delay of its first
         * entry starts counting with the following tick. */
        if (rt->scheduleSwitchPending) {
            schedulePtr = LinIf_FindSchedule(ch, rt->requestedSchedule);
            rt->scheduleSwitchPending = FALSE;
            if (schedulePtr == NULL_PTR) { continue; }
            LinIf_StartSchedule(chIdx, schedulePtr);
            LinIf_ScheduleRequestConfirmation(chIdx, schedulePtr->Schedule);
            continue;
        }

        if (rt->state != LINIF_CHANNEL_ONLINE) { continue; }

        schedulePtr = LinIf_FindSchedule(ch, rt->currentSchedule);
        if ((schedulePtr == NULL_PTR) || (schedulePtr->Entries == NULL_PTR) || (schedulePtr->EntryCount == 0U)) {
            continue;
        }

        if (rt->entryDelayCounter > 0U) {
            rt->entryDelayCounter--;
        }
        if (rt->entryDelayCounter == 0U) {
            LinIf_ExecuteEntry(chIdx, ch, schedulePtr);
            rt->currentEntry = (uint8)((rt->currentEntry + 1U) % schedulePtr->EntryCount);
            rt->entryDelayCounter = (schedulePtr->Entries[rt->currentEntry].DelayMs > 0U)
                                    ? schedulePtr->Entries[rt->currentEntry].DelayMs : 1U;
        }
    }
}

/** @req SWS_LinIf_00007 */
void LinIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETVERSIONINFO, LINIF_E_PARAM_POINTER);
#endif
        return;
    }
    versioninfo->vendorID = LINIF_VENDOR_ID;
    versioninfo->moduleID = LINIF_MODULE_ID;
    versioninfo->sw_major_version = LINIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = LINIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = LINIF_SW_PATCH_VERSION;
}

/*=============================================================================
 * Transceiver control (AD2) - delegated to the LinTrcv driver. The LinIf
 * channel ID is passed through as the LinTrcv channel ID.
 *===========================================================================*/

static Std_ReturnType LinIf_TrcvModeToLinTrcv(LinIf_TrcvModeType Mode, LinTrcv_OpmodeType* TrcvModePtr)
{
    Std_ReturnType ret = E_OK;
    switch (Mode) {
        case LINIF_TRCV_MODE_NORMAL:  *TrcvModePtr = LINTRCV_OPMODE_NORMAL;  break;
        case LINIF_TRCV_MODE_STANDBY: *TrcvModePtr = LINTRCV_OPMODE_STANDBY; break;
        case LINIF_TRCV_MODE_SLEEP:   *TrcvModePtr = LINTRCV_OPMODE_SLEEP;   break;
        default:                      ret = E_NOT_OK;                        break;
    }
    return ret;
}

static Std_ReturnType LinIf_TrcvModeFromLinTrcv(LinTrcv_OpmodeType TrcvMode, LinIf_TrcvModeType* ModePtr)
{
    Std_ReturnType ret = E_OK;
    switch (TrcvMode) {
        case LINTRCV_OPMODE_NORMAL:  *ModePtr = LINIF_TRCV_MODE_NORMAL;  break;
        case LINTRCV_OPMODE_STANDBY: *ModePtr = LINIF_TRCV_MODE_STANDBY; break;
        case LINTRCV_OPMODE_SLEEP:   *ModePtr = LINIF_TRCV_MODE_SLEEP;   break;
        default:                     ret = E_NOT_OK;                     break;
    }
    return ret;
}

static Std_ReturnType LinIf_WuReasonFromLinTrcv(LinTrcv_WakeupReasonType TrcvReason, LinIf_TrcvWakeupReasonType* ReasonPtr)
{
    Std_ReturnType ret = E_OK;
    switch (TrcvReason) {
        case LINTRCV_WU_ERROR:         *ReasonPtr = LINIF_TRCV_WU_ERROR;         break;
        case LINTRCV_WU_BY_BUS:
        case LINTRCV_WU_BY_BUS_CS:     *ReasonPtr = LINIF_TRCV_WU_BY_BUS;        break;
        case LINTRCV_WU_BY_PIN:        *ReasonPtr = LINIF_TRCV_WU_BY_PIN;        break;
        case LINTRCV_WU_INTERNAL:      *ReasonPtr = LINIF_TRCV_WU_INTERNALLY;    break;
        case LINTRCV_WU_NOT_SUPPORTED: *ReasonPtr = LINIF_TRCV_WU_NOT_SUPPORTED; break;
        case LINTRCV_WU_POWER_ON:      *ReasonPtr = LINIF_TRCV_WU_POWER_ON;      break;
        case LINTRCV_WU_RESET:         *ReasonPtr = LINIF_TRCV_WU_RESET;         break;
        case LINTRCV_WU_BY_SYSERR:     *ReasonPtr = LINIF_TRCV_WU_BY_SYSERR;     break;
        default:                       ret = E_NOT_OK;                           break;
    }
    return ret;
}

/** @req SWS_LinIf_00679 */
Std_ReturnType LinIf_SetTrcvMode(uint8 Channel, LinIf_TrcvModeType Mode)
{
    LinTrcv_OpmodeType trcvMode;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVMODE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVMODE, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (LinIf_TrcvModeToLinTrcv(Mode, &trcvMode) != E_OK) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVMODE, LINIF_E_PARAM_VALUE);
#endif
        return E_NOT_OK;
    }
    return LinTrcv_SetOpMode(Channel, trcvMode);
}

/** @req SWS_LinIf_00680 */
Std_ReturnType LinIf_GetTrcvMode(uint8 Channel, LinIf_TrcvModeType* ModePtr)
{
    LinTrcv_OpmodeType trcvMode;
    Std_ReturnType ret;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVMODE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVMODE, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (NULL_PTR == ModePtr) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVMODE, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    ret = LinTrcv_GetOpMode(Channel, &trcvMode);
    if (ret != E_OK) {
        return ret;
    }
    if (LinIf_TrcvModeFromLinTrcv(trcvMode, ModePtr) != E_OK) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVMODE, LINIF_E_PARAM_VALUE);
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

/** @req SWS_LinIf_00681 */
Std_ReturnType LinIf_SetTrcvWakeupMode(uint8 Channel, LinIf_TrcvWakeupModeType Mode)
{
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVWAKEUPMODE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVWAKEUPMODE, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if ((Mode != LINIF_TRCV_WU_ENABLE)
        && (Mode != LINIF_TRCV_WU_DISABLE)
        && (Mode != LINIF_TRCV_WU_CLEAR)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETTRCVWAKEUPMODE, LINIF_E_PARAM_VALUE);
#endif
        return E_NOT_OK;
    }

    /* The LinTrcv driver exposes no SetWakeupMode API: the requested mode is
     * tracked in the LinIf channel runtime. */
    rt = &LinIf_State.channels[Channel];
    rt->trcvWakeupMode = Mode;
    return E_OK;
}

/** @req SWS_LinIf_00682 */
Std_ReturnType LinIf_GetTrcvWakeupReason(uint8 Channel, LinIf_TrcvWakeupReasonType* ReasonPtr)
{
    LinTrcv_WakeupReasonType trcvReason;
    Std_ReturnType ret;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVWAKEUPREASON, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVWAKEUPREASON, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (NULL_PTR == ReasonPtr) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVWAKEUPREASON, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    ret = LinTrcv_GetBusWuReason(Channel, &trcvReason);
    if (ret != E_OK) {
        return ret;
    }
    if (LinIf_WuReasonFromLinTrcv(trcvReason, ReasonPtr) != E_OK) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETTRCVWAKEUPREASON, LINIF_E_PARAM_VALUE);
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

/** @req SWS_LinIf_00683 */
Std_ReturnType LinIf_CheckWakeup(uint8 Channel)
{
    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_CHECKWAKEUP, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_CHECKWAKEUP, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    return LinTrcv_CheckWakeup(Channel);
}

/*=============================================================================
 * Node configuration (AD2) - runtime state per channel
 *===========================================================================*/

/** @req SWS_LinIf_00580 */
Std_ReturnType LinIf_SetConfiguredNAD(uint8 Channel, uint8 NAD)
{
    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETCONFIGUREDNAD, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETCONFIGUREDNAD, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }

    LinIf_State.channels[Channel].configuredNad = NAD;
    return E_OK;
}

/** @req SWS_LinIf_00581 */
Std_ReturnType LinIf_GetConfiguredNAD(uint8 Channel, uint8* NADPtr)
{
    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETCONFIGUREDNAD, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETCONFIGUREDNAD, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (NULL_PTR == NADPtr) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETCONFIGUREDNAD, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    *NADPtr = LinIf_State.channels[Channel].configuredNad;
    return E_OK;
}

/** @req SWS_LinIf_00582 */
Std_ReturnType LinIf_SetPIDTable(uint8 Channel, const uint8* PidTable, uint8 NumFrames)
{
    const LinIf_ChannelConfigType* ch;
    LinIf_ChannelRuntimeType* rt;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETPIDTABLE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    ch = LinIf_GetChannelConfig(Channel);
    if (ch == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETPIDTABLE, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if (NULL_PTR == PidTable) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETPIDTABLE, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }
    if ((NumFrames > ch->NumFrames) || (NumFrames > LINIF_MAX_CHANNEL_FRAMES)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SETPIDTABLE, LINIF_E_PARAM_VALUE);
#endif
        return E_NOT_OK;
    }

    rt = &LinIf_State.channels[Channel];
    (void)memcpy(rt->pidTable, PidTable, NumFrames);
    rt->pidFrameCount = NumFrames;
    return E_OK;
}

/** @req SWS_LinIf_00583 */
Std_ReturnType LinIf_GetPIDTable(uint8 Channel, uint8* PidTable, uint8* NumFramesPtr)
{
    const LinIf_ChannelRuntimeType* rt;
    uint8 count;

    if (LinIf_State.configPtr == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETPIDTABLE, LINIF_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    if (LinIf_GetChannelConfig(Channel) == NULL_PTR) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETPIDTABLE, LINIF_E_PARAM_CHANNEL);
#endif
        return E_NOT_OK;
    }
    if ((NULL_PTR == PidTable) || (NULL_PTR == NumFramesPtr)) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GETPIDTABLE, LINIF_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    rt = &LinIf_State.channels[Channel];
    count = rt->pidFrameCount;
    (void)memcpy(PidTable, rt->pidTable, count);
    *NumFramesPtr = count;
    return E_OK;
}

/** @req SWS_LinIf_00585 */
boolean LinIf_IsSupportTpTransmit(uint8 Channel)
{
    const LinIf_ChannelConfigType* ch;
    uint8 i;

    if (LinIf_State.configPtr == NULL_PTR) {
        return FALSE;
    }
    ch = LinIf_GetChannelConfig(Channel);
    if ((ch == NULL_PTR) || (ch->Frames == NULL_PTR)) {
        return FALSE;
    }
    if (LinIf_State.channels[Channel].state < LINIF_CHANNEL_INIT) {
        return FALSE;
    }

    for (i = 0U; (i < ch->NumFrames) && (i < LINIF_MAX_FRAMES); i++) {
        if (ch->Frames[i].FrameType == LINIF_DIAGNOSTIC_FRAME) {
            return TRUE;
        }
    }
    return FALSE;
}
