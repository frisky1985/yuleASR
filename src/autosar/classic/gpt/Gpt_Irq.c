/******************************************************************************
 * @file    Gpt_Irq.c
 * @brief   GPT (General Purpose Timer) Driver Interrupt Handling
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "autosar/classic/gpt/gpt.h"
#include "autosar/classic/gpt/Gpt_Cfg.h"

#if (GPT_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/******************************************************************************
 * External Variables
 ******************************************************************************/
#define GPT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern boolean Gpt_DriverInitialized;
extern const Gpt_ConfigType* Gpt_ConfigPtr;
extern boolean Gpt_ChannelRunning[GPT_MAX_CHANNELS];
extern boolean Gpt_ChannelNotificationEnabled[GPT_MAX_CHANNELS];
extern Gpt_ChannelStatusType Gpt_ChannelStatus[GPT_MAX_CHANNELS];

#define GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/******************************************************************************
 * External Functions
 ******************************************************************************/
#define GPT_START_SEC_CODE
#include "MemMap.h"

extern void Gpt_ProcessExpiration(Gpt_ChannelType Channel);
extern void Gpt_ProcessCapture(Gpt_ChannelType Channel);
extern uint32 Gpt_GetBaseAddr(Gpt_ChannelType channel);
extern uint8 Gpt_GetChannelOffset(Gpt_ChannelType channel);

/******************************************************************************
 * Hardware Register Definitions
 ******************************************************************************/
#define GPT_SR                                  (0x08U)   /* Status Register */
#define GPT_IR                                  (0x0CU)   /* Interrupt Register */

/* Status Register Bits */
#define GPT_SR_OF1                              (0x00000001U)
#define GPT_SR_OF2                              (0x00000002U)
#define GPT_SR_OF3                              (0x00000004U)
#define GPT_SR_IF1                              (0x00000008U)
#define GPT_SR_IF2                              (0x00000010U)
#define GPT_SR_ROV                              (0x00000020U)

/* Interrupt Register Bits */
#define GPT_IR_OF1IE                            (0x00000001U)
#define GPT_IR_OF2IE                            (0x00000002U)
#define GPT_IR_OF3IE                            (0x00000004U)
#define GPT_IR_IF1IE                            (0x00000008U)
#define GPT_IR_IF2IE                            (0x00000010U)
#define GPT_IR_ROVIE                            (0x00000020U)

/******************************************************************************
 * Register Access Macros
 ******************************************************************************/
#define REG_READ32(addr)                        (*(volatile uint32*)(addr))
#define REG_WRITE32(addr, val)                  (*(volatile uint32*)(addr) = (val))

/******************************************************************************
 * Local Variables
 ******************************************************************************/
static volatile uint32 Gpt_IrqCounter[GPT_MAX_CHANNELS];

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get channel ID from base address and offset
 */
static Gpt_ChannelType Gpt_GetChannelFromOffset(uint32 baseAddr, uint8 offset)
{
    Gpt_ChannelType channel = GPT_MAX_CHANNELS;
    
    if (baseAddr == GPT1_BASE_ADDR) {
        if (offset < GPT_CHANNELS_PER_MODULE) {
            channel = offset;
        }
    } else if (baseAddr == GPT2_BASE_ADDR) {
        if (offset < GPT_CHANNELS_PER_MODULE) {
            channel = offset + GPT_CHANNELS_PER_MODULE;
        }
    }
    
    return channel;
}

/**
 * @brief Process output compare interrupt
 */
static void Gpt_ProcessOutputCompareIsr(uint32 baseAddr, uint8 offset)
{
    Gpt_ChannelType channel;
    uint32 srValue;
    uint32 ofMask = GPT_SR_OF1 << offset;
    
    /* Read and clear status */
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & ofMask) != 0U) {
        /* Clear the interrupt flag */
        REG_WRITE32(baseAddr + GPT_SR, ofMask);
        
        /* Get channel ID */
        channel = Gpt_GetChannelFromOffset(baseAddr, offset);
        
        if (channel < GPT_MAX_CHANNELS) {
            Gpt_IrqCounter[channel]++;
            
            /* Process timer expiration */
            Gpt_ProcessExpiration(channel);
        }
    }
}

/**
 * @brief Process input capture interrupt
 */
static void Gpt_ProcessInputCaptureIsr(uint32 baseAddr, uint8 captureId)
{
    Gpt_ChannelType channel = GPT_MAX_CHANNELS;
    uint32 srValue;
    uint32 ifMask = GPT_SR_IF1 << captureId;
    
    /* Read and clear status */
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & ifMask) != 0U) {
        /* Clear the interrupt flag */
        REG_WRITE32(baseAddr + GPT_SR, ifMask);
        
        /* Map capture ID to channels - capture is typically shared */
        if (baseAddr == GPT1_BASE_ADDR) {
            channel = 0U;  /* Channel 0 handles capture for GPT1 */
        } else if (baseAddr == GPT2_BASE_ADDR) {
            channel = 4U;  /* Channel 4 handles capture for GPT2 */
        }
        
        if (channel < GPT_MAX_CHANNELS) {
            /* Process capture event */
            Gpt_ProcessCapture(channel);
        }
    }
}

