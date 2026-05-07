/**
 * @file Icu.c
 * @brief ICU (Input Capture Unit) Driver implementation for i.MX8M Mini (TPM)
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * 
 * AutoSAR Standard: ICU Driver 4.4.0
 * MISRA C:2012 compliant
 * ASIL-D compatible
 */

#include "Icu_Private.h"
#include "Det.h"
#include "SchM.h"

/*==================================================================================================
*                                    LOCAL DEFINITIONS
==================================================================================================*/

/* TPM Module Base Addresses */
#define ICU_TPM1_BASE                       (0x30660000UL)
#define ICU_TPM2_BASE                       (0x30670000UL)
#define ICU_TPM3_BASE                       (0x30680000UL)
#define ICU_TPM4_BASE                       (0x30690000UL)

/* TPM Register Offsets */
#define ICU_TPM_SC                          (0x00U)
#define ICU_TPM_CNT                         (0x04U)
#define ICU_TPM_MOD                         (0x08U)
#define ICU_TPM_C0SC                        (0x0CU)
#define ICU_TPM_C0V                         (0x10U)
#define ICU_TPM_STATUS                      (0x50U)
#define ICU_TPM_CONF                        (0x84U)
#define ICU_TPM_CH_OFFSET                   (0x08U)

/* TPM SC Register Bits */
#define ICU_TPM_SC_PS_MASK                  (0x00000007U)
#define ICU_TPM_SC_CMOD_SHIFT               (3U)
#define ICU_TPM_SC_CMOD_MASK                (0x00000018U)
#define ICU_TPM_SC_CPWMS                    (0x00000020U)
#define ICU_TPM_SC_TOIE                     (0x00000040U)
#define ICU_TPM_SC_TOF                      (0x00000080U)
#define ICU_TPM_SC_DMA                      (0x00000100U)
#define ICU_TPM_SC_TOF_MASK                 (0x00000100U)

/* TPM CnSC Register Bits */
#define ICU_TPM_CnSC_DMA                    (0x01U)
#define ICU_TPM_CnSC_RST                    (0x02U)
#define ICU_TPM_CnSC_ELSA                   (0x04U)
#define ICU_TPM_CnSC_ELSB                   (0x08U)
#define ICU_TPM_CnSC_MSA                    (0x10U)
#define ICU_TPM_CnSC_MSB                    (0x20U)
#define ICU_TPM_CnSC_CHIE                   (0x40U)
#define ICU_TPM_CnSC_CHF                    (0x80U)

/* TPM CONF Register Bits */
#define ICU_TPM_CONF_DOZEEN                 (0x00000020U)
#define ICU_TPM_CONF_DBGMODE_MASK           (0x000000C0U)
#define ICU_TPM_CONF_GTBEEN                 (0x00000200U)

/*==================================================================================================
*                                    GLOBAL VARIABLES
==================================================================================================*/

#define ICU_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Channel runtime states */
Icu_ChannelStateType Icu_ChannelState[ICU_NUM_CHANNELS];

/* Driver state */
Icu_DriverStateType Icu_DriverState = {
    .Initialized = FALSE,
    .CurrentMode = ICU_MODE_NORMAL,
    .ConfigPtr = NULL_PTR
};

#define ICU_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    STATIC FUNCTIONS
==================================================================================================*/

/**
 * @brief Get TPM base address for a channel
 */
static uint32 Icu_GetTpmBaseAddr(Icu_ChannelType Channel)
{
    uint32 baseAddr;
    
    switch (Channel) {
        case ICU_CHANNEL_0:
        case ICU_CHANNEL_1:
            baseAddr = ICU_TPM1_BASE;
            break;
        case ICU_CHANNEL_2:
        case ICU_CHANNEL_3:
            baseAddr = ICU_TPM2_BASE;
            break;
        case ICU_CHANNEL_4:
        case ICU_CHANNEL_5:
            baseAddr = ICU_TPM3_BASE;
            break;
        case ICU_CHANNEL_6:
        case ICU_CHANNEL_7:
            baseAddr = ICU_TPM4_BASE;
            break;
        default:
            baseAddr = 0U;
            break;
    }
    return baseAddr;
}

