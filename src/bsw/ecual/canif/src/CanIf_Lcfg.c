/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*
 * CanIf_Lcfg.c
 * CAN Interface Link-Time Configuration
 * AUTOSAR-compliant implementation
 */

#include "CanIf.h"

/*=============================================================================
 * Hardware Object Handle (HOH) Configuration
 *=============================================================================
 * Maps logical HOHs to physical CAN driver objects
 */
const CanIf_HohCfgType CanIf_HohCfg[CANIF_HOH_CNT] =
{
    /* HTH 0 - Controller 0, Tx HOH, driver object 0 */
    {
        /* controllerId */    CANIF_CONTROLLER_0,
        /* isTx */            TRUE,
        /* driverObjId */     0U
    },
    /* HTH 1 - Controller 0, Tx HOH, driver object 1 */
    {
        /* controllerId */    CANIF_CONTROLLER_0,
        /* isTx */            TRUE,
        /* driverObjId */     1U
    },
    /* HRH 0 - Controller 0, Rx HOH, driver object 0 */
    {
        /* controllerId */    CANIF_CONTROLLER_0,
        /* isTx */            FALSE,
        /* driverObjId */     0U
    },
    /* HRH 1 - Controller 0, Rx HOH, driver object 1 */
    {
        /* controllerId */    CANIF_CONTROLLER_0,
        /* isTx */            FALSE,
        /* driverObjId */     1U
    }
};

/*=============================================================================
 * Transmit L-PDU Configuration
 *=============================================================================
 * Configuration for outgoing CAN messages
 */
const CanIf_TxPduCfgType CanIf_TxPduCfg[CANIF_TX_LPDU_CNT] =
{
    /* Tx L-PDU 0: Engine RPM (0x100) via HTH_0 */
    {
        /* pduId */           CANIF_TX_LPDU_0,
        /* canId */           0x100U,
        /* hthId */           CANIF_HTH_0,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             8U
    },
    /* Tx L-PDU 1: Vehicle Speed (0x200) via HTH_0 */
    {
        /* pduId */           CANIF_TX_LPDU_1,
        /* canId */           0x200U,
        /* hthId */           CANIF_HTH_0,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             4U
    },
    /* Tx L-PDU 2: Temperature (0x300) via HTH_1 */
    {
        /* pduId */           CANIF_TX_LPDU_2,
        /* canId */           0x300U,
        /* hthId */           CANIF_HTH_1,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             2U
    },
    /* Tx L-PDU 3: Diagnostics (0x700) via HTH_1 */
    {
        /* pduId */           CANIF_TX_LPDU_3,
        /* canId */           0x700U,
        /* hthId */           CANIF_HTH_1,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             8U
    }
};

/*=============================================================================
 * Receive L-PDU Configuration
 *=============================================================================
 * Configuration for incoming CAN message reception
 */
const CanIf_RxPduCfgType CanIf_RxPduCfg[CANIF_RX_LPDU_CNT] =
{
    /* Rx L-PDU 0: Throttle Position (0x150) via HRH_0 */
    {
        /* pduId */           CANIF_RX_LPDU_0,
        /* canId */           0x150U,
        /* canIdMask */       0x7FFU,  /* Exact match */
        /* hohId */           CANIF_HRH_0,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             2U
    },
    /* Rx L-PDU 1: Brake Pressure (0x250) via HRH_0 */
    {
        /* pduId */           CANIF_RX_LPDU_1,
        /* canId */           0x250U,
        /* canIdMask */       0x7FFU,  /* Exact match */
        /* hohId */           CANIF_HRH_0,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             4U
    },
    /* Rx L-PDU 2: Steering Angle (0x350) via HRH_1 */
    {
        /* pduId */           CANIF_RX_LPDU_2,
        /* canId */           0x350U,
        /* canIdMask */       0x7FFU,  /* Exact match */
        /* hohId */           CANIF_HRH_1,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             4U
    },
    /* Rx L-PDU 3: Diagnostic Requests (0x600) via HRH_1 */
    {
        /* pduId */           CANIF_RX_LPDU_3,
        /* canId */           0x600U,
        /* canIdMask */       0x7FFU,  /* Exact match */
        /* hohId */           CANIF_HRH_1,
        /* controllerId */    CANIF_CONTROLLER_0,
        /* dlc */             8U
    }
};

/*=============================================================================
 * Controller Configuration
 *=============================================================================
 * CAN controller configuration
 */
const CanIf_ControllerCfgType CanIf_ControllerCfg[CANIF_CONTROLLER_CNT] =
{
    {
        /* controllerId */    CANIF_CONTROLLER_0,
        /* initMode */        CANIF_CS_STOPPED
    }
};

/*=============================================================================
 * Rx L-PDU to HOH Mapping Table
 *=============================================================================
 * Lookup table for fast PDU dispatching on reception
 * Maps HOH indices to Rx PDU indices (or 0xFF for unused)
 */
const CanIf_PduIdType CanIf_RxPduHohMap[CANIF_HOH_CNT][CANIF_RX_LPDU_CNT] =
{
    /* HTH 0 - Not used for Rx */
    { 0xFFU, 0xFFU, 0xFFU, 0xFFU },
    /* HTH 1 - Not used for Rx */
    { 0xFFU, 0xFFU, 0xFFU, 0xFFU },
    /* HRH 0 - Receives PDUs 0 and 1 */
    { 0x00U, 0x01U, 0xFFU, 0xFFU },
    /* HRH 1 - Receives PDUs 2 and 3 */
    { 0xFFU, 0xFFU, 0x02U, 0x03U }
};
