/**
 * @file Eth_Irq.c
 * @brief Eth (Ethernet Driver) Interrupt Handling
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Eth Module - Interrupt Service Routines
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x11
 * MISRA C:2012 compliant
 */

#include "mcal/eth/Eth.h"
#include "mcal/eth/Eth_Cfg.h"
#include <string.h>

/*============================================================================*
 * External Configuration Reference
 *============================================================================*/
extern const Eth_ControllerConfigType gEth_DefaultControllers[ETH_CFG_CONTROLLER_COUNT];

/*============================================================================*
 * Static Variables for ISR Context
 *============================================================================*/
static volatile uint32_t gEth_IrqCounters[ETH_CFG_CONTROLLER_COUNT][16U];

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Generic IRQ handler for a controller
 *
 * @param ctrlIdx Controller index
 */
static void Eth_HandleIrq(uint8_t ctrlIdx)
{
    const Eth_HwInterfaceType* hwIf;
    Eth_IrqEventType event = ETH_IRQ_NONE;

    if (ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) {
        return;
    }

    hwIf = Eth_GetHwInterface();
    if (hwIf == NULL) {
        return;
    }

    /* Process all pending interrupts */
    if (hwIf->ProcessIrq != NULL) {
        do {
            hwIf->ProcessIrq(ctrlIdx, &event);

            if (event != ETH_IRQ_NONE) {
                /* Update counter */
                switch (event) {
                    case ETH_IRQ_TX_COMPLETE:
                        gEth_IrqCounters[ctrlIdx][0U]++;
                        break;
                    case ETH_IRQ_RX_COMPLETE:
                        gEth_IrqCounters[ctrlIdx][1U]++;
                        break;
                    case ETH_IRQ_TX_ERROR:
                        gEth_IrqCounters[ctrlIdx][2U]++;
                        break;
                    case ETH_IRQ_RX_ERROR:
                        gEth_IrqCounters[ctrlIdx][3U]++;
                        break;
                    case ETH_IRQ_DMA_ERROR:
                        gEth_IrqCounters[ctrlIdx][4U]++;
                        break;
                    case ETH_IRQ_PHY_EVENT:
                        gEth_IrqCounters[ctrlIdx][5U]++;
                        break;
                    case ETH_IRQ_TIMESTAMP:
                        gEth_IrqCounters[ctrlIdx][6U]++;
                        break;
                    case ETH_IRQ_BUS_ERROR:
                        gEth_IrqCounters[ctrlIdx][7U]++;
                        break;
                    case ETH_IRQ_WAKEUP:
                        gEth_IrqCounters[ctrlIdx][8U]++;
                        break;
                    default:
                        gEth_IrqCounters[ctrlIdx][15U]++;
                        break;
                }

                /* Call application callback via Eth_ProcessIrq */
                (void)Eth_ProcessIrq(ctrlIdx, &event);
            }
        } while (event != ETH_IRQ_NONE);
    }
}

/*============================================================================*
 * Interrupt Service Routines
 *============================================================================*/

/**
 * @brief Controller 0 Main Interrupt Handler
 *
 * Handles all Ethernet interrupts for controller 0.
 * This ISR is triggered by:
 * - TX Complete
 * - RX Complete
 * - TX Error
 * - RX Error
 * - DMA Error
 * - PHY Event
 * - Timestamp
 */
void Eth_IrqHandler_0(void)
{
    Eth_HandleIrq(0U);
}

/**
 * @brief Controller 1 Main Interrupt Handler
 *
 * Handles all Ethernet interrupts for controller 1.
 */
#if (ETH_CFG_CONTROLLER_COUNT > 1U)
void Eth_IrqHandler_1(void)
{
    Eth_HandleIrq(1U);
}
#endif

/**
 * @brief Controller 2 Main Interrupt Handler
 */
#if (ETH_CFG_CONTROLLER_COUNT > 2U)
void Eth_IrqHandler_2(void)
{
    Eth_HandleIrq(2U);
}
#endif

/**
 * @brief Controller 3 Main Interrupt Handler
 */
