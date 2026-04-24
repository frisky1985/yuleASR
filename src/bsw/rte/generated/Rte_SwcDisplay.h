/**
 * @file Rte_SwcDisplay.h
 * @brief RTE Interface for SwcDisplay following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-24
 * @author RTE Generator
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Generated RTE interface for Software Component SwcDisplay
 *
 * @note THIS FILE IS AUTO-GENERATED. DO NOT MODIFY MANUALLY.
 */

#ifndef RTE_SWCDISPLAY_H
#define RTE_SWCDISPLAY_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"


/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define RTE_SWCDISPLAY_VENDOR_ID                   (0x01U)
#define RTE_SWCDISPLAY_MODULE_ID                   (0x70U)
#define RTE_SWCDISPLAY_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define RTE_SWCDISPLAY_AR_RELEASE_MINOR_VERSION    (0x04U)
#define RTE_SWCDISPLAY_SW_MAJOR_VERSION            (0x01U)
#define RTE_SWCDISPLAY_SW_MINOR_VERSION            (0x00U)
#define RTE_SWCDISPLAY_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* No local types defined */

/*==================================================================================================
*                                    RTE API FUNCTION PROTOTYPES
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Std_ReturnType Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed(uint16* data);
extern Std_ReturnType Rte_Receive_SwcDisplay_RpEngineSpeed(uint16* data);
extern Std_ReturnType Rte_Read_SwcDisplay_RpVehicleSpeed_VehicleSpeed(uint16* data);
extern Std_ReturnType Rte_Receive_SwcDisplay_RpVehicleSpeed(uint16* data);
extern Std_ReturnType Rte_Result_SwcDisplay_PpDisplayServices_ShowWarning(uint8 warningId);
extern Std_ReturnType Rte_Server_SwcDisplay_PpDisplayServices_ShowWarning(uint8 warningId);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE API MACROS
==================================================================================================*/
#define RTE_READ_SWCDISPLAY_RPENGINESPEED_ENGINESPEED(data) \
    Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed(data)
#define RTE_RECEIVE_SWCDISPLAY_RPENGINESPEED(data) \
    Rte_Receive_SwcDisplay_RpEngineSpeed(data)
#define RTE_READ_SWCDISPLAY_RPVEHICLESPEED_VEHICLESPEED(data) \
    Rte_Read_SwcDisplay_RpVehicleSpeed_VehicleSpeed(data)
#define RTE_RECEIVE_SWCDISPLAY_RPVEHICLESPEED(data) \
    Rte_Receive_SwcDisplay_RpVehicleSpeed(data)
#define RTE_RESULT_SWCDISPLAY_PPDISPLAYSERVICES_SHOWWARNING(warningId) \
    Rte_Result_SwcDisplay_PpDisplayServices_ShowWarning(warningId)

#endif /* RTE_SWCDISPLAY_H */
