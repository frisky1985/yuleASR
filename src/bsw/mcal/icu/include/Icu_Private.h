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

/**
 * @file Icu_Private.h
 * @brief ICU Driver private header file with internal definitions
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ICU_PRIVATE_H
#define ICU_PRIVATE_H

/*==================================================================================================
*                                    INCLUDE FILES
==================================================================================================*/
#include "Icu.h"
#include "Icu_Cfg.h"
#include "Icu_Lcfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ICU_PRIVATE_VENDOR_ID               (0x01U)
#define ICU_PRIVATE_MODULE_ID               (0x10U)
#define ICU_PRIVATE_SW_MAJOR_VERSION        (0x01U)
#define ICU_PRIVATE_SW_MINOR_VERSION        (0x00U)
#define ICU_PRIVATE_SW_PATCH_VERSION        (0x00U)

/*==================================================================================================
*                                    INTERNAL MACROS
==================================================================================================*/

/* Register access macros */
#define ICU_REG_READ32(addr)                (*(volatile uint32*)(uintptr)(addr))
#define ICU_REG_WRITE32(addr, val)          (*(volatile uint32*)(uintptr)(addr) = (val))
#define ICU_REG_READ16(addr)                (*(volatile uint16*)(addr))
#define ICU_REG_WRITE16(addr, val)          (*(volatile uint16*)(addr) = (val))

/* Channel validation */
#define ICU_IS_VALID_CHANNEL(ch)            ((ch) < ICU_NUM_CHANNELS)

/* Mode validation */
#define ICU_IS_VALID_MODE(mode)             ((mode) <= ICU_MODE_EDGE_COUNTER)

/* Edge validation */
#define ICU_IS_VALID_EDGE(edge)             ((edge) <= ICU_BOTH_EDGES)

/* Property validation */
#define ICU_IS_VALID_PROPERTY(prop)         ((prop) <= ICU_DUTY_CYCLE)

/*==================================================================================================
*                                    INTERNAL STRUCTURES
==================================================================================================*/

/* ICU channel runtime state */
typedef struct {
    Icu_StateType State;
    Icu_InputStateType InputState;
    Icu_ValueType CapturedValue;
    Icu_ValueType PreviousValue;
    Icu_ValueType PeriodTime;
    Icu_ValueType ActiveTime;
    Icu_EdgeNumberType EdgeCount;
    Icu_IndexType BufferIndex;
    Icu_IndexType BufferSize;
    Icu_IndexType NotifyInterval;
    Icu_IndexType NotifyCounter;
    Icu_ValueType* TimestampBuffer;
    boolean NotificationEnabled;
    boolean WakeupEnabled;
    boolean IsRunning;
    Icu_SignalEdgeType CurrentEdge;
} Icu_ChannelStateType;

/* ICU driver state */
typedef struct {
    boolean Initialized;
    Icu_ModeType CurrentMode;
    const Icu_ConfigType* ConfigPtr;
} Icu_DriverStateType;

/*==================================================================================================
*                                    EXTERNAL DECLARATIONS
==================================================================================================*/

#define ICU_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Channel runtime states */
extern Icu_ChannelStateType Icu_ChannelState[ICU_NUM_CHANNELS];

/* Driver state */
extern Icu_DriverStateType Icu_DriverState;

#define ICU_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    INTERNAL FUNCTION PROTOTYPES
==================================================================================================*/

#define ICU_START_SEC_CODE
#include "MemMap.h"

/* Hardware abstraction functions */
static uint32 Icu_GetTpmBaseAddr(Icu_ChannelType Channel);
static uint8 Icu_GetTpmChannelOffset(Icu_ChannelType Channel);
static void Icu_EnableTpmClock(Icu_ChannelType Channel);
static void Icu_DisableTpmClock(Icu_ChannelType Channel);

/* Channel operation functions */
static void Icu_ConfigureChannel(Icu_ChannelType Channel, const Icu_ChannelConfigType* Config);
static void Icu_ResetChannelState(Icu_ChannelType Channel);
static void Icu_ProcessEdgeDetection(Icu_ChannelType Channel);
static void Icu_ProcessSignalMeasurement(Icu_ChannelType Channel);
static void Icu_ProcessTimestamp(Icu_ChannelType Channel, Icu_ValueType CurrentValue);
static void Icu_ProcessEdgeCount(Icu_ChannelType Channel);

/* Utility functions */
static void Icu_SetupInputCapture(Icu_ChannelType Channel, Icu_SignalEdgeType Edge);
static Icu_SignalEdgeType Icu_GetEdgeConfig(uint32 CnscReg);
static void Icu_ClearChannelFlag(Icu_ChannelType Channel);
static boolean Icu_IsChannelFlagSet(Icu_ChannelType Channel);

#define ICU_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ICU_PRIVATE_H */
