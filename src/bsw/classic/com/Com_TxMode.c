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

/******************************************************************************
 * @file    Com_TxMode.c
 * @brief   COM Module - Transmission Mode Manager Implementation
 *
 * This file implements the transmission mode manager for the AUTOSAR COM module.
 * Features:
 * - Four transmission modes: DIRECT, MIXED, PERIODIC, NONE
 * - ComTxModeTrue/ComTxModeFalse configuration support
 * - Signal-based Transmission Mode Condition (TMC) evaluation
 * - Periodic transmission scheduling
 * - Transmission mode switching logic
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x1E (COM)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/*==================[Includes]=============================================*/

#include "Com_TxMode.h"
#include <string.h>

/*==================[Version Check]=========================================*/

#if (COM_SW_MAJOR_VERSION != COM_TXMODE_SW_MAJOR_VERSION)
#error "Com_TxMode.c: Major version mismatch with Com.h"
#endif

#if (COM_SW_MINOR_VERSION != COM_TXMODE_SW_MINOR_VERSION)
#error "Com_TxMode.c: Minor version mismatch with Com.h"
#endif

/*==================[Global Variables]=====================================*/

/** Transmission mode runtime states for all I-PDUs */
Com_TxModeStateType Com_TxModeStates[COM_MAX_IPDUS];

/** Signal change tracking for TMC evaluation */
Com_TxModeSignalChangeType Com_TxModeSignalChanges[COM_MAX_SIGNALS];

/** Timestamp of last main function call (for timer calculations) */
static uint32 Com_TxModeLastTimeMs = 0u;

/** Main function period in milliseconds */
#ifndef COM_MAIN_FUNCTION_PERIOD_MS
#define COM_MAIN_FUNCTION_PERIOD_MS 10u
#endif

/*==================[Local Function Declarations]===========================*/

static void Com_TxModeProcessDirect(Com_IPduIdType PduId);
static void Com_TxModeProcessPeriodicMode(Com_IPduIdType PduId);
static void Com_TxModeProcessMixed(Com_IPduIdType PduId);
static void Com_TxModeProcessNone(Com_IPduIdType PduId);
static boolean Com_TxModeCheckSignalAgainstThreshold(Com_SignalIdType SignalId, uint32 Threshold, boolean UseGreaterThan);
static void Com_TxModeExecuteTransmission(Com_IPduIdType PduId);

/*==================[Transmission Mode Manager Implementation]==============*/

/**
 * @brief Initialize the transmission mode manager
 */
void Com_TxModeInit(void)
{
    /* Initialize all transmission mode states */
    for (uint16 i = 0u; i < COM_MAX_IPDUS; i++) {
        Com_TxModeStates[i].State = COM_TXMODESTATE_IDLE;
        Com_TxModeStates[i].CurrentTxMode = NULL_PTR;
        Com_TxModeStates[i].CycleTimer = 0u;
        Com_TxModeStates[i].RepetitionTimer = 0u;
        Com_TxModeStates[i].RepetitionCounter = 0u;
        Com_TxModeStates[i].ModeSwitched = FALSE;
        Com_TxModeStates[i].TimeInCurrentMode = 0u;
    }

    /* Initialize signal change tracking */
    Com_TxModeInitSignalChanges();

    /* Initialize timers */
    Com_TxModeInitPeriodicTimers();

    /* Record initial time */
    Com_TxModeLastTimeMs = Com_TxModeGetTimeMs();
}

/**
 * @brief Process transmission modes for all send I-PDUs
 */