/**
 * @brief Get TPM channel offset (0 or 1 for each TPM module)
 */
static uint8 Icu_GetTpmChannelOffset(Icu_ChannelType Channel)
{
    return (uint8)(Channel % 2U);
}

/**
 * @brief Enable TPM clock for a channel
 */
static void Icu_EnableTpmClock(Icu_ChannelType Channel)
{
    (void)Channel;
    /* Clock enabling handled by MCU driver */
}

/**
 * @brief Disable TPM clock for a channel
 */
static void Icu_DisableTpmClock(Icu_ChannelType Channel)
{
    (void)Channel;
    /* Clock disabling handled by MCU driver */
}

/**
 * @brief Setup input capture mode for a channel
 */
static void Icu_SetupInputCapture(Icu_ChannelType Channel, Icu_SignalEdgeType Edge)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    uint32 cnscValue;
    
    /* Calculate edge selection */
    switch (Edge) {
        case ICU_RISING_EDGE:
            cnscValue = ICU_TPM_CnSC_ELSB; /* Capture on rising edge */
            break;
        case ICU_FALLING_EDGE:
            cnscValue = ICU_TPM_CnSC_ELSA; /* Capture on falling edge */
            break;
        case ICU_BOTH_EDGES:
            cnscValue = (ICU_TPM_CnSC_ELSA | ICU_TPM_CnSC_ELSB); /* Both edges */
            break;
        default:
            cnscValue = ICU_TPM_CnSC_ELSB;
            break;
    }
    
    /* Set input capture mode (MSA=0, MSB=0) with edge selection */
    cnscValue &= ~(ICU_TPM_CnSC_MSA | ICU_TPM_CnSC_MSB);
    
    /* Write CnSC register */
    ICU_REG_WRITE32(cnscAddr, cnscValue);
}

/**
 * @brief Get current edge configuration from CnSC register
 */
static Icu_SignalEdgeType Icu_GetEdgeConfig(uint32 CnscReg)
{
    Icu_SignalEdgeType edge;
    uint8 elsa = ((CnscReg & ICU_TPM_CnSC_ELSA) != 0U) ? 1U : 0U;
    uint8 elsb = ((CnscReg & ICU_TPM_CnSC_ELSB) != 0U) ? 1U : 0U;
    
    if ((elsa != 0U) && (elsb != 0U)) {
        edge = ICU_BOTH_EDGES;
    } else if (elsa != 0U) {
        edge = ICU_FALLING_EDGE;
    } else if (elsb != 0U) {
        edge = ICU_RISING_EDGE;
    } else {
        edge = ICU_RISING_EDGE; /* Default */
    }
    return edge;
}

/**
 * @brief Clear channel flag
 */
static void Icu_ClearChannelFlag(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    /* Write 0 to CHF bit to clear it (W1C) */
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHF;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
}

/**
 * @brief Check if channel flag is set
 */
static boolean Icu_IsChannelFlagSet(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    return ((ICU_REG_READ32(cnscAddr) & ICU_TPM_CnSC_CHF) != 0U);
}

/**
 * @brief Reset channel state to default
 */
static void Icu_ResetChannelState(Icu_ChannelType Channel)
{
    Icu_ChannelState[Channel].State = ICU_STATE_INITIALIZED;
    Icu_ChannelState[Channel].InputState = ICU_IDLE;
    Icu_ChannelState[Channel].CapturedValue = 0U;
    Icu_ChannelState[Channel].PreviousValue = 0U;
    Icu_ChannelState[Channel].PeriodTime = 0U;
    Icu_ChannelState[Channel].ActiveTime = 0U;
    Icu_ChannelState[Channel].EdgeCount = 0U;
    Icu_ChannelState[Channel].BufferIndex = 0U;
    Icu_ChannelState[Channel].NotifyCounter = 0U;
    Icu_ChannelState[Channel].TimestampBuffer = NULL_PTR;
    Icu_ChannelState[Channel].NotificationEnabled = FALSE;
    Icu_ChannelState[Channel].WakeupEnabled = FALSE;
    Icu_ChannelState[Channel].IsRunning = FALSE;
}

