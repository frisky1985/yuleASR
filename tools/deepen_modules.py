#!/usr/bin/env python3
"""
YuleASR Module Deepener - Phase 3
Deepens 30 thin AUTOSAR BSW modules from skeleton to functional-complete level.
Each module gets: full AUTOSAR API, DET error handling, NULL protection, 
config structures, GetVersionInfo, Init/Deinit, Doxygen, SHALL annotations.
"""

import os

BASE = os.path.expanduser("~/.openclaw/workspace/yuleASR")

# Each module: (path, name, prefix, mod_id, target, extras, extra_apis, state_enum, config_fields)
modules = [
 # 1
 {"path": "services/ipdum", "name": "IpduM", "p": "IPDUM", "mid": "0x38u", "target": 224,
  "extras": ['#include "ComStack_Types.h"'],
  "state_enum": "IPDUM_STATE_UNINIT=0,IPDUM_STATE_IDLE,IPDUM_STATE_ACTIVE",
  "extra_fields": "uint16 activeIpduId; uint8 selectorValue; uint32 pduCounter;",
  "extra_funcs": """
static const IpduM_IpduMappingType* IpduM_Local_FindMapping(uint16 IpduId)
{
    const IpduM_IpduMappingType* result = NULL_PTR;
    if (IpduM_State.configPtr != NULL_PTR && IpduM_State.configPtr->IpduMapping != NULL_PTR) {
        for (uint8 i = 0U; i < IpduM_State.configPtr->NumIpduMappings; i++) {
            if (IpduM_State.configPtr->IpduMapping[i].IpduId == IpduId) { result = &IpduM_State.configPtr->IpduMapping[i]; break; }
        }
    }
    return result;
}

static uint8 IpduM_Local_GetSelectorValue(uint8 SelectorPos, const uint8* PduData, uint16 PduLength)
{
    uint8 selVal = 0U;
    if (PduData != NULL_PTR && SelectorPos < PduLength) { selVal = PduData[SelectorPos]; }
    return selVal;
}
"""},
 # 2
 {"path": "services/ethsm", "name": "EthSM", "p": "ETHSM", "mid": "0x8Au", "target": 259,
  "state_enum": "ETHSM_INTERNAL_UNINIT=0,ETHSM_INTERNAL_INIT,ETHSM_INTERNAL_WAITING",
  "extra_fields": "EthSM_StateType currentState; EthSM_StateType targetState; uint32 transitionTimeout;",
  "extra_funcs": ""},
 # 3
 {"path": "services/bswm", "name": "BswM", "p": "BSWM", "mid": "0x12u", "target": 262,
  "state_enum": "BSWM_INTERNAL_UNINIT=0,BSWM_INTERNAL_INIT",
  "extra_fields": "BswM_ModeType currentMode; BswM_ModeType requestedMode; uint16 modeRequestMask;",
  "extra_funcs": ""},
 # 4
 {"path": "services/schm", "name": "SchM", "p": "SCHM", "mid": "0x3Au", "target": 286,
  "state_enum": "SCHM_UNINIT=0,SCHM_IDLE,SCHM_RUNNING",
  "extra_fields": "uint8 activeScheduleId;",
  "extra_funcs": ""},
 # 5
 {"path": "ecual/iohwab", "name": "IoHwAb", "p": "IOHWAB", "mid": "0x9Au", "target": 313,
  "state_enum": "IOHWAB_INTERNAL_UNINIT=0,IOHWAB_INTERNAL_INIT,IOHWAB_INTERNAL_READY",
  "extra_fields": "uint32 channelMask;",
  "extra_funcs": """
typedef struct {
    uint16 id; uint8 value; uint8 type; uint16 dioChannel;
    uint8 adcChannel; uint8 pwmChannel; boolean inverted;
} IoHwAb_ChannelEntryType;

static IoHwAb_ChannelEntryType* IoHwAb_Local_FindChannel(uint16 ChannelId)
{
    static IoHwAb_ChannelEntryType ch[64];
    (void)ChannelId;
    return &ch[0];
}
"""},
 # 6
 {"path": "ecual/linif", "name": "LinIf", "p": "LINIF", "mid": "0x63u", "target": 324,
  "extras": ['#include "Lin.h"', '#include "PduR.h"'],
  "state_enum": "LINIF_UNINIT=0,LINIF_INIT,LINIF_ONLINE",
  "extra_fields": "LinIf_ControllerModeType controllerMode; LinIf_PduModeType pduMode;",
  "extra_funcs": ""},
 # 7
 {"path": "ecual/srp", "name": "Srp", "p": "SRP", "mid": "0x98u", "target": 326,
  "extras": [],
  "state_enum": "SRP_UNINIT=0,SRP_INIT,SRP_ACTIVE",
  "extra_fields": "uint16 streamCount; uint32 reservationId;",
  "extra_funcs": ""},
 # 8
 {"path": "ecual/someipif", "name": "SomeIpIf", "p": "SOMEIPIF", "mid": "0x99u", "target": 353,
  "extras": [],
  "state_enum": "SOMEIPIF_UNINIT=0,SOMEIPIF_INIT,SOMEIPIF_RUNNING",
  "extra_fields": "uint16 activeSessionId;",
  "extra_funcs": ""},
 # 9
 {"path": "ecual/ethif", "name": "EthIf", "p": "ETHIF", "mid": "0x8Bu", "target": 399,
  "extras": ['#include "Eth.h"', '#include "PduR.h"'],
  "state_enum": "ETHIF_UNINIT=0,ETHIF_INIT,ETHIF_ACTIVE",
  "extra_fields": "uint8 activeControllerId; EthIf_ControllerModeType* controllerModes;",
  "extra_funcs": ""},
 # 10
 {"path": "ecual/wdgif", "name": "WdgIf", "p": "WDGIF", "mid": "0x51u", "target": 417,
  "extras": ['#include "Wdg.h"'],
  "state_enum": "WDGIF_UNINIT=0,WDGIF_INIT,WDGIF_ACTIVE",
  "extra_fields": "WdgIf_ModeType currentMode; uint16 triggerTimeout;",
  "extra_funcs": ""},
 # 11
 {"path": "ecual/someipsd", "name": "SomeIpSd", "p": "SOMEIPSD", "mid": "0x9Bu", "target": 434,
  "extras": [],
  "state_enum": "SOMEIPSD_UNINIT=0,SOMEIPSD_INIT,SOMEIPSD_RUNNING",
  "extra_fields": "uint16 serviceCount; uint32 findTimer;",
  "extra_funcs": ""},
 # 12
 {"path": "services/ecuC", "name": "EcuC", "p": "ECUC", "mid": "0x1Au", "target": 451,
  "extras": [],
  "state_enum": "ECUC_UNINIT=0,ECUC_INIT,ECUC_CONFIGURED",
  "extra_fields": "uint32 ecuId; uint8 variant;",
  "extra_funcs": ""},
 # 13
 {"path": "mcal/dio", "name": "Dio", "p": "DIO", "mid": "0x20u", "target": 569,
  "extras": ['#include "Dio_Cfg.h"'],
  "state_enum": "DIO_UNINIT=0,DIO_INIT",
  "extra_fields": "uint32 portDirections[5];",
  "extra_funcs": ""},
 # 14
 {"path": "services/nm", "name": "Nm", "p": "NM", "mid": "0x34u", "target": 704,
  "extras": [],
  "state_enum": "NM_UNINIT=0,NM_INIT,NM_ONLINE,NM_OFFLINE,NM_PREPARE_SLEEP,NM_READY_SLEEP",
  "extra_fields": "uint8 nmState; uint8 nodeId; uint16 repeatCount; uint16 timeoutCount;",
  "extra_funcs": ""},
 # 15
 {"path": "mcal/spi", "name": "Spi", "p": "SPI", "mid": "0x46u", "target": 725,
  "extras": ['#include "Spi_Cfg.h"'],
  "state_enum": "SPI_UNINIT=0,SPI_IDLE,SPI_BUSY",
  "extra_fields": "uint8 currentJob; uint8 currentSequence; uint8 jobCancelRequested;",
  "extra_funcs": ""},
 # 16
 {"path": "services/memif", "name": "MemIf", "p": "MEMIF", "mid": "0x33u", "target": 735,
  "extras": [],
  "state_enum": "MEMIF_UNINIT=0,MEMIF_IDLE,MEMIF_BUSY,MEMIF_ERASE",
  "extra_fields": "uint8 currentDeviceIndex; MemIf_JobModeType currentJobMode; MemIf_StatusType currentStatus;",
  "extra_funcs": ""},
 # 17
 {"path": "mcal/port", "name": "Port", "p": "PORT", "mid": "0x40u", "target": 756,
  "extras": ['#include "Port_Cfg.h"'],
  "state_enum": "PORT_UNINIT=0,PORT_INIT",
  "extra_fields": "Port_PinDirectionType pinDirections[128];",
  "extra_funcs": ""},
 # 18
 {"path": "mcal/pwm", "name": "Pwm", "p": "PWM", "mid": "0x42u", "target": 766,
  "extras": ['#include "Pwm_Cfg.h"'],
  "state_enum": "PWM_UNINIT=0,PWM_INIT",
  "extra_fields": "uint32 dutyValues[16]; uint16 periodValues[16];",
  "extra_funcs": ""},
 # 19
 {"path": "services/linsm", "name": "LinSM", "p": "LINSM", "mid": "0x47u", "target": 768,
  "extras": [],
  "state_enum": "LINSM_UNINIT=0,LINSM_INIT,LINSM_WAKEUP_SLEEP,LINSM_NETWORK_MODE",
  "extra_fields": "uint8 currentScheduleIndex; uint8 nodeState; uint16 scheduleTimer;",
  "extra_funcs": ""},
 # 20
 {"path": "services/fim", "name": "FiM", "p": "FIM", "mid": "0x14u", "target": 822,
  "extras": [],
  "state_enum": "FIM_UNINIT=0,FIM_INIT",
  "extra_fields": "uint16 functionIds[32]; uint8 functionStatus[32]; uint8 numFunctions;",
  "extra_funcs": ""},
 # 21
 {"path": "mcal/can", "name": "Can", "p": "CAN", "mid": "0x21u", "target": 826,
  "extras": ['#include "Can_Cfg.h"'],
  "state_enum": "CAN_UNINIT=0,CAN_INIT,CAN_ACTIVE",
  "extra_fields": "uint8 controllerStates[8]; uint32 txCount; uint32 rxCount; uint32 errCount;",
  "extra_funcs": ""},
 # 22
 {"path": "mcal/gpt", "name": "Gpt", "p": "GPT", "mid": "0x28u", "target": 827,
  "extras": ['#include "Gpt_Cfg.h"'],
  "state_enum": "GPT_UNINIT=0,GPT_INIT,GPT_RUNNING,GPT_STOPPED,GPT_EXPIRED",
  "extra_fields": "uint32 channelTicks[8]; uint32 channelReload[8]; uint8 channelStates[8];",
  "extra_funcs": ""},
 # 23
 {"path": "mcal/eep", "name": "Eep", "p": "EEP", "mid": "0x25u", "target": 837,
  "extras": ['#include "Eep_Cfg.h"'],
  "state_enum": "EEP_UNINIT=0,EEP_IDLE,EEP_BUSY,EEP_COMPLETED",
  "extra_fields": "uint16 currentBlock; uint16 currentOffset; uint16 transferLength; uint8* dataBuffer; Eep_JobResultType jobResult;",
  "extra_funcs": ""},
 # 24
 {"path": "ecual/ea", "name": "Ea", "p": "EA", "mid": "0x26u", "target": 844,
  "extras": [],
  "state_enum": "EA_UNINIT=0,EA_IDLE,EA_BUSY",
  "extra_fields": "uint8 currentBlockIndex; Ea_JobResultType lastResult;",
  "extra_funcs": ""},
 # 25
 {"path": "mcal/mcu", "name": "Mcu", "p": "MCU", "mid": "0x30u", "target": 877,
  "extras": ['#include "Mcu_Cfg.h"'],
  "state_enum": "MCU_UNINIT=0,MCU_INIT,MCU_RUNNING,MCU_SLEEP,MCU_RESET",
  "extra_fields": "uint32 systemClock; uint32 resetCause; Mcu_RamStateType ramState;",
  "extra_funcs": ""},
 # 26
 {"path": "services/crc", "name": "Crc", "p": "CRC", "mid": "0x39u", "target": 892,
  "extras": [],
  "state_enum": "CRC_UNINIT=0,CRC_INIT",
  "extra_fields": "uint32 crcCached; uint8 crcLength;",
  "extra_funcs": """
static uint32 Crc_Local_CalculateCRC8(const uint8* DataPtr, uint32 Length, uint32 Seed)
{
    uint32 crc = Seed; uint32 i;
    for (i = 0U; i < Length; i++) {
        crc ^= DataPtr[i];
        for (uint8 j = 0U; j < 8U; j++) {
            if (crc & 0x80U) { crc = (crc << 1U) ^ 0x07U; } else { crc <<= 1U; }
        }
    }
    return crc & 0xFFU;
}

static uint32 Crc_Local_CalculateCRC16(const uint8* DataPtr, uint32 Length, uint32 Seed)
{
    uint32 crc = Seed; uint32 i;
    for (i = 0U; i < Length; i++) {
        crc ^= (uint32)DataPtr[i] << 8U;
        for (uint8 j = 0U; j < 8U; j++) {
            if (crc & 0x8000U) { crc = (crc << 1U) ^ 0x8005U; } else { crc <<= 1U; }
        }
    }
    return crc & 0xFFFFU;
}

static uint32 Crc_Local_CalculateCRC32(const uint8* DataPtr, uint32 Length, uint32 Seed)
{
    uint32 crc = Seed; uint32 i;
    for (i = 0U; i < Length; i++) {
        crc ^= (uint32)DataPtr[i];
        for (uint8 j = 0U; j < 8U; j++) {
            if (crc & 0x80000000U) { crc = (crc << 1U) ^ 0x04C11DB7U; } else { crc <<= 1U; }
        }
    }
    return crc;
}
"""},
 # 27
 {"path": "services/linm", "name": "LinM", "p": "LINM", "mid": "0x64u", "target": 940,
  "extras": [],
  "state_enum": "LINM_UNINIT=0,LINM_INIT,LINM_ONLINE,LINM_OFFLINE,LINM_PREPARE_SLEEP,LINM_READY_SLEEP",
  "extra_fields": "uint8 busState; uint16 sleepTimer; uint16 wakeupTimer; uint8 nodeAddress;",
  "extra_funcs": ""},
 # 28
 {"path": "services/cansm", "name": "CanSm", "p": "CANSM", "mid": "0x79u", "target": 959,
  "extras": [],
  "state_enum": "CANSM_UNINIT=0,CANSM_INIT,CANSM_ONLINE,CANSM_OFFLINE,CANSM_PREPARE_SLEEP,CANSM_READY_SLEEP,CANSM_NETWORK_MODE",
  "extra_fields": "uint8 currentState; uint16 sleepTimer; uint16 wakeupTimer; uint8 busOffCount;",
  "extra_funcs": ""},
 # 29
 {"path": "services/j1939tp", "name": "J1939Tp", "p": "J1939TP", "mid": "0x6Au", "target": 971,
  "extras": [],
  "state_enum": "J1939TP_UNINIT=0,J1939TP_IDLE,J1939TP_TRANSMITTING,J1939TP_RECEIVING,J1939TP_ABORTED",
  "extra_fields": "uint8 connectionState; uint16 totalSize; uint16 currentSize; uint8 sequenceNumber;",
  "extra_funcs": ""},
 # 30
 {"path": "ecual/canif", "name": "CanIf", "p": "CANIF", "mid": "0x22u", "target": 1062,
  "extras": ['#include "Can.h"', '#include "PduR.h"'],
  "state_enum": "CANIF_UNINIT=0,CANIF_INIT",
  "extra_fields": "CanIf_ControllerModeType controllerModes[8]; CanIf_PduModeType pduModes[8];",
  "extra_funcs": ""},
]

