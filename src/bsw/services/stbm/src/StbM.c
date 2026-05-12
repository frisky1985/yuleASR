/**
 * @file StbM.c
 * @brief Synchronized Time Base Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : Ethernet
     5|* Dependencies         : Eth, Det
     6|*
     7|* SW Version           : 4.7.0
     8|* Build Version        : YULETECH_AUTOSAR_4.7.0
     9|* Build Date           : 2026-04-29
    10|* Author               : AI Agent (StbM Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "StbM.h"
    20|#include "StbM_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|#include <string.h>
    24|
    25|/*==================================================================================================
    26|*                                  LOCAL CONSTANT DEFINITIONS
    27|==================================================================================================*/
    28|#define STBM_STATE_UNINIT                       (0x00U)
    29|#define STBM_STATE_INIT                         (0x01U)
    30|
    31|/* Time conversion constants */
    32|#define STBM_NS_PER_SECOND                      (1000000000ULL)
    33|#define STBM_US_PER_SECOND                      (1000000ULL)
    34|#define STBM_MS_PER_SECOND                      (1000ULL)
    35|
    36|/*==================================================================================================
    37|*                                  LOCAL MACRO DEFINITIONS
    38|==================================================================================================*/
    39|#if (STBM_DEV_ERROR_DETECT == STD_ON)
    40|    #define STBM_DET_REPORT_ERROR(ApiId, ErrorId) \
    41|        Det_ReportError(STBM_MODULE_ID, STBM_INSTANCE_ID, (ApiId), (ErrorId))
    42|#else
    43|    #define STBM_DET_REPORT_ERROR(ApiId, ErrorId)
    44|#endif
    45|
    46|#define STBM_IS_VALID_TIMEBASE_ID(Id) \
    47|    (((Id) < STBM_NUMBER_OF_TIMEBASES) ? TRUE : FALSE)
    48|
    49|/*==================================================================================================
    50|*                                  LOCAL TYPE DEFINITIONS
    51|==================================================================================================*/
    52|typedef struct {
    53|    StbM_TimeStampType globalTime;
    54|    StbM_VirtualLocalTimeType localTime;
    55|    StbM_VirtualLocalTimeType lastSyncLocalTime;
    56|    StbM_UserDataType userData;
    57|    uint8 syncStatus;
    58|    uint8 timeBaseStatus;
    59|    uint32 updateCounter;
    60|    StbM_RateDeviationType rateDeviation;
    61|    uint32 timeoutCounter;
    62|    boolean isMaster;
    63|    boolean timeValid;
    64|} StbM_TimeBaseType;
    65|
    66|typedef struct {
    67|    uint8 State;
    68|    const StbM_ConfigType* ConfigPtr;
    69|    StbM_TimeBaseType TimeBases[STBM_NUMBER_OF_TIMEBASES];
    70|} StbM_InternalStateType;
    71|
    72|/*==================================================================================================
    73|*                                  LOCAL VARIABLE DECLARATIONS
    74|==================================================================================================*/
    75|#define STBM_START_SEC_VAR_CLEARED_UNSPECIFIED
    76|#include "MemMap.h"
    77|
    78|STATIC StbM_InternalStateType StbM_InternalState;
    79|
    80|#define STBM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    81|#include "MemMap.h"
    82|
    83|/*==================================================================================================
    84|*                                  LOCAL FUNCTION PROTOTYPES
    85|==================================================================================================*/
    86|STATIC Std_ReturnType StbM_FindTimeBaseConfig(uint8 timeBaseId, const StbM_TimeBaseConfigType** configPtr);
    87|STATIC void StbM_UpdateTimeBases(void);
    88|STATIC void StbM_UpdateTimeouts(void);
    89|STATIC StbM_VirtualLocalTimeType StbM_GetVirtualLocalTime(uint8 timeBaseId);
    90|STATIC void StbM_UpdateGlobalTime(uint8 timeBaseId, StbM_VirtualLocalTimeType currentLocalTime);
    91|
    92|/*==================================================================================================
    93|*                                      LOCAL FUNCTIONS
    94|==================================================================================================*/
    95|#define STBM_START_SEC_CODE
    96|#include "MemMap.h"
    97|
    98|/**
    99| * @brief   Find time base configuration
   100| */
   101|STATIC Std_ReturnType StbM_FindTimeBaseConfig(uint8 timeBaseId, const StbM_TimeBaseConfigType** configPtr)
   102|{
   103|    Std_ReturnType result = E_NOT_OK;
   104|    const StbM_ConfigType* cfgPtr = StbM_InternalState.ConfigPtr;
   105|    uint8 i;
   106|
   107|    if (cfgPtr != NULL_PTR)
   108|    {
   109|        for (i = 0U; i < cfgPtr->numTimeBases; i++)
   110|        {
   111|            if (cfgPtr->timeBaseConfigs[i].timeBaseId == timeBaseId)
   112|            {
   113|                *configPtr = &cfgPtr->timeBaseConfigs[i];
   114|                result = E_OK;
   115|                break;
   116|            }
   117|        }
   118|    }
   119|
   120|    return result;
   121|}
   122|
   123|/**
   124| * @brief   Get virtual local time from hardware
   125| */
   126|STATIC StbM_VirtualLocalTimeType StbM_GetVirtualLocalTime(uint8 timeBaseId)
   127|{
   128|    StbM_VirtualLocalTimeType localTime = 0ULL;
   129|    const StbM_TimeBaseConfigType* configPtr;
   130|    Eth_TimeStampType ethTimeStamp;
   131|    Eth_RxStatusType ethStatus;
   132|
   133|    (void)timeBaseId;
   134|
   135|    /* Get time from Ethernet hardware timestamp */
   136|    if (StbM_FindTimeBaseConfig(timeBaseId, &configPtr) == E_OK)
   137|    {
   138|        if (Eth_GetCurrentTime(configPtr->ethControllerId, &ethTimeStamp, &ethStatus) == E_OK)
   139|        {
   140|            /* Convert Eth_TimeStampType to VirtualLocalTimeType */
   141|            localTime = ((StbM_VirtualLocalTimeType)ethTimeStamp.seconds * STBM_NS_PER_SECOND) +
   142|                        (StbM_VirtualLocalTimeType)ethTimeStamp.nanoseconds;
   143|        }
   144|    }
   145|
   146|    return localTime;
   147|}
   148|
   149|/**
   150| * @brief   Update global time based on local time and rate deviation
   151| */
   152|STATIC void StbM_UpdateGlobalTime(uint8 timeBaseId, StbM_VirtualLocalTimeType currentLocalTime)
   153|{
   154|    StbM_TimeBaseType* tbPtr = &StbM_InternalState.TimeBases[timeBaseId];
   155|    sint64 timeDiff;
   156|    sint64 timeIncrement;
   157|
   158|    if (tbPtr->timeValid)
   159|    {
   160|        /* Calculate elapsed time in local ticks */
   161|        timeDiff = (sint64)(currentLocalTime - tbPtr->localTime);
   162|
   163|        /* Apply rate correction */
   164|        timeIncrement = (timeDiff * (1000000LL + tbPtr->rateDeviation)) / 1000000LL;
   165|
   166|        /* Update global time */
   167|        tbPtr->globalTime.nanoseconds += (uint32)(timeIncrement % STBM_NS_PER_SECOND);
   168|        tbPtr->globalTime.seconds += (uint32)(timeIncrement / STBM_NS_PER_SECOND);
   169|
   170|        /* Handle nanoseconds overflow */
   171|        if (tbPtr->globalTime.nanoseconds >= STBM_NS_PER_SECOND)
   172|        {
   173|            tbPtr->globalTime.nanoseconds -= (uint32)STBM_NS_PER_SECOND;
   174|            tbPtr->globalTime.seconds++;
   175|        }
   176|    }
   177|
   178|    /* Update local time */
   179|    tbPtr->localTime = currentLocalTime;
   180|}
   181|
   182|/**
   183| * @brief   Update all time bases
   184| */
   185|STATIC void StbM_UpdateTimeBases(void)
   186|{
   187|    uint8 i;
   188|    StbM_VirtualLocalTimeType currentTime;
   189|
   190|    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
   191|    {
   192|        currentTime = StbM_GetVirtualLocalTime(i);
   193|        StbM_UpdateGlobalTime(i, currentTime);
   194|    }
   195|}
   196|
   197|/**
   198| * @brief   Update timeouts for all time bases
   199| */
   200|STATIC void StbM_UpdateTimeouts(void)
   201|{
   202|    uint8 i;
   203|    StbM_TimeBaseType* tbPtr;
   204|    const StbM_TimeBaseConfigType* configPtr;
   205|
   206|    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
   207|    {
   208|        tbPtr = &StbM_InternalState.TimeBases[i];
   209|
   210|        if (tbPtr->syncStatus != STBM_SYNC_STATUS_UNKNOWN)
   211|        {
   212|            if (StbM_FindTimeBaseConfig(i, &configPtr) == E_OK)
   213|            {
   214|                tbPtr->timeoutCounter += STBM_MAIN_FUNCTION_PERIOD_MS;
   215|
   216|                if (tbPtr->timeoutCounter >= configPtr->syncTimeout)
   217|                {
   218|                    tbPtr->syncStatus = STBM_SYNC_STATUS_SYNC_LOST;
   219|                    tbPtr->timeBaseStatus = STBM_TIMEBASE_STATUS_TIMEOUT;
   220|                    tbPtr->timeValid = FALSE;
   221|                }
   222|            }
   223|        }
   224|    }
   225|}
   226|
   227|/*==================================================================================================
   228|*                                      GLOBAL FUNCTIONS
   229|==================================================================================================*/
   230|
   231|/**
   232| * @brief   Initializes the StbM module
   233| */
   234|void StbM_Init(const StbM_ConfigType* ConfigPtr)
   235|{
   236|    uint8 i;
   237|
   238|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   239|    if (StbM_InternalState.State == STBM_STATE_INIT)
   240|    {
   241|        STBM_DET_REPORT_ERROR(STBM_SID_INIT, STBM_E_ALREADY_INITIALIZED);
   242|        return;
   243|    }
   244|
   245|    if (ConfigPtr == NULL_PTR)
   246|    {
   247|        STBM_DET_REPORT_ERROR(STBM_SID_INIT, STBM_E_PARAM_POINTER);
   248|        return;
   249|    }
   250|#endif
   251|
   252|    /* Store configuration pointer */
   253|    StbM_InternalState.ConfigPtr = ConfigPtr;
   254|
   255|    /* Initialize time bases */
   256|    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
   257|    {
   258|        StbM_InternalState.TimeBases[i].globalTime.seconds = 0U;
   259|        StbM_InternalState.TimeBases[i].globalTime.nanoseconds = 0U;
   260|        StbM_InternalState.TimeBases[i].globalTime.secondsHi = 0U;
   261|        StbM_InternalState.TimeBases[i].localTime = 0ULL;
   262|        StbM_InternalState.TimeBases[i].lastSyncLocalTime = 0ULL;
   263|        StbM_InternalState.TimeBases[i].syncStatus = STBM_SYNC_STATUS_UNKNOWN;
   264|        StbM_InternalState.TimeBases[i].timeBaseStatus = STBM_TIMEBASE_STATUS_PENDING;
   265|        StbM_InternalState.TimeBases[i].updateCounter = 0U;
   266|        StbM_InternalState.TimeBases[i].rateDeviation = 0;
   267|        StbM_InternalState.TimeBases[i].timeoutCounter = 0U;
   268|        StbM_InternalState.TimeBases[i].timeValid = FALSE;
   269|        StbM_InternalState.TimeBases[i].userData.userByte0 = 0U;
   270|        StbM_InternalState.TimeBases[i].userData.userByte1 = 0U;
   271|        StbM_InternalState.TimeBases[i].userData.userByte2 = 0U;
   272|
   273|        /* Determine master/slave from configuration */
   274|        if (i < ConfigPtr->numTimeBases)
   275|        {
   276|            StbM_InternalState.TimeBases[i].isMaster = 
   277|                (ConfigPtr->timeBaseConfigs[i].masterConfig == STBM_MASTER_CONFIG_MASTER) ?
   278|                TRUE : FALSE;
   279|        }
   280|    }
   281|
   282|    /* Set module state to initialized */
   283|    StbM_InternalState.State = STBM_STATE_INIT;
   284|}
   285|
   286|/**
   287| * @brief   Deinitializes the StbM module
   288| */
   289|void StbM_DeInit(void)
   290|{
   291|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   292|    if (StbM_InternalState.State != STBM_STATE_INIT)
   293|    {
   294|        STBM_DET_REPORT_ERROR(STBM_SID_DEINIT, STBM_E_UNINIT);
   295|        return;
   296|    }
   297|#endif
   298|
   299|    /* Clear configuration pointer */
   300|    StbM_InternalState.ConfigPtr = NULL_PTR;
   301|
   302|    /* Set module state to uninitialized */
   303|    StbM_InternalState.State = STBM_STATE_UNINIT;
   304|}
   305|
   306|/**
   307| * @brief   Gets version information
   308| */
   309|#if (STBM_VERSION_INFO_API == STD_ON)
   310|void StbM_GetVersionInfo(Std_VersionInfoType* versioninfo)
   311|{
   312|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   313|    if (versioninfo == NULL_PTR)
   314|    {
   315|        STBM_DET_REPORT_ERROR(STBM_SID_GETVERSIONINFO, STBM_E_PARAM_POINTER);
   316|        return;
   317|    }
   318|#endif
   319|
   320|    versioninfo->vendorID = STBM_VENDOR_ID;
   321|    versioninfo->moduleID = STBM_MODULE_ID;
   322|    versioninfo->sw_major_version = STBM_SW_MAJOR_VERSION;
   323|    versioninfo->sw_minor_version = STBM_SW_MINOR_VERSION;
   324|    versioninfo->sw_patch_version = STBM_SW_PATCH_VERSION;
   325|}
   326|#endif
   327|
   328|/**
   329| * @brief   Gets current synchronized time
   330| */
   331|Std_ReturnType StbM_GetCurrentTime(uint8 timeBaseId, 
   332|                                    StbM_TimeStampType* timeStampPtr,
   333|                                    StbM_UserDataType* userDataPtr)
   334|{
   335|    Std_ReturnType result = E_NOT_OK;
   336|    StbM_TimeBaseType* tbPtr;
   337|
   338|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   339|    if (StbM_InternalState.State != STBM_STATE_INIT)
   340|    {
   341|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_UNINIT);
   342|        return E_NOT_OK;
   343|    }
   344|
   345|    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
   346|    {
   347|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_INVALID_TIMEBASE_ID);
   348|        return E_NOT_OK;
   349|    }
   350|
   351|    if (timeStampPtr == NULL_PTR)
   352|    {
   353|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_PARAM_POINTER);
   354|        return E_NOT_OK;
   355|    }
   356|#endif
   357|
   358|    tbPtr = &StbM_InternalState.TimeBases[timeBaseId];
   359|
   360|    if (tbPtr->timeValid)
   361|    {
   362|        /* Update time before returning */
   363|        StbM_VirtualLocalTimeType currentTime = StbM_GetVirtualLocalTime(timeBaseId);
   364|        StbM_UpdateGlobalTime(timeBaseId, currentTime);
   365|
   366|        /* Copy current time */
   367|        timeStampPtr->seconds = tbPtr->globalTime.seconds;
   368|        timeStampPtr->nanoseconds = tbPtr->globalTime.nanoseconds;
   369|        timeStampPtr->secondsHi = tbPtr->globalTime.secondsHi;
   370|
   371|        /* Copy user data if requested */
   372|        if (userDataPtr != NULL_PTR)
   373|        {
   374|            userDataPtr->userByte0 = tbPtr->userData.userByte0;
   375|            userDataPtr->userByte1 = tbPtr->userData.userByte1;
   376|            userDataPtr->userByte2 = tbPtr->userData.userByte2;
   377|        }
   378|
   379|        result = E_OK;
   380|    }
   381|
   382|    return result;
   383|}
   384|
   385|/**
   386| * @brief   Gets current virtual local time
   387| */
   388|Std_ReturnType StbM_GetCurrentVirtualTime(uint8 timeBaseId,
   389|                                           StbM_VirtualLocalTimeType* virtualLocalTimePtr)
   390|{
   391|    Std_ReturnType result = E_NOT_OK;
   392|
   393|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   394|    if (StbM_InternalState.State != STBM_STATE_INIT)
   395|    {
   396|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_UNINIT);
   397|        return E_NOT_OK;
   398|    }
   399|
   400|    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
   401|    {
   402|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_INVALID_TIMEBASE_ID);
   403|        return E_NOT_OK;
   404|    }
   405|
   406|    if (virtualLocalTimePtr == NULL_PTR)
   407|    {
   408|        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_PARAM_POINTER);
   409|        return E_NOT_OK;
   410|    }
   411|#endif
   412|
   413|    *virtualLocalTimePtr = StbM_GetVirtualLocalTime(timeBaseId);
   414|    result = E_OK;
   415|
   416|    return result;
   417|}
   418|
   419|/**
   420| * @brief   Sets global time
   421| */
   422|Std_ReturnType StbM_SetGlobalTime(uint8 timeBaseId,
   423|                                   const StbM_TimeStampType* timeStampPtr,
   424|                                   const StbM_UserDataType* userDataPtr)
   425|{
   426|    Std_ReturnType result = E_NOT_OK;
   427|    StbM_TimeBaseType* tbPtr;
   428|
   429|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   430|    if (StbM_InternalState.State != STBM_STATE_INIT)
   431|    {
   432|        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_UNINIT);
   433|        return E_NOT_OK;
   434|    }
   435|
   436|    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
   437|    {
   438|        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_INVALID_TIMEBASE_ID);
   439|        return E_NOT_OK;
   440|    }
   441|
   442|    if (timeStampPtr == NULL_PTR)
   443|    {
   444|        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_PARAM_POINTER);
   445|        return E_NOT_OK;
   446|    }
   447|#endif
   448|
   449|    tbPtr = &StbM_InternalState.TimeBases[timeBaseId];
   450|
   451|    /* Only master can set global time directly */
   452|    if (tbPtr->isMaster)
   453|    {
   454|        tbPtr->globalTime.seconds = timeStampPtr->seconds;
   455|        tbPtr->globalTime.nanoseconds = timeStampPtr->nanoseconds;
   456|        tbPtr->globalTime.secondsHi = timeStampPtr->secondsHi;
   457|        tbPtr->localTime = StbM_GetVirtualLocalTime(timeBaseId);
   458|        tbPtr->timeValid = TRUE;
   459|        tbPtr->updateCounter++;
   460|
   461|        if (userDataPtr != NULL_PTR)
   462|        {
   463|            tbPtr->userData.userByte0 = userDataPtr->userByte0;
   464|            tbPtr->userData.userByte1 = userDataPtr->userByte1;
   465|            tbPtr->userData.userByte2 = userDataPtr->userByte2;
   466|        }
   467|
   468|        result = E_OK;
   469|    }
   470|
   471|    return result;
   472|}
   473|
   474|/**
   475| * @brief   Sets global time from bus (time sync protocol)
   476| */
   477|Std_ReturnType StbM_BusSetGlobalTime(uint8 timeBaseId,
   478|                                      const StbM_TimeStampType* timeStampPtr,
   479|                                      const StbM_VirtualLocalTimeType* virtualLocalTimePtr,
   480|                                      const StbM_UserDataType* userDataPtr)
   481|{
   482|    Std_ReturnType result = E_NOT_OK;
   483|    StbM_TimeBaseType* tbPtr;
   484|    StbM_VirtualLocalTimeType rxLocalTime;
   485|
   486|#if (STBM_DEV_ERROR_DETECT == STD_ON)
   487|    if (StbM_InternalState.State != STBM_STATE_INIT)
   488|    {
   489|        STBM_DET_REPORT_ERROR(STBM_SID_BUSSETGLOBALTIME, STBM_E_UNINIT);
   490|        return E_NOT_OK;
   491|    }
   492|
   493|    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
   494|    {
   495|        STBM_DET_REPORT_ERROR(STBM_SID_BUSSETGLOBALTIME, STBM_E_INVALID_TIMEBASE_ID);
   496|        return E_NOT_OK;
   497|    }
   498|
   499|    if ((timeStampPtr == NULL_PTR) || (virtualLocalTimePtr == NULL_PTR))
   500|    {
   501|