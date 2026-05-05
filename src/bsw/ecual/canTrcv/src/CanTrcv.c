/**
 * @file CanTrcv.c
 * @brief AUTOSAR CAN Transceiver Driver Implementation
 * @version 4.4.0
 * @date 2026-05-05
 * 
 * Supports TJA1043, TJA1042, and generic CAN transceivers
 */

#include "CanTrcv.h"
#include "CanTrcv_Cfg.h"
#include "Det.h"
#include "Dio.h"
#include "Spi.h"

#if (CANTRCV_DEV_ERROR_DETECT == STD_ON)
#define CANTRCV_DET_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(CANTRCV_MODULE_ID, CANTRCV_INSTANCE_ID, (ApiId), (ErrorId))
#else
#define CANTRCV_DET_REPORT_ERROR(ApiId, ErrorId) ((void)0)
#endif

/* Module initialization state */
static boolean CanTrcv_InitStatus = FALSE;

/* Transceiver runtime data */
typedef struct {
    CanTrcv_TrcvModeType CurrentMode;
    CanTrcv_TrcvWakeupModeType WakeupMode;
    CanTrcv_TrcvWakeupReasonType WakeupReason;
    boolean WakeupPending;
    boolean WakeupByBusEnabled;
} CanTrcv_RuntimeType;

static CanTrcv_RuntimeType CanTrcv_Runtime[CANTRCV_MAX_CHANNELS];

/**
 * @brief Get transceiver index from Transceiver ID
 */
static inline uint8 CanTrcv_GetChannelIndex(CanTrcv_TrcvChnlType Transceiver)
{
    return (uint8)Transceiver;
}

/**
 * @brief Update transceiver mode via hardware (DIO/Spi)
 */
static void CanTrcv_HwSetMode(uint8 channelIndex, CanTrcv_TrcvModeType Mode)
{
    const CanTrcv_ConfigType* Config = CanTrcv_ConfigPtr;
    const CanTrcv_ChannelConfigType* ChCfg = &Config->ChannelConfig[channelIndex];
    
    if (ChCfg->TransceiverType == CANTRCV_TJA1043 ||
        ChCfg->TransceiverType == CANTRCV_TJA1042 ||
        ChCfg->TransceiverType == CANTRCV_GENERIC)
    {
        /* Control via DIO pins */
        switch (Mode)
        {
            case CANTRCV_TRCVMODE_NORMAL:
                /* STB pin HIGH (if inverted logic: LOW) */
                if (ChCfg->PinConfig.StbPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.StbPin, 
                        ChCfg->PinConfig.StbPinInverted ? STD_LOW : STD_HIGH);
                }
                /* EN pin HIGH */
                if (ChCfg->PinConfig.EnPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.EnPin, STD_HIGH);
                }
                break;
                
            case CANTRCV_TRCVMODE_STANDBY:
                /* STB pin LOW */
                if (ChCfg->PinConfig.StbPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.StbPin,
                        ChCfg->PinConfig.StbPinInverted ? STD_HIGH : STD_LOW);
                }
                /* EN pin HIGH */
                if (ChCfg->PinConfig.EnPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.EnPin, STD_HIGH);
                }
                break;
                
            case CANTRCV_TRCVMODE_SLEEP:
                /* STB pin LOW */
                if (ChCfg->PinConfig.StbPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.StbPin,
                        ChCfg->PinConfig.StbPinInverted ? STD_HIGH : STD_LOW);
                }
                /* EN pin LOW */
                if (ChCfg->PinConfig.EnPin != DIO_INVALID_CHANNEL)
                {
                    Dio_WriteChannel(ChCfg->PinConfig.EnPin, STD_LOW);
                }
                break;
                
            default:
                /* No change */
                break;
        }
    }
    
    /* Spi-based transceivers handled separately */
    if (ChCfg->UsesSpi == TRUE)
    {
        /* Spi sequence to set mode would be sent here */
    }
}

/**
 * @brief Read transceiver mode from hardware
 */
static CanTrcv_TrcvModeType CanTrcv_HwGetMode(uint8 channelIndex)
{
    const CanTrcv_ConfigType* Config = CanTrcv_ConfigPtr;
    const CanTrcv_ChannelConfigType* ChCfg = &Config->ChannelConfig[channelIndex];
    
    if (ChCfg->TransceiverType == CANTRCV_TJA1043 ||
        ChCfg->TransceiverType == CANTRCV_TJA1042)
    {
        /* Read ERR/NERR pin to determine mode */
        Dio_LevelType ErrPinLevel = Dio_ReadChannel(ChCfg->PinConfig.ErrPin);
        Dio_LevelType StbPinLevel = Dio_ReadChannel(ChCfg->PinConfig.StbPin);
        
        /* Simplified mode detection logic */
        if (ErrPinLevel == STD_HIGH && StbPinLevel == STD_HIGH)
        {
            return CANTRCV_TRCVMODE_NORMAL;
        }
        else if (StbPinLevel == STD_LOW)
        {
            return CANTRCV_TRCVMODE_SLEEP;
        }
        else
        {
            return CANTRCV_TRCVMODE_STANDBY;
        }
    }
    
    return CanTrcv_Runtime[channelIndex].CurrentMode;
}

