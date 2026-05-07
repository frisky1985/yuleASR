/******************************************************************************
 * @file    Com_TxMode.h
 * @brief   COM Module - Transmission Mode Manager Header
 *
 * This file defines the transmission mode manager for the AUTOSAR COM module.
 * It provides:
 * - Four transmission modes: DIRECT, MIXED, PERIODIC, NONE
 * - ComTxModeTrue/ComTxModeFalse configuration support
 * - Signal-based Transmission Mode Condition (TMC) evaluation
 * - Periodic transmission scheduling with FreeRTOS software timers
 * - Transmission mode switching logic
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x1E (COM)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef COM_TXMODE_H
#define COM_TXMODE_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "Com_Private.h"
#include "Com_Transmit.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define COM_TXMODE_SW_MAJOR_VERSION       1u
#define COM_TXMODE_SW_MINOR_VERSION       0u
#define COM_TXMODE_SW_PATCH_VERSION       0u

/******************************************************************************
 * Transmission Mode Type Definitions
 ******************************************************************************/

/**
 * @brief Transmission mode mode enumeration (AUTOSAR ComTxModeMode)
 *
 * Defines the four transmission modes supported by AUTOSAR COM:
 * - DIRECT: Event-triggered transmission only
 * - PERIODIC: Cyclic transmission only
 * - MIXED: Combination of cyclic and event-triggered
 * - NONE: No transmission allowed
 */
typedef enum {
    COM_TXMODE_DIRECT = 0,      /*!< Event-triggered transmission */
    COM_TXMODE_PERIODIC,        /*!< Cyclic transmission */
    COM_TXMODE_MIXED,           /*!< Cyclic + event-triggered */
    COM_TXMODE_NONE             /*!< No transmission */
} Com_TxModeModeType;

/**
 * @brief Transmission mode state enumeration
 *
 * Internal state tracking for transmission mode processing
 */
typedef enum {
    COM_TXMODESTATE_IDLE = 0,           /*!< No transmission active */
    COM_TXMODESTATE_WAITING,            /*!< Waiting for next cycle */
    COM_TXMODESTATE_TRIGGERED,          /*!< Event triggered, pending transmission */
    COM_TXMODESTATE_REPEATING,          /*!< In repetition phase */
    COM_TXMODESTATE_PERIODIC            /*!< Periodic transmission active */
} Com_TxModeStateType;

/**
 * @brief Signal change detection type
 *
 * Tracks signal value changes for TMC evaluation
 */
typedef struct {
    uint8 LastValue[8];         /*!< Last signal value for change detection */
    uint16 SignalId;            /*!< Associated signal ID */
    boolean HasChanged;         /*!< TRUE if signal changed since last check */
    boolean IsValid;            /*!< TRUE if LastValue contains valid data */
} Com_TxModeSignalChangeType;

/**
 * @brief Transmission Mode Condition (TMC) evaluation result
 */
typedef enum {
    COM_TMC_FALSE = 0,          /*!< TMC condition evaluates to FALSE */
    COM_TMC_TRUE,               /*!< TMC condition evaluates to TRUE */
    COM_TMC_NONE                /*!< No TMC configured */
} Com_TmcResultType;

/******************************************************************************
 * Transmission Mode Configuration Types
 ******************************************************************************/

/**
 * @brief Complete Transmission Mode Configuration (ComTxMode)
 *
 * This structure contains all timing parameters for a transmission mode.
 * Used for both ComTxModeTrue and ComTxModeFalse configurations.
 */
typedef struct {
    Com_TxModeModeType Mode;            /*!< Transmission mode (DIRECT/PERIODIC/MIXED/NONE) */
    uint32 CycleTime;                   /*!< Period between transmissions in ms (for PERIODIC/MIXED) */
    uint32 RepetitionPeriod;            /*!< Time between repetitions in ms */
    uint8 NumRepetitions;               /*!< Number of repetitions for direct transmission */
    uint32 TimeOffset;                  /*!< Initial time offset before first transmission */
    boolean RepeatingEnabled;           /*!< TRUE if repetitions are enabled */
} Com_TxModeType;