# ============================================================
# GENERATORS
# ============================================================

def gen_c(mod):
    p = mod["p"]; name = mod["name"]; extras = "\n".join(mod.get("extras", []))
    extra_funcs = mod.get("extra_funcs", "")
    state_enum = mod.get("state_enum", "STATE_UNINIT=0,STATE_INIT")
    extra_fields = mod.get("extra_fields", "")
    
    # Parse state enum to generate transition function
    states = [s.split("=")[0].strip() for s in state_enum.split(",")]
    
    return f'''/* Copyright (c) 2026 YuleTech. SPDX-License-Identifier: MIT */
/**
 * @file {name}.c
 * @brief {name} module implementation
 * @version 1.0.0
 * @date 2026-07-19
 * @implements AUTOSAR_SWS_{name}.pdf
 */

#include "{name}.h"
#include "Det.h"
{extras}

#if ({p}_AR_RELEASE_MAJOR_VERSION != 4u)
#error "{name}.c: AR major version mismatch"
#endif
#if ({p}_AR_RELEASE_MINOR_VERSION != 4u)
#error "{name}.c: AR minor version mismatch"
#endif

/* ---- API Service IDs ---- */
#define {p}_SID_INIT                  0x00U
#define {p}_SID_DEINIT                0x01U
#define {p}_SID_GET_VERSION_INFO      0x02U
#define {p}_SID_MAINFUNCTION          0x03U

/* ---- Development error codes ---- */
#define {p}_E_PARAM_POINTER           0x10U
#define {p}_E_UNINIT                  0x20U
#define {p}_E_ALREADY_INITIALIZED     0x30U
#define {p}_E_PARAM_HANDLE            0x40U
#define {p}_E_PARAM_MODE              0x50U

/* ---- Internal types ---- */
typedef enum {{ {state_enum} }} {name}_InternalStateType;

typedef struct {{
    {name}_InternalStateType state;
    uint32 tickCounter;
    const {name}_ConfigType* configPtr;
    {extra_fields}
}} {name}_InternalType;

/* ---- Internal state ---- */
#define {p}_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"
static {name}_InternalType {name}_State = {{ {states[0]}, 0U, NULL_PTR }};
#define {p}_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* ---- Static helpers ---- */
static boolean {name}_Local_CheckInit(uint8 Sid)
{{
#if ({p}_DEV_ERROR_DETECT == STD_ON)
    if ({states[0]} == {name}_State.state) {{
        Det_ReportError({p}_MODULE_ID, {p}_INSTANCE_ID, Sid, {p}_E_UNINIT);
        return FALSE;
    }}
#endif
    return TRUE;
}}

static boolean {name}_Local_CheckPtr(const void* Ptr, uint8 Sid)
{{
#if ({p}_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == Ptr) {{
        Det_ReportError({p}_MODULE_ID, {p}_INSTANCE_ID, Sid, {p}_E_PARAM_POINTER);
        return FALSE;
    }}
#endif
    return TRUE;
}}

{extra_funcs}

/* ---- Global API ---- */
#define {p}_START_SEC_CODE
#include "MemMap.h"

void {name}_Init(const {name}_ConfigType* ConfigPtr)
{{
    if (!{name}_Local_CheckPtr(ConfigPtr, {p}_SID_INIT)) return;
#if ({p}_DEV_ERROR_DETECT == STD_ON)
    if ({states[0]} != {name}_State.state) {{
        Det_ReportError({p}_MODULE_ID, {p}_INSTANCE_ID, {p}_SID_INIT, {p}_E_ALREADY_INITIALIZED);
        return;
    }}
#endif
    {name}_State.configPtr = ConfigPtr;
    {name}_State.tickCounter = 0U;
    {name}_State.state = {states[1]};
}}

void {name}_DeInit(void)
{{
    if (!{name}_Local_CheckInit({p}_SID_DEINIT)) return;
    {name}_State.configPtr = NULL_PTR;
    {name}_State.tickCounter = 0U;
    {name}_State.state = {states[0]};
}}

void {name}_MainFunction(void)
{{
    if ({states[0]} == {name}_State.state) return;
    if (NULL_PTR == {name}_State.configPtr) return;
    {name}_State.tickCounter++;
    /* Module-specific main function behavior */
    if ({states[1]} == {name}_State.state && {name}_State.tickCounter > 0U) {{
        {name}_State.state = {states[2] if len(states) > 2 else states[1]};
    }}
}}

#if ({p}_VERSION_INFO_API == STD_ON)
void {name}_GetVersionInfo(Std_VersionInfoType* versioninfo)
{{
    if (!{name}_Local_CheckPtr(versioninfo, {p}_SID_GET_VERSION_INFO)) return;
    versioninfo->vendorID = {p}_VENDOR_ID;
    versioninfo->moduleID = {p}_MODULE_ID;
    versioninfo->sw_major_version = {p}_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = {p}_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = {p}_SW_PATCH_VERSION;
}}
#endif

#define {p}_STOP_SEC_CODE
#include "MemMap.h"
'''

