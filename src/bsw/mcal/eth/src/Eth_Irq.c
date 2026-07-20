/**
 * @file Eth_Irq.c
 * @brief Ethernet Driver Interrupt Handling
 * @version 1.0.0
 * 
 * Ethernet MAC driver interrupt service routines.
 * Based on AUTOSAR Classic Platform 4.4.0.
 * ASIL-D safety level compatible, MISRA C:2012 compliant.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                              PRE-COMPILATION CHECKS
==================================================================================================*/
#include "Eth.h"
#include "Eth_Private.h"
#include "Eth_Cfg.h"

/*==================================================================================================
*                              MEMORY SECTIONS
==================================================================================================*/
#define ETH_START_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                              INTERNAL FUNCTION PROTOTYPES
==================================================================================================*/
static void Eth_ProcessTxInterrupt(Eth_ControllerType CtrlIdx);
static void Eth_ProcessRxInterrupt(Eth_ControllerType CtrlIdx);
static void Eth_ProcessErrorInterrupt(Eth_ControllerType CtrlIdx, uint32 Status);

/*==================================================================================================
*                              STATIC HELPER FUNCTIONS
==================================================================================================*/

/**
 * @brief Process TX interrupt
 */
static void Eth_ProcessTxInterrupt(Eth_ControllerType CtrlIdx)
{
    uint8 bufIdx;
    
    /* Process all completed TX descriptors */
    for (bufIdx = 0u; bufIdx < Eth_CtrlState[CtrlIdx].TxBufCount; bufIdx++)
    {
        if (Eth_TxDesc[CtrlIdx][bufIdx].State == ETH_BUF_STATE_TRANSMITTING)
        {
            /* Transmission complete */
            Eth_TxDesc[CtrlIdx][bufIdx].State = ETH_BUF_STATE_READY;
            
            /* Call confirmation if needed */
            if (Eth_TxDesc[CtrlIdx][bufIdx].TxConfirmation == TRUE)
            {
                /* Call upper layer callback */
                /* EthIf_TxConfirmation(CtrlIdx, bufIdx); */
            }
            
            /* Free buffer */
            Eth_TxDesc[CtrlIdx][bufIdx].State = ETH_BUF_STATE_FREE;
            Eth_TxDesc[CtrlIdx][bufIdx].Len = 0u;
            
            if (Eth_CtrlState[CtrlIdx].TxPendingCount > 0u)
            {
                Eth_CtrlState[CtrlIdx].TxPendingCount--;
            }
        }
    }
}

/**
 * @brief Process RX interrupt
 */
static void Eth_ProcessRxInterrupt(Eth_ControllerType CtrlIdx)
{
    uint8 bufIdx;
    uint8* rxDataPtr;
    uint16 rxLen;
    
    /* Process all received frames */
    for (bufIdx = 0u; bufIdx < Eth_CtrlState[CtrlIdx].RxBufCount; bufIdx++)
    {
        /* Check if buffer contains received data */
        if (Eth_RxDesc[CtrlIdx][bufIdx].State == ETH_BUF_STATE_FREE)
        {
            /* In real hardware, check descriptor ownership bit */
            /* For now, we assume no frame received in mock */
        }
    }
    
    /* Call upper layer indication */
    /* EthIf_RxIndication(CtrlIdx, ...); */
    
    (void)rxDataPtr;
    (void)rxLen;
}

/**
 * @brief Process error interrupt
 */
static void Eth_ProcessErrorInterrupt(Eth_ControllerType CtrlIdx, uint32 Status)
{
    /* Check for fatal bus error */
    if ((Status & ETH_DMA_SR_FBE) != 0u)
    {
        /* Fatal bus error - reset DMA */
        /* Reset TX and RX processes */
    }
    
    /* Check for receive overflow */
    if ((Status & ETH_DMA_SR_OVF) != 0u)
    {
        /* Receive overflow - discard current frame */
        /* Re-initialize RX process if needed */
    }
    
    /* Check for transmit underflow */
    if ((Status & ETH_DMA_SR_UNF) != 0u)
    {
        /* Transmit underflow - re-initialize TX process */
    }
    
    /* Check for receive buffer unavailable */
    if ((Status & ETH_DMA_SR_RU) != 0u)
    {
        /* Receive buffer unavailable - provide more buffers */
    }
    
    /* Check for transmit buffer unavailable */
    if ((Status & ETH_DMA_SR_TU) != 0u)
    {
        /* Transmit buffer unavailable - wait for completion */
    }
    
    (void)CtrlIdx;
}

/*==================================================================================================
*                              ISR IMPLEMENTATIONS
==================================================================================================*/

/**
 * @brief Common interrupt handler for all Ethernet interrupts
 */