/**
 * @brief Transmission Mode Condition (TMC) Configuration
 *
 * Defines which signal and condition controls the transmission mode switch.
 * When the condition changes, Com automatically switches between
 * ComTxModeTrue and ComTxModeFalse.
 */
typedef struct {
    Com_SignalIdType SignalId;          /*!< Signal ID for TMC evaluation */
    uint32 ThresholdValue;              /*!< Threshold for comparison */
    boolean UseGreaterThan;             /*!< TRUE: signal > threshold, FALSE: signal < threshold */
    boolean IsConfigured;               /*!< TRUE if TMC is configured for this I-PDU */
} Com_TmcConfigType;

/**
 * @brief Complete I-PDU Transmission Mode Configuration
 *
 * Contains both True and False transmission mode configurations
 * along with TMC evaluation settings.
 */
typedef struct {
    Com_TxModeType TxModeTrue;          /*!< Configuration when TMC is TRUE */
    Com_TxModeType TxModeFalse;         /*!< Configuration when TMC is FALSE */
    Com_TmcConfigType TmcConfig;        /*!< TMC evaluation configuration */
    boolean UseTmc;                     /*!< TRUE if TMC-based switching is enabled */
} Com_IPduTxModeConfigType;

/******************************************************************************
 * Runtime State Types
 ******************************************************************************/

/**
 * @brief Transmission mode runtime state per I-PDU
 *
 * Tracks the current state of transmission mode processing for each I-PDU.
 */
typedef struct {
    Com_TxModeStateType State;          /*!< Current transmission mode state */
    Com_TxModeType* CurrentTxMode;      /*!< Pointer to active TxMode (True or False) */
    uint32 CycleTimer;                  /*!< Timer for periodic transmission */
    uint32 RepetitionTimer;             /*!< Timer for repetition handling */
    uint8 RepetitionCounter;            /*!< Current repetition count */
    boolean ModeSwitched;               /*!< TRUE if mode just switched */
    uint32 TimeInCurrentMode;           /*!< Time spent in current mode */
} Com_TxModeStateType;

/******************************************************************************
 * Global Variables
 ******************************************************************************/

/** Transmission mode runtime states for all I-PDUs */
extern Com_TxModeStateType Com_TxModeStates[COM_MAX_IPDUS];

/** Signal change tracking for TMC evaluation */
extern Com_TxModeSignalChangeType Com_TxModeSignalChanges[COM_MAX_SIGNALS];

/******************************************************************************
 * Transmission Mode Manager API
 ******************************************************************************/

/**
 * @brief Initialize the transmission mode manager
 *
 * Initializes all transmission mode states and signal change tracking.
 * Called during Com_Init().
 */
void Com_TxModeInit(void);

/**
 * @brief Process transmission modes for all send I-PDUs
 *
 * Main processing function called from Com_MainFunctionTx().
 * Handles:
 * - Periodic transmission timing
 * - Repetition timing
 * - Mode switching evaluation
 *
 * @param PduId I-PDU identifier to process
 */
void Com_TxModeProcessIPdu(Com_IPduIdType PduId);

/**
 * @brief Notify transmission mode manager of signal change
 *
 * Called when a signal is updated to track changes for TMC evaluation.
 *
 * @param SignalId Signal identifier that changed
 * @param SignalDataPtr Pointer to new signal data
 */
