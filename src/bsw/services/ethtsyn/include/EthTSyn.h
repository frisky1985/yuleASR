/**
 * @file EthTSyn.h
 * @brief Ethernet Time Synchronization (EthTSyn) — AUTOSAR BSW Module
 *
 * AUTOSAR R21-11 §12.9: EthTSyn provides IEEE 802.1AS / gPTP
 * time synchronization over Ethernet.
 *
 * This is a lightweight stub for COMPLETENESS — not full implementation.
 */
#ifndef ETH_TSYN_H
#define ETH_TSYN_H

#include "Std_Types.h"
#include "Eth.h"

/* Module ID */
#define ETHTSYN_MODULE_ID        0x0AUL

/* EthTSyn Configuration */
typedef struct {
    uint8 domainNumber;
    boolean masterOnly;
    uint16 logSyncInterval;
    uint16 logAnnounceInterval;
    uint16 logPdelayReqInterval;
    uint16 priority1;
    uint16 priority2;
    uint8 clockClass;
    uint8 clockAccuracy;
    uint16 offsetScaledLogVariance;
} EthTSyn_ConfigType;

/* EthTSyn clock identity */
typedef struct {
    uint8 id[8];
} EthTSyn_ClockIdentityType;

/* EthTSyn time stamp */
typedef struct {
    uint64 seconds;
    uint32 nanoseconds;
} EthTSyn_TimestampType;

/* EthTSyn port state */
typedef enum {
    ETHTSYN_PORT_INIT,
    ETHTSYN_PORT_FAULTY,
    ETHTSYN_PORT_DISABLED,
    ETHTSYN_PORT_LISTENING,
    ETHTSYN_PORT_PRE_MASTER,
    ETHTSYN_PORT_MASTER,
    ETHTSYN_PORT_PASSIVE,
    ETHTSYN_PORT_UNCALIBRATED,
    ETHTSYN_PORT_SLAVE,
} EthTSyn_PortStateType;

/* Initialization */
Std_ReturnType EthTSyn_Init(const EthTSyn_ConfigType* config);
void EthTSyn_DeInit(void);

/* Main function */
void EthTSyn_MainFunction(void);

/* Time synchronization */
Std_ReturnType EthTSyn_GetTime(EthTSyn_TimestampType* timestamp);
Std_ReturnType EthTSyn_SetTime(const EthTSyn_TimestampType* timestamp);
Std_ReturnType EthTSyn_AdjustRate(int32 rateNumerator, int32 rateDenominator);

/* Port management */
Std_ReturnType EthTSyn_GetPortState(uint8 portIndex, EthTSyn_PortStateType* state);
Std_ReturnType EthTSyn_GetClockIdentity(EthTSyn_ClockIdentityType* identity);

/* Version info */
#if (ETHTSYN_VERSION_INFO_API == STD_ON)
void EthTSyn_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#endif /* ETH_TSYN_H */