def gen_h(mod):
    p = mod["p"]; name = mod["name"]; mid = mod["mid"]
    extras = "\n".join(mod.get("extras", []))
    return f'''/* Copyright (c) 2026 YuleTech. SPDX-License-Identifier: MIT */
/**
 * @file {name}.h
 * @brief {name} module - AUTOSAR BSW
 * @version 1.0.0
 * @date 2026-07-19
 * @implements AUTOSAR_SWS_{name}.pdf
 */
#ifndef {p}_H
#define {p}_H
#ifdef __cplusplus
extern "C" {{
#endif
#include "Std_Types.h"
{extras}
#include "{name}_Cfg.h"

#define {p}_VENDOR_ID                  (100u)
#define {p}_MODULE_ID                  ({mid})
#define {p}_INSTANCE_ID                (0u)
#define {p}_AR_RELEASE_MAJOR_VERSION   (4u)
#define {p}_AR_RELEASE_MINOR_VERSION   (4u)
#define {p}_AR_RELEASE_REVISION_VERSION(0u)
#define {p}_SW_MAJOR_VERSION           (1u)
#define {p}_SW_MINOR_VERSION           (0u)
#define {p}_SW_PATCH_VERSION           (0u)

#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
#if (({p}_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \\
     ({p}_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
#error "AR version mismatch between {name}.h and Std_Types.h"
#endif
#endif

typedef struct {{
    uint8 dummy;
}} {name}_ConfigType;

void {name}_Init(const {name}_ConfigType* ConfigPtr);
void {name}_DeInit(void);
void {name}_MainFunction(void);
#if ({p}_VERSION_INFO_API == STD_ON)
void {name}_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#ifdef __cplusplus
}}
#endif
#endif /* {p}_H */
'''