void Com_TxModeProcessIPdu(Com_IPduIdType PduId)
{
    /* Validate PDU ID */
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }

    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];

    /* Only process send I-PDUs */
    if (ipduConfig->Direction != COM_SEND) {
        return;
    }

    /* Check if IPdu group is started */
    if (Com_GlobalState.IPduRunTime[PduId].GroupStatus != COM_IPDU_GROUP_STARTED) {
        return;
    }

    /* Evaluate TMC and switch mode if necessary */
    if (ipduConfig->TxMode.UseTmc) {
        Com_TmcResultType tmcResult = Com_TxModeEvaluateTmc(PduId);
        if (tmcResult == COM_TMC_TRUE) {
            if (Com_TxModeStates[PduId].CurrentTxMode != &ipduConfig->TxMode.TxModeTrue) {
                Com_TxModeSwitch(PduId, TRUE);
            }
        } else if (tmcResult == COM_TMC_FALSE) {
            if (Com_TxModeStates[PduId].CurrentTxMode != &ipduConfig->TxMode.TxModeFalse) {
                Com_TxModeSwitch(PduId, FALSE);
            }
        }
    }

    /* Get pointer to current transmission mode configuration */
    Com_TxModeType* txMode;
    if (Com_TxModeStates[PduId].CurrentTxMode != NULL_PTR) {
        txMode = Com_TxModeStates[PduId].CurrentTxMode;
    } else {
        /* Default to TxModeFalse if not initialized */
        txMode = (Com_TxModeType*)&ipduConfig->TxMode.TxModeFalse;
        Com_TxModeStates[PduId].CurrentTxMode = txMode;
    }

    /* Process based on transmission mode */
    switch (txMode->Mode) {
        case COM_TXMODE_DIRECT:
            Com_TxModeProcessDirect(PduId);
            break;

        case COM_TXMODE_PERIODIC:
            Com_TxModeProcessPeriodicMode(PduId);
            break;

        case COM_TXMODE_MIXED:
            Com_TxModeProcessMixed(PduId);
            break;

        case COM_TXMODE_NONE:
            Com_TxModeProcessNone(PduId);
            break;

        default:
            /* Invalid mode - do nothing */
            break;
    }
}

/**
 * @brief Process DIRECT transmission mode
 */
static void Com_TxModeProcessDirect(Com_IPduIdType PduId)
{
    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Check if triggered and handle repetitions */
    if (modeState->State == COM_TXMODESTATE_TRIGGERED) {
        /* Execute transmission */
        Com_TxModeExecuteTransmission(PduId);

        /* Start repetitions if configured */
        if (modeState->CurrentTxMode->NumRepetitions > 0u) {
            modeState->RepetitionCounter = modeState->CurrentTxMode->NumRepetitions;
            modeState->RepetitionTimer = modeState->CurrentTxMode->RepetitionPeriod;
            modeState->State = COM_TXMODESTATE_REPEATING;
        } else {
            modeState->State = COM_TXMODESTATE_IDLE;
        }
    } else if (modeState->State == COM_TXMODESTATE_REPEATING) {
        /* Process repetition timing */
        if ((Com_TxModeProcessRepetition(PduId)) != 0U) {
            /* Execute repetition transmission */
            Com_TxModeExecuteTransmission(PduId);

            /* Decrement repetition counter */
            if (modeState->RepetitionCounter > 0u) {
                modeState->RepetitionCounter--;
            }

            /* Check if repetitions complete */
            if (modeState->RepetitionCounter == 0u) {
                modeState->State = COM_TXMODESTATE_IDLE;
            } else {
                /* Reset repetition timer */
                modeState->RepetitionTimer = modeState->CurrentTxMode->RepetitionPeriod;
            }
        }
    }
}

/**
 * @brief Process PERIODIC transmission mode
 */
static void Com_TxModeProcessPeriodicMode(Com_IPduIdType PduId)
{
    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Check for periodic transmission */
    if ((Com_TxModeProcessPeriodic(PduId)) != 0U) {
        /* Execute periodic transmission */
        Com_TxModeExecuteTransmission(PduId);

        /* Reset cycle timer */
        modeState->CycleTimer = modeState->CurrentTxMode->CycleTime;
        modeState->State = COM_TXMODESTATE_PERIODIC;
    }
}

/**
 * @brief Process MIXED transmission mode
 */
