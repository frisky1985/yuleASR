/** @file Rte_WdgIf.h
 * @brief RTE interface for Watchdog Interface (WdgIf)
 * @details RTE generation configuration for AUTOSAR R22-11
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef RTE_WDGIF_H
#define RTE_WDGMIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ecual/WdgIf/WdgIf.h"
#include "common/autosar_types.h"

/******************************************************************************
 * RTE Event IDs for WdgIf
 ******************************************************************************/
#define RTE_E_WDGIF_TRIGGERFAILURE        0x01U
#define RTE_E_WDGIF_MODESWITCHFAILED      0x02U

/******************************************************************************
 * RTE Port Interface: WdgIf_Control
 * @details Provides WdgIf control services to WdgM
 ******************************************************************************/
Std_ReturnType Rte_Call_WdgIf_Control_SetMode(
    WdgIf_DeviceType Device,
    WdgIf_ModeType Mode
);

Std_ReturnType Rte_Call_WdgIf_Control_SetTriggerCondition(
    WdgIf_DeviceType Device,
    uint16 Timeout
);

/******************************************************************************
 * RTE Port Interface: WdgIf_Status
 * @details Provides WdgIf status information
 ******************************************************************************/
Std_ReturnType Rte_Read_WdgIf_Status_DriverStatus(
    WdgIf_DeviceType Device,
    WdgIf_DriverStatusType* Status
);

/******************************************************************************
 * RTE Runnable: WdgIf_MainFunction
 * @details Cyclic main function invoked by RTE
 ******************************************************************************/
void Rte_WdgIf_MainFunction(void);

/******************************************************************************
 * RTE Initialization
 ******************************************************************************/
Std_ReturnType Rte_Start_WdgIf(void);
Std_ReturnType Rte_Stop_WdgIf(void);

#ifdef __cplusplus
}
#endif

#endif /* RTE_WDGMIF_H */
