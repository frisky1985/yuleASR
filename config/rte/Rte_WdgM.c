/** @file Rte_WdgM.c
 * @brief RTE implementation for Watchdog Manager (WdgM)
 * @details RTE generation for AUTOSAR R22-11
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "Rte_WdgM.h"
#include "autosar/service/WdgM/WdgM.h"
#include "autosar/service/BswM/BswM.h"

/******************************************************************************
 * RTE Mode Switch Interface Implementation
 ******************************************************************************/
Std_ReturnType Rte_Call_WdgM_ModeRequest_SetMode(WdgM_ModeType Mode)
{
    /* Direct call to WdgM_SetMode */
    return WdgM_SetMode(Mode);
}

Std_ReturnType Rte_Write_WdgM_ModeNotification_CurrentMode(Rte_ModeType_WdgM_Mode Mode)
{
    /* Notify BswM about mode change */
    /* In a full RTE implementation, this would trigger BswM rules */
    (void)Mode;
    return E_OK;
}

/******************************************************************************
 * RTE Status Interface Implementation
 ******************************************************************************/
Std_ReturnType Rte_Read_WdgM_GlobalStatus_Status(WdgM_GlobalStatusType* Status)
{
    if (Status == NULL_PTR) {
        return E_NOT_OK;
    }
    *Status = WdgM_GetGlobalStatus();
    return E_OK;
}

Std_ReturnType Rte_Read_WdgM_LocalStatus_Status(
    WdgM_SupervisedEntityIdType SEID,
    WdgM_LocalStatusType* Status)
{
    if (Status == NULL_PTR) {
        return E_NOT_OK;
    }
    *Status = WdgM_GetLocalStatus(SEID);
    return E_OK;
}

/******************************************************************************
 * RTE Runnable Implementation
 ******************************************************************************/
void Rte_WdgM_MainFunction(void)
{
    WdgM_MainFunction();
}

/******************************************************************************
 * RTE Initialization
 ******************************************************************************/
Std_ReturnType Rte_Start_WdgM(void)
{
    /* RTE would initialize ports and connections here */
    return E_OK;
}

Std_ReturnType Rte_Stop_WdgM(void)
{
    /* RTE would deinitialize ports and connections here */
    return E_OK;
}