/**
 * @brief Configure a channel based on its configuration
 */
static void Icu_ConfigureChannel(Icu_ChannelType Channel, const Icu_ChannelConfigType* Config)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    if (baseAddr == 0U) {
        return;
    }
    
    /* Enable TPM clock */
    Icu_EnableTpmClock(Channel);
    
    /* Configure SC register */
    uint32 scValue = ICU_REG_READ32(baseAddr + ICU_TPM_SC);
    scValue &= ~ICU_TPM_SC_PS_MASK;
    scValue |= (Config->ClockPrescaler & ICU_TPM_SC_PS_MASK);
    scValue |= ((1U << ICU_TPM_SC_CMOD_SHIFT) & ICU_TPM_SC_CMOD_MASK); /* Enable clock */
    scValue &= ~ICU_TPM_SC_CPWMS; /* Up counting mode */
    ICU_REG_WRITE32(baseAddr + ICU_TPM_SC, scValue);
    
    /* Configure CONF register */
    uint32 confValue = ICU_TPM_CONF_DBGMODE_MASK; /* Debug mode active */
    ICU_REG_WRITE32(baseAddr + ICU_TPM_CONF, confValue);
    
    /* Reset counter */
    ICU_REG_WRITE32(baseAddr + ICU_TPM_CNT, 0U);
    
    /* Set max modulo value */
    ICU_REG_WRITE32(baseAddr + ICU_TPM_MOD, 0xFFFFU);
    
    /* Clear channel status and control register */
    ICU_REG_WRITE32(cnscAddr, 0U);
    
    /* Setup input capture based on mode */
    switch (Config->Mode) {
        case ICU_MODE_SIGNAL_EDGE_DETECT:
        case ICU_MODE_TIMESTAMP:
        case ICU_MODE_EDGE_COUNTER:
            Icu_SetupInputCapture(Channel, Config->Edge);
            break;
            
        case ICU_MODE_SIGNAL_MEASUREMENT:
            /* Start with first edge for duty cycle/period measurement */
            if (Config->Property == ICU_DUTY_CYCLE) {
                Icu_SetupInputCapture(Channel, ICU_RISING_EDGE);
            } else {
                Icu_SetupInputCapture(Channel, Config->Edge);
            }
            break;
            
        default:
            break;
    }
    
    /* Reset channel state */
    Icu_ResetChannelState(Channel);
    Icu_ChannelState[Channel].CurrentEdge = Config->Edge;
}

/*==================================================================================================
*                                    API IMPLEMENTATION
==================================================================================================*/

#define ICU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the ICU driver
 */
void Icu_Init(const Icu_ConfigType* ConfigPtr)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_INIT, ICU_E_PARAM_POINTER);
        return;
    }
    if (Icu_DriverState.Initialized == TRUE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_INIT, ICU_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    /* Store configuration pointer */
    Icu_DriverState.ConfigPtr = ConfigPtr;
    
    /* Initialize all channels */
    for (uint8 i = 0U; i < ICU_NUM_CHANNELS; i++) {
        const Icu_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        Icu_ConfigureChannel(i, chConfig);
    }
    
    /* Set driver state */
    Icu_DriverState.CurrentMode = ConfigPtr->DefaultMode;
    Icu_DriverState.Initialized = TRUE;
}

#if (ICU_DE_INIT_API == STD_ON)
/**
 * @brief Deinitializes the ICU driver
 */
