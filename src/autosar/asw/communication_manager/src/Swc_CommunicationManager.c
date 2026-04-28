/**
 * @file Swc_CommunicationManager.c
 * @brief Communication Manager Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Communication management and signal routing at application layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_CommunicationManager.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_COMMUNICATIONMANAGER_MODULE_ID  0x83
#define SWC_COMMUNICATIONMANAGER_INSTANCE_ID 0x00

/* Maximum signals and PDUs */
#define COMM_MAX_SIGNALS                    100
#define COMM_MAX_PDUS                       50

/* Signal timeout detection */
#define COMM_SIGNAL_TIMEOUT_DEFAULT         100

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_CommStateType state;
    Swc_SignalValueType signals[COMM_MAX_SIGNALS];
    Swc_PduInfoType rxPdus[COMM_MAX_PDUS];
    Swc_PduInfoType txPdus[COMM_MAX_PDUS];
    Swc_CommStatisticsType statistics;
    uint8 numSignals;
    uint8 numRxPdus;
    uint8 numTxPdus;
    boolean isInitialized;
} Swc_CommunicationManagerInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_CommunicationManagerInternalType swcCommManager = {
    .state = COMM_STATE_OFF,
    .signals = {{0}},
    .rxPdus = {{0}},
    .txPdus = {{0}},
    .statistics = {0},
    .numSignals = 0,
    .numRxPdus = 0,
    .numTxPdus = 0,
    .isInitialized = FALSE
};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_CommunicationManager_ProcessRxSignals(void);
STATIC void Swc_CommunicationManager_ProcessTxSignals(void);
STATIC void Swc_CommunicationManager_CheckTimeouts(void);
STATIC sint16 Swc_CommunicationManager_FindSignal(uint16 signalId);
STATIC sint16 Swc_CommunicationManager_FindRxPdu(uint16 pduId);
STATIC sint16 Swc_CommunicationManager_FindTxPdu(uint16 pduId);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Processes received signals from PDUs
 */
STATIC void Swc_CommunicationManager_ProcessRxSignals(void)
{
    uint8 i;
    uint8 j;
    Swc_PduInfoType pdu;

    /* Read PDU from RTE */
    if (Rte_Read_PduData(&pdu) == RTE_E_OK) {
        /* Find or create PDU entry */
        sint16 pduIndex = Swc_CommunicationManager_FindRxPdu(pdu.pduId);

        if (pduIndex < 0) {
            /* Add new PDU */
            if (swcCommManager.numRxPdus < COMM_MAX_PDUS) {
                pduIndex = swcCommManager.numRxPdus;
                swcCommManager.rxPdus[pduIndex] = pdu;
                swcCommManager.numRxPdus++;
            }
        } else {
            /* Update existing PDU */
            swcCommManager.rxPdus[pduIndex] = pdu;
        }

        if (pduIndex >= 0) {
            swcCommManager.statistics.rxPdus++;

            /* Extract signals from PDU data */
            /* Simplified - in real implementation, use signal database */
            for (i = 0; i < swcCommManager.numSignals; i++) {
                if (swcCommManager.signals[i].signalId >= (pdu.pduId * 10) &&
                    swcCommManager.signals[i].signalId < ((pdu.pduId + 1) * 10)) {
                    /* Update signal value from PDU data */
                    swcCommManager.signals[i].value = 0;
                    for (j = 0; j < 8 && j < pdu.length; j++) {
                        swcCommManager.signals[i].value |=
                            ((uint64)pdu.data[j] << (j * 8));
                    }
                    swcCommManager.signals[i].timestamp = pdu.timestamp;
                    swcCommManager.signals[i].isValid = TRUE;
                    swcCommManager.signals[i].isUpdated = TRUE;
                    swcCommManager.statistics.rxSignals++;
                }
            }
        }
    }
}

/**
 * @brief Processes transmit signals to PDUs
 */
STATIC void Swc_CommunicationManager_ProcessTxSignals(void)
{
    uint8 i;
    uint8 j;

    /* Process each TX PDU */
    for (i = 0; i < swcCommManager.numTxPdus; i++) {
        Swc_PduInfoType* pdu = &swcCommManager.txPdus[i];

        /* Pack signals into PDU data */
        /* Simplified - in real implementation, use signal database */
        for (j = 0; j < 8; j++) {
            pdu->data[j] = (uint8)(pdu->pduId + j);
        }
        pdu->length = 8;
        pdu->timestamp = Rte_GetTime();

        /* Send PDU via RTE */
        if (Rte_Write_PduData(pdu) == RTE_E_OK) {
            swcCommManager.statistics.txPdus++;
        } else {
            swcCommManager.statistics.txErrors++;
        }
    }

    /* Count TX signals */
    for (i = 0; i < swcCommManager.numSignals; i++) {
        if (swcCommManager.signals[i].isUpdated) {
            swcCommManager.signals[i].isUpdated = FALSE;
            swcCommManager.statistics.txSignals++;
        }
    }
}

