/**
 * @file Ocu_Irq.c
 * @brief OCU (Output Compare Unit) Driver interrupt handling
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver Interrupt Handling
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 * MISRA C:2012 Compliant
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Ocu.h"
#include "Ocu_Private.h"

#if (OCU_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
/**
 * @brief Interrupt status flags
 */
#define OCU_IRQ_COMPARE_MATCH               (0x01U)
#define OCU_IRQ_OVERFLOW                    (0x02U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* None */

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void Ocu_IrqHandler(Ocu_ChannelType Channel);

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
/* None */

/*==================================================================================================
*                                    GLOBAL VARIABLES
==================================================================================================*/
/* None */

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define OCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Common interrupt handler for all channels
 * @param Channel Channel that triggered the interrupt
 */
/** @req SWS_Ocu_00011 */
static void Ocu_IrqHandler(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs;
    uint32 status;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Read interrupt status */
    status = hwRegs->Status;

    /* Check for compare match */
    if ((status & OCU_STATUS_MATCH_BIT) != 0U)
    {
        /* Clear compare match flag */
        hwRegs->Status = OCU_STATUS_MATCH_BIT;

        /* Process compare match */
        Ocu_ProcessCompareMatch(Channel);

        /* Execute notification if enabled */
        #if (OCU_NOTIFICATION_SUPPORTED == STD_ON)
        if ((Ocu_ChannelState[Channel].NotificationEnabled) &&
            (Ocu_CurrentConfig->Channels[Channel].Notification != NULL_PTR))
        {
            Ocu_CurrentConfig->Channels[Channel].Notification();
        }
        #endif
    }

    /* Check for overflow (if applicable) */
    if ((status & OCU_STATUS_OVERFLOW_BIT) != 0U)
    {
        /* Clear overflow flag */
        hwRegs->Status = OCU_STATUS_OVERFLOW_BIT;

        /* Handle overflow event - can be used for PWM period completion */
        /* Additional overflow handling logic can be added here */
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Process compare match event
 * @param Channel Channel that triggered compare match
 * @implements Ocu_ProcessCompareMatch
 */
/** @req SWS_Ocu_00007 */
void Ocu_ProcessCompareMatch(Ocu_ChannelType Channel)
{
    Ocu_PinActionType action;
    Ocu_OutputPinStateType currentState;
    Ocu_OutputPinStateType newState;

    /* Get current pin action */
    action = Ocu_ChannelState[Channel].PinAction;
    currentState = Ocu_ChannelState[Channel].CurrentPinState;

    /* Perform action based on configuration */
    switch (action)
    {
        case OCU_SET_HIGH:
            newState = OCU_HIGH;
            break;

        case OCU_SET_LOW:
            newState = OCU_LOW;
            break;

        case OCU_TOGGLE:
            newState = (currentState == OCU_HIGH) ? OCU_LOW : OCU_HIGH;
            break;

        case OCU_HOLD:
        default:
            /* Keep current state */
            newState = currentState;
            break;
    }

    /* Update pin state if changed */
    if (newState != currentState)
    {
        Ocu_HwSetPinState(Channel, newState);
        Ocu_ChannelState[Channel].CurrentPinState = newState;
    }
}

/*==================================================================================================
*                                    INTERRUPT SERVICE ROUTINES
==================================================================================================*/

/**
 * @brief OCU Channel 0 Interrupt Handler
 * @implements Ocu_Channel0_IrqHandler
 */
/** @req SWS_Ocu_00011 */
void Ocu_Channel0_IrqHandler(void)
{
    #if (OCU_CHANNEL_0_ENABLE == STD_ON)
    Ocu_IrqHandler(OCU_CHANNEL_0);
    #endif
}

/**
 * @brief OCU Channel 1 Interrupt Handler
 * @implements Ocu_Channel1_IrqHandler
 */
/** @req SWS_Ocu_00011 */
void Ocu_Channel1_IrqHandler(void)
{
    #if (OCU_CHANNEL_1_ENABLE == STD_ON)
    Ocu_IrqHandler(OCU_CHANNEL_1);
    #endif
}

/**
 * @brief OCU Channel 2 Interrupt Handler
 * @implements Ocu_Channel2_IrqHandler
 */
/** @req SWS_Ocu_00011 */
void Ocu_Channel2_IrqHandler(void)
{
    #if (OCU_CHANNEL_2_ENABLE == STD_ON)
    Ocu_IrqHandler(OCU_CHANNEL_2);
    #endif
}

/**
 * @brief OCU Channel 3 Interrupt Handler
 * @implements Ocu_Channel3_IrqHandler
 */
/** @req SWS_Ocu_00011 */
void Ocu_Channel3_IrqHandler(void)
{
    #if (OCU_CHANNEL_3_ENABLE == STD_ON)
    Ocu_IrqHandler(OCU_CHANNEL_3);
    #endif
}

#define OCU_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    HARDWARE ABSTRACTION LAYER
==================================================================================================*/

#define OCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initialize hardware for a channel
 * @param Channel Channel to initialize
 * @param Config Channel configuration
 */
/** @req SWS_Ocu_00001 */
void Ocu_HwInitChannel(Ocu_ChannelType Channel, const Ocu_ChannelConfigType* Config)
{
    Ocu_HwRegisterType* hwRegs;
    uint32 ctrlValue;

    (void)Config;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Disable channel first */
    hwRegs->Control = 0U;

    /* Clear status */
    hwRegs->Status = 0xFFFFFFFFU;

    /* Reset counter */
    hwRegs->Counter = 0U;

    /* Set default compare value */
    hwRegs->Compare = Config->DefaultThreshold;

    /* Configure control register */
    ctrlValue = 0U;

    #if (OCU_NOTIFICATION_SUPPORTED == STD_ON)
    /* Enable interrupt if notification configured */
    if (Config->Notification != NULL_PTR)
    {
        ctrlValue |= OCU_CTRL_INTERRUPT_BIT;
    }
    #endif

    /* Set prescaler */
    ctrlValue |= ((uint32)OCU_DEFAULT_PRESCALER << OCU_CTRL_PRESCALER_SHIFT);

    /* Write control register */
    hwRegs->Control = ctrlValue;

    /* Configure pin action register */
    hwRegs->Action = (uint32)OCU_TOGGLE;
}

/**
 * @brief Deinitialize hardware for a channel
 * @param Channel Channel to deinitialize
 */
/** @req SWS_Ocu_00002 */
void Ocu_HwDeInitChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Disable channel */
    hwRegs->Control = 0U;

    /* Clear status */
    hwRegs->Status = 0xFFFFFFFFU;

    /* Reset counter */
    hwRegs->Counter = 0U;

    /* Reset compare value */
    hwRegs->Compare = 0U;
}

/**
 * @brief Start hardware channel
 * @param Channel Channel to start
 */
/** @req SWS_Ocu_00003 */
void Ocu_HwStartChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs;
    uint32 ctrlValue;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Read current control value */
    ctrlValue = hwRegs->Control;

    /* Enable channel */
    ctrlValue |= OCU_CTRL_ENABLE_BIT;

    /* Write control register */
    hwRegs->Control = ctrlValue;
}

