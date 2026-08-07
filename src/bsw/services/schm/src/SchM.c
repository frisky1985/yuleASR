/** @file SchM.c
 *  @brief Schedule Manager implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_ScheduleManager.pdf
 */

#include "SchM.h"
#include "Det.h"

/* Version check */
#if defined(SCHM_AR_RELEASE_MAJOR_VERSION) && (SCHM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "SchM: AR major mismatch"
#endif
#if defined(SCHM_AR_RELEASE_MINOR_VERSION) && (SCHM_AR_RELEASE_MINOR_VERSION != 4u)
#error "SchM: AR minor mismatch"
#endif

#define SCHM_SID_INIT               0x00U
#define SCHM_SID_DEINIT             0x01U
#define SCHM_SID_START              0x02U
#define SCHM_SID_STOP               0x03U
#define SCHM_SID_SET_SCHEDULE_TABLE 0x04U
#define SCHM_SID_GET_SCHEDULE_TABLE 0x05U
#define SCHM_SID_MAINFUNCTION       0x06U

#define SCHM_E_PARAM_POINTER        0x10U
#define SCHM_E_UNINIT               0x20U
#define SCHM_E_PARAM_SCHEDULE       0x30U
#define SCHM_E_BUSY                 0x40U

typedef enum { SCHM_UNINIT = 0, SCHM_IDLE, SCHM_RUNNING } SchM_StateType;

typedef struct {
    SchM_StateType state;
    uint8 activeScheduleId;
    uint32 tickCounter;
    const SchM_ConfigType* configPtr;
} SchM_InternalType;

static SchM_InternalType SchM_State = { SCHM_UNINIT, 0U, 0U, NULL_PTR };

void SchM_Init(const SchM_ConfigType* ConfigPtr)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(SCHM_MODULE_ID, 0U, SCHM_SID_INIT, SCHM_E_PARAM_POINTER);
        return;
    }
#endif
    SchM_State.configPtr = ConfigPtr;
    SchM_State.activeScheduleId = 0U;
    SchM_State.tickCounter = 0U;
    SchM_State.state = SCHM_IDLE;
}

void SchM_DeInit(void)
{
    SchM_State.state = SCHM_UNINIT;
    SchM_State.configPtr = NULL_PTR;
}

Std_ReturnType SchM_Start(void)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (SchM_State.state == SCHM_UNINIT) {
        Det_ReportError(SCHM_MODULE_ID, 0U, SCHM_SID_START, SCHM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    SchM_State.state = SCHM_RUNNING;
    SchM_State.tickCounter = 0U;
    return E_OK;
}

Std_ReturnType SchM_Stop(void)
{
    if (SchM_State.state != SCHM_RUNNING) return E_NOT_OK;
    SchM_State.state = SCHM_IDLE;
    return E_OK;
}

Std_ReturnType SchM_SetScheduleTable(uint8 ScheduleId)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (SchM_State.state == SCHM_UNINIT) {
        Det_ReportError(SCHM_MODULE_ID, 0U, SCHM_SID_SET_SCHEDULE_TABLE, SCHM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if ((SchM_State.configPtr != NULL_PTR) && (ScheduleId < SchM_State.configPtr->NumScheduleTables)) {
        SchM_State.activeScheduleId = ScheduleId;
        SchM_State.tickCounter = 0U;
        return E_OK;
    }
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(SCHM_MODULE_ID, 0U, SCHM_SID_SET_SCHEDULE_TABLE, SCHM_E_PARAM_SCHEDULE);
#endif
    return E_NOT_OK;
}

uint8 SchM_GetScheduleTable(void)
{
    return SchM_State.activeScheduleId;
}

void SchM_MainFunction(void)
{
    if ((SchM_State.state != SCHM_RUNNING) || (SchM_State.configPtr == NULL_PTR)) return;

    SchM_State.tickCounter++;

    /* Execute active schedule table */
    const SchM_ScheduleTableType* table = &SchM_State.configPtr->ScheduleTables[SchM_State.activeScheduleId];
    for (uint8 i = 0U; i < table->NumSchedulePoints; i++) {
        if (table->SchedulePoints[i].TickOffset == SchM_State.tickCounter) {
            if (table->SchedulePoints[i].Callback != NULL_PTR) {
                table->SchedulePoints[i].Callback();
            }
        }
    }

    /* Handle table wrap */
    if (SchM_State.tickCounter >= table->TableDuration) {
        if (table->TableRepeat) {
            SchM_State.tickCounter = 0U;
        }
    }
}

void SchM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(SCHM_MODULE_ID, 0U, 0xFFU, SCHM_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = SCHM_VENDOR_ID;
    versioninfo->moduleID = SCHM_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}