/**
 * @brief Process rollover interrupt
 */
static void Gpt_ProcessRolloverIsr(uint32 baseAddr)
{
    uint32 srValue;
    
    /* Read and clear status */
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_ROV) != 0U) {
        /* Clear the interrupt flag */
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_ROV);
        
        /* Update predefined timer values */
        /* This would typically increment high-word counters */
    }
}

/******************************************************************************
 * ISR Functions
 ******************************************************************************/

/**
 * @brief GPT1 Channel 0-3 Combined ISR
 * Handles all channels in GPT1 module
 */
ISR(Gpt_Isr_GPT1)
{
    uint32 baseAddr = GPT1_BASE_ADDR;
    uint32 srValue;
    uint32 irValue;
    
    /* Read status and interrupt enable */
    srValue = REG_READ32(baseAddr + GPT_SR);
    irValue = REG_READ32(baseAddr + GPT_IR);
    
    /* Only process enabled interrupts */
    srValue &= irValue;
    
    if (srValue == 0U) {
        return;
    }
    
    /* Process output compare interrupts for channels 0-2 (OF1, OF2, OF3) */
    if ((srValue & GPT_SR_OF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_0);
        }
    }
    
    if ((srValue & GPT_SR_OF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_1);
        }
    }
    
    if ((srValue & GPT_SR_OF3) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF3);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_2);
        }
    }
    
    /* Process input capture interrupts */
    if ((srValue & GPT_SR_IF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_IF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessCapture(GPT_CHANNEL_0);
        }
    }
    
    if ((srValue & GPT_SR_IF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_IF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessCapture(GPT_CHANNEL_0);
        }
    }
    
    /* Process rollover interrupt */
    if ((srValue & GPT_SR_ROV) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_ROV);
    }
}

/**
 * @brief GPT2 Channel 4-7 Combined ISR
 * Handles all channels in GPT2 module
 */
ISR(Gpt_Isr_GPT2)
{
    uint32 baseAddr = GPT2_BASE_ADDR;
    uint32 srValue;
    uint32 irValue;
    
    /* Read status and interrupt enable */
    srValue = REG_READ32(baseAddr + GPT_SR);
    irValue = REG_READ32(baseAddr + GPT_IR);
    
    /* Only process enabled interrupts */
    srValue &= irValue;
    
    if (srValue == 0U) {
        return;
    }
    
    /* Process output compare interrupts for channels 4-6 (OF1, OF2, OF3) */
    if ((srValue & GPT_SR_OF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_4);
        }
    }
    
    if ((srValue & GPT_SR_OF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_5);
        }
    }
    
    if ((srValue & GPT_SR_OF3) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF3);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_6);
        }
    }
    
    /* Process input capture interrupts */
    if ((srValue & GPT_SR_IF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_IF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessCapture(GPT_CHANNEL_4);
        }
    }
    
    if ((srValue & GPT_SR_IF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_IF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessCapture(GPT_CHANNEL_4);
        }
    }
    
    /* Process rollover interrupt */
    if ((srValue & GPT_SR_ROV) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_ROV);
    }
}

/******************************************************************************
 * Individual Channel ISRs (Alternative Implementation)
 ******************************************************************************/

#if defined(GPT_USE_INDIVIDUAL_ISRS) && (GPT_USE_INDIVIDUAL_ISRS == STD_ON)

/**
 * @brief GPT Channel 0 ISR
 */
ISR(Gpt_Isr_Channel0)
{
    uint32 baseAddr = GPT1_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_0);
        }
    }
}

/**
 * @brief GPT Channel 1 ISR
 */
ISR(Gpt_Isr_Channel1)
{
    uint32 baseAddr = GPT1_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_1);
        }
    }
}

/**
 * @brief GPT Channel 2 ISR
 */
ISR(Gpt_Isr_Channel2)
{
    uint32 baseAddr = GPT1_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF3) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF3);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_2);
        }
    }
}

/**
 * @brief GPT Channel 3 ISR (Uses separate compare logic or chaining)
 */
ISR(Gpt_Isr_Channel3)
{
    uint32 baseAddr = GPT1_BASE_ADDR;
    
    /* Channel 3 may use chained mode or rollover detection */
    /* Implementation depends on hardware capabilities */
    (void)baseAddr;
}

/**
 * @brief GPT Channel 4 ISR
 */
ISR(Gpt_Isr_Channel4)
{
    uint32 baseAddr = GPT2_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF1) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF1);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_4);
        }
    }
}