/**
 * @brief Check for wake-up event
 */
static void CanTrcv_CheckWakeup(uint8 channelIndex)
{
    const CanTrcv_ConfigType* Config = CanTrcv_ConfigPtr;
    const CanTrcv_ChannelConfigType* ChCfg = &Config->ChannelConfig[channelIndex];
    
    if (CanTrcv_Runtime[channelIndex].WakeupMode == CANTRCV_WUMODE_ENABLE)
    {
        /* Check ERR pin for wake-up indication */
        Dio_LevelType ErrPinLevel = Dio_ReadChannel(ChCfg->PinConfig.ErrPin);
        
        if (ErrPinLevel == STD_LOW) /* Active low error/wake indication */
        {
            CanTrcv_Runtime[channelIndex].WakeupPending = TRUE;
            CanTrcv_Runtime[channelIndex].WakeupReason = CANTRCV_WU_BY_BUS;
            
            /* Report to EcuM */
            EcuM_SetWakeupEvent(ChCfg->WakeupSource);
        }
    }
}

/*==================================================================================================
 *                                       API FUNCTIONS
 *=================================================================================================*/

/**
 * @brief Initialize CAN Transceiver driver
 */
void CanTrcv_Init(const CanTrcv_ConfigType* ConfigPtr)
{
    uint8 i;
    
    if (ConfigPtr == NULL_PTR)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_INIT, CANTRCV_E_PARAM_POINTER);
        return;
    }
    
    CanTrcv_ConfigPtr = ConfigPtr;
    
    for (i = 0; i < CANTRCV_MAX_CHANNELS; i++)
    {
        CanTrcv_Runtime[i].CurrentMode = CANTRCV_TRCVMODE_SLEEP;
        CanTrcv_Runtime[i].WakeupMode = CANTRCV_WUMODE_ENABLE;
        CanTrcv_Runtime[i].WakeupReason = CANTRCV_WU_NOT_SUPPORTED;
        CanTrcv_Runtime[i].WakeupPending = FALSE;
        CanTrcv_Runtime[i].WakeupByBusEnabled = TRUE;
        
        /* Initialize hardware to sleep mode */
        CanTrcv_HwSetMode(i, CANTRCV_TRCVMODE_SLEEP);
    }
    
    CanTrcv_InitStatus = TRUE;
}

/**
 * @brief De-initialize CAN Transceiver driver
 */
Std_ReturnType CanTrcv_DeInit(void)
{
    uint8 i;
    
    if (CanTrcv_InitStatus == FALSE)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_DEINIT, CANTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    for (i = 0; i < CANTRCV_MAX_CHANNELS; i++)
    {
        /* Set all transceivers to sleep mode */
        CanTrcv_HwSetMode(i, CANTRCV_TRCVMODE_SLEEP);
        CanTrcv_Runtime[i].CurrentMode = CANTRCV_TRCVMODE_SLEEP;
    }
    
    CanTrcv_InitStatus = FALSE;
    return E_OK;
}

/**
 * @brief Set transceiver operation mode
 */
Std_ReturnType CanTrcv_SetOpMode(CanTrcv_TrcvChnlType Transceiver, 
                                  CanTrcv_TrcvModeType OpMode)
{
    uint8 chIdx = CanTrcv_GetChannelIndex(Transceiver);
    
    if (CanTrcv_InitStatus == FALSE)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETOPMODE, CANTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (chIdx >= CANTRCV_MAX_CHANNELS)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETOPMODE, CANTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    if (OpMode > CANTRCV_TRCVMODE_SLEEP)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETOPMODE, CANTRCV_E_PARAM_TRCV_OPMODE);
        return E_NOT_OK;
    }
    
    /* Set hardware mode */
    CanTrcv_HwSetMode(chIdx, OpMode);
    CanTrcv_Runtime[chIdx].CurrentMode = OpMode;
    
    /* Clear pending wakeup when entering normal mode */
    if (OpMode == CANTRCV_TRCVMODE_NORMAL)
    {
        CanTrcv_Runtime[chIdx].WakeupPending = FALSE;
        CanTrcv_Runtime[chIdx].WakeupReason = CANTRCV_WU_NOT_SUPPORTED;
    }
    
    return E_OK;
}

/**
 * @brief Get transceiver operation mode
 */
