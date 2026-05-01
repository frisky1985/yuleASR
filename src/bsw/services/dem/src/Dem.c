/**
 * @file Dem.c
 * @brief Diagnostic Event Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : N/A (Service Layer)
     5|* Dependencies         : NvM
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-15
    10|* Author               : AI Agent (Dem Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "Dem.h"
    20|#include "Dem_Cfg.h"
    21|#include "NvM.h"
    22|#include "Det.h"
    23|#include "MemMap.h"
    24|#include "string.h"
    25|
    26|/*==================================================================================================
    27|*                                  LOCAL CONSTANT DEFINITIONS
    28|==================================================================================================*/
    29|#define DEM_INSTANCE_ID                 (0x00U)
    30|
    31|/* Module state */
    32|#define DEM_STATE_UNINIT                (0x00U)
    33|#define DEM_STATE_INIT                  (0x01U)
    34|
    35|/* Debounce algorithm types */
    36|#define DEM_DEBOUNCE_ALGORITHM_NONE     (0x00U)
    37|#define DEM_DEBOUNCE_ALGORITHM_COUNTER  (0x01U)
    38|#define DEM_DEBOUNCE_ALGORITHM_TIME     (0x02U)
    39|#define DEM_DEBOUNCE_ALGORITHM_MONITOR  (0x03U)
    40|
    41|/*==================================================================================================
    42|*                                  LOCAL MACRO DEFINITIONS
    43|==================================================================================================*/
    44|#if (DEM_DEV_ERROR_DETECT == STD_ON)
    45|    #define DEM_DET_REPORT_ERROR(ApiId, ErrorId) \
    46|        Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, (ApiId), (ErrorId))
    47|#else
    48|    #define DEM_DET_REPORT_ERROR(ApiId, ErrorId)
    49|#endif
    50|
    51|/*==================================================================================================
    52|*                                  LOCAL TYPE DEFINITIONS
    53|==================================================================================================*/
    54|/* DTC entry type */
    55|typedef struct
    56|{
    57|    Dem_DTCType DTC;
    58|    Dem_DTCStatusType Status;
    59|    uint32 OccurrenceCounter;
    60|    uint32 AgingCounter;
    61|    boolean IsAged;
    62|    boolean IsSuppressed;
    63|} Dem_DTCEntryType;
    64|
    65|/* Module internal state */
    66|typedef struct
    67|{
    68|    uint8 State;
    69|    const Dem_ConfigType* ConfigPtr;
    70|    Dem_EventStateType EventStates[DEM_NUM_EVENTS];
    71|    Dem_DTCEntryType DTCEntries[DEM_NUM_DTCS];
    72|    uint8 OperationCycleStates[DEM_NUM_OPERATION_CYCLES];
    73|    boolean EnableConditions[DEM_NUM_ENABLE_CONDITIONS];
    74|    boolean StorageConditions[DEM_NUM_STORAGE_CONDITIONS];
    75|    Dem_DTCType SelectedDTC;
    76|    boolean DTCRecordUpdateDisabled;
    77|} Dem_InternalStateType;
    78|
    79|/*==================================================================================================
    80|*                                  LOCAL VARIABLE DECLARATIONS
    81|==================================================================================================*/
    82|#define DEM_START_SEC_VAR_CLEARED_UNSPECIFIED
    83|#include "MemMap.h"
    84|
    85|STATIC Dem_InternalStateType Dem_InternalState;
    86|
    87|#define DEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    88|#include "MemMap.h"
    89|
    90|/*==================================================================================================
    91|*                                  LOCAL FUNCTION PROTOTYPES
    92|==================================================================================================*/
    93|STATIC const Dem_EventParameterType* Dem_FindEventConfig(Dem_EventIdType EventId);
    94|STATIC const Dem_DTCParameterType* Dem_FindDTCConfig(Dem_DTCType DTC);
    95|STATIC uint8 Dem_FindDTCIndex(Dem_DTCType DTC);
    96|STATIC void Dem_UpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
    97|STATIC void Dem_UpdateDTCStatus(Dem_EventIdType EventId);
    98|STATIC void Dem_ProcessAging(void);
    99|STATIC void Dem_ResetDebounceCounter(Dem_EventIdType EventId);
   100|
   101|/*==================================================================================================
   102|*                                      LOCAL FUNCTIONS
   103|==================================================================================================*/
   104|#define DEM_START_SEC_CODE
   105|#include "MemMap.h"
   106|
   107|/**
   108| * @brief   Find event configuration by Event ID
   109| */
   110|STATIC const Dem_EventParameterType* Dem_FindEventConfig(Dem_EventIdType EventId)
   111|{
   112|    const Dem_EventParameterType* result = NULL_PTR;
   113|    uint8 i;
   114|
   115|    if ((Dem_InternalState.ConfigPtr != NULL_PTR) && (EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
   116|    {
   117|        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumEvents; i++)
   118|        {
   119|            if (Dem_InternalState.ConfigPtr->Events[i].EventId == EventId)
   120|            {
   121|                result = &Dem_InternalState.ConfigPtr->Events[i];
   122|                break;
   123|            }
   124|        }
   125|    }
   126|
   127|    return result;
   128|}
   129|
   130|/**
   131| * @brief   Find DTC configuration by DTC value
   132| */
   133|STATIC const Dem_DTCParameterType* Dem_FindDTCConfig(Dem_DTCType DTC)
   134|{
   135|    const Dem_DTCParameterType* result = NULL_PTR;
   136|    uint8 i;
   137|
   138|    if (Dem_InternalState.ConfigPtr != NULL_PTR)
   139|    {
   140|        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumDTCs; i++)
   141|        {
   142|            if (Dem_InternalState.ConfigPtr->DTCs[i].DTC == DTC)
   143|            {
   144|                result = &Dem_InternalState.ConfigPtr->DTCs[i];
   145|                break;
   146|            }
   147|        }
   148|    }
   149|
   150|    return result;
   151|}
   152|
   153|/**
   154| * @brief   Find DTC index by DTC value
   155| */
   156|STATIC uint8 Dem_FindDTCIndex(Dem_DTCType DTC)
   157|{
   158|    uint8 result = 0xFFU;
   159|    uint8 i;
   160|
   161|    for (i = 0U; i < DEM_NUM_DTCS; i++)
   162|    {
   163|        if (Dem_InternalState.DTCEntries[i].DTC == DTC)
   164|        {
   165|            result = i;
   166|            break;
   167|        }
   168|    }
   169|
   170|    return result;
   171|}
   172|
   173|/**
   174| * @brief   Reset debounce counter for an event
   175| */
   176|STATIC void Dem_ResetDebounceCounter(Dem_EventIdType EventId)
   177|{
   178|    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
   179|    {
   180|        Dem_InternalState.EventStates[EventId - 1U].DebounceCounter = 0;
   181|    }
   182|}
   183|
   184|/**
   185| * @brief   Update debounce counter based on event status
   186| */
   187|STATIC void Dem_UpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
   188|{
   189|    const Dem_EventParameterType* eventConfig;
   190|    Dem_EventStateType* eventState;
   191|
   192|    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
   193|    {
   194|        eventConfig = Dem_FindEventConfig(EventId);
   195|        eventState = &Dem_InternalState.EventStates[EventId - 1U];
   196|
   197|        if (eventConfig != NULL_PTR)
   198|        {
   199|            switch (EventStatus)
   200|            {
   201|                case DEM_EVENT_STATUS_PASSED:
   202|                    /* Reset debounce counter */
   203|                    eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
   204|                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
   205|                    break;
   206|
   207|                case DEM_EVENT_STATUS_FAILED:
   208|                    /* Set debounce counter to failed threshold */
   209|                    eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
   210|                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
   211|                    break;
   212|
   213|                case DEM_EVENT_STATUS_PREPASSED:
   214|                    /* Decrement debounce counter */
   215|                    if (eventState->DebounceCounter > DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
   216|                    {
   217|                        eventState->DebounceCounter -= DEM_DEBOUNCE_COUNTER_DECREMENT_STEP;
   218|                    }
   219|                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
   220|                    break;
   221|
   222|                case DEM_EVENT_STATUS_PREFAILED:
   223|                    /* Increment debounce counter */
   224|                    if (eventState->DebounceCounter < DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
   225|                    {
   226|                        eventState->DebounceCounter += DEM_DEBOUNCE_COUNTER_INCREMENT_STEP;
   227|                    }
   228|                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
   229|                    break;
   230|
   231|                default:
   232|                    /* Do nothing */
   233|                    break;
   234|            }
   235|        }
   236|    }
   237|}
   238|
   239|/**
   240| * @brief   Update DTC status based on event state
   241| */
   242|STATIC void Dem_UpdateDTCStatus(Dem_EventIdType EventId)
   243|{
   244|    const Dem_EventParameterType* eventConfig;
   245|    Dem_EventStateType* eventState;
   246|    uint8 dtcIndex;
   247|
   248|    eventConfig = Dem_FindEventConfig(EventId);
   249|
   250|    if (eventConfig != NULL_PTR)
   251|    {
   252|        eventState = &Dem_InternalState.EventStates[EventId - 1U];
   253|        dtcIndex = Dem_FindDTCIndex(eventConfig->DTC);
   254|
   255|        if (dtcIndex != 0xFFU)
   256|        {
   257|            Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[dtcIndex];
   258|
   259|            /* Update Test Failed status */
   260|            if (eventState->DebounceCounter >= DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
   261|            {
   262|                /* Set Test Failed */
   263|                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED;
   264|                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE;
   265|                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR;
   266|
   267|                /* Set Pending DTC */
   268|                dtcEntry->Status |= DEM_DTC_STATUS_PENDING_DTC;
   269|
   270|                /* Increment occurrence counter */
   271|                if (dtcEntry->OccurrenceCounter < DEM_MAX_OCCURRENCE_COUNTER)
   272|                {
   273|                    dtcEntry->OccurrenceCounter++;
   274|                }
   275|
   276|                /* Set Confirmed DTC after sufficient occurrences */
   277|                if (dtcEntry->OccurrenceCounter >= 2U)
   278|                {
   279|                    dtcEntry->Status |= DEM_DTC_STATUS_CONFIRMED_DTC;
   280|                }
   281|
   282|                /* Reset aging counter */
   283|                dtcEntry->AgingCounter = 0U;
   284|                dtcEntry->IsAged = FALSE;
   285|            }
   286|            else if (eventState->DebounceCounter <= DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
   287|            {
   288|                /* Clear Test Failed */
   289|                dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_FAILED;
   290|
   291|                /* Clear Pending DTC if test passed this cycle */
   292|                if (eventState->TestCompletedThisOperationCycle)
   293|                {
   294|                    dtcEntry->Status &= ~DEM_DTC_STATUS_PENDING_DTC;
   295|                }
   296|            }
   297|
   298|            /* Update Test Not Completed This Operation Cycle */
   299|            dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
   300|
   301|            /* Update Test Not Completed Since Last Clear */
   302|            dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
   303|        }
   304|    }
   305|}
   306|
   307|/**
   308| * @brief   Process DTC aging
   309| */
   310|STATIC void Dem_ProcessAging(void)
   311|{
   312|    uint8 i;
   313|
   314|    for (i = 0U; i < DEM_NUM_DTCS; i++)
   315|    {
   316|        Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[i];
   317|
   318|        /* Check if DTC can be aged */
   319|        if ((dtcEntry->Status & DEM_DTC_STATUS_CONFIRMED_DTC) &&
   320|            !(dtcEntry->Status & DEM_DTC_STATUS_TEST_FAILED))
   321|        {
   322|            /* Increment aging counter */
   323|            if (dtcEntry->AgingCounter < DEM_AGING_CYCLE_COUNTER_THRESHOLD)
   324|            {
   325|                dtcEntry->AgingCounter++;
   326|            }
   327|
   328|            /* Check if aging threshold reached */
   329|            if (dtcEntry->AgingCounter >= DEM_AGING_THRESHOLD)
   330|            {
   331|                /* Age the DTC */
   332|                dtcEntry->Status &= ~DEM_DTC_STATUS_CONFIRMED_DTC;
   333|                dtcEntry->Status &= ~DEM_DTC_STATUS_PENDING_DTC;
   334|                dtcEntry->IsAged = TRUE;
   335|            }
   336|        }
   337|    }
   338|}
   339|
   340|/*==================================================================================================
   341|*                                      GLOBAL FUNCTIONS
   342|==================================================================================================*/
   343|
   344|/**
   345| * @brief   Initializes the DEM module
   346| */
   347|void Dem_Init(const Dem_ConfigType* ConfigPtr)
   348|{
   349|    uint8 i;
   350|
   351|#if (DEM_DEV_ERROR_DETECT == STD_ON)
   352|    if (ConfigPtr == NULL_PTR)
   353|    {
   354|        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_INIT, DEM_E_PARAM_POINTER);
   355|        return;
   356|    }
   357|#endif
   358|
   359|    /* Store configuration pointer */
   360|    Dem_InternalState.ConfigPtr = ConfigPtr;
   361|
   362|    /* Initialize event states */
   363|    for (i = 0U; i < DEM_NUM_EVENTS; i++)
   364|    {
   365|        Dem_InternalState.EventStates[i].LastReportedStatus = DEM_EVENT_STATUS_PASSED;
   366|        Dem_InternalState.EventStates[i].DTCStatus = 0U;
   367|        Dem_InternalState.EventStates[i].FaultDetectionCounter = 0;
   368|        Dem_InternalState.EventStates[i].DebounceCounter = 0;
   369|        Dem_InternalState.EventStates[i].TestFailedThisOperationCycle = FALSE;
   370|        Dem_InternalState.EventStates[i].TestCompletedThisOperationCycle = FALSE;
   371|        Dem_InternalState.EventStates[i].OccurrenceCounter = 0U;
   372|        Dem_InternalState.EventStates[i].AgingCounter = 0U;
   373|        Dem_InternalState.EventStates[i].IsAged = FALSE;
   374|    }
   375|
   376|    /* Initialize DTC entries */
   377|    for (i = 0U; i < DEM_NUM_DTCS; i++)
   378|    {
   379|        if (i < ConfigPtr->NumDTCs)
   380|        {
   381|            Dem_InternalState.DTCEntries[i].DTC = ConfigPtr->DTCs[i].DTC;
   382|        }
   383|        else
   384|        {
   385|            Dem_InternalState.DTCEntries[i].DTC = 0U;
   386|        }
   387|        Dem_InternalState.DTCEntries[i].Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
   388|                                                  DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
   389|        Dem_InternalState.DTCEntries[i].OccurrenceCounter = 0U;
   390|        Dem_InternalState.DTCEntries[i].AgingCounter = 0U;
   391|        Dem_InternalState.DTCEntries[i].IsAged = FALSE;
   392|        Dem_InternalState.DTCEntries[i].IsSuppressed = FALSE;
   393|    }
   394|
   395|    /* Initialize operation cycle states */
   396|    for (i = 0U; i < DEM_NUM_OPERATION_CYCLES; i++)
   397|    {
   398|        Dem_InternalState.OperationCycleStates[i] = DEM_CYCLE_STATE_END;
   399|    }
   400|
   401|    /* Initialize enable conditions */
   402|    for (i = 0U; i < DEM_NUM_ENABLE_CONDITIONS; i++)
   403|    {
   404|        Dem_InternalState.EnableConditions[i] = TRUE;
   405|    }
   406|
   407|    /* Initialize storage conditions */
   408|    for (i = 0U; i < DEM_NUM_STORAGE_CONDITIONS; i++)
   409|    {
   410|        Dem_InternalState.StorageConditions[i] = TRUE;
   411|    }
   412|
   413|    /* Initialize selected DTC */
   414|    Dem_InternalState.SelectedDTC = 0U;
   415|    Dem_InternalState.DTCRecordUpdateDisabled = FALSE;
   416|
   417|    /* Set module state to initialized */
   418|    Dem_InternalState.State = DEM_STATE_INIT;
   419|}
   420|
   421|/**
   422| * @brief   Deinitializes the DEM module
   423| */
   424|void Dem_DeInit(void)
   425|{
   426|#if (DEM_DEV_ERROR_DETECT == STD_ON)
   427|    if (Dem_InternalState.State != DEM_STATE_INIT)
   428|    {
   429|        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_DEINIT, DEM_E_UNINIT);
   430|        return;
   431|    }
   432|#endif
   433|
   434|    /* Clear configuration pointer */
   435|    Dem_InternalState.ConfigPtr = NULL_PTR;
   436|
   437|    /* Set module state to uninitialized */
   438|    Dem_InternalState.State = DEM_STATE_UNINIT;
   439|}
   440|
   441|/**
   442| * @brief   Set event status
   443| */
   444|Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
   445|{
   446|    Std_ReturnType result = E_NOT_OK;
   447|    Dem_EventStateType* eventState;
   448|
   449|#if (DEM_DEV_ERROR_DETECT == STD_ON)
   450|    if (Dem_InternalState.State != DEM_STATE_INIT)
   451|    {
   452|        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_UNINIT);
   453|        return E_NOT_OK;
   454|    }
   455|
   456|    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
   457|    {
   458|        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
   459|        return E_NOT_OK;
   460|    }
   461|#endif
   462|
   463|    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
   464|    {
   465|        eventState = &Dem_InternalState.EventStates[EventId - 1U];
   466|
   467|        /* Store last reported status */
   468|        eventState->LastReportedStatus = EventStatus;
   469|
   470|        /* Update debounce counter */
   471|        Dem_UpdateDebounceCounter(EventId, EventStatus);
   472|
   473|        /* Mark test as completed this operation cycle */
   474|        eventState->TestCompletedThisOperationCycle = TRUE;
   475|
   476|        /* Update DTC status */
   477|        Dem_UpdateDTCStatus(EventId);
   478|
   479|        result = E_OK;
   480|    }
   481|
   482|    return result;
   483|}
   484|
   485|/**
   486| * @brief   Reset event status
   487| */
   488|Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId)
   489|{
   490|    Std_ReturnType result = E_NOT_OK;
   491|    Dem_EventStateType* eventState;
   492|
   493|#if (DEM_DEV_ERROR_DETECT == STD_ON)
   494|    if (Dem_InternalState.State != DEM_STATE_INIT)
   495|    {
   496|        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_RESETEVENTSTATUS, DEM_E_UNINIT);
   497|        return E_NOT_OK;
   498|    }
   499|
   500|    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
   501|