/**
 * @brief GPT Channel 5 ISR
 */
ISR(Gpt_Isr_Channel5)
{
    uint32 baseAddr = GPT2_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF2) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF2);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_5);
        }
    }
}

/**
 * @brief GPT Channel 6 ISR
 */
ISR(Gpt_Isr_Channel6)
{
    uint32 baseAddr = GPT2_BASE_ADDR;
    uint32 srValue;
    
    srValue = REG_READ32(baseAddr + GPT_SR);
    
    if ((srValue & GPT_SR_OF3) != 0U) {
        REG_WRITE32(baseAddr + GPT_SR, GPT_SR_OF3);
        if (Gpt_DriverInitialized) {
            Gpt_ProcessExpiration(GPT_CHANNEL_6);
        }
    }
}

/**
 * @brief GPT Channel 7 ISR
 */
ISR(Gpt_Isr_Channel7)
{
    uint32 baseAddr = GPT2_BASE_ADDR;
    
    /* Channel 7 may use chained mode or rollover detection */
    (void)baseAddr;
}

#endif /* GPT_USE_INDIVIDUAL_ISRS */

/******************************************************************************
 * Interrupt Management Functions
 ******************************************************************************/

/**
 * @brief Initialize interrupt settings for all channels
 */
void Gpt_Irq_Init(void)
{
    uint8 i;
    
    /* Clear all interrupt counters */
    for (i = 0U; i < GPT_MAX_CHANNELS; i++) {
        Gpt_IrqCounter[i] = 0U;
    }
    
    /* Clear hardware interrupt registers */
    REG_WRITE32(GPT1_BASE_ADDR + GPT_SR, 0x3FU);
    REG_WRITE32(GPT1_BASE_ADDR + GPT_IR, 0U);
    REG_WRITE32(GPT2_BASE_ADDR + GPT_SR, 0x3FU);
    REG_WRITE32(GPT2_BASE_ADDR + GPT_IR, 0U);
}

/**
 * @brief Deinitialize interrupt settings
 */
void Gpt_Irq_DeInit(void)
{
    uint8 i;
    
    /* Disable all interrupts */
    REG_WRITE32(GPT1_BASE_ADDR + GPT_IR, 0U);
    REG_WRITE32(GPT2_BASE_ADDR + GPT_IR, 0U);
    
    /* Clear all status flags */
    REG_WRITE32(GPT1_BASE_ADDR + GPT_SR, 0x3FU);
    REG_WRITE32(GPT2_BASE_ADDR + GPT_SR, 0x3FU);
    
    /* Clear interrupt counters */
    for (i = 0U; i < GPT_MAX_CHANNELS; i++) {
        Gpt_IrqCounter[i] = 0U;
    }
}

/**
 * @brief Enable interrupt for a specific channel
 */
void Gpt_Irq_Enable(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    uint8 chOffset;
    uint32 irValue;
    
    if (Channel >= GPT_MAX_CHANNELS) {
        return;
    }
    
    baseAddr = Gpt_GetBaseAddr(Channel);
    chOffset = Gpt_GetChannelOffset(Channel);
    
    if (baseAddr == 0U) {
        return;
    }
    
    if (chOffset < 3U) {
        irValue = REG_READ32(baseAddr + GPT_IR);
        irValue |= (GPT_IR_OF1IE << chOffset);
        REG_WRITE32(baseAddr + GPT_IR, irValue);
    }
}

/**
 * @brief Disable interrupt for a specific channel
 */
void Gpt_Irq_Disable(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    uint8 chOffset;
    uint32 irValue;
    
    if (Channel >= GPT_MAX_CHANNELS) {
        return;
    }
    
    baseAddr = Gpt_GetBaseAddr(Channel);
    chOffset = Gpt_GetChannelOffset(Channel);
    
    if (baseAddr == 0U) {
        return;
    }
    
    if (chOffset < 3U) {
        irValue = REG_READ32(baseAddr + GPT_IR);
        irValue &= ~(GPT_IR_OF1IE << chOffset);
        REG_WRITE32(baseAddr + GPT_IR, irValue);
    }
}

/**
 * @brief Get interrupt counter for a channel
 */
uint32 Gpt_Irq_GetCounter(Gpt_ChannelType Channel)
{
    if (Channel < GPT_MAX_CHANNELS) {
        return Gpt_IrqCounter[Channel];
    }
    return 0U;
}

/**
 * @brief Clear interrupt counter for a channel
 */
void Gpt_Irq_ClearCounter(Gpt_ChannelType Channel)
{
    if (Channel < GPT_MAX_CHANNELS) {
        Gpt_IrqCounter[Channel] = 0U;
    }
}

#define GPT_STOP_SEC_CODE
#include "MemMap.h"