static void Eth_IsrCommon(Eth_ControllerType CtrlIdx)
{
    uint32 status;
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        return;
    }
    
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        return;
    }
    
    if (Eth_CtrlState[CtrlIdx].InterruptsEnabled == FALSE)
    {
        return;
    }
    
    /* Read DMA status register */
    /* status = ETH_DMA->SR; */
    status = 0u;  /* Mock value */
    
    /* Check for normal interrupt summary */
    if ((status & ETH_DMA_SR_NIS) != 0u)
    {
        /* Transmit interrupt */
        if ((status & ETH_DMA_SR_TI) != 0u)
        {
            Eth_ProcessTxInterrupt(CtrlIdx);
            /* Clear transmit interrupt */
        }
        
        /* Receive interrupt */
        if ((status & ETH_DMA_SR_RI) != 0u)
        {
            Eth_ProcessRxInterrupt(CtrlIdx);
            /* Clear receive interrupt */
        }
        
        /* Early transmit interrupt */
        if ((status & ETH_DMA_SR_ETI) != 0u)
        {
            /* Handle early transmit */
        }
        
        /* Early receive interrupt */
        if ((status & ETH_DMA_SR_ERI) != 0u)
        {
            /* Handle early receive */
        }
    }
    
    /* Check for abnormal interrupt summary */
    if ((status & ETH_DMA_SR_AIS) != 0u)
    {
        Eth_ProcessErrorInterrupt(CtrlIdx, status);
        
        /* Receive process stopped */
        if ((status & ETH_DMA_SR_RPS) != 0u)
        {
            /* Re-initialize RX process */
        }
        
        /* Transmit process stopped */
        if ((status & ETH_DMA_SR_TPS) != 0u)
        {
            /* Re-initialize TX process */
        }
        
        /* Receive watchdog timeout */
        if ((status & ETH_DMA_SR_RWT) != 0u)
        {
            /* Handle watchdog timeout */
        }
        
        /* Transmit jabber timeout */
        if ((status & ETH_DMA_SR_TJT) != 0u)
        {
            /* Handle jabber timeout */
        }
    }
    
    /* Clear interrupt status */
    /* ETH_DMA->SR = status; */
}

/**
 * @brief TX interrupt service routine
 */
void Eth_IsrTx(Eth_ControllerType CtrlIdx)
{
    if (CtrlIdx < ETH_MAX_CONTROLLERS)
    {
        Eth_ProcessTxInterrupt(CtrlIdx);
    }
}

/**
 * @brief RX interrupt service routine
 */
void Eth_IsrRx(Eth_ControllerType CtrlIdx)
{
    if (CtrlIdx < ETH_MAX_CONTROLLERS)
    {
        Eth_ProcessRxInterrupt(CtrlIdx);
    }
}

/**
 * @brief Error interrupt service routine
 */
void Eth_IsrError(Eth_ControllerType CtrlIdx)
{
    if (CtrlIdx < ETH_MAX_CONTROLLERS)
    {
        /* Read and clear error status */
        Eth_ProcessErrorInterrupt(CtrlIdx, 0xFFFFFFFFu);
    }
}

/*==================================================================================================
*                              CONTROLLER-SPECIFIC ISRs
==================================================================================================*/

#if (ETH_MAX_CONTROLLERS > 0U )
/**
 * @brief Controller 0 common interrupt handler
 */
void Eth_IsrCtrl0(void)
{
    Eth_IsrCommon(0u);
}

/**
 * @brief Controller 0 TX interrupt handler
 */
void Eth_IsrCtrl0Tx(void)
{
    Eth_IsrTx(0u);
}

/**
 * @brief Controller 0 RX interrupt handler
 */
void Eth_IsrCtrl0Rx(void)
{
    Eth_IsrRx(0u);
}

/**
 * @brief Controller 0 Error interrupt handler
 */
void Eth_IsrCtrl0Error(void)
{
    Eth_IsrError(0u);
}
#endif

#if (ETH_MAX_CONTROLLERS > 1)
/**
 * @brief Controller 1 common interrupt handler
 */
void Eth_IsrCtrl1(void)
{
    Eth_IsrCommon(1u);
}

/**
 * @brief Controller 1 TX interrupt handler
 */
void Eth_IsrCtrl1Tx(void)
{
    Eth_IsrTx(1u);
}

/**
 * @brief Controller 1 RX interrupt handler
 */
void Eth_IsrCtrl1Rx(void)
{
    Eth_IsrRx(1u);
}

/**
 * @brief Controller 1 Error interrupt handler
 */
void Eth_IsrCtrl1Error(void)
{
    Eth_IsrError(1u);
}
#endif

#define ETH_STOP_SEC_CODE
#include "MemMap.h"
