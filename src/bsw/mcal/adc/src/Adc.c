/**
 * @file Adc.c
 * @brief ADC Driver implementation for i.MX8M Mini (ADC)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Adc.h"
#include "Adc_Cfg.h"
#include "Det.h"

#define ADC1_BASE_ADDR                  (0x30610000UL)

#define ADC_HC0                         (0x00)
#define ADC_HS                          (0x04)
#define ADC_R0                          (0x08)
#define ADC_CFG                         (0x0C)
#define ADC_GC                          (0x10)
#define ADC_GS                          (0x14)
#define ADC_CV                          (0x18)
#define ADC_OFS                         (0x1C)
#define ADC_CAL                         (0x20)

#define ADC_HC_AIEN                     (0x00000080U)
#define ADC_HC_ADCH_MASK                (0x0000001FU)

#define ADC_HS_COCO0                    (0x00000001U)

#define ADC_CFG_ADICLK_MASK             (0x00000003U)
#define ADC_CFG_MODE_MASK               (0x0000000CU)
#define ADC_CFG_ADLSMP                  (0x00000010U)
#define ADC_CFG_ADIV_MASK               (0x00000060U)
#define ADC_CFG_ADLPC                   (0x00000080U)

#define ADC_GC_ADACKEN                  (0x00000001U)
#define ADC_GC_ADCONV                   (0x00000004U)
#define ADC_GC_ADTRG                    (0x00000008U)
#define ADC_GC_AVGE                     (0x00000020U)
#define ADC_GC_ADCO                     (0x00000080U)

#define ADC_GS_ADACT                    (0x00000001U)
#define ADC_GS_AWOD                     (0x00000002U)
#define ADC_GS_CALF                     (0x00000004U)

#define ADC_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Adc_DriverInitialized = FALSE;
static Adc_StatusType Adc_GroupStatus[ADC_NUM_GROUPS];
static Adc_ValueGroupType Adc_GroupResults[ADC_NUM_GROUPS][ADC_NUM_CHANNELS];
static const Adc_ConfigType* Adc_ConfigPtr = NULL_PTR;

#define ADC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Adc_GetBaseAddr(Adc_HWUnitType hwUnit)
{
    uint32 baseAddr;
    switch (hwUnit) {
        case ADC_HWUNIT_0: baseAddr = ADC1_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

static void Adc_EnableClock(Adc_HWUnitType hwUnit)
{
    (void)hwUnit;
}

static void Adc_DisableClock(Adc_HWUnitType hwUnit)
{
    (void)hwUnit;
}

static uint32 Adc_GetResolutionBits(Adc_ResolutionType resolution)
{
    uint32 bits;
    switch (resolution) {
        case ADC_RESOLUTION_6BIT: bits = 6U; break;
        case ADC_RESOLUTION_8BIT: bits = 8U; break;
        case ADC_RESOLUTION_10BIT: bits = 10U; break;
        case ADC_RESOLUTION_12BIT: bits = 12U; break;
        default: bits = 12U; break;
    }
    return bits;
}

#define ADC_START_SEC_CODE
#include "MemMap.h"

void Adc_Init(const Adc_ConfigType* ConfigPtr)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_INIT, ADC_E_PARAM_CONFIG);
        return;
    }
    if (Adc_DriverInitialized == TRUE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_INIT, ADC_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    Adc_ConfigPtr = ConfigPtr;
    
    for (uint8 i = 0U; i < ADC_NUM_HW_UNITS; i++) {
        uint32 baseAddr = Adc_GetBaseAddr(ConfigPtr->HwUnits[i].HwUnitId);
        if (baseAddr == 0U) continue;
        
        Adc_EnableClock(ConfigPtr->HwUnits[i].HwUnitId);
        
        /* Configure ADC */
        uint32 cfgValue = 0U;
        cfgValue |= (0x01U << 0); /* IPG clock */
        cfgValue |= (0x02U << 2); /* 12-bit mode */
        cfgValue |= (0x01U << 5); /* Clock divide by 2 */
        REG_WRITE32(baseAddr + ADC_CFG, cfgValue);
        
        /* Enable ADC */
        uint32 gcValue = ADC_GC_ADACKEN;
        REG_WRITE32(baseAddr + ADC_GC, gcValue);
        
        /* Calibration */
        gcValue |= ADC_GC_ADCONV;
        REG_WRITE32(baseAddr + ADC_GC, gcValue);
        while ((REG_READ32(baseAddr + ADC_GS) & ADC_GS_ADACT) != 0U);
    }
    
    for (uint8 i = 0U; i < ADC_NUM_GROUPS; i++) {
        Adc_GroupStatus[i] = ADC_IDLE;
    }
    
    Adc_DriverInitialized = TRUE;
}