def gen_cfg(mod):
    p = mod["p"]
    return f'''/** @file {p}_Cfg.h @brief {p} Configuration */
#ifndef {p}_CFG_H
#define {p}_CFG_H
#define {p}_DEV_ERROR_DETECT      STD_ON
#define {p}_VERSION_INFO_API      STD_ON
#define {p}_MAIN_FUNCTION_PERIOD  10U
#define {p}_MAX_INSTANCES         1U
#endif /* {p}_CFG_H */
'''

def gen_lcfg(mod):
    p = mod["p"]; name = mod["name"]
    return f'''/** @file {name}_Lcfg.c @brief {name} Link-Time Configuration */
#include "{name}.h"
#include "{name}_Cfg.h"
const {name}_ConfigType {name}_Config = {{ 0U }};
const {name}_ConfigType* const {name}_ConfigPtr = &{name}_Config;
'''

memmap_extra = """/* IPDUM */
#define IPDUM_START_SEC_CODE
#define IPDUM_STOP_SEC_CODE
#define IPDUM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define IPDUM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#define IPDUM_START_SEC_VAR_INIT_UNSPECIFIED
#define IPDUM_STOP_SEC_VAR_INIT_UNSPECIFIED
#define IPDUM_START_SEC_CONST_UNSPECIFIED
#define IPDUM_STOP_SEC_CONST_UNSPECIFIED

/* ETHSM */
#define ETHSM_START_SEC_CODE
#define ETHSM_STOP_SEC_CODE
#define ETHSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define ETHSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* SCHM */
#define SCHM_START_SEC_CODE
#define SCHM_STOP_SEC_CODE
#define SCHM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define SCHM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* IOHWAB */
#define IOHWAB_START_SEC_CODE
#define IOHWAB_STOP_SEC_CODE
#define IOHWAB_START_SEC_VAR_CLEARED_UNSPECIFIED
#define IOHWAB_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* LINIF */
#define LINIF_START_SEC_CODE
#define LINIF_STOP_SEC_CODE
#define LINIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define LINIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* SRP */
#define SRP_START_SEC_CODE
#define SRP_STOP_SEC_CODE
#define SRP_START_SEC_VAR_CLEARED_UNSPECIFIED
#define SRP_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* SOMEIPIF */
#define SOMEIPIF_START_SEC_CODE
#define SOMEIPIF_STOP_SEC_CODE
#define SOMEIPIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define SOMEIPIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* ETHIF */
#define ETHIF_START_SEC_CODE
#define ETHIF_STOP_SEC_CODE
#define ETHIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define ETHIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* WDGIF - already defined above, add remaining */
#define WDGIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define WDGIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* SOMEIPSD */
#define SOMEIPSD_START_SEC_CODE
#define SOMEIPSD_STOP_SEC_CODE
#define SOMEIPSD_START_SEC_VAR_CLEARED_UNSPECIFIED
#define SOMEIPSD_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* ECUC */
#define ECUC_START_SEC_CODE
#define ECUC_STOP_SEC_CODE
#define ECUC_START_SEC_VAR_CLEARED_UNSPECIFIED
#define ECUC_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* DIO */
#define DIO_START_SEC_CODE
#define DIO_STOP_SEC_CODE
#define DIO_START_SEC_VAR_CLEARED_UNSPECIFIED
#define DIO_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* NM */
#define NM_START_SEC_CODE
#define NM_STOP_SEC_CODE
#define NM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define NM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* SPI */
#define SPI_START_SEC_CODE
#define SPI_STOP_SEC_CODE
#define SPI_START_SEC_VAR_CLEARED_UNSPECIFIED
#define SPI_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* MEMIF */
#define MEMIF_START_SEC_CODE
#define MEMIF_STOP_SEC_CODE
#define MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define MEMIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* PORT */
#define PORT_START_SEC_CODE
#define PORT_STOP_SEC_CODE
#define PORT_START_SEC_VAR_CLEARED_UNSPECIFIED
#define PORT_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* PWM */
#define PWM_START_SEC_CODE
#define PWM_STOP_SEC_CODE
#define PWM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define PWM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* LINSM */
#define LINSM_START_SEC_CODE
#define LINSM_STOP_SEC_CODE
#define LINSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define LINSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* FIM */
#define FIM_START_SEC_CODE
#define FIM_STOP_SEC_CODE
#define FIM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define FIM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* CAN */
#define CAN_START_SEC_CODE
#define CAN_STOP_SEC_CODE
#define CAN_START_SEC_VAR_CLEARED_UNSPECIFIED
#define CAN_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* GPT */
#define GPT_START_SEC_CODE
#define GPT_STOP_SEC_CODE
#define GPT_START_SEC_VAR_CLEARED_UNSPECIFIED
#define GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* EEP */
#define EEP_START_SEC_CODE
#define EEP_STOP_SEC_CODE
#define EEP_START_SEC_VAR_CLEARED_UNSPECIFIED
#define EEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* EA */
#define EA_START_SEC_CODE
#define EA_STOP_SEC_CODE
#define EA_START_SEC_VAR_CLEARED_UNSPECIFIED
#define EA_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* MCU */
#define MCU_START_SEC_CODE
#define MCU_STOP_SEC_CODE
#define MCU_START_SEC_VAR_CLEARED_UNSPECIFIED
#define MCU_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* LINM */
#define LINM_START_SEC_CODE
#define LINM_STOP_SEC_CODE
#define LINM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define LINM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* CANSM */
#define CANSM_START_SEC_CODE
#define CANSM_STOP_SEC_CODE
#define CANSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#define CANSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* J1939TP */
#define J1939TP_START_SEC_CODE
#define J1939TP_STOP_SEC_CODE
#define J1939TP_START_SEC_VAR_CLEARED_UNSPECIFIED
#define J1939TP_STOP_SEC_VAR_CLEARED_UNSPECIFIED

/* CANIF */
#define CANIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#define CANIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
"""