/**
 * @brief Checks signal timeouts
 */
STATIC void Swc_CommunicationManager_CheckTimeouts(void)
{
    uint8 i;
    uint32 currentTime = Rte_GetTime();

    for (i = 0; i < swcCommManager.numSignals; i++) {
        if (swcCommManager.signals[i].isValid) {
            if ((currentTime - swcCommManager.signals[i].timestamp) >
                COMM_SIGNAL_TIMEOUT_DEFAULT) {
                swcCommManager.signals[i].isValid = FALSE;
                swcCommManager.statistics.timeouts++;
            }
        }
    }
}

/**
 * @brief Finds signal by ID
 */
STATIC sint16 Swc_CommunicationManager_FindSignal(uint16 signalId)
{
    uint8 i;

    for (i = 0; i < swcCommManager.numSignals; i++) {
        if (swcCommManager.signals[i].signalId == signalId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds RX PDU by ID
 */
STATIC sint16 Swc_CommunicationManager_FindRxPdu(uint16 pduId)
{
    uint8 i;

    for (i = 0; i < swcCommManager.numRxPdus; i++) {
        if (swcCommManager.rxPdus[i].pduId == pduId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds TX PDU by ID
 */
STATIC sint16 Swc_CommunicationManager_FindTxPdu(uint16 pduId)
{
    uint8 i;

    for (i = 0; i < swcCommManager.numTxPdus; i++) {
        if (swcCommManager.txPdus[i].pduId == pduId) {
            return (sint16)i;
        }
    }

    return -1;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Communication Manager component
 */
void Swc_CommunicationManager_Init(void)
{
    uint8 i;

    if (swcCommManager.isInitialized) {
        return;
    }

    /* Initialize state */
    swcCommManager.state = COMM_STATE_INIT;

    /* Initialize signals */
    for (i = 0; i < COMM_MAX_SIGNALS; i++) {
        swcCommManager.signals[i].signalId = 0;
        swcCommManager.signals[i].value = 0;
        swcCommManager.signals[i].timestamp = 0;
        swcCommManager.signals[i].isValid = FALSE;
        swcCommManager.signals[i].isUpdated = FALSE;
    }
    swcCommManager.numSignals = 0;

    /* Initialize PDUs */
    for (i = 0; i < COMM_MAX_PDUS; i++) {
        swcCommManager.rxPdus[i].pduId = 0;
        swcCommManager.rxPdus[i].length = 0;
        swcCommManager.txPdus[i].pduId = 0;
        swcCommManager.txPdus[i].length = 0;
    }
    swcCommManager.numRxPdus = 0;
    swcCommManager.numTxPdus = 0;

    /* Initialize statistics */
    swcCommManager.statistics.txSignals = 0;
    swcCommManager.statistics.rxSignals = 0;
    swcCommManager.statistics.txPdus = 0;
    swcCommManager.statistics.rxPdus = 0;
    swcCommManager.statistics.txErrors = 0;
    swcCommManager.statistics.rxErrors = 0;
    swcCommManager.statistics.timeouts = 0;

    swcCommManager.isInitialized = TRUE;
    swcCommManager.state = COMM_STATE_READY;

    Det_ReportError(SWC_COMMUNICATIONMANAGER_MODULE_ID, SWC_COMMUNICATIONMANAGER_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 10ms cyclic runnable
 */
void Swc_CommunicationManager_10ms(void)
{
    if (!swcCommManager.isInitialized) {
        return;
    }

    /* Check timeouts */
    Swc_CommunicationManager_CheckTimeouts();

    /* Write state via RTE */
    (void)Rte_Write_CommState(&swcCommManager.state);
}

/**
 * @brief RX process runnable
 */
void Swc_CommunicationManager_RxProcess(void)
{
    if (!swcCommManager.isInitialized) {
        return;
    }

    if (swcCommManager.state != COMM_STATE_ACTIVE) {
        return;
    }

    /* Process received signals */
    Swc_CommunicationManager_ProcessRxSignals();
}

/**
 * @brief TX process runnable
 */
void Swc_CommunicationManager_TxProcess(void)
{
    if (!swcCommManager.isInitialized) {
        return;
    }

    if (swcCommManager.state != COMM_STATE_ACTIVE) {
        return;
    }

    /* Process transmit signals */
    Swc_CommunicationManager_ProcessTxSignals();
}

/**
 * @brief Gets communication state
 */
Rte_StatusType Swc_CommunicationManager_GetState(Swc_CommStateType* state)
{
    if (state == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *state = swcCommManager.state;
    return RTE_E_OK;
}

/**
 * @brief Sets communication state
 */
Rte_StatusType Swc_CommunicationManager_SetState(Swc_CommStateType state)
{
    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (state > COMM_STATE_FAULT) {
        return RTE_E_INVALID;
    }

    swcCommManager.state = state;
    return RTE_E_OK;
}

/**
 * @brief Sends signal
 */
Rte_StatusType Swc_CommunicationManager_SendSignal(uint16 signalId, uint64 value)
{
    sint16 signalIndex;

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (swcCommManager.state != COMM_STATE_ACTIVE) {
        return RTE_E_NOT_ACTIVE;
    }

    /* Find or create signal */
    signalIndex = Swc_CommunicationManager_FindSignal(signalId);

    if (signalIndex < 0) {
        /* Create new signal */
        if (swcCommManager.numSignals >= COMM_MAX_SIGNALS) {
            return RTE_E_LIMIT;
        }
        signalIndex = swcCommManager.numSignals;
        swcCommManager.signals[signalIndex].signalId = signalId;
        swcCommManager.numSignals++;
    }

    /* Update signal value */
    swcCommManager.signals[signalIndex].value = value;
    swcCommManager.signals[signalIndex].timestamp = Rte_GetTime();
    swcCommManager.signals[signalIndex].isValid = TRUE;
    swcCommManager.signals[signalIndex].isUpdated = TRUE;

    return RTE_E_OK;
}

/**
 * @brief Receives signal
 */
Rte_StatusType Swc_CommunicationManager_ReceiveSignal(uint16 signalId, uint64* value)
{
    sint16 signalIndex;

    if (value == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    /* Find signal */
    signalIndex = Swc_CommunicationManager_FindSignal(signalId);

    if (signalIndex < 0) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.signals[signalIndex].isValid) {
        return RTE_E_NOT_OK;
    }

    *value = swcCommManager.signals[signalIndex].value;
    return RTE_E_OK;
}

/**
 * @brief Sends PDU
 */
Rte_StatusType Swc_CommunicationManager_SendPdu(const Swc_PduInfoType* pdu)
{
    sint16 pduIndex;

    if (pdu == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (swcCommManager.state != COMM_STATE_ACTIVE) {
        return RTE_E_NOT_ACTIVE;
    }

    /* Find or create TX PDU */
    pduIndex = Swc_CommunicationManager_FindTxPdu(pdu->pduId);

    if (pduIndex < 0) {
        /* Create new PDU */
        if (swcCommManager.numTxPdus >= COMM_MAX_PDUS) {
            return RTE_E_LIMIT;
        }
        pduIndex = swcCommManager.numTxPdus;
        swcCommManager.numTxPdus++;
    }

    /* Copy PDU data */
    swcCommManager.txPdus[pduIndex] = *pdu;

    /* Send immediately via RTE */
    if (Rte_Write_PduData(pdu) == RTE_E_OK) {
        swcCommManager.statistics.txPdus++;
        return RTE_E_OK;
    } else {
        swcCommManager.statistics.txErrors++;
        return RTE_E_NOT_OK;
    }
}

/**
 * @brief Receives PDU
 */
Rte_StatusType Swc_CommunicationManager_ReceivePdu(uint16 pduId, Swc_PduInfoType* pdu)
{
    sint16 pduIndex;

    if (pdu == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    /* Find PDU */
    pduIndex = Swc_CommunicationManager_FindRxPdu(pduId);

    if (pduIndex < 0) {
        return RTE_E_INVALID;
    }

    /* Copy PDU data */
    *pdu = swcCommManager.rxPdus[pduIndex];
    return RTE_E_OK;
}

/**
 * @brief Gets communication statistics
 */
Rte_StatusType Swc_CommunicationManager_GetStatistics(Swc_CommStatisticsType* stats)
{
    if (stats == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *stats = swcCommManager.statistics;
    return RTE_E_OK;
}

/**
 * @brief Resets communication statistics
 */
Rte_StatusType Swc_CommunicationManager_ResetStatistics(void)
{
    if (!swcCommManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    swcCommManager.statistics.txSignals = 0;
    swcCommManager.statistics.rxSignals = 0;
    swcCommManager.statistics.txPdus = 0;
    swcCommManager.statistics.rxPdus = 0;
    swcCommManager.statistics.txErrors = 0;
    swcCommManager.statistics.rxErrors = 0;
    swcCommManager.statistics.timeouts = 0;

    return RTE_E_OK;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
