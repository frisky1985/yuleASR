/**
 * @file Rte_SwcEngineCtrl.h
 * @brief RTE Interface for SwcEngineCtrl following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-24
 * @author RTE Generator
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Generated RTE interface for Software Component SwcEngineCtrl
 *
 * @note THIS FILE IS AUTO-GENERATED. DO NOT MODIFY MANUALLY.
 */

#ifndef RTE_SWCENGINECTRL_H
#define RTE_SWCENGINECTRL_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "NvM.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define RTE_SWCENGINECTRL_VENDOR_ID                   (0x01U)
#define RTE_SWCENGINECTRL_MODULE_ID                   (0x70U)
#define RTE_SWCENGINECTRL_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define RTE_SWCENGINECTRL_AR_RELEASE_MINOR_VERSION    (0x04U)
#define RTE_SWCENGINECTRL_SW_MAJOR_VERSION            (0x01U)
#define RTE_SWCENGINECTRL_SW_MINOR_VERSION            (0x00U)
#define RTE_SWCENGINECTRL_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct
{
    uint16 maxRpm;
    uint8 idleSpeed;
} Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType;

/*==================================================================================================
*                                    RTE API FUNCTION PROTOTYPES
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Std_ReturnType Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed(const uint16* data);
extern Std_ReturnType Rte_Send_SwcEngineCtrl_PpEngineSpeed(const uint16* data);
extern Std_ReturnType Rte_Write_SwcEngineCtrl_PpThrottlePos_ThrottlePosition(const uint8* data);
extern Std_ReturnType Rte_Send_SwcEngineCtrl_PpThrottlePos(const uint8* data);
extern Std_ReturnType Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType* data);
extern Std_ReturnType Rte_Write_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(const Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType* data);
extern Std_ReturnType Rte_Call_SwcEngineCtrl_PpDiagServices_ReadDTC(uint16 DTC, uint8* Status);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE API MACROS
==================================================================================================*/
#define RTE_WRITE_SWCENGINECTRL_PPENGINESPEED_ENGINESPEED(data) \
    Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed(data)
#define RTE_SEND_SWCENGINECTRL_PPENGINESPEED(data) \
    Rte_Send_SwcEngineCtrl_PpEngineSpeed(data)
#define RTE_WRITE_SWCENGINECTRL_PPTHROTTLEPOS_THROTTLEPOSITION(data) \
    Rte_Write_SwcEngineCtrl_PpThrottlePos_ThrottlePosition(data)
#define RTE_SEND_SWCENGINECTRL_PPTHROTTLEPOS(data) \
    Rte_Send_SwcEngineCtrl_PpThrottlePos(data)
#define RTE_READ_SWCENGINECTRL_PPNVMENGINECONFIG_ENGINECONFIG(data) \
    Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(data)
#define RTE_WRITE_SWCENGINECTRL_PPNVMENGINECONFIG_ENGINECONFIG(data) \
    Rte_Write_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(data)
#define RTE_CALL_SWCENGINECTRL_PPDIAGSERVICES_READDTC(DTC, Status) \
    Rte_Call_SwcEngineCtrl_PpDiagServices_ReadDTC(DTC, Status)

#endif /* RTE_SWCENGINECTRL_H */