void Icu_DeInit(void)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DEINIT, ICU_E_UNINIT);
        return;
    }
    #endif
    
    /* Check if any channel is running */
    for (uint8 i = 0U; i < ICU_NUM_CHANNELS; i++) {
        if (Icu_ChannelState[i].IsRunning) {
            return; /* Cannot deinitialize while running */
        }
    }
    
    /* Deinitialize all channels */
    for (uint8 i = 0U; i < ICU_NUM_CHANNELS; i++) {
        uint32 baseAddr = Icu_GetTpmBaseAddr(i);
        uint8 chOffset = Icu_GetTpmChannelOffset(i);
        
        if (baseAddr != 0U) {
            /* Disable channel interrupt */
            uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
            ICU_REG_WRITE32(cnscAddr, 0U);
            
            /* Disable TPM clock */
            uint32 scValue = ICU_REG_READ32(baseAddr + ICU_TPM_SC);
            scValue &= ~ICU_TPM_SC_CMOD_MASK;
            ICU_REG_WRITE32(baseAddr + ICU_TPM_SC, scValue);
            
            Icu_DisableTpmClock(i);
        }
        
        Icu_ResetChannelState(i);
    }
    
    Icu_DriverState.Initialized = FALSE;
    Icu_DriverState.ConfigPtr = NULL_PTR;
}
#endif

#if (ICU_SET_MODE_API == STD_ON)
/**
 * @brief Sets the operation mode
 */
void Icu_SetMode(Icu_ModeType Mode)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETMODE, ICU_E_UNINIT);
        return;
    }
    if (Mode > ICU_MODE_SLEEP) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETMODE, ICU_E_PARAM_MODE);
        return;
    }
    #endif
    
    if (Mode == ICU_MODE_SLEEP) {
        /* Stop all running channels in sleep mode */
        for (uint8 i = 0U; i < ICU_NUM_CHANNELS; i++) {
            if (Icu_ChannelState[i].IsRunning) {
                #if (ICU_SIGNAL_MEASUREMENT_API == STD_ON)
                if (Icu_DriverState.ConfigPtr->Channels[i].Mode == ICU_MODE_SIGNAL_MEASUREMENT) {
                    Icu_StopSignalMeasurement(i);
                }
                #endif
                #if (ICU_TIMESTAMP_API == STD_ON)
                if (Icu_DriverState.ConfigPtr->Channels[i].Mode == ICU_MODE_TIMESTAMP) {
                    Icu_StopTimestamp(i);
                }
                #endif
                #if (ICU_EDGE_COUNT_API == STD_ON)
                if (Icu_DriverState.ConfigPtr->Channels[i].Mode == ICU_MODE_EDGE_COUNTER) {
                    Icu_DisableEdgeCount(i);
                }
                #endif
            }
        }
    }
    
    Icu_DriverState.CurrentMode = Mode;
}
#endif

#if (ICU_WAKEUP_FUNCTIONALITY_API == STD_ON)
/**
 * @brief Disables wakeup for a channel
 */
void Icu_DisableWakeup(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_ChannelState[Channel].WakeupEnabled = FALSE;
}

/**
 * @brief Enables wakeup for a channel
 */
void Icu_EnableWakeup(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Icu_DriverState.ConfigPtr->Channels[Channel].WakeupSupport == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_WAKEUP_CANNOT_BE_ENABLED);
        return;
    }
    #endif
    
    Icu_ChannelState[Channel].WakeupEnabled = TRUE;
}
#endif

/**
 * @brief Sets the activation condition (edge type)
 */
void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_SignalEdgeType Activation)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Activation > ICU_BOTH_EDGES) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_ACTIVATION);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_00();
    
    /* Update edge configuration */
    Icu_SetupInputCapture(Channel, Activation);
    Icu_ChannelState[Channel].CurrentEdge = Activation;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_00();
}

/**
 * @brief Disables notification for a channel
 */
void Icu_DisableNotification(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_01();
    
    /* Disable channel interrupt */
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].NotificationEnabled = FALSE;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_01();
}

