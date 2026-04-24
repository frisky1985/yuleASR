#define RTE_COMSIGNAL_SWCDISPLAY_RPENGINESPEED_ENGINESPEED    (100U)
#define RTE_COMSIGNAL_SWCDISPLAY_RPVEHICLESPEED_VEHICLESPEED    (200U)

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
#include "Rte_SwcDisplay.h"
#include "Com.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_SWCDISPLAY_INSTANCE_ID             (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* No local types */

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC uint16 Rte_Buffer_SwcDisplay_RpEngineSpeed_EngineSpeed;
STATIC boolean Rte_IsUpdated_SwcDisplay_RpEngineSpeed_EngineSpeed = FALSE;
STATIC uint16 Rte_Buffer_SwcDisplay_RpVehicleSpeed_VehicleSpeed;
STATIC boolean Rte_IsUpdated_SwcDisplay_RpVehicleSpeed_VehicleSpeed = FALSE;

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
 * @brief   Read EngineSpeed from RpEngineSpeed
 */
Std_ReturnType Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        uint8 comResult = Com_ReceiveSignal(RTE_COMSIGNAL_SWCDISPLAY_RPENGINESPEED_ENGINESPEED, data);
        if (comResult == COM_SERVICE_OK)
        {
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Receive EngineSpeed from RpEngineSpeed
 */
Std_ReturnType Rte_Receive_SwcDisplay_RpEngineSpeed(uint16* data)
{
    return Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed(data);
}

/**
 * @brief   Read VehicleSpeed from RpVehicleSpeed
 */
Std_ReturnType Rte_Read_SwcDisplay_RpVehicleSpeed_VehicleSpeed(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        uint8 comResult = Com_ReceiveSignal(RTE_COMSIGNAL_SWCDISPLAY_RPVEHICLESPEED_VEHICLESPEED, data);
        if (comResult == COM_SERVICE_OK)
        {
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Receive VehicleSpeed from RpVehicleSpeed
 */
Std_ReturnType Rte_Receive_SwcDisplay_RpVehicleSpeed(uint16* data)
{
    return Rte_Read_SwcDisplay_RpVehicleSpeed_VehicleSpeed(data);
}

/**
 * @brief   Server implementation of ShowWarning on PpDisplayServices
 */
Std_ReturnType Rte_Server_SwcDisplay_PpDisplayServices_ShowWarning(uint8 warningId)
{
    Std_ReturnType result = E_OK;

    /* TODO: Implement server operation logic */
        (void)warningId;\n

    return result;
}

/**
 * @brief   Get result of ShowWarning on PpDisplayServices
 */
Std_ReturnType Rte_Result_SwcDisplay_PpDisplayServices_ShowWarning(uint8 warningId)
{
    Std_ReturnType result = E_OK;

    /* TODO: Implement async result retrieval */
        (void)warningId;\n

    return result;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
