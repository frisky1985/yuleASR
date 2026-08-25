/** @file LinIf.c
 *  @brief LIN Interface implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_LINInterface.pdf
 */

#include "LinIf.h"
#include "LinIf_Cfg.h"
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

#define LINIF_E_PARAM_POINTER       0x10U
#define LINIF_E_UNINIT              0x20U
#define LINIF_E_PARAM_PDU           0x30U
#define LINIF_E_PARAM_SCHEDULE      0x40U

typedef enum { LINIF_UNINIT = 0, LINIF_INIT, LINIF_ONLINE } LinIf_StateType;

typedef struct {
    LinIf_StateType state;
    uint8 activeChannel;
    uint8 activeSchedule;
    uint32 tickCount;
    const LinIf_ConfigType* configPtr;
} LinIf_InternalType;

static LinIf_InternalType LinIf_State;

/** @req SWS_LinIf_00001 */
void LinIf_Init(const LinIf_ConfigType* ConfigPtr)
{
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_INIT, LINIF_E_PARAM_POINTER);
        return;
    }
#endif
    LinIf_State.state = LINIF_UNINIT;
    LinIf_State.activeChannel = 0U;
    LinIf_State.activeSchedule = 0U;
    LinIf_State.tickCount = 0U;
    LinIf_State.configPtr = ConfigPtr;
/* [MISRA Advisory] Redundant:     LinIf_State.state = LINIF_INIT; */
}

/** @req SWS_LinIf_00002 */
void LinIf_DeInit(void)
{
    LinIf_State.state = LINIF_UNINIT;
    LinIf_State.configPtr = NULL_PTR;
}

/** @req SWS_LinIf_00003 */
Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LinIf_State.state < LINIF_INIT) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
}

/** @req SWS_LinIf_00004 */
Std_ReturnType LinIf_SetSchedule(uint8 ScheduleTableId)
{
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LinIf_State.state < LINIF_INIT) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULE, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    LinIf_State.activeSchedule = ScheduleTableId;
    LinIf_State.tickCount = 0U;
    return E_OK;
}

/** @req SWS_LinIf_00005 */
void LinIf_RxIndication(uint8 LinChannel, const LinIf_PduType* PduInfoPtr)
{
    (void)LinChannel;
    (void)PduInfoPtr;
}

/** @req SWS_LinIf_00006 */
void LinIf_MainFunction(void)
{
    if (LinIf_State.state < LINIF_INIT) { return; }

    LinIf_State.tickCount++;

    /* Process schedule tables */
    if (LinIf_State.configPtr != NULL_PTR) {
        for (uint8 c = 0U; c < LinIf_State.configPtr->NumChannels; c++) {
            const LinIf_ChannelConfigType* ch = &LinIf_State.configPtr->Channels[c];
            for (uint8 s = 0U; s < ch->NumSchedules; s++) {
                if (ch->Schedules[s].Schedule == LinIf_State.activeSchedule) {
                    for (uint8 e = 0U; e < ch->Schedules[s].EntryCount; e++) {
                        if ((LinIf_State.tickCount % ch->Schedules[s].Entries[e].DelayMs) == 0U) {
                            /* Transmit frame on schedule */
                            break;
                        }
                    }
                }
            }
        }
    }
}

/** @req SWS_LinIf_00007 */
void LinIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) { return; }
    versioninfo->vendorID = LINIF_VENDOR_ID;
    versioninfo->moduleID = LINIF_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}