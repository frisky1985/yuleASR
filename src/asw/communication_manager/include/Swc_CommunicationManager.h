/**
 * @file Swc_CommunicationManager.h
 * @brief Communication Manager Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Communication management and signal routing at application layer
 */

#ifndef SWC_COMMUNICATIONMANAGER_H
#define SWC_COMMUNICATIONMANAGER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Communication bus type
 */
typedef enum {
    COMM_BUS_CAN = 0,
    COMM_BUS_LIN,
    COMM_BUS_FLEXRAY,
    COMM_BUS_ETHERNET,
    COMM_BUS_INTERNAL
} Swc_CommBusType;

/**
 * @brief Communication state
 */
typedef enum {
    COMM_STATE_OFF = 0,
    COMM_STATE_INIT,
    COMM_STATE_READY,
    COMM_STATE_ACTIVE,
    COMM_STATE_FAULT
} Swc_CommStateType;

/**
 * @brief Signal direction
 */
typedef enum {
    SIGNAL_DIRECTION_TX = 0,
    SIGNAL_DIRECTION_RX
} Swc_SignalDirectionType;

/**
 * @brief Signal configuration
 */
typedef struct {
    uint16 signalId;
    Swc_CommBusType busType;
    Swc_SignalDirectionType direction;
    uint8 length;                 /* Bits */
    uint16 cycleTime;             /* ms */
    uint16 timeout;               /* ms */
    boolean isEventTriggered;
} Swc_SignalConfigType;

/**
 * @brief Signal value
 */
typedef struct {
    uint16 signalId;
    uint64 value;
    uint32 timestamp;
    boolean isValid;
    boolean isUpdated;
} Swc_SignalValueType;

/**
 * @brief PDU information
 */
typedef struct {
    uint16 pduId;
    Swc_CommBusType busType;
    uint8 length;
    uint8 data[64];
    uint32 timestamp;
} Swc_PduInfoType;

/**
 * @brief Communication statistics
 */
typedef struct {
    uint32 txSignals;
    uint32 rxSignals;
    uint32 txPdus;
    uint32 rxPdus;
    uint32 txErrors;
    uint32 rxErrors;
    uint32 timeouts;
} Swc_CommStatisticsType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_CommunicationManager_Init       0x31
#define RTE_RUNNABLE_CommunicationManager_10ms       0x32
#define RTE_RUNNABLE_CommunicationManager_RxProcess  0x33
#define RTE_RUNNABLE_CommunicationManager_TxProcess  0x34

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_COMMUNICATIONMANAGER_PORT_COMM_STATE_P   0x31
#define SWC_COMMUNICATIONMANAGER_PORT_SIGNAL_DATA_P  0x32
#define SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_R     0x33
#define SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_P     0x34

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Communication Manager component
 */
extern void Swc_CommunicationManager_Init(void);

/**
 * @brief 10ms cyclic runnable - communication management
 */
extern void Swc_CommunicationManager_10ms(void);

/**
 * @brief RX process runnable
 */
extern void Swc_CommunicationManager_RxProcess(void);

/**
 * @brief TX process runnable
 */
extern void Swc_CommunicationManager_TxProcess(void);

/**
 * @brief Gets communication state
 * @param state Pointer to store state
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_GetState(Swc_CommStateType* state);

/**
 * @brief Sets communication state
 * @param state Target state
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_SetState(Swc_CommStateType state);

/**
 * @brief Sends signal
 * @param signalId Signal ID
 * @param value Signal value
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_SendSignal(uint16 signalId, uint64 value);

/**
 * @brief Receives signal
 * @param signalId Signal ID
 * @param value Pointer to store value
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_ReceiveSignal(uint16 signalId, uint64* value);

/**
 * @brief Sends PDU
 * @param pdu PDU information
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_SendPdu(const Swc_PduInfoType* pdu);

/**
 * @brief Receives PDU
 * @param pduId PDU ID
 * @param pdu Pointer to store PDU
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_ReceivePdu(uint16 pduId, Swc_PduInfoType* pdu);

/**
 * @brief Gets communication statistics
 * @param stats Pointer to store statistics
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_GetStatistics(Swc_CommStatisticsType* stats);

/**
 * @brief Resets communication statistics
 * @return RTE status
 */
extern Rte_StatusType Swc_CommunicationManager_ResetStatistics(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_CommState(data) \
    Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_COMM_STATE_P(data)

#define Rte_Write_SignalData(data) \
    Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_SIGNAL_DATA_P(data)

#define Rte_Read_PduData(data) \
    Rte_Read_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_R(data)

#define Rte_Write_PduData(data) \
    Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_P(data)

#endif /* SWC_COMMUNICATIONMANAGER_H */