#if (ETH_CFG_CONTROLLER_COUNT > 3U)
void Eth_IrqHandler_3(void)
{
    Eth_HandleIrq(3U);
}
#endif

/*============================================================================*
 * TX/RX Specific Interrupt Handlers
 *============================================================================*/

/**
 * @brief TX Interrupt Handler (Controller 0)
 *
 * Optimized handler for TX-only interrupts.
 */
void Eth_TxIrqHandler_0(void)
{
    Eth_IrqEventType event = ETH_IRQ_TX_COMPLETE;
    (void)Eth_ProcessIrq(0U, &event);
}

/**
 * @brief RX Interrupt Handler (Controller 0)
 *
 * Optimized handler for RX-only interrupts.
 */
void Eth_RxIrqHandler_0(void)
{
    Eth_IrqEventType event = ETH_IRQ_RX_COMPLETE;
    (void)Eth_ProcessIrq(0U, &event);
}

/**
 * @brief Error Interrupt Handler (Controller 0)
 *
 * Handles DMA errors, bus errors, and other error conditions.
 */
void Eth_ErrIrqHandler_0(void)
{
    Eth_IrqEventType event = ETH_IRQ_DMA_ERROR;
    (void)Eth_ProcessIrq(0U, &event);
}

#if (ETH_CFG_CONTROLLER_COUNT > 1U)
void Eth_TxIrqHandler_1(void)
{
    Eth_IrqEventType event = ETH_IRQ_TX_COMPLETE;
    (void)Eth_ProcessIrq(1U, &event);
}

void Eth_RxIrqHandler_1(void)
{
    Eth_IrqEventType event = ETH_IRQ_RX_COMPLETE;
    (void)Eth_ProcessIrq(1U, &event);
}

void Eth_ErrIrqHandler_1(void)
{
    Eth_IrqEventType event = ETH_IRQ_DMA_ERROR;
    (void)Eth_ProcessIrq(1U, &event);
}
#endif

/*============================================================================*
 * Interrupt Vector Table Entry Points
 *============================================================================*/

/**
 * @brief Weak alias for default interrupt handler
 *
 * Can be overridden by hardware-specific implementation.
 */
__attribute__((weak)) void Eth_DefaultIrqHandler(void)
{
    /* Default empty handler */
}

/*============================================================================*
 * Interrupt Configuration Functions
 *============================================================================*/

/**
 * @brief Initialize interrupt system for a controller
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_IrqInit(uint8_t ctrlIdx)
{
    const Eth_ControllerConfigType* cfg;

    if (ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) {
        return ETH_E_INV_CTRL;
    }

    /* Clear interrupt counters */
    (void)memset((void*)gEth_IrqCounters[ctrlIdx], 0, sizeof(gEth_IrqCounters[ctrlIdx]));

    /* Get configuration - just validate index, config is in the array */
    switch (ctrlIdx) {
        case 0U:
            /* Controller 0 is valid */
            break;
#if (ETH_CFG_CONTROLLER_COUNT > 1U)
        case 1U:
            /* Controller 1 is valid */
            break;
#endif
        default:
            return ETH_E_INV_CTRL;
    }

    /* Configure NVIC or equivalent interrupt controller */
    /* This would be platform-specific */
    (void)cfg;

    return ETH_OK;
}

/**
 * @brief Deinitialize interrupt system for a controller
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_IrqDeinit(uint8_t ctrlIdx)
{
    if (ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) {
        return ETH_E_INV_CTRL;
    }

    /* Disable all interrupts for this controller */
    Eth_DisableIrq(ctrlIdx, 0xFFFFFFFFU);

    return ETH_OK;
}

