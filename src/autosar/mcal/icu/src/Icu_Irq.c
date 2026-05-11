/**
 * @file Icu_Irq.c
 * @brief ICU (Input Capture Unit) Driver interrupt handlers for i.MX8M Mini (TPM)
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

/*==================================================================================================
*                                    LOCAL DEFINITIONS
==================================================================================================*/

/* TPM Register Offsets */
#define ICU_TPM_SC                          (0x00U)
#define ICU_TPM_CNT                         (0x04U)
#define ICU_TPM_C0SC                        (0x0CU)
#define ICU_TPM_C0V                         (0x10U)
#define ICU_TPM_STATUS                      (0x50U)
#define ICU_TPM_CH_OFFSET                   (0x08U)

/* TPM CnSC Register Bits */
#define ICU_TPM_CnSC_CHF                    (0x80U)
#define ICU_TPM_CnSC_CHIE                   (0x40U)

/*==================================================================================================
*                                    STATIC FUNCTIONS
==================================================================================================*/

/**
 * @brief Process edge detection interrupt
 */
static void Icu_ProcessEdgeDetection(Icu_ChannelType Channel)
{
    /* Set input state to active */
    Icu_ChannelState[Channel].InputState = ICU_ACTIVE;
    
    /* Call notification if enabled */
    if ((Icu_ChannelState[Channel].NotificationEnabled) && 
        (Icu_DriverState.ConfigPtr->Channels[Channel].Notification != NULL_PTR)) {
        Icu_DriverState.ConfigPtr->Channels[Channel].Notification();
    }
}

/**
 * @brief Process signal measurement interrupt
 */
static void Icu_ProcessSignalMeasurement(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnvAddr = baseAddr + ICU_TPM_C0V + (chOffset * ICU_TPM_CH_OFFSET);
    
    Icu_ValueType currentValue = (Icu_ValueType)ICU_REG_READ32(cnvAddr);
    Icu_ChannelState[Channel].CapturedValue = currentValue;
    
    const Icu_ChannelConfigType* config = &Icu_DriverState.ConfigPtr->Channels[Channel];
    
    if (config->Property == ICU_DUTY_CYCLE) {
        /* Two-step measurement for duty cycle */
        static uint8 firstEdge[ICU_NUM_CHANNELS] = {0};
        
        if (firstEdge[Channel] == 0U) {
            /* First edge (rising) - start of period */
            Icu_ChannelState[Channel].PreviousValue = currentValue;
            firstEdge[Channel] = 1U;
            
            /* Switch to falling edge detection */
            Icu_SetupInputCapture(Channel, ICU_FALLING_EDGE);
        } else {
            /* Second edge (falling) - end of active time */
            if (currentValue > Icu_ChannelState[Channel].PreviousValue) {
                Icu_ChannelState[Channel].ActiveTime = 
                    currentValue - Icu_ChannelState[Channel].PreviousValue;
            } else {
                /* Counter overflow */
                Icu_ChannelState[Channel].ActiveTime = 
                    (0xFFFFU - Icu_ChannelState[Channel].PreviousValue) + currentValue;
            }
            
            firstEdge[Channel] = 0U;
            
            /* Switch back to rising edge for next period */
            Icu_SetupInputCapture(Channel, ICU_RISING_EDGE);
            
            /* Calculate period time on next rising edge */
            if (Icu_ChannelState[Channel].PreviousValue != 0U) {
                /* Store for next calculation */
            }
        }
    } else if (config->Property == ICU_PERIOD_TIME) {
        /* Period measurement */
        if (Icu_ChannelState[Channel].PreviousValue != 0U) {
            if (currentValue > Icu_ChannelState[Channel].PreviousValue) {
                Icu_ChannelState[Channel].PeriodTime = 
                    currentValue - Icu_ChannelState[Channel].PreviousValue;
            } else {
                Icu_ChannelState[Channel].PeriodTime = 
                    (0xFFFFU - Icu_ChannelState[Channel].PreviousValue) + currentValue;
            }
        }
        Icu_ChannelState[Channel].PreviousValue = currentValue;
    } else {
        /* Low time or high time measurement */
        if (Icu_ChannelState[Channel].PreviousValue != 0U) {
            if (currentValue > Icu_ChannelState[Channel].PreviousValue) {
                Icu_ChannelState[Channel].ActiveTime = 
                    currentValue - Icu_ChannelState[Channel].PreviousValue;
            } else {
                Icu_ChannelState[Channel].ActiveTime = 
                    (0xFFFFU - Icu_ChannelState[Channel].PreviousValue) + currentValue;
            }
        }
        Icu_ChannelState[Channel].PreviousValue = currentValue;
    }
    
    /* Call notification if enabled */
    if ((Icu_ChannelState[Channel].NotificationEnabled) && 
        (config->Notification != NULL_PTR)) {
        config->Notification();
    }
}

/**
 * @brief Process timestamp interrupt
 */
