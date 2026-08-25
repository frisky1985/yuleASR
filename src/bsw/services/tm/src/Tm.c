/**
 * @file Tm.c
 * @brief Time Manager — Stub Implementation
 */
#include "Tm.h"
#include "Det.h"

#define TM_DEV_ERROR_DETECT STD_ON

static const Tm_ConfigType* Tm_ConfigPtr = NULL_PTR;
static boolean Tm_Initialized = FALSE;
static Tm_TimeBaseType Tm_LocalTime = 0U;

/** @req SWS_Tm_00001 */
Std_ReturnType Tm_Init(const Tm_ConfigType* Config)
{
#if (TM_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(TM_MODULE_ID, 0U, 0U, DET_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if ((Tm_Initialized) != 0U) {
        return E_NOT_OK;
    }
    Tm_ConfigPtr = Config;
    Tm_LocalTime = 0U;
    Tm_Initialized = TRUE;
    return E_OK;
}

/** @req SWS_Tm_00002 */
void Tm_DeInit(void)
{
    Tm_ConfigPtr = NULL_PTR;
    Tm_LocalTime = 0U;
    Tm_Initialized = FALSE;
}

/** @req SWS_Tm_00003 */
void Tm_MainFunction(void)
{
    /* Cyclic time base update stub */
    if ((Tm_Initialized) != 0U) {
        Tm_LocalTime++;
    }
}

/** @req SWS_Tm_00004 */
Std_ReturnType Tm_GetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType* value)
{
    (void)timeBaseId;
#if (TM_DEV_ERROR_DETECT == STD_ON)
    if (!Tm_Initialized) {
        Det_ReportError(TM_MODULE_ID, 0U, 1U, DET_E_UNINIT);
        return E_NOT_OK;
    }
    if (value == NULL_PTR) {
        Det_ReportError(TM_MODULE_ID, 0U, 1U, DET_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    *value = Tm_LocalTime;
    return E_OK;
}

/** @req SWS_Tm_00005 */
Std_ReturnType Tm_SetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType value)
{
    (void)timeBaseId;
    Tm_LocalTime = value;
    return E_OK;
}

/** @req SWS_Tm_00006 */
Std_ReturnType Tm_GetTimeBaseInfo(uint8 timeBaseId, Tm_TimeBaseInfoType* info)
{
    (void)timeBaseId;
#if (TM_DEV_ERROR_DETECT == STD_ON)
    if (info == NULL_PTR) {
        return E_NOT_OK;
    }
#endif
    if (info != NULL_PTR) {
        info->currentValue = Tm_LocalTime;
        info->resolution = 1000U;
        info->isSynchronized = FALSE;
        info->status = TM_STATUS_RUNNING;
    }
    return E_OK;
}

/** @req SWS_Tm_00007 */
Std_ReturnType Tm_GetGlobalTime(Tm_GlobalTimeType* time)
{
#if (TM_DEV_ERROR_DETECT == STD_ON)
    if (time == NULL_PTR) {
        return E_NOT_OK;
    }
#endif
    if (time != NULL_PTR) {
        time->secondsHigh = 0U;
        time->secondsLow = (uint32)(Tm_LocalTime / 1000U);
        time->nanoseconds = (uint32)((Tm_LocalTime % 1000U) * 1000000U);
    }
    return E_OK;
}

/** @req SWS_Tm_00008 */
Std_ReturnType Tm_SetGlobalTime(const Tm_GlobalTimeType* time)
{
    (void)time;
    return E_OK;
}

/** @req SWS_Tm_00009 */
Std_ReturnType Tm_SyncTimeBase(uint8 sourceId, uint8 targetId)
{
    (void)sourceId;
    (void)targetId;
    return E_OK;
}

Tm_DurationType Tm_GetElapsedDuration(uint8 timeBaseId, Tm_TimeBaseType since)
{
    (void)timeBaseId;
    return (Tm_LocalTime > since) ? (Tm_DurationType)(Tm_LocalTime - since) : 0U;
}