/**
 * @brief Enable TX interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_EnableTxIrq(uint8_t ctrlIdx)
{
    Eth_EnableIrq(ctrlIdx, ETH_IRQ_TX_COMPLETE | ETH_IRQ_TX_ERROR);
}

/**
 * @brief Disable TX interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_DisableTxIrq(uint8_t ctrlIdx)
{
    Eth_DisableIrq(ctrlIdx, ETH_IRQ_TX_COMPLETE | ETH_IRQ_TX_ERROR);
}

/**
 * @brief Enable RX interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_EnableRxIrq(uint8_t ctrlIdx)
{
    Eth_EnableIrq(ctrlIdx, ETH_IRQ_RX_COMPLETE | ETH_IRQ_RX_ERROR);
}

/**
 * @brief Disable RX interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_DisableRxIrq(uint8_t ctrlIdx)
{
    Eth_DisableIrq(ctrlIdx, ETH_IRQ_RX_COMPLETE | ETH_IRQ_RX_ERROR);
}

/**
 * @brief Enable error interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_EnableErrorIrq(uint8_t ctrlIdx)
{
    Eth_EnableIrq(ctrlIdx, ETH_IRQ_DMA_ERROR | ETH_IRQ_BUS_ERROR);
}

/**
 * @brief Disable error interrupts
 *
 * @param ctrlIdx Controller index
 */
void Eth_DisableErrorIrq(uint8_t ctrlIdx)
{
    Eth_DisableIrq(ctrlIdx, ETH_IRQ_DMA_ERROR | ETH_IRQ_BUS_ERROR);
}

/*============================================================================*
 * Interrupt Statistics
 *============================================================================*/

/**
 * @brief Get interrupt counter
 *
 * @param ctrlIdx Controller index
 * @param irqType Interrupt type (0-15)
 * @return Counter value
 */
uint32_t Eth_IrqGetCounter(uint8_t ctrlIdx, uint8_t irqType)
{
    if ((ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) || (irqType >= 16U)) {
        return 0U;
    }

    return gEth_IrqCounters[ctrlIdx][irqType];
}

/**
 * @brief Reset interrupt counters
 *
 * @param ctrlIdx Controller index
 */
void Eth_IrqResetCounters(uint8_t ctrlIdx)
{
    if (ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) {
        return;
    }

    (void)memset((void*)gEth_IrqCounters[ctrlIdx], 0, sizeof(gEth_IrqCounters[ctrlIdx]));
}

/**
 * @brief Get interrupt load (percentage of time in ISR)
 *
 * @param ctrlIdx Controller index
 * @return Interrupt load percentage (0-100)
 */
uint8_t Eth_IrqGetLoad(uint8_t ctrlIdx)
{
    (void)ctrlIdx;
    /* Would require timing measurements in a real implementation */
    return 0U;
}

/*============================================================================*
 * Link Status Polling (if using polling instead of interrupt)
 *============================================================================*/

/**
 * @brief Poll link status
 *
 * Called periodically to check link status when not using PHY interrupts.
 *
 * @param ctrlIdx Controller index
 */
void Eth_PollLinkStatus(uint8_t ctrlIdx)
{
    bool linkUp;
    static bool prevLinkState[ETH_CFG_CONTROLLER_COUNT] = {false};

    if (ctrlIdx >= ETH_CFG_CONTROLLER_COUNT) {
        return;
    }

    linkUp = Eth_GetLinkState(ctrlIdx);

    if (linkUp != prevLinkState[ctrlIdx]) {
        prevLinkState[ctrlIdx] = linkUp;
        /* Link state change callback is called by GetLinkState via UpdateLinkState */
    }
}

/*============================================================================*
 * Platform-Specific IRQ Hooks
 *============================================================================*/

/**
 * @brief Weak hook for Aurix TC3xx specific interrupt handling
 */
__attribute__((weak)) void Eth_Aurix_GethIrqHandler(void)
{
    /* Override in Eth_Hw_Aurix.c */
}

/**
 * @brief Weak hook for S32G specific interrupt handling
 */
__attribute__((weak)) void Eth_S32G_EnetQosIrqHandler(void)
{
    /* Override in Eth_Hw_S32G.c */
}

/**
 * @brief Weak hook for STM32H7 specific interrupt handling
 */
__attribute__((weak)) void Eth_STM32_EthIrqHandler(void)
{
    /* Override in Eth_Hw_STM32.c */
}