static void Icu_ProcessTimestamp(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnvAddr = baseAddr + ICU_TPM_C0V + (chOffset * ICU_TPM_CH_OFFSET);
    
    Icu_ValueType currentValue = (Icu_ValueType)ICU_REG_READ32(cnvAddr);
    
    /* Store timestamp in buffer */
    if (Icu_ChannelState[Channel].TimestampBuffer != NULL_PTR) {
        Icu_IndexType index = Icu_ChannelState[Channel].BufferIndex;
        
        if (index < Icu_ChannelState[Channel].BufferSize) {
            Icu_ChannelState[Channel].TimestampBuffer[index] = currentValue;
            Icu_ChannelState[Channel].BufferIndex++;
            
            /* Check for notification interval */
            Icu_ChannelState[Channel].NotifyCounter++;
            if (Icu_ChannelState[Channel].NotifyCounter >= 
                Icu_ChannelState[Channel].NotifyInterval) {
                Icu_ChannelState[Channel].NotifyCounter = 0U;
                
                /* Call notification if enabled */
                if ((Icu_ChannelState[Channel].NotificationEnabled) && 
                    (Icu_DriverState.ConfigPtr->Channels[Channel].Notification != NULL_PTR)) {
                    Icu_DriverState.ConfigPtr->Channels[Channel].Notification();
                }
            }
        }
        
        /* Wrap around buffer if full */
        if (Icu_ChannelState[Channel].BufferIndex >= 
            Icu_ChannelState[Channel].BufferSize) {
            Icu_ChannelState[Channel].BufferIndex = 0U;
        }
    }
}

/**
 * @brief Process edge count interrupt
 */
static void Icu_ProcessEdgeCount(Icu_ChannelType Channel)
{
    /* Increment edge counter */
    if (Icu_ChannelState[Channel].EdgeCount < ICU_MAX_EDGE_COUNT) {
        Icu_ChannelState[Channel].EdgeCount++;
    }
    
    /* Call notification if enabled (optional for edge count mode) */
    if ((Icu_ChannelState[Channel].NotificationEnabled) && 
        (Icu_DriverState.ConfigPtr->Channels[Channel].Notification != NULL_PTR)) {
        Icu_DriverState.ConfigPtr->Channels[Channel].Notification();
    }
}

/**
 * @brief Clear channel interrupt flag
 */
static void Icu_ClearInterruptFlag(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    /* Write 0 to CHF bit to clear (W1C) */
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    cnscValue &= ~ICU_TPM_CnSC_CHF;
    ICU_REG_WRITE32(cnscAddr, cnscValue);
}

/**
 * @brief Process interrupt for a single channel
 */
static void Icu_ProcessChannelInterrupt(Icu_ChannelType Channel)
{
    uint32 baseAddr = Icu_GetTpmBaseAddr(Channel);
    uint8 chOffset = Icu_GetTpmChannelOffset(Channel);
    uint32 cnscAddr = baseAddr + ICU_TPM_C0SC + (chOffset * ICU_TPM_CH_OFFSET);
    
    /* Check if interrupt flag is set and enabled */
    uint32 cnscValue = ICU_REG_READ32(cnscAddr);
    if ((cnscValue & ICU_TPM_CnSC_CHF) != 0U) {
        /* Clear interrupt flag first */
        Icu_ClearInterruptFlag(Channel);
        
        /* Process based on mode */
        Icu_MeasurementModeType mode = Icu_DriverState.ConfigPtr->Channels[Channel].Mode;
        
        switch (mode) {
            case ICU_MODE_SIGNAL_EDGE_DETECT:
                Icu_ProcessEdgeDetection(Channel);
                break;
                
            case ICU_MODE_SIGNAL_MEASUREMENT:
                Icu_ProcessSignalMeasurement(Channel);
                break;
                
            case ICU_MODE_TIMESTAMP:
                Icu_ProcessTimestamp(Channel);
                break;
                
            case ICU_MODE_EDGE_COUNTER:
                Icu_ProcessEdgeCount(Channel);
                break;
                
            default:
                /* Unknown mode - do nothing */
                break;
        }
    }
}

/*==================================================================================================
*                                    INTERRUPT HANDLERS
==================================================================================================*/

#define ICU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief TPM1 Interrupt Handler (Channels 0-1)
 */
void Icu_TPM1_IRQHandler(void)
{
    /* Process Channel 0 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_0);
    
    /* Process Channel 1 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_1);
}

/**
 * @brief TPM2 Interrupt Handler (Channels 2-3)
 */
void Icu_TPM2_IRQHandler(void)
{
    /* Process Channel 2 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_2);
    
    /* Process Channel 3 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_3);
}

/**
 * @brief TPM3 Interrupt Handler (Channels 4-5)
 */
void Icu_TPM3_IRQHandler(void)
{
    /* Process Channel 4 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_4);
    
    /* Process Channel 5 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_5);
}

/**
 * @brief TPM4 Interrupt Handler (Channels 6-7)
 */
void Icu_TPM4_IRQHandler(void)
{
    /* Process Channel 6 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_6);
    
    /* Process Channel 7 */
    Icu_ProcessChannelInterrupt(ICU_CHANNEL_7);
}

#define ICU_STOP_SEC_CODE
#include "MemMap.h"