Std_ReturnType CanTrcv_GetOpMode(CanTrcv_TrcvChnlType Transceiver,
                                  CanTrcv_TrcvModeType* OpMode)
{
    uint8 chIdx = CanTrcv_GetChannelIndex(Transceiver);
    
    if (CanTrcv_InitStatus == FALSE)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETOPMODE, CANTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (chIdx >= CANTRCV_MAX_CHANNELS)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETOPMODE, CANTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    if (OpMode == NULL_PTR)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETOPMODE, CANTRCV_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    *OpMode = CanTrcv_Runtime[chIdx].CurrentMode;
    return E_OK;
}

/**
 * @brief Get wake-up reason
 */
Std_ReturnType CanTrcv_GetBusWuReason(CanTrcv_TrcvChnlType Transceiver,
                                       CanTrcv_TrcvWakeupReasonType* Reason)
{
    uint8 chIdx = CanTrcv_GetChannelIndex(Transceiver);
    
    if (CanTrcv_InitStatus == FALSE)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETBUSWUREASON, CANTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (chIdx >= CANTRCV_MAX_CHANNELS)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETBUSWUREASON, CANTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    if (Reason == NULL_PTR)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETBUSWUREASON, CANTRCV_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    *Reason = CanTrcv_Runtime[chIdx].WakeupReason;
    return E_OK;
}

/**
 * @brief Set wake-up mode
 */
Std_ReturnType CanTrcv_SetWakeupMode(CanTrcv_TrcvChnlType Transceiver,
                                      CanTrcv_TrcvWakeupModeType TrcvWakeupMode)
{
    uint8 chIdx = CanTrcv_GetChannelIndex(Transceiver);
    
    if (CanTrcv_InitStatus == FALSE)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETWAKEUPMODE, CANTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (chIdx >= CANTRCV_MAX_CHANNELS)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETWAKEUPMODE, CANTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    if (TrcvWakeupMode > CANTRCV_WUMODE_CLEAR)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_SETWAKEUPMODE, CANTRCV_E_PARAM_TRCV_WAKEUP_MODE);
        return E_NOT_OK;
    }
    
    switch (TrcvWakeupMode)
    {
        case CANTRCV_WUMODE_ENABLE:
            CanTrcv_Runtime[chIdx].WakeupMode = CANTRCV_WUMODE_ENABLE;
            CanTrcv_Runtime[chIdx].WakeupByBusEnabled = TRUE;
            break;
            
        case CANTRCV_WUMODE_DISABLE:
            CanTrcv_Runtime[chIdx].WakeupMode = CANTRCV_WUMODE_DISABLE;
            CanTrcv_Runtime[chIdx].WakeupByBusEnabled = FALSE;
            break;
            
        case CANTRCV_WUMODE_CLEAR:
            CanTrcv_Runtime[chIdx].WakeupPending = FALSE;
            CanTrcv_Runtime[chIdx].WakeupReason = CANTRCV_WU_NOT_SUPPORTED;
            break;
            
        default:
            break;
    }
    
    return E_OK;
}

/**
 * @brief Main function - cyclic processing
 */
void CanTrcv_MainFunction(void)
{
    uint8 i;
    
    if (CanTrcv_InitStatus == FALSE)
    {
        return;
    }
    
    for (i = 0; i < CANTRCV_MAX_CHANNELS; i++)
    {
        /* Check for wake-up events */
        CanTrcv_CheckWakeup(i);
        
        /* Update runtime mode from hardware (optional) */
        /* CanTrcv_Runtime[i].CurrentMode = CanTrcv_HwGetMode(i); */
    }
}

/**
 * @brief Get version information
 */
#if (CANTRCV_VERSION_INFO_API == STD_ON)
void CanTrcv_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR)
    {
        CANTRCV_DET_REPORT_ERROR(CANTRCV_SID_GETVERSIONINFO, CANTRCV_E_PARAM_POINTER);
        return;
    }
    
    VersionInfo->vendorID = CANTRCV_VENDOR_ID;
    VersionInfo->moduleID = CANTRCV_MODULE_ID;
    VersionInfo->sw_major_version = CANTRCV_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = CANTRCV_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = CANTRCV_SW_PATCH_VERSION;
}
#endif

/*==================================================================================================
 *                                       CALLBACK FUNCTIONS
 *=================================================================================================*/

/**
 * @brief Check wake-up by transceiver (called by EcuM)
 */
void CanTrcv_CheckWakeupByTransceiver(CanTrcv_TrcvChnlType Transceiver)
{
    uint8 chIdx = CanTrcv_GetChannelIndex(Transceiver);
    
    if (CanTrcv_InitStatus == FALSE)
    {
        return;
    }
    
    if (chIdx < CANTRCV_MAX_CHANNELS)
    {
        CanTrcv_CheckWakeup(chIdx);
    }
}