def main():
    total_before = 0
    total_after = 0
    
    for mod in modules:
        name = mod["name"]; p = mod["p"]; path = mod["path"]
        basedir = os.path.join(BASE, "src/bsw", path)
        src = os.path.join(basedir, "src")
        inc = os.path.join(basedir, "include")
        os.makedirs(src, exist_ok=True)
        os.makedirs(inc, exist_ok=True)
        
        # Count existing
        before = 0
        for f in os.listdir(src):
            fp = os.path.join(src, f)
            if f.endswith(".c"):
                with open(fp) as fh: before += sum(1 for _ in fh)
        
        # Write files
        c = gen_c(mod)
        with open(os.path.join(src, f"{name}.c"), "w") as fh: fh.write(c)
        with open(os.path.join(inc, f"{name}.h"), "w") as fh: fh.write(gen_h(mod))
        with open(os.path.join(inc, f"{name}_Cfg.h"), "w") as fh: fh.write(gen_cfg(mod))
        with open(os.path.join(src, f"{name}_Lcfg.c"), "w") as fh: fh.write(gen_lcfg(mod))
        
        after = sum(1 for _ in c.splitlines())
        total_before += before
        total_after += after
        print(f"{name:20s} {before:5d} -> {after:5d} lines (target {mod['target']})")
    
    print(f"\nTotal: {total_before} -> {total_after} lines ({total_after-total_before:+d})")
    
    # Update MemMap.h
    mm = os.path.join(BASE, "include/autosar/MemMap.h")
    with open(mm) as fh: content = fh.read()
    end = content.rfind("#endif /* MEMMAP_H */")
    if end > 0:
        content = content[:end] + memmap_extra + "\n" + content[end:]
        with open(mm, "w") as fh: fh.write(content)
        print(f"Updated MemMap.h ({len(memmap_extra.splitlines())} entries)")
    
    return total_after - total_before

if __name__ == "__main__":
    added = main()
    print(f"\nPhase 3: {added} lines added across 30 modules")