static void Com_TxModeProcessMixed(Com_IPduIdType PduId)
{
    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* First handle any triggered direct transmission */
    if (modeState->State == COM_TXMODESTATE_TRIGGERED) {
        /* Execute immediate transmission */
        Com_TxModeExecuteTransmission(PduId);

        /* Start repetitions if configured */
        if (modeState->CurrentTxMode->NumRepetitions > 0u) {
            modeState->RepetitionCounter = modeState->CurrentTxMode->NumRepetitions;
            modeState->RepetitionTimer = modeState->CurrentTxMode->RepetitionPeriod;
            modeState->State = COM_TXMODESTATE_REPEATING;
        } else {
            /* Return to periodic waiting state */
            modeState->State = COM_TXMODESTATE_WAITING;
        }
    } else if (modeState->State == COM_TXMODESTATE_REPEATING) {
        /* Process repetition timing */
        if ((Com_TxModeProcessRepetition(PduId)) != 0U) {
            /* Execute repetition transmission */
            Com_TxModeExecuteTransmission(PduId);

            /* Decrement repetition counter */
            if (modeState->RepetitionCounter > 0u) {
                modeState->RepetitionCounter--;
            }

            /* Check if repetitions complete */
            if (modeState->RepetitionCounter == 0u) {
                modeState->State = COM_TXMODESTATE_WAITING;
            } else {
                /* Reset repetition timer */
                modeState->RepetitionTimer = modeState->CurrentTxMode->RepetitionPeriod;
            }
        }
    } else {
        /* Process periodic part of MIXED mode */
        if ((Com_TxModeProcessPeriodic(PduId)) != 0U) {
            /* Execute periodic transmission */
            Com_TxModeExecuteTransmission(PduId);

            /* Reset cycle timer */
            modeState->CycleTimer = modeState->CurrentTxMode->CycleTime;
            modeState->State = COM_TXMODESTATE_PERIODIC;
        }
    }
}

/**
 * @brief Process NONE transmission mode (no transmission)
 */
static void Com_TxModeProcessNone(Com_IPduIdType PduId)
{
    /* In NONE mode, reset any pending transmission states */
    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    if (modeState->State != COM_TXMODESTATE_IDLE) {
        modeState->State = COM_TXMODESTATE_IDLE;
        modeState->RepetitionCounter = 0u;
        modeState->RepetitionTimer = 0u;
    }
}

/**
 * @brief Execute transmission for an I-PDU
 */
static void Com_TxModeExecuteTransmission(Com_IPduIdType PduId)
{
    /* Queue the transmission request */
    Std_ReturnType result = Com_TxQueueAddRequest(
        COM_TXREQ_TRIGGERED,
        PduId,
        0u,  /* SignalId not applicable for triggered requests */
        0u   /* SignalGroupId not applicable */
    );

    if (result == E_OK) {
        /* Transmission queued successfully */
    }
    /* Note: Actual transmission happens in Com_MainFunctionTx */
}

/*==================[Signal Change Detection Implementation]================*/

/**
 * @brief Initialize signal change tracking
 */
void Com_TxModeInitSignalChanges(void)
{
    for (uint16 i = 0u; i < COM_MAX_SIGNALS; i++) {
        Com_TxModeSignalChanges[i].SignalId = i;
        Com_TxModeSignalChanges[i].HasChanged = FALSE;
        Com_TxModeSignalChanges[i].IsValid = FALSE;
        memset(Com_TxModeSignalChanges[i].LastValue, 0, sizeof(Com_TxModeSignalChanges[i].LastValue));
    }
}

/**
 * @brief Notify transmission mode manager of signal change
 */
void Com_TxModeSignalChanged(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
        return;
    }

    if (SignalDataPtr == NULL_PTR) {
        return;
    }

    /* Update signal change tracking */
    (void)Com_TxModeUpdateSignalChange(SignalId, SignalDataPtr);
}

/**
 * @brief Update signal change tracking
 */
boolean Com_TxModeUpdateSignalChange(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
        return FALSE;
    }

    const Com_SignalConfigType* signalConfig = &Com_GlobalState.Config->Signals[SignalId];
    uint8 signalSize = (signalConfig->BitSize + 7u) / 8u;  /* Round up to bytes */

    if (signalSize > 8u) {
        signalSize = 8u;  /* Limit to max tracked size */
    }

    Com_TxModeSignalChangeType* changeTrack = &Com_TxModeSignalChanges[SignalId];

    /* Compare with last value */
    boolean changed = FALSE;
    if ((changeTrack->IsValid) != 0U) {
        if (memcmp(changeTrack->LastValue, SignalDataPtr, signalSize) != 0 ) {
            changed = TRUE;
        }
    } else {
        /* First value - consider as changed */
        changed = TRUE;
    }

    /* Update tracking */
    memcpy(changeTrack->LastValue, SignalDataPtr, signalSize);
    changeTrack->IsValid = TRUE;

    if ((changed) != 0U) {
        changeTrack->HasChanged = TRUE;
    }

    return changed;
}

