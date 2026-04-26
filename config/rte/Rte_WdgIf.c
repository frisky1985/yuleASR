/** @file Rte_WdgIf.c
 * @brief RTE implementation for Watchdog Interface (WdgIf)
 * @details RTE generation for AUTOSAR R22-11
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "Rte_WdgIf.h"
#include "ecual/WdgIf/WdgIf.h"

/******************************************************************************
 * RTE Port Interface Implementation
 ******************************************************************************/
Std_ReturnType Rte_Call_WdgIf_Control_SetMode(
    WdgIf_DeviceType Device,
    WdgIf_ModeType Mode)
{
    return WdgIf_SetMode(Device, Mode);
}

Std_ReturnType Rte_Call_WdgIf_Control_SetTriggerCondition(
    WdgIf_DeviceType Device,
    uint16 Timeout)
{
    WdgIf_SetTriggerCondition(Device, Timeout);
    return E_OK;
}

Std_ReturnType Rte_Read_WdgIf_Status_DriverStatus(
    WdgIf_DeviceType Device,
    WdgIf_DriverStatusType* Status)
{
    if (Status == NULL_PTR) {
        return E_NOT_OK;
    }
    *Status = WdgIf_GetDriverStatus(Device);
    return E_OK;
}

/******************************************************************************
 * RTE Runnable Implementation
 ******************************************************************************/
void Rte_WdgIf_MainFunction(void)
{
    WdgIf_MainFunction();
}

/******************************************************************************
 * RTE Initialization
 ******************************************************************************/
Std_ReturnType Rte_Start_WdgIf(void)
{
    return E_OK;
}

Std_ReturnType Rte_Stop_WdgIf(void)
{
    return E_OK;
}