/**
 * @brief Enables notification for a channel
 */
void Icu_EnableNotification(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_02();
    
    /* Enable channel interrupt */
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue |= ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].NotificationEnabled = TRUE;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_02();
}

/**
 * @brief Gets the input state of a channel
 */
Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTSTATE, ICU_E_UNINIT);
        return ICU_IDLE;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTSTATE, ICU_E_PARAM_CHANNEL);
        return ICU_IDLE;
    }
    #endif
    
    Icu_InputStateType state;
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_03();
    
    if (Icu_IsChannelFlagSet(Channel)) {
        state = ICU_ACTIVE;
        /* Clear the flag after reading */
        Icu_ClearChannelFlag(Channel);
    } else {
        state = ICU_IDLE;
    }
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_03();
    
    return state;
}

#if (ICU_TIMESTAMP_API == STD_ON)
/**
 * @brief Starts timestamp capture
 */
void Icu_StartTimestamp(Icu_ChannelType Channel, Icu_ValueType* BufferPtr, 
                        Icu_IndexType BufferSize, Icu_IndexType NotifyInterval)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (BufferPtr == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_POINTER);
        return;
    }
    if (BufferSize == 0U) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_BUFFER_SIZE);
        return;
    }
    if (Icu_DriverState.ConfigPtr->Channels[Channel].Mode != ICU_MODE_TIMESTAMP) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_04();
    
    /* Initialize timestamp buffer */
    Icu_ChannelState[Channel].TimestampBuffer = BufferPtr;
    Icu_ChannelState[Channel].BufferSize = BufferSize;
    Icu_ChannelState[Channel].BufferIndex = 0U;
    Icu_ChannelState[Channel].NotifyInterval = NotifyInterval;
    Icu_ChannelState[Channel].NotifyCounter = 0U;
    
    /* Clear buffer */
    for (Icu_IndexType i = 0U; i < BufferSize; i++) {
        BufferPtr[i] = 0U;
    }
    
    /* Enable input capture interrupt */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue |= ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = TRUE;
    Icu_ChannelState[Channel].State = ICU_STATE_RUNNING;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_04();
}

/**
 * @brief Stops timestamp capture
 */
void Icu_StopTimestamp(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_05();
    
    /* Disable input capture interrupt */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = FALSE;
    Icu_ChannelState[Channel].State = ICU_STATE_STOPPED;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_05();
}

/**
 * @brief Gets the current timestamp buffer index
 */
Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    return Icu_ChannelState[Channel].BufferIndex;
}
#endif

#if (ICU_EDGE_COUNT_API == STD_ON)
/**
 * @brief Resets the edge counter
 */
void Icu_ResetEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_06();
    Icu_ChannelState[Channel].EdgeCount = 0U;
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_06();
}

/**
 * @brief Enables edge counting
 */
void Icu_EnableEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Icu_DriverState.ConfigPtr->Channels[Channel].Mode != ICU_MODE_EDGE_COUNTER) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_07();
    
    /* Enable input capture interrupt for edge counting */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue |= ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = TRUE;
    Icu_ChannelState[Channel].State = ICU_STATE_RUNNING;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_07();
}

/**
 * @brief Disables edge counting
 */
void Icu_DisableEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_08();
    
    /* Disable input capture interrupt */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = FALSE;
    Icu_ChannelState[Channel].State = ICU_STATE_STOPPED;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_08();
}

/**
 * @brief Gets the number of counted edges
 */
Icu_EdgeNumberType Icu_GetEdgeNumbers(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    return Icu_ChannelState[Channel].EdgeCount;
}
#endif

#if (ICU_SIGNAL_MEASUREMENT_API == STD_ON)
/**
 * @brief Starts signal measurement
 */
