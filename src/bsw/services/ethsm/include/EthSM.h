/**
 * @file EthSM.h
 * @brief Ethernet State Manager - AUTOSAR Services Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_EthernetStateManager.pdf
 */

#ifndef ETHSM_H
#define ETHSM_H

#include "Std_Types.h"
#include "EthSM_Cfg.h"

#define ETHSM_MODULE_ID             0x8AU
#define ETHSM_VENDOR_ID             0x0055U

typedef enum {
    ETHSM_STATE_OFF    = 0,
    ETHSM_STATE_ON     = 1,
    ETHSM_STATE_SLEEP  = 2
} EthSM_StateType;

typedef struct {
    uint8 ChannelId;
    uint32 StartupTimeout;
    uint32 ShutdownTimeout;
    uint8 ControllerId;
} EthSM_ControllerConfigType;

/* Alias for Lcfg compatibility */
typedef EthSM_ControllerConfigType EthSM_ChannelConfigType;

typedef struct {
    uint8 NumChannels;
    uint8 NumControllers;
    const EthSM_ControllerConfigType* Channels;
    const EthSM_ControllerConfigType* Controllers;
} EthSM_ConfigType;

void EthSM_Init(const EthSM_ConfigType* ConfigPtr);
void EthSM_DeInit(void);
Std_ReturnType EthSM_Start(void);
Std_ReturnType EthSM_Stop(void);
Std_ReturnType EthSM_SetState(EthSM_StateType State);
EthSM_StateType EthSM_GetState(void);
void EthSM_MainFunction(void);
void EthSM_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* ETHSM_H */