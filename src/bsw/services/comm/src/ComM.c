/**
 * @file ComM.c
 * @brief Communication Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/**
     2| * @file ComM.c
     3| * @brief Communication Manager Implementation
     4| */
     5|
     6|#include "ComM.h"
     7|#include "ComM_Cfg.h"
     8|#include "Det.h"
     9|
    10|/* Internal State */
    11|static ComM_StateType ComM_State = COMM_STATE_UNINIT;
    12|static ComM_ModeType ComM_RequestedMode[COMM_MAX_USERS];
    13|static ComM_ModeType ComM_CurrentMode[COMM_MAX_CHANNELS];
    14|static boolean ComM_CommunicationAllowedFlag[COMM_MAX_CHANNELS];
    15|static boolean ComM_DiagnosticActive[COMM_MAX_CHANNELS];
    16|static uint32 ComM_UserRequestMask[COMM_MAX_CHANNELS];
    17|
    18|/* Version Info */
    19|#define COMM_VENDOR_ID                      0x0001
    20|#define COMM_INSTANCE_ID                    0x00
    21|
    22|void ComM_Init(const ComM_ConfigType* ConfigPtr)
    23|{
    24|#if (COMM_DEV_ERROR_DETECT == STD_ON)
    25|    if (ConfigPtr == NULL_PTR)
    26|    {
    27|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_INIT_SID, COMM_E_WRONG_PARAMETERS);
    28|        return;
    29|    }
    30|#endif
    31|
    32|    /* Initialize state */
    33|    ComM_State = COMM_STATE_INIT;
    34|    
    35|    /* Initialize user request arrays */
    36|    for (uint8 i = 0; i < COMM_MAX_USERS; i++)
    37|    {
    38|        ComM_RequestedMode[i] = COMM_NO_COMMUNICATION;
    39|    }
    40|    
    41|    /* Initialize channel states */
    42|    for (uint8 i = 0; i < COMM_MAX_CHANNELS; i++)
    43|    {
    44|        ComM_CurrentMode[i] = COMM_NO_COMMUNICATION;
    45|        ComM_CommunicationAllowedFlag[i] = FALSE;
    46|        ComM_DiagnosticActive[i] = FALSE;
    47|        ComM_UserRequestMask[i] = 0;
    48|    }
    49|}
    50|
    51|void ComM_DeInit(void)
    52|{
    53|#if (COMM_DEV_ERROR_DETECT == STD_ON)
    54|    if (ComM_State == COMM_STATE_UNINIT)
    55|    {
    56|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_DEINIT_SID, COMM_E_NOT_INIT);
    57|        return;
    58|    }
    59|#endif
    60|
    61|    ComM_State = COMM_STATE_UNINIT;
    62|}
    63|
    64|void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
    65|{
    66|#if (COMM_VERSION_INFO_API == STD_ON)
    67|    if (VersionInfo != NULL_PTR)
    68|    {
    69|        VersionInfo->vendorID = COMM_VENDOR_ID;
    70|        VersionInfo->moduleID = COMM_MODULE_ID;
    71|        VersionInfo->sw_major_version = COMM_SW_MAJOR_VERSION;
    72|        VersionInfo->sw_minor_version = COMM_SW_MINOR_VERSION;
    73|        VersionInfo->sw_patch_version = COMM_SW_PATCH_VERSION;
    74|    }
    75|#endif
    76|}
    77|
    78|Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode)
    79|{
    80|#if (COMM_DEV_ERROR_DETECT == STD_ON)
    81|    if (ComM_State == COMM_STATE_UNINIT)
    82|    {
    83|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_NOT_INIT);
    84|        return E_NOT_OK;
    85|    }
    86|    if (User >= COMM_MAX_USERS)
    87|    {
    88|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
    89|        return E_NOT_OK;
    90|    }
    91|    if (ComMode > COMM_FULL_COMMUNICATION)
    92|    {
    93|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
    94|        return E_NOT_OK;
    95|    }
    96|#endif
    97|
    98|    /* Store the requested mode */
    99|    ComM_RequestedMode[User] = ComMode;
   100|    
   101|    /* TODO: Notify BusSM of mode change request */
   102|    /* TODO: Update channel state based on requests */
   103|    
   104|    return E_OK;
   105|}
   106|
   107|Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
   108|{
   109|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   110|    if (ComM_State == COMM_STATE_UNINIT)
   111|    {
   112|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_NOT_INIT);
   113|        return E_NOT_OK;
   114|    }
   115|    if (User >= COMM_MAX_USERS)
   116|    {
   117|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   118|        return E_NOT_OK;
   119|    }
   120|    if (ComModePtr == NULL_PTR)
   121|    {
   122|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   123|        return E_NOT_OK;
   124|    }
   125|#endif
   126|
   127|    /* Return highest requested mode across all users */
   128|    ComM_ModeType maxMode = COMM_NO_COMMUNICATION;
   129|    for (uint8 i = 0; i < COMM_MAX_USERS; i++)
   130|    {
   131|        if (ComM_RequestedMode[i] > maxMode)
   132|        {
   133|            maxMode = ComM_RequestedMode[i];
   134|        }
   135|    }
   136|    *ComModePtr = maxMode;
   137|    
   138|    return E_OK;
   139|}
   140|
   141|Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
   142|{
   143|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   144|    if (ComM_State == COMM_STATE_UNINIT)
   145|    {
   146|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_NOT_INIT);
   147|        return E_NOT_OK;
   148|    }
   149|    if (User >= COMM_MAX_USERS)
   150|    {
   151|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   152|        return E_NOT_OK;
   153|    }
   154|    if (ComModePtr == NULL_PTR)
   155|    {
   156|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   157|        return E_NOT_OK;
   158|    }
   159|#endif
   160|
   161|    *ComModePtr = ComM_RequestedMode[User];
   162|    return E_OK;
   163|}
   164|
   165|Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
   166|{
   167|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   168|    if (ComM_State == COMM_STATE_UNINIT)
   169|    {
   170|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_NOT_INIT);
   171|        return E_NOT_OK;
   172|    }
   173|    if (User >= COMM_MAX_USERS)
   174|    {
   175|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   176|        return E_NOT_OK;
   177|    }
   178|    if (ComModePtr == NULL_PTR)
   179|    {
   180|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
   181|        return E_NOT_OK;
   182|    }
   183|#endif
   184|
   185|    /* Return the highest current mode across all channels */
   186|    ComM_ModeType maxMode = COMM_NO_COMMUNICATION;
   187|    for (uint8 i = 0; i < COMM_MAX_CHANNELS; i++)
   188|    {
   189|        if (ComM_CurrentMode[i] > maxMode)
   190|        {
   191|            maxMode = ComM_CurrentMode[i];
   192|        }
   193|    }
   194|    *ComModePtr = maxMode;
   195|    
   196|    return E_OK;
   197|}
   198|
   199|void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed)
   200|{
   201|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   202|    if (ComM_State == COMM_STATE_UNINIT)
   203|    {
   204|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_COMMUNICATIONALLOWED_SID, COMM_E_NOT_INIT);
   205|        return;
   206|    }
   207|    if (Channel >= COMM_MAX_CHANNELS)
   208|    {
   209|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_COMMUNICATIONALLOWED_SID, COMM_E_WRONG_PARAMETERS);
   210|        return;
   211|    }
   212|#endif
   213|
   214|    ComM_CommunicationAllowedFlag[Channel] = Allowed;
   215|}
   216|
   217|void ComM_MainFunction(void)
   218|{
   219|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   220|    if (ComM_State == COMM_STATE_UNINIT)
   221|    {
   222|        return;
   223|    }
   224|#endif
   225|
   226|    /* Process mode changes for each channel */
   227|    for (uint8 channel = 0; channel < COMM_MAX_CHANNELS; channel++)
   228|    {
   229|        /* Check if diagnostic session is active */
   230|        if (ComM_DiagnosticActive[channel])
   231|        {
   232|            ComM_CurrentMode[channel] = COMM_FULL_COMMUNICATION;
   233|            continue;
   234|        }
   235|        
   236|        /* Check communication allowed flag */
   237|        if (!ComM_CommunicationAllowedFlag[channel])
   238|        {
   239|            ComM_CurrentMode[channel] = COMM_NO_COMMUNICATION;
   240|            continue;
   241|        }
   242|        
   243|        /* TODO: Implement proper state machine */
   244|        /* For now, just reflect the highest requested mode */
   245|        ComM_ModeType maxRequested = COMM_NO_COMMUNICATION;
   246|        for (uint8 user = 0; user < COMM_MAX_USERS; user++)
   247|        {
   248|            if (ComM_RequestedMode[user] > maxRequested)
   249|            {
   250|                maxRequested = ComM_RequestedMode[user];
   251|            }
   252|        }
   253|        ComM_CurrentMode[channel] = maxRequested;
   254|    }
   255|}
   256|
   257|/* DCM Interface */
   258|Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel)
   259|{
   260|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   261|    if (Channel >= COMM_MAX_CHANNELS)
   262|    {
   263|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x80, COMM_E_WRONG_PARAMETERS);
   264|        return E_NOT_OK;
   265|    }
   266|#endif
   267|
   268|    ComM_DiagnosticActive[Channel] = TRUE;
   269|    return E_OK;
   270|}
   271|
   272|Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel)
   273|{
   274|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   275|    if (Channel >= COMM_MAX_CHANNELS)
   276|    {
   277|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x81, COMM_E_WRONG_PARAMETERS);
   278|        return E_NOT_OK;
   279|    }
   280|#endif
   281|
   282|    ComM_DiagnosticActive[Channel] = FALSE;
   283|    return E_OK;
   284|}
   285|
   286|/* BusSM Interface */
   287|void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode)
   288|{
   289|#if (COMM_DEV_ERROR_DETECT == STD_ON)
   290|    if (Channel >= COMM_MAX_CHANNELS)
   291|    {
   292|        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x82, COMM_E_WRONG_PARAMETERS);
   293|        return;
   294|    }
   295|#endif
   296|
   297|    ComM_CurrentMode[Channel] = Mode;
   298|}
   299|
   300|/* ECU Manager Interface */
   301|Std_ReturnType ComM_EcuM_WakeUpIndication(void)
   302|{
   303|    /* Wake up indication received from ECU Manager */
   304|    /* TODO: Implement wake up handling */
   305|    return E_OK;
   306|}
   307|
   308|Std_ReturnType ComM_EcuM_RunRequestIndication(void)
   309|{
   310|    /* RUN request from ECU Manager */
   311|    return E_OK;
   312|}
   313|