void Icu_StartSignalMeasurement(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Icu_DriverState.ConfigPtr->Channels[Channel].Mode != ICU_MODE_SIGNAL_MEASUREMENT) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Icu_ChannelState[Channel].IsRunning) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_BUSY_OPERATION);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_09();
    
    /* Reset measurement state */
    Icu_ChannelState[Channel].CapturedValue = 0U;
    Icu_ChannelState[Channel].PreviousValue = 0U;
    Icu_ChannelState[Channel].PeriodTime = 0U;
    Icu_ChannelState[Channel].ActiveTime = 0U;
    
    /* Configure initial edge detection based on property */
    const Icu_ChannelConfigType* config = &Icu_DriverState.ConfigPtr->Channels[Channel];
    if (config->Property == ICU_DUTY_CYCLE) {
        /* For duty cycle, start with rising edge */
        Icu_SetupInputCapture(Channel, ICU_RISING_EDGE);
    } else {
        Icu_SetupInputCapture(Channel, config->Edge);
    }
    
    /* Enable input capture interrupt */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue |= ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = TRUE;
    Icu_ChannelState[Channel].State = ICU_STATE_RUNNING;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_09();
}

/**
 * @brief Stops signal measurement
 */
void Icu_StopSignalMeasurement(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_10();
    
    /* Disable input capture interrupt */
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHIE;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
    
    Icu_ChannelState[Channel].IsRunning = FALSE;
    Icu_ChannelState[Channel].State = ICU_STATE_STOPPED;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_10();
}

/**
 * @brief Gets the elapsed time
 */
Icu_ValueType Icu_GetTimeElapsed(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    Icu_ValueType elapsedTime = 0U;
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_11();
    
    const Icu_ChannelConfigType* config = &Icu_DriverState.ConfigPtr->Channels[Channel];
    
    switch (config->Property) {
        case ICU_LOW_TIME:
        case ICU_HIGH_TIME:
            elapsedTime = Icu_ChannelState[Channel].ActiveTime;
            break;
        case ICU_PERIOD_TIME:
            elapsedTime = Icu_ChannelState[Channel].PeriodTime;
            break;
        case ICU_DUTY_CYCLE:
            /* For duty cycle, return period time as elapsed time */
            elapsedTime = Icu_ChannelState[Channel].PeriodTime;
            break;
        default:
            elapsedTime = 0U;
            break;
    }
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_11();
    
    return elapsedTime;
}

/**
 * @brief Gets the duty cycle values
 */
void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (DutyCycleValues == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_POINTER);
        return;
    }
    if (Icu_DriverState.ConfigPtr->Channels[Channel].Property != ICU_DUTY_CYCLE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    SchM_Enter_Icu_ICU_EXCLUSIVE_AREA_12();
    
    DutyCycleValues->ActiveTime = Icu_ChannelState[Channel].ActiveTime;
    DutyCycleValues->PeriodTime = Icu_ChannelState[Channel].PeriodTime;
    
    SchM_Exit_Icu_ICU_EXCLUSIVE_AREA_12();
}
#endif

/**
 * @brief Gets version information
 */
void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETVERSIONINFO, ICU_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = ICU_VENDOR_ID;
    versioninfo->moduleID = ICU_MODULE_ID;
    versioninfo->sw_major_version = ICU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ICU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ICU_SW_PATCH_VERSION;
}

#if (ICU_WAKEUP_FUNCTIONALITY_API == STD_ON)
/**
 * @brief Checks for wakeup events
 */
Std_ReturnType Icu_CheckWakeup(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverState.Initialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_CHECKWAKEUP, ICU_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_CHECKWAKEUP, ICU_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    Std_ReturnType result = E_NOT_OK;
    
    if (Icu_ChannelState[Channel].WakeupEnabled) {
        if (Icu_IsChannelFlagSet(Channel)) {
            result = E_OK;
            Icu_ClearChannelFlag(Channel);
        }
    }
    
    return result;
}
#endif

#define ICU_STOP_SEC_CODE
#include "MemMap.h"