/**
 * @brief Check if signal has changed since last check
 */
boolean Com_TxModeHasSignalChanged(Com_SignalIdType SignalId)
{
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
        return FALSE;
    }

    return Com_TxModeSignalChanges[SignalId].HasChanged;
}

/**
 * @brief Clear signal change flag
 */
void Com_TxModeClearSignalChange(Com_SignalIdType SignalId)
{
    if (SignalId < Com_GlobalState.Config->NumSignals) {
        Com_TxModeSignalChanges[SignalId].HasChanged = FALSE;
    }
}

/*==================[TMC Evaluation Implementation]=========================*/

/**
 * @brief Evaluate Transmission Mode Condition (TMC) for an I-PDU
 */
Com_TmcResultType Com_TxModeEvaluateTmc(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return COM_TMC_NONE;
    }

    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];

    /* Check if TMC is configured */
    if (!ipduConfig->TxMode.UseTmc || !ipduConfig->TxMode.TmcConfig.IsConfigured) {
        return COM_TMC_NONE;
    }

    const Com_TmcConfigType* tmcConfig = &ipduConfig->TxMode.TmcConfig;

    /* Check if signal ID is valid */
    if (tmcConfig->SignalId >= Com_GlobalState.Config->NumSignals) {
        return COM_TMC_NONE;
    }

    /* Evaluate condition */
    boolean conditionMet = Com_TxModeCheckSignalAgainstThreshold(
        tmcConfig->SignalId,
        tmcConfig->ThresholdValue,
        tmcConfig->UseGreaterThan
    );

    return conditionMet ? COM_TMC_TRUE : COM_TMC_FALSE;
}

/**
 * @brief Check signal value against threshold
 */
static boolean Com_TxModeCheckSignalAgainstThreshold(
    Com_SignalIdType SignalId,
    uint32 Threshold,
    boolean UseGreaterThan)
{
    const Com_SignalConfigType* signalConfig = &Com_GlobalState.Config->Signals[SignalId];

    /* Extract current signal value from IPDU buffer */
    uint64 currentValue = Com_ExtractSignal(
        signalConfig->DataPtr,
        signalConfig->BitPosition,
        signalConfig->BitSize,
        signalConfig->Endianness
    );

    /* Compare based on operator */
    if ((UseGreaterThan) != 0U) {
        return (uint32)currentValue > Threshold;
    } else {
        return (uint32)currentValue < Threshold;
    }
}

/**
 * @brief Switch transmission mode for an I-PDU
 */
void Com_TxModeSwitch(Com_IPduIdType PduId, boolean NewModeIsTrue)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }

    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Get new mode configuration */
    Com_TxModeType* newMode;
    if ((NewModeIsTrue) != 0U) {
        newMode = (Com_TxModeType*)&ipduConfig->TxMode.TxModeTrue;
    } else {
        newMode = (Com_TxModeType*)&ipduConfig->TxMode.TxModeFalse;
    }

    /* Check if actually switching */
    if (modeState->CurrentTxMode == newMode) {
        return;  /* Already in target mode */
    }

    /* Perform mode switch */
    modeState->CurrentTxMode = newMode;
    modeState->ModeSwitched = TRUE;
    modeState->State = COM_TXMODESTATE_IDLE;
    modeState->RepetitionCounter = 0u;

    /* Initialize timer for new mode */
    if ((newMode->Mode == COM_TXMODE_PERIODIC) || (newMode->Mode == COM_TXMODE_MIXED)) {
        modeState->CycleTimer = (newMode->TimeOffset > 0u) ? newMode->TimeOffset : newMode->CycleTime;
    } else {
        modeState->CycleTimer = 0u;
    }

    modeState->RepetitionTimer = 0u;
}

/*==================[Periodic Transmission Manager Implementation]==========*/

/**
 * @brief Initialize periodic transmission timers
 */
