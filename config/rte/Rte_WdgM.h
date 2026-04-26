/** @file Rte_WdgM.h
 * @brief RTE interface for Watchdog Manager (WdgM)
 * @details RTE generation configuration for AUTOSAR R22-11
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef RTE_WDGM_H
#define RTE_WDGM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar/service/WdgM/WdgM.h"
#include "common/autosar_types.h"

/******************************************************************************
 * RTE Event IDs for WdgM
 ******************************************************************************/
#define RTE_E_WDGM_GLOBALSTATUSCHANGED    0x01U
#define RTE_E_WDGM_LOCALSTATUSCHANGED     0x02U
#define RTE_E_WDGM_SUPERVISIONEXPIRED     0x03U

/******************************************************************************
 * RTE Mode Switch Interface (WdgM <-> BswM)
 ******************************************************************************/
typedef uint8 Rte_ModeType_WdgM_Mode;

#define RTE_MODE_WDGM_WDGM_OFF            0x00U
#define RTE_MODE_WDGM_WDGM_SLOW           0x01U
#define RTE_MODE_WDGM_WDGM_FAST           0x02U

/******************************************************************************
 * RTE Port Interface: WdgM_ModeRequest
 * @details BswM requests mode changes to WdgM via this port
 ******************************************************************************/
Std_ReturnType Rte_Call_WdgM_ModeRequest_SetMode(
    WdgM_ModeType Mode
);

/******************************************************************************
 * RTE Port Interface: WdgM_ModeNotification
 * @details WdgM notifies BswM about mode changes
 ******************************************************************************/
Std_ReturnType Rte_Write_WdgM_ModeNotification_CurrentMode(
    Rte_ModeType_WdgM_Mode Mode
);

/******************************************************************************
 * RTE Port Interface: WdgM_GlobalStatus
 * @details Provides WdgM global status to other components
 ******************************************************************************/
Std_ReturnType Rte_Read_WdgM_GlobalStatus_Status(
    WdgM_GlobalStatusType* Status
);

/******************************************************************************
 * RTE Port Interface: WdgM_LocalStatus
 * @details Provides WdgM local status for specific SE
 ******************************************************************************/
Std_ReturnType Rte_Read_WdgM_LocalStatus_Status(
    WdgM_SupervisedEntityIdType SEID,
    WdgM_LocalStatusType* Status
);

/******************************************************************************
 * RTE Runnable: WdgM_MainFunction
 * @details Cyclic main function invoked by RTE
 ******************************************************************************/
void Rte_WdgM_MainFunction(void);

/******************************************************************************
 * RTE Initialization
 ******************************************************************************/
Std_ReturnType Rte_Start_WdgM(void);
Std_ReturnType Rte_Stop_WdgM(void);

#ifdef __cplusplus
}
#endif

#endif /* RTE_WDGM_H */