#if (ADC_DE_INIT_API == STD_ON)
void Adc_DeInit(void)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_DEINIT, ADC_E_UNINIT);
        return;
    }
    #endif
    
    for (uint8 i = 0U; i < ADC_NUM_GROUPS; i++) {
        if (Adc_GroupStatus[i] == ADC_BUSY) {
            return;
        }
    }
    
    for (uint8 i = 0U; i < ADC_NUM_HW_UNITS; i++) {
        uint32 baseAddr = Adc_GetBaseAddr(Adc_ConfigPtr->HwUnits[i].HwUnitId);
        if (baseAddr == 0U) continue;
        
        /* Disable ADC */
        REG_WRITE32(baseAddr + ADC_GC, 0U);
        
        Adc_DisableClock(Adc_ConfigPtr->HwUnits[i].HwUnitId);
    }
    
    Adc_DriverInitialized = FALSE;
}
#endif

void Adc_StartGroupConversion(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_STARTGROUPCONVERSION, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_STARTGROUPCONVERSION, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    
    if (Adc_GroupStatus[Group] == ADC_BUSY) {
        return;
    }
    
    const Adc_GroupConfigType* groupConfig = &Adc_ConfigPtr->Groups[Group];
    uint32 baseAddr = Adc_GetBaseAddr(groupConfig->HwUnit);
    
    Adc_GroupStatus[Group] = ADC_BUSY;
    
    /* Configure for software trigger */
    uint32 gcValue = REG_READ32(baseAddr + ADC_GC);
    gcValue &= ~ADC_GC_ADTRG; /* Software trigger */
    REG_WRITE32(baseAddr + ADC_GC, gcValue);
    
    /* Start conversion for each channel */
    for (uint8 i = 0U; i < groupConfig->NumChannels; i++) {
        Adc_ChannelType channel = groupConfig->Channels[i];
        
        /* Select channel */
        uint32 hcValue = channel & ADC_HC_ADCH_MASK;
        REG_WRITE32(baseAddr + ADC_HC0, hcValue);
        
        /* Start conversion */
        gcValue = REG_READ32(baseAddr + ADC_GC);
        gcValue |= ADC_GC_ADCONV;
        REG_WRITE32(baseAddr + ADC_GC, gcValue);
        
        /* Wait for conversion complete */
        while ((REG_READ32(baseAddr + ADC_HS) & ADC_HS_COCO0) == 0U);
        
        /* Read result */
        Adc_GroupResults[Group][i] = (Adc_ValueGroupType)(REG_READ32(baseAddr + ADC_R0) & 0xFFFU);
    }
    
    Adc_GroupStatus[Group] = ADC_STREAM_COMPLETED;
    
    /* Call notification if enabled */
    if (groupConfig->GroupNotification) {
        if (groupConfig->NotificationFn != NULL_PTR) {
            groupConfig->NotificationFn();
        }
    }
}

void Adc_StopGroupConversion(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_STOPGROUPCONVERSION, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_STOPGROUPCONVERSION, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    
    if (Adc_GroupStatus[Group] != ADC_BUSY) {
        return;
    }
    
    const Adc_GroupConfigType* groupConfig = &Adc_ConfigPtr->Groups[Group];
    uint32 baseAddr = Adc_GetBaseAddr(groupConfig->HwUnit);
    
    /* Stop conversion */
    uint32 gcValue = REG_READ32(baseAddr + ADC_GC);
    gcValue &= ~ADC_GC_ADCONV;
    gcValue &= ~ADC_GC_ADCO;
    REG_WRITE32(baseAddr + ADC_GC, gcValue);
    
    Adc_GroupStatus[Group] = ADC_IDLE;
}

Std_ReturnType Adc_ReadGroup(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_READGROUP, ADC_E_UNINIT);
        return E_NOT_OK;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_READGROUP, ADC_E_PARAM_GROUP);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_READGROUP, ADC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    const Adc_GroupConfigType* groupConfig = &Adc_ConfigPtr->Groups[Group];
    
    for (uint8 i = 0U; i < groupConfig->NumChannels; i++) {
        DataBufferPtr[i] = Adc_GroupResults[Group][i];
    }
    
    return E_OK;
}

#if (ADC_HW_TRIGGER_API == STD_ON)
void Adc_EnableHardwareTrigger(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_ENABLEHARDWARETRIGGER, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_ENABLEHARDWARETRIGGER, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    
    const Adc_GroupConfigType* groupConfig = &Adc_ConfigPtr->Groups[Group];
    if (groupConfig->TriggerSource != ADC_TRIGG_SRC_HW) {
        return;
    }
    
    uint32 baseAddr = Adc_GetBaseAddr(groupConfig->HwUnit);
    
    /* Enable hardware trigger */
    uint32 gcValue = REG_READ32(baseAddr + ADC_GC);
    gcValue |= ADC_GC_ADTRG;
    REG_WRITE32(baseAddr + ADC_GC, gcValue);
}