void Com_TxModeInitPeriodicTimers(void)
{
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];

        if (ipduConfig->Direction == COM_SEND) {
            Com_TxModeStateType* modeState = &Com_TxModeStates[i];

            /* Initialize cycle timer with offset if configured, otherwise use cycle time */
            if (ipduConfig->TxMode.TxModeFalse.TimeOffset > 0u) {
                modeState->CycleTimer = ipduConfig->TxMode.TxModeFalse.TimeOffset;
            } else {
                modeState->CycleTimer = ipduConfig->TxMode.TxModeFalse.CycleTime;
            }

            modeState->RepetitionTimer = 0u;
            modeState->RepetitionCounter = 0u;
        }
    }
}

/**
 * @brief Process periodic transmission for an I-PDU
 */
boolean Com_TxModeProcessPeriodic(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Check if cycle timer has expired */
    if (modeState->CycleTimer == 0u) {
        return TRUE;
    }

    /* Decrement timer */
    Com_TxModeDecrementTimer(&modeState->CycleTimer, COM_MAIN_FUNCTION_PERIOD_MS);

    return FALSE;
}

/**
 * @brief Process repetition timing for an I-PDU
 */
boolean Com_TxModeProcessRepetition(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Check if repetition timer has expired */
    if (modeState->RepetitionTimer == 0u) {
        return TRUE;
    }

    /* Decrement timer */
    Com_TxModeDecrementTimer(&modeState->RepetitionTimer, COM_MAIN_FUNCTION_PERIOD_MS);

    return FALSE;
}

/*==================[Timer Management Implementation]=======================*/

/**
 * @brief Get current system time in milliseconds
 */
uint32 Com_TxModeGetTimeMs(void)
{
    /* Use the same time source as Com_Transmit.c */
    return Com_GetCurrentTimestamp();
}

/**
 * @brief Decrement timer with underflow protection
 */
void Com_TxModeDecrementTimer(uint32* TimerPtr, uint32 Decrement)
{
    if (TimerPtr == NULL_PTR) {
        return;
    }

    if (*TimerPtr > Decrement) {
        *TimerPtr -= Decrement;
    } else {
        *TimerPtr = 0u;
    }
}

/*==================[Transmission Decision API]=============================*/

/**
 * @brief Check if I-PDU should be transmitted based on current mode
 */
boolean Com_TxModeShouldTransmit(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Check if in a state that requires transmission */
    if (modeState->State == COM_TXMODESTATE_TRIGGERED) {
        return TRUE;
    }

    /* Check current mode */
    if (modeState->CurrentTxMode == NULL_PTR) {
        return FALSE;
    }

    switch (modeState->CurrentTxMode->Mode) {
        case COM_TXMODE_DIRECT:
            /* Transmit if triggered or repeating */
            return (modeState->State == COM_TXMODESTATE_REPEATING);

        case COM_TXMODE_PERIODIC:
        case COM_TXMODE_MIXED:
            /* Check periodic timer */
            return (modeState->CycleTimer == 0u);

        case COM_TXMODE_NONE:
        default:
            return FALSE;
    }
}

/**
 * @brief Handle transmission confirmation for mode state machine
 */
void Com_TxModeHandleConfirmation(Com_IPduIdType PduId, Std_ReturnType Result)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    /* Clear mode switched flag after first successful transmission in new mode */
    if (modeState->ModeSwitched && (Result == E_OK)) {
        modeState->ModeSwitched = FALSE;
    }
}

/**
 * @brief Trigger direct transmission for an I-PDU
 */
void Com_TxModeTriggerDirect(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];
    Com_TxModeType* txMode = modeState->CurrentTxMode;

    /* Can only trigger in modes that support direct transmission */
    if (txMode == NULL_PTR) {
        return;
    }

    if ((txMode->Mode == COM_TXMODE_DIRECT) || (txMode->Mode == COM_TXMODE_MIXED)) {
        modeState->State = COM_TXMODESTATE_TRIGGERED;
    }
}

/**
 * @brief Get current transmission mode for an I-PDU
 */
Com_TxModeModeType Com_TxModeGetCurrentMode(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return COM_TXMODE_NONE;
    }

    Com_TxModeStateType* modeState = &Com_TxModeStates[PduId];

    if (modeState->CurrentTxMode == NULL_PTR) {
        return COM_TXMODE_NONE;
    }

    return modeState->CurrentTxMode->Mode;
}

/*==================[End of File]==========================================*/
