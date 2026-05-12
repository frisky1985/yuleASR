/**
 * @file FiM.c
 * @brief Function Inhibition Manager
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
     5|* Dependencies         : Dem, Det
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-30
    10|* Author               : AI Agent (FiM Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "FiM.h"
    20|#include "FiM_Cfg.h"
    21|#include "Dem.h"
    22|#include "Det.h"
    23|#include "MemMap.h"
    24|#include "string.h"
    25|
    26|/*==================================================================================================
    27|*                                  LOCAL CONSTANT DEFINITIONS
    28|==================================================================================================*/
    29|#define FIM_INSTANCE_ID                 (0x00U)
    30|
    31|/* Module state */
    32|#define FIM_STATE_UNINIT                (0x00U)
    33|#define FIM_STATE_INIT                  (0x01U)
    34|
    35|/*==================================================================================================
    36|*                                  LOCAL MACRO DEFINITIONS
    37|==================================================================================================*/
    38|#if (FIM_DEV_ERROR_DETECT == STD_ON)
    39|    #define FIM_DET_REPORT_ERROR(ApiId, ErrorId) \
    40|        Det_ReportError(FIM_MODULE_ID, FIM_INSTANCE_ID, (ApiId), (ErrorId))
    41|#else
    42|    #define FIM_DET_REPORT_ERROR(ApiId, ErrorId)
    43|#endif
    44|
    45|/*==================================================================================================
    46|*                                  LOCAL TYPE DEFINITIONS
    47|==================================================================================================*/
    48|/* Function state type */
    49|typedef struct {
    50|    FiM_PermissionStateType Permission;
    51|    boolean Available;
    52|    FiM_InhibitionStatusType InhibitionStatus;
    53|    uint8 LastCalculatedInhibitionMask;
    54|} FiM_FunctionStateType;
    55|
    56|/* Summary event state type */
    57|typedef struct {
    58|    boolean IsFailed;
    59|    uint8 InhibitionMask;
    60|} FiM_SummaryEventStateType;
    61|
    62|/* Module internal state */
    63|typedef struct {
    64|    uint8 State;
    65|    const FiM_ConfigType* ConfigPtr;
    66|    FiM_FunctionStateType FunctionStates[FIM_NUM_FUNCTIONS];
    67|    FiM_SummaryEventStateType SummaryEventStates[FIM_NUM_SUMMARY_EVENTS];
    68|} FiM_InternalStateType;
    69|
    70|/*==================================================================================================
    71|*                                  LOCAL VARIABLE DECLARATIONS
    72|==================================================================================================*/
    73|#define FIM_START_SEC_VAR_CLEARED_UNSPECIFIED
    74|#include "MemMap.h"
    75|
    76|STATIC FiM_InternalStateType FiM_InternalState;
    77|
    78|#define FIM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    79|#include "MemMap.h"
    80|
    81|/*==================================================================================================
    82|*                                  LOCAL FUNCTION PROTOTYPES
    83|==================================================================================================*/
    84|STATIC const FiM_FunctionConfigType* FiM_FindFunctionConfig(FiM_FunctionIdType FID);
    85|STATIC Std_ReturnType FiM_CalculateInhibitionMask(FiM_FunctionIdType FID, uint8* InhibitionMask);
    86|STATIC boolean FiM_CheckEventInhibition(const FiM_EventInhibitionType* EventInhibition, uint8 CurrentDtcStatus);
    87|STATIC void FiM_UpdateFunctionPermission(FiM_FunctionIdType FID);
    88|STATIC void FiM_UpdateSummaryEventState(FiM_SummaryEventIdType SummaryEventId);
    89|
    90|/*==================================================================================================
    91|*                                      LOCAL FUNCTIONS
    92|==================================================================================================*/
    93|#define FIM_START_SEC_CODE
    94|#include "MemMap.h"
    95|
    96|/**
    97| * @brief   Find function configuration by Function ID
    98| */
    99|STATIC const FiM_FunctionConfigType* FiM_FindFunctionConfig(FiM_FunctionIdType FID)
   100|{
   101|    const FiM_FunctionConfigType* result = NULL_PTR;
   102|    uint16 i;
   103|
   104|    if ((FiM_InternalState.ConfigPtr != NULL_PTR) && 
   105|        (FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
   106|    {
   107|        for (i = 0U; i < FiM_InternalState.ConfigPtr->NumFunctions; i++)
   108|        {
   109|            if (FiM_InternalState.ConfigPtr->FunctionConfigs[i].FunctionId == FID)
   110|            {
   111|                result = &FiM_InternalState.ConfigPtr->FunctionConfigs[i];
   112|                break;
   113|            }
   114|        }
   115|    }
   116|
   117|    return result;
   118|}
   119|
   120|/**
   121| * @brief   Calculate inhibition mask for a function based on all related events
   122| */
   123|STATIC Std_ReturnType FiM_CalculateInhibitionMask(FiM_FunctionIdType FID, uint8* InhibitionMask)
   124|{
   125|    Std_ReturnType result = E_NOT_OK;
   126|    const FiM_FunctionConfigType* functionConfig;
   127|    boolean eventFailed;
   128|    Dem_UdsStatusByteType dtcStatus;
   129|    uint8 calculatedMask = FIM_INHIBITION_MASK_NONE;
   130|    uint8 i;
   131|
   132|    functionConfig = FiM_FindFunctionConfig(FID);
   133|
   134|    if ((functionConfig != NULL_PTR) && (InhibitionMask != NULL_PTR))
   135|    {
   136|        /* Iterate through all event inhibitions for this function */
   137|        for (i = 0U; i < functionConfig->NumEventInhibitions; i++)
   138|        {
   139|            const FiM_EventInhibitionType* eventInhibition = &functionConfig->EventInhibitions[i];
   140|            
   141|            /* Check if using summary event */
   142|            if (eventInhibition->UseSummaryEvent)
   143|            {
   144|                /* Use summary event state */
   145|                if ((eventInhibition->SummaryEventId < FIM_NUM_SUMMARY_EVENTS) &&
   146|                    (FiM_InternalState.SummaryEventStates[eventInhibition->SummaryEventId].IsFailed))
   147|                {
   148|                    calculatedMask |= eventInhibition->InhibitionMask;
   149|                }
   150|            }
   151|            else
   152|            {
   153|                /* Check event failed status using DEM API */
   154|                if (Dem_GetEventFailed(eventInhibition->EventId, &eventFailed) == E_OK)
   155|                {
   156|                    /* Build a UDS-like status byte from event status */
   157|                    dtcStatus = 0U;
   158|                    
   159|                    if (eventFailed)
   160|                    {
   161|                        dtcStatus |= DEM_UDS_STATUS_TF;  /* Test Failed */
   162|                        dtcStatus |= DEM_UDS_STATUS_TFTOC; /* Test Failed This Operation Cycle */
   163|                        dtcStatus |= DEM_UDS_STATUS_PDTC;  /* Pending DTC */
   164|                    }
   165|                    
   166|                    /* Check if event inhibits this function */
   167|                    if (FiM_CheckEventInhibition(eventInhibition, dtcStatus))
   168|                    {
   169|                        calculatedMask |= eventInhibition->InhibitionMask;
   170|                    }
   171|                }
   172|            }
   173|        }
   174|
   175|        *InhibitionMask = calculatedMask;
   176|        result = E_OK;
   177|    }
   178|
   179|    return result;
   180|}
   181|
   182|/**
   183| * @brief   Check if event should inhibit function based on current DTC status
   184| */
   185|STATIC boolean FiM_CheckEventInhibition(const FiM_EventInhibitionType* EventInhibition, uint8 CurrentDtcStatus)
   186|{
   187|    boolean inhibit = FALSE;
   188|
   189|    if (EventInhibition != NULL_PTR)
   190|    {
   191|        /* Check if any of the configured inhibition conditions match */
   192|        if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_FAILED) &&
   193|            (CurrentDtcStatus & DEM_UDS_STATUS_TF))
   194|        {
   195|            inhibit = TRUE;
   196|        }
   197|        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_FAILED_TOC) &&
   198|                 (CurrentDtcStatus & DEM_UDS_STATUS_TFTOC))
   199|        {
   200|            inhibit = TRUE;
   201|        }
   202|        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_PENDING) &&
   203|                 (CurrentDtcStatus & DEM_UDS_STATUS_PDTC))
   204|        {
   205|            inhibit = TRUE;
   206|        }
   207|        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_CONFIRMED) &&
   208|                 (CurrentDtcStatus & DEM_UDS_STATUS_CDTC))
   209|        {
   210|            inhibit = TRUE;
   211|        }
   212|        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_NOT_COMPLETED) &&
   213|                 (CurrentDtcStatus & DEM_UDS_STATUS_TNCTOC))
   214|        {
   215|            inhibit = TRUE;
   216|        }
   217|        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_WARNING_INDICATOR) &&
   218|                 (CurrentDtcStatus & DEM_UDS_STATUS_WIR))
   219|        {
   220|            inhibit = TRUE;
   221|        }
   222|    }
   223|
   224|    return inhibit;
   225|}
   226|
   227|/**
   228| * @brief   Update function permission based on inhibition mask
   229| */
   230|STATIC void FiM_UpdateFunctionPermission(FiM_FunctionIdType FID)
   231|{
   232|    uint8 inhibitionMask = FIM_INHIBITION_MASK_NONE;
   233|    uint16 functionIndex;
   234|
   235|    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
   236|    {
   237|        functionIndex = FID - FIM_FID_MIN;
   238|
   239|        /* Calculate current inhibition mask */
   240|        if (FiM_CalculateInhibitionMask(FID, &inhibitionMask) == E_OK)
   241|        {
   242|            FiM_InternalState.FunctionStates[functionIndex].LastCalculatedInhibitionMask = inhibitionMask;
   243|
   244|            /* Update inhibition status */
   245|            if (inhibitionMask != FIM_INHIBITION_MASK_NONE)
   246|            {
   247|                FiM_InternalState.FunctionStates[functionIndex].InhibitionStatus = FIM_INHIBITED_YES;
   248|                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_DENIED;
   249|            }
   250|            else
   251|            {
   252|                FiM_InternalState.FunctionStates[functionIndex].InhibitionStatus = FIM_INHIBITED_NO;
   253|                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_ALLOWED;
   254|            }
   255|        }
   256|    }
   257|}
   258|
   259|/**
   260| * @brief   Update summary event state based on its component events
   261| */
   262|STATIC void FiM_UpdateSummaryEventState(FiM_SummaryEventIdType SummaryEventId)
   263|{
   264|    if ((SummaryEventId >= FIM_SUMMARY_EVENT_ID_MIN) && 
   265|        (SummaryEventId < FIM_SUMMARY_EVENT_ID_MAX + FIM_SUMMARY_EVENT_ID_MIN))
   266|    {
   267|        uint16 index = SummaryEventId - FIM_SUMMARY_EVENT_ID_MIN;
   268|        boolean isFailed = FALSE;
   269|        
   270|        /* Check if the summary event is a valid DEM event */
   271|        if (SummaryEventId > 0U)
   272|        {
   273|            if (Dem_GetEventFailed(SummaryEventId, &isFailed) != E_OK)
   274|            {
   275|                isFailed = FALSE;
   276|            }
   277|        }
   278|        
   279|        FiM_InternalState.SummaryEventStates[index].IsFailed = isFailed;
   280|    }
   281|}
   282|
   283|/*==================================================================================================
   284|*                                      GLOBAL FUNCTIONS
   285|==================================================================================================*/
   286|
   287|/**
   288| * @brief   Initializes the FiM module
   289| */
   290|void FiM_Init(const FiM_ConfigType* ConfigPtr)
   291|{
   292|    uint16 i;
   293|
   294|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   295|    if (ConfigPtr == NULL_PTR)
   296|    {
   297|        FIM_DET_REPORT_ERROR(FIM_SID_INIT, FIM_E_PARAM_POINTER);
   298|        return;
   299|    }
   300|
   301|    if (ConfigPtr->NumFunctions > FIM_NUM_FUNCTIONS)
   302|    {
   303|        FIM_DET_REPORT_ERROR(FIM_SID_INIT, FIM_E_PARAM_CONFIG);
   304|        return;
   305|    }
   306|#endif
   307|
   308|    /* Store configuration pointer */
   309|    FiM_InternalState.ConfigPtr = ConfigPtr;
   310|
   311|    /* Initialize function states */
   312|    for (i = 0U; i < FIM_NUM_FUNCTIONS; i++)
   313|    {
   314|        FiM_InternalState.FunctionStates[i].Permission = FIM_DEFAULT_PERMISSION;
   315|        FiM_InternalState.FunctionStates[i].Available = FIM_DEFAULT_AVAILABILITY;
   316|        FiM_InternalState.FunctionStates[i].InhibitionStatus = FIM_INHIBITED_NO;
   317|        FiM_InternalState.FunctionStates[i].LastCalculatedInhibitionMask = FIM_INHIBITION_MASK_NONE;
   318|    }
   319|
   320|    /* Initialize summary event states */
   321|    for (i = 0U; i < FIM_NUM_SUMMARY_EVENTS; i++)
   322|    {
   323|        FiM_InternalState.SummaryEventStates[i].IsFailed = FALSE;
   324|        FiM_InternalState.SummaryEventStates[i].InhibitionMask = FIM_INHIBITION_MASK_NONE;
   325|    }
   326|
   327|    /* Calculate initial permissions for all configured functions */
   328|    for (i = 0U; i < ConfigPtr->NumFunctions; i++)
   329|    {
   330|        if (ConfigPtr->FunctionConfigs[i].FunctionId != FIM_FID_INVALID)
   331|        {
   332|            FiM_UpdateFunctionPermission(ConfigPtr->FunctionConfigs[i].FunctionId);
   333|        }
   334|    }
   335|
   336|    /* Set module state to initialized */
   337|    FiM_InternalState.State = FIM_STATE_INIT;
   338|}
   339|
   340|/**
   341| * @brief   Deinitializes the FiM module
   342| */
   343|void FiM_DeInit(void)
   344|{
   345|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   346|    if (FiM_InternalState.State != FIM_STATE_INIT)
   347|    {
   348|        FIM_DET_REPORT_ERROR(FIM_SID_DEINIT, FIM_E_UNINIT);
   349|        return;
   350|    }
   351|#endif
   352|
   353|    /* Clear configuration pointer */
   354|    FiM_InternalState.ConfigPtr = NULL_PTR;
   355|
   356|    /* Set module state to uninitialized */
   357|    FiM_InternalState.State = FIM_STATE_UNINIT;
   358|}
   359|
   360|/**
   361| * @brief   Set function availability
   362| */
   363|Std_ReturnType FiM_SetFunctionAvailable(FiM_FunctionIdType FID, boolean Availability)
   364|{
   365|    Std_ReturnType result = E_NOT_OK;
   366|
   367|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   368|    if (FiM_InternalState.State != FIM_STATE_INIT)
   369|    {
   370|        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONAVAILABLE, FIM_E_UNINIT);
   371|        return E_NOT_OK;
   372|    }
   373|
   374|    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
   375|    {
   376|        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONAVAILABLE, FIM_E_PARAM_FID);
   377|        return E_NOT_OK;
   378|    }
   379|#endif
   380|
   381|    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
   382|    {
   383|        uint16 functionIndex = FID - FIM_FID_MIN;
   384|        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);
   385|
   386|        if (functionConfig != NULL_PTR)
   387|        {
   388|            FiM_InternalState.FunctionStates[functionIndex].Available = Availability;
   389|            
   390|            /* Recalculate permission based on availability */
   391|            if (Availability == FALSE)
   392|            {
   393|                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_DENIED;
   394|            }
   395|            else
   396|            {
   397|                FiM_UpdateFunctionPermission(FID);
   398|            }
   399|            
   400|            result = E_OK;
   401|        }
   402|    }
   403|
   404|    return result;
   405|}
   406|
   407|/**
   408| * @brief   Get function permission
   409| */
   410|Std_ReturnType FiM_GetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType* Permission)
   411|{
   412|    Std_ReturnType result = E_NOT_OK;
   413|
   414|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   415|    if (FiM_InternalState.State != FIM_STATE_INIT)
   416|    {
   417|        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_UNINIT);
   418|        return E_NOT_OK;
   419|    }
   420|
   421|    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
   422|    {
   423|        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_PARAM_FID);
   424|        return E_NOT_OK;
   425|    }
   426|
   427|    if (Permission == NULL_PTR)
   428|    {
   429|        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_PARAM_POINTER);
   430|        return E_NOT_OK;
   431|    }
   432|#endif
   433|
   434|    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX) && (Permission != NULL_PTR))
   435|    {
   436|        uint16 functionIndex = FID - FIM_FID_MIN;
   437|        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);
   438|
   439|        if (functionConfig != NULL_PTR)
   440|        {
   441|            /* Return permission - also check if function is available */
   442|            if (FiM_InternalState.FunctionStates[functionIndex].Available == FALSE)
   443|            {
   444|                *Permission = FIM_PERMISSION_DENIED;
   445|            }
   446|            else
   447|            {
   448|                *Permission = FiM_InternalState.FunctionStates[functionIndex].Permission;
   449|            }
   450|            result = E_OK;
   451|        }
   452|    }
   453|
   454|    return result;
   455|}
   456|
   457|/**
   458| * @brief   Set function permission (for testing purposes)
   459| */
   460|Std_ReturnType FiM_SetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType Permission)
   461|{
   462|    Std_ReturnType result = E_NOT_OK;
   463|
   464|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   465|    if (FiM_InternalState.State != FIM_STATE_INIT)
   466|    {
   467|        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONPERMISSION, FIM_E_UNINIT);
   468|        return E_NOT_OK;
   469|    }
   470|
   471|    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
   472|    {
   473|        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONPERMISSION, FIM_E_PARAM_FID);
   474|        return E_NOT_OK;
   475|    }
   476|#endif
   477|
   478|    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
   479|    {
   480|        uint16 functionIndex = FID - FIM_FID_MIN;
   481|        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);
   482|
   483|        if (functionConfig != NULL_PTR)
   484|        {
   485|            FiM_InternalState.FunctionStates[functionIndex].Permission = Permission;
   486|            result = E_OK;
   487|        }
   488|    }
   489|
   490|    return result;
   491|}
   492|
   493|/**
   494| * @brief   Get inhibition status
   495| */
   496|Std_ReturnType FiM_GetInhibitionStatus(FiM_FunctionIdType FID, FiM_InhibitionStatusType* InhibitionStatus)
   497|{
   498|    Std_ReturnType result = E_NOT_OK;
   499|
   500|#if (FIM_DEV_ERROR_DETECT == STD_ON)
   501|