void Adc_DisableHardwareTrigger(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_DISABLEHARDWARETRIGGER, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_DISABLEHARDWARETRIGGER, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    
    const Adc_GroupConfigType* groupConfig = &Adc_ConfigPtr->Groups[Group];
    uint32 baseAddr = Adc_GetBaseAddr(groupConfig->HwUnit);
    
    /* Disable hardware trigger */
    uint32 gcValue = REG_READ32(baseAddr + ADC_GC);
    gcValue &= ~ADC_GC_ADTRG;
    REG_WRITE32(baseAddr + ADC_GC, gcValue);
}
#endif

#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
void Adc_EnableGroupNotification(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_ENABLEGROUPNOTIFICATION, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_ENABLEGROUPNOTIFICATION, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    (void)Group;
}

void Adc_DisableGroupNotification(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_DISABLEGROUPNOTIFICATION, ADC_E_UNINIT);
        return;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_DISABLEGROUPNOTIFICATION, ADC_E_PARAM_GROUP);
        return;
    }
    #endif
    (void)Group;
}
#endif

Adc_StatusType Adc_GetGroupStatus(Adc_GroupType Group)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETGROUPSTATUS, ADC_E_UNINIT);
        return ADC_IDLE;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETGROUPSTATUS, ADC_E_PARAM_GROUP);
        return ADC_IDLE;
    }
    #endif
    return Adc_GroupStatus[Group];
}

void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETVERSIONINFO, ADC_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = ADC_VENDOR_ID;
    versioninfo->moduleID = ADC_MODULE_ID;
    versioninfo->sw_major_version = ADC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ADC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ADC_SW_PATCH_VERSION;
}

#if (ADC_GET_STREAM_LAST_POINTER_API == STD_ON)
Adc_StreamNumSampleType Adc_GetStreamLastPointer(Adc_GroupType Group, Adc_ValueGroupType** PtrToSamplePtr)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETSTREAMLASTPOINTER, ADC_E_UNINIT);
        return 0U;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETSTREAMLASTPOINTER, ADC_E_PARAM_GROUP);
        return 0U;
    }
    if (PtrToSamplePtr == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETSTREAMLASTPOINTER, ADC_E_PARAM_POINTER);
        return 0U;
    }
    #endif
    
    *PtrToSamplePtr = Adc_GroupResults[Group];
    return Adc_ConfigPtr->Groups[Group].NumChannels;
}
#endif

#if (ADC_ENABLE_QUEUING == STD_ON)
Std_ReturnType Adc_SetupResultBuffer(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_SETUPRESULTBUFFER, ADC_E_UNINIT);
        return E_NOT_OK;
    }
    if (Group >= ADC_NUM_GROUPS) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_SETUPRESULTBUFFER, ADC_E_PARAM_GROUP);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_SETUPRESULTBUFFER, ADC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    (void)Group;
    (void)DataBufferPtr;
    return E_OK;
}
#endif

#if (ADC_POWER_STATE_SUPPORTED == STD_ON)
void Adc_SetPowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_SETPOWERSTATE, ADC_E_UNINIT);
        return;
    }
    if (Result == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_SETPOWERSTATE, ADC_E_PARAM_POINTER);
        return;
    }
    #endif
    (void)PowerState;
    *Result = ADC_SERVICE_ACCEPTED;
}

void Adc_GetTargetPowerState(Adc_PowerStateType* TargetPowerState, Adc_PowerStateRequestResultType* Result)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETTARGETPOWERSTATE, ADC_E_UNINIT);
        return;
    }
    if (TargetPowerState == NULL_PTR || Result == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETTARGETPOWERSTATE, ADC_E_PARAM_POINTER);
        return;
    }
    #endif
    *TargetPowerState = ADC_FULL_POWER;
    *Result = ADC_SERVICE_ACCEPTED;
}

void Adc_GetCurrentPowerState(Adc_PowerStateType* CurrentPowerState, Adc_PowerStateRequestResultType* Result)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETCURRENTPOWERSTATE, ADC_E_UNINIT);
        return;
    }
    if (CurrentPowerState == NULL_PTR || Result == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_GETCURRENTPOWERSTATE, ADC_E_PARAM_POINTER);
        return;
    }
    #endif
    *CurrentPowerState = ADC_FULL_POWER;
    *Result = ADC_SERVICE_ACCEPTED;
}

void Adc_PreparePowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result)
{
    #if (ADC_DEV_ERROR_DETECT == STD_ON)
    if (Adc_DriverInitialized == FALSE) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_PREPAREPOWERSTATE, ADC_E_UNINIT);
        return;
    }
    if (Result == NULL_PTR) {
        Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_PREPAREPOWERSTATE, ADC_E_PARAM_POINTER);
        return;
    }
    #endif
    (void)PowerState;
    *Result = ADC_SERVICE_ACCEPTED;
}
#endif

#define ADC_STOP_SEC_CODE
#include "MemMap.h"