/**
 * @brief Stop hardware channel
 * @param Channel Channel to stop
 */
/** @req SWS_Ocu_00004 */
void Ocu_HwStopChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs;
    uint32 ctrlValue;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Read current control value */
    ctrlValue = hwRegs->Control;

    /* Disable channel */
    ctrlValue &= ~OCU_CTRL_ENABLE_BIT;

    /* Write control register */
    hwRegs->Control = ctrlValue;
}

/**
 * @brief Set hardware pin state
 * @param Channel Channel to set
 * @param PinState Pin state to set
 */
/** @req SWS_Ocu_00005 */
void Ocu_HwSetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState)
{
    Ocu_HwRegisterType* hwRegs;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Set pin control register */
    hwRegs->PinCtrl = (uint32)PinState;
}

/**
 * @brief Set hardware pin action
 * @param Channel Channel to set
 * @param PinAction Pin action to set
 */
/** @req SWS_Ocu_00006 */
void Ocu_HwSetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction)
{
    Ocu_HwRegisterType* hwRegs;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Set action register */
    hwRegs->Action = (uint32)PinAction;
}

/**
 * @brief Set hardware compare value
 * @param Channel Channel to set
 * @param Value Compare value
 */
/** @req SWS_Ocu_00007 */
void Ocu_HwSetCompareValue(Ocu_ChannelType Channel, Ocu_ValueType Value)
{
    Ocu_HwRegisterType* hwRegs;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Set compare register */
    hwRegs->Compare = Value;
}

/**
 * @brief Get hardware counter value
 * @param Channel Channel to read
 * @return Counter value
 */
/** @req SWS_Ocu_00009 */
Ocu_ValueType Ocu_HwGetCounter(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs;

    /* Get hardware register base */
    hwRegs = Ocu_HwGetRegisterBase(Channel);

    /* Read counter register */
    return (Ocu_ValueType)hwRegs->Counter;
}

/**
 * @brief Get hardware register base address
 * @param Channel Channel
 * @return Register base address
 */
Ocu_HwRegisterType* Ocu_HwGetRegisterBase(Ocu_ChannelType Channel)
{
    /* Array of hardware base addresses - to be mapped to actual hardware */
    static Ocu_HwRegisterType Ocu_HwRegisters[OCU_NUM_CHANNELS];

    /* Return pointer to channel's register set */
    return &Ocu_HwRegisters[Channel];
}

#define OCU_STOP_SEC_CODE
#include "MemMap.h"
