#define RTE_COMSIGNAL_SWCENGINECTRL_PPENGINESPEED_ENGINESPEED    (100U)
#define RTE_COMSIGNAL_SWCENGINECTRL_PPTHROTTLEPOS_THROTTLEPOSITION    (101U)
#define RTE_NVMBLOCK_SWCENGINECTRL_PPNVMENGINECONFIG_ENGINECONFIG    (10U)

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : Rte, Com, NvM, Dcm
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-24
* Author               : RTE Generator
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "Rte_SwcEngineCtrl.h"
#include "Com.h"
#include "NvM.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_SWCENGINECTRL_INSTANCE_ID             (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* No local types */

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC uint16 Rte_Buffer_SwcEngineCtrl_PpEngineSpeed_EngineSpeed;
STATIC boolean Rte_IsUpdated_SwcEngineCtrl_PpEngineSpeed_EngineSpeed = FALSE;
STATIC uint8 Rte_Buffer_SwcEngineCtrl_PpThrottlePos_ThrottlePosition;
STATIC boolean Rte_IsUpdated_SwcEngineCtrl_PpThrottlePos_ThrottlePosition = FALSE;
STATIC Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType Rte_Buffer_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig;
STATIC boolean Rte_Buffer_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig_IsValid = FALSE;

#define RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/* No local prototypes */

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/* No local functions */

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Write EngineSpeed to PpEngineSpeed
 */
Std_ReturnType Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed(const uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        uint8 comResult = Com_SendSignal(RTE_COMSIGNAL_SWCENGINECTRL_PPENGINESPEED_ENGINESPEED, data);
        if (comResult == E_OK)
        {
            Rte_Buffer_SwcEngineCtrl_PpEngineSpeed_EngineSpeed = *data;
            Rte_IsUpdated_SwcEngineCtrl_PpEngineSpeed_EngineSpeed = TRUE;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Send EngineSpeed via PpEngineSpeed
 */
Std_ReturnType Rte_Send_SwcEngineCtrl_PpEngineSpeed(const uint16* data)
{
    return Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed(data);
}

/**
 * @brief   Write ThrottlePosition to PpThrottlePos
 */
Std_ReturnType Rte_Write_SwcEngineCtrl_PpThrottlePos_ThrottlePosition(const uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        uint8 comResult = Com_SendSignal(RTE_COMSIGNAL_SWCENGINECTRL_PPTHROTTLEPOS_THROTTLEPOSITION, data);
        if (comResult == E_OK)
        {
            Rte_Buffer_SwcEngineCtrl_PpThrottlePos_ThrottlePosition = *data;
            Rte_IsUpdated_SwcEngineCtrl_PpThrottlePos_ThrottlePosition = TRUE;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Send ThrottlePosition via PpThrottlePos
 */
Std_ReturnType Rte_Send_SwcEngineCtrl_PpThrottlePos(const uint8* data)
{
    return Rte_Write_SwcEngineCtrl_PpThrottlePos_ThrottlePosition(data);
}

/**
 * @brief   Read EngineConfig from NV memory via PpNvMEngineConfig
 */
Std_ReturnType Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        result = NvM_ReadBlock(RTE_NVMBLOCK_SWCENGINECTRL_PPNVMENGINECONFIG_ENGINECONFIG, data);
        if (result == E_OK)
        {
            Rte_Buffer_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig_IsValid = TRUE;
        }
    }

    return result;
}

/**
 * @brief   Write EngineConfig to NV memory via PpNvMEngineConfig
 */
Std_ReturnType Rte_Write_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(const Rte_SwcEngineCtrl_PpNvMEngineConfig_EngineConfigType* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        result = NvM_WriteBlock(RTE_NVMBLOCK_SWCENGINECTRL_PPNVMENGINECONFIG_ENGINECONFIG, data);
        if (result == E_OK)
        {
            Rte_Buffer_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig = *data;
            Rte_Buffer_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig_IsValid = TRUE;
        }
    }

    return result;
}

/**
 * @brief   Call ReadDTC on PpDiagServices (Client)
 */
Std_ReturnType Rte_Call_SwcEngineCtrl_PpDiagServices_ReadDTC(uint16 DTC, uint8* Status)
{
    Std_ReturnType result = E_OK;

    /* TODO: Implement client-server call dispatch */
    (void)result;
        (void)DTC;\n    (void)Status;\n

    return result;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