void Com_TxModeSignalChanged(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Evaluate Transmission Mode Condition (TMC) for an I-PDU
 *
 * Evaluates the configured TMC condition for the given I-PDU.
 *
 * @param PduId I-PDU identifier
 * @return Com_TmcResultType TMC evaluation result (TRUE/FALSE/NONE)
 */
Com_TmcResultType Com_TxModeEvaluateTmc(Com_IPduIdType PduId);

/**
 * @brief Switch transmission mode for an I-PDU
 *
 * Switches between TxModeTrue and TxModeFalse based on TMC evaluation.
 *
 * @param PduId I-PDU identifier
 * @param NewModeIsTrue TRUE to use TxModeTrue, FALSE to use TxModeFalse
 */
void Com_TxModeSwitch(Com_IPduIdType PduId, boolean NewModeIsTrue);

/**
 * @brief Check if I-PDU should be transmitted based on current mode
 *
 * Determines if the I-PDU should be transmitted now based on:
 * - Current transmission mode
 * - Timers
 * - Trigger flags
 *
 * @param PduId I-PDU identifier
 * @return boolean TRUE if transmission should occur
 */
boolean Com_TxModeShouldTransmit(Com_IPduIdType PduId);

/**
 * @brief Handle transmission confirmation for mode state machine
 *
 * Updates transmission mode state after successful/failed transmission.
 *
 * @param PduId I-PDU identifier
 * @param Result Transmission result (E_OK or E_NOT_OK)
 */
void Com_TxModeHandleConfirmation(Com_IPduIdType PduId, Std_ReturnType Result);

/**
 * @brief Trigger direct transmission for an I-PDU
 *
 * Initiates direct/event-triggered transmission including repetitions.
 *
 * @param PduId I-PDU identifier
 */
void Com_TxModeTriggerDirect(Com_IPduIdType PduId);

/**
 * @brief Get current transmission mode for an I-PDU
 *
 * @param PduId I-PDU identifier
 * @return Com_TxModeModeType Current transmission mode
 */
Com_TxModeModeType Com_TxModeGetCurrentMode(Com_IPduIdType PduId);

/******************************************************************************
 * Signal Change Detection API
 ******************************************************************************/

/**
 * @brief Initialize signal change tracking
 *
 * Initializes all signal change tracking structures.
 */
void Com_TxModeInitSignalChanges(void);

/**
 * @brief Update signal change tracking
 *
 * Compares new signal value with last value and updates change flag.
 *
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to new signal data
 * @return boolean TRUE if signal value changed
 */
boolean Com_TxModeUpdateSignalChange(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Check if signal has changed since last check
 *
 * @param SignalId Signal identifier
 * @return boolean TRUE if signal changed
 */
boolean Com_TxModeHasSignalChanged(Com_SignalIdType SignalId);

/**
 * @brief Clear signal change flag
 *
 * @param SignalId Signal identifier
 */
void Com_TxModeClearSignalChange(Com_SignalIdType SignalId);

/******************************************************************************
 * Periodic Transmission Manager API
 ******************************************************************************/

/**
 * @brief Initialize periodic transmission timers
 *
 * Sets up initial timer values for all send I-PDUs.
 */
void Com_TxModeInitPeriodicTimers(void);

/**
 * @brief Process periodic transmission for an I-PDU
 *
 * Handles periodic transmission timing.
 *
 * @param PduId I-PDU identifier
 * @return boolean TRUE if periodic transmission should occur
 */
boolean Com_TxModeProcessPeriodic(Com_IPduIdType PduId);

/**
 * @brief Process repetition timing for an I-PDU
 *
 * Handles direct transmission repetition timing.
 *
 * @param PduId I-PDU identifier
 * @return boolean TRUE if repetition transmission should occur
 */
boolean Com_TxModeProcessRepetition(Com_IPduIdType PduId);

/******************************************************************************
 * Timer Management API
 ******************************************************************************/

/**
 * @brief Get current system time in milliseconds
 *
 * Platform-specific time source for transmission mode timing.
 *
 * @return uint32 Current time in milliseconds
 */
uint32 Com_TxModeGetTimeMs(void);

/**
 * @brief Decrement timer with underflow protection
 *
 * @param TimerPtr Pointer to timer variable
 * @param Decrement Amount to decrement
 */
void Com_TxModeDecrementTimer(uint32* TimerPtr, uint32 Decrement);

#ifdef __cplusplus
}
#endif

#endif /* COM_TXMODE_H */
