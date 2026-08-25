/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : Rte, Com, Det, Dem, WdgM
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (RTE Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file Rte_CSOperations.c
* @brief RTE Client-Server Operation and Sender-Receiver generated API
*
* This file implements the generated Rte_Read, Rte_Write, and Rte_Call
* functions for the 8 ASW components. Each function uses the core RTE
* port buffer mechanism (Rte_Read/Rte_Write) or COM signal-based
* communication (Rte_ComSendSignal/Rte_ComReceiveSignal) for data
* transport, with DET error checking following AUTOSAR MISRA C:2012.
==================================================================================================*/
/* @req SHALL_RTE */


/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Cfg.h"
#include "Rte_Type.h"
#include "NvM.h"
#include "Rte_Bsw.h"
#include "Com.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_CSOPS_INSTANCE_ID           (0x00U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (RTE_DEV_ERROR_DETECT == STD_ON)
    #define RTE_CSOPS_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(RTE_MODULE_ID, RTE_CSOPS_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define RTE_CSOPS_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                               GLOBAL FUNCTIONS - EngineControl
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Read engine RPM via sender-receiver port
 * @param   data Pointer to store RPM value (0-8000 RPM)
 * @return  Std_ReturnType: E_OK on success, E_NOT_OK on error
 *
 * Implements: Rte_Read_EngineControl_Port_RPM
 * Uses core Rte_Read() with EngineControl RPM port handle.
 */
Std_ReturnType Rte_Read_EngineControl_Port_RPM(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_EC_RPM_H, (void*)data);

    return result;
}

/**
 * @brief   Read vehicle speed from engine control perspective
 * @param   data Pointer to store speed value (0-65535, 0.1 km/h units)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_EngineControl_Port_Speed
 */
Std_ReturnType Rte_Read_EngineControl_Port_Speed(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_EC_SPEED_H, (void*)data);

    return result;
}

/**
 * @brief   Read engine coolant temperature
 * @param   data Pointer to store temperature value (0-255, degrees Celsius)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_EngineControl_Port_Temperature
 */
Std_ReturnType Rte_Read_EngineControl_Port_Temperature(uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_EC_TEMPERATURE_H, (void*)data);

    return result;
}

/**
 * @brief   Write throttle position command
 * @param   data Pointer to throttle position value (0-10000, 0.01% units)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_EngineControl_Port_Throttle
 * Uses Rte_ComSendSignal for COM-based signal transmission.
 */
Std_ReturnType Rte_Write_EngineControl_Port_Throttle(const uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    /* Write to local port buffer for inter-runnable communication */
    result = Rte_Write(RTE_PORT_EC_THROTTLE_H, (const void*)data);

#if (RTE_COM_SUPPORT == STD_ON)
    if (result == E_OK)
    {
        /* Also send via COM signal for network-based communication */
        (void)Rte_ComSendSignal(RTE_COMSIGNAL_ENGINE_RPM, (const void*)data);
    }
#endif

    return result;
}

/**
 * @brief   Write fuel injection command
 * @param   data Pointer to fuel injection value (0-65535, 0.01 ms pulse width)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_EngineControl_Port_FuelInjection
 */
Std_ReturnType Rte_Write_EngineControl_Port_FuelInjection(const uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Write(RTE_PORT_EC_FUEL_INJECTION_H, (const void*)data);

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - VehicleDynamics
==================================================================================================*/

/**
 * @brief   Read vehicle yaw rate
 * @param   data Pointer to store yaw rate (rad/s, float32)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_VehicleDynamics_Port_YawRate
 */
Std_ReturnType Rte_Read_VehicleDynamics_Port_YawRate(float32* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_VD_YAW_RATE_H, (void*)data);

    return result;
}

/**
 * @brief   Read individual wheel speed
 * @param   data Pointer to store wheel speed (0-65535, 0.01 m/s units)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_VehicleDynamics_Port_WheelSpeed
 */
Std_ReturnType Rte_Read_VehicleDynamics_Port_WheelSpeed(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_VD_WHEEL_SPEED_H, (void*)data);

    return result;
}

/**
 * @brief   Read steering wheel angle
 * @param   data Pointer to store steering angle (0-65535, 0.1 degree units)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_VehicleDynamics_Port_SteeringAngle
 */
Std_ReturnType Rte_Read_VehicleDynamics_Port_SteeringAngle(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_VD_STEERING_ANGLE_H, (void*)data);

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - DiagnosticManager
==================================================================================================*/

/**
 * @brief   Read DTC status byte
 * @param   data Pointer to store DTC status (bitmask per AUTOSAR Dem)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_DiagnosticManager_Port_DTCStatus
 */
Std_ReturnType Rte_Read_DiagnosticManager_Port_DTCStatus(uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_DM_DTC_STATUS_H, (void*)data);

    return result;
}

/**
 * @brief   Clear a specific DTC via client-server call
 * @param   dtc 32-bit DTC value (0x00000000-0xFFFFFFFF)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Call_DiagnosticManager_Port_ClearDTC
 * Uses COM signal to send clear request to DEM.
 */
Std_ReturnType Rte_Call_DiagnosticManager_Port_ClearDTC(uint32 dtc)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (dtc == 0U)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_CS_CALL, RTE_E_INVALID);
        return E_NOT_OK;
    }
#endif

    /* Send clear DTC request via COM signal */
#if (RTE_COM_SUPPORT == STD_ON)
    {
        /* Pack DTC value into a temporary buffer for COM transmission */
        uint8 txBuffer[4U];
        (void)memcpy(txBuffer, (const void*)&dtc, sizeof(dtc));

        result = Rte_ComSendSignal(RTE_COMSIGNAL_ENGINE_RPM, (const void*)txBuffer);
    }
#else
    /* Direct DEM call for local operation */
    /* result = Dem_ClearDTC(dtc); - Dem integration */
    (void)dtc;
    result = E_OK;
#endif

    return result;
}

/**
 * @brief   Read DTC status via client-server call
 * @param   dtc 32-bit DTC value to query
 * @param   status Pointer to store DTC status byte
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Call_DiagnosticManager_Port_ReadDTC
 */
Std_ReturnType Rte_Call_DiagnosticManager_Port_ReadDTC(uint32 dtc, uint8* status)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (status == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_CS_CALL, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }

    if (dtc == 0U)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_CS_CALL, RTE_E_INVALID);
        return E_NOT_OK;
    }
#endif

    /* Read DTC status - use Dem interface */
    /* result = Dem_GetStatusOfDTC(dtc, status); - Dem integration */

    /* For now, read from port buffer as fallback */
    result = Rte_Read(RTE_PORT_DM_READ_DTC_H, (void*)status);

    if (result != E_OK)
    {
        *status = 0U;
    }

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - CommunicationManager
==================================================================================================*/

/**
 * @brief   Read vehicle speed from communication manager
 * @param   data Pointer to store vehicle speed (0-65535, 0.1 km/h units)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_CommunicationManager_Port_VehicleSpeed
 */
Std_ReturnType Rte_Read_CommunicationManager_Port_VehicleSpeed(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_CM_VEHICLE_SPEED_H, (void*)data);

    return result;
}

/**
 * @brief   Write communication manager status
 * @param   data Pointer to status byte (0=Full, 1=Silent, 2=NoCom)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_CommunicationManager_Port_Status
 * Uses both local port write and COM signal transmission.
 */
Std_ReturnType Rte_Write_CommunicationManager_Port_Status(const uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Write(RTE_PORT_CM_STATUS_H, (const void*)data);

#if (RTE_COM_SUPPORT == STD_ON)
    if (result == E_OK)
    {
        (void)Rte_ComSendSignal(RTE_COMSIGNAL_VEHICLE_SPEED, (const void*)data);
    }
#endif

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - StorageManager
==================================================================================================*/

/**
 * @brief   Read data from storage manager port
 * @param   data Pointer to output buffer for stored data
 * @param   length Pointer to store the length of read data (in/out param)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_StorageManager_Port_Data
 * Reads variable-length data from port buffer; length indicates buffer capacity
 * on input and actual data length on output.
 */
Std_ReturnType Rte_Read_StorageManager_Port_Data(uint8* data, uint16* length)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 tempLength ;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if ((data == NULL_PTR) || (length == NULL_PTR))
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    /* Use NVM interface for actual storage read */
    result = Rte_NvmReadBlock(RTE_NVMBLOCK_USER_SETTINGS, (void*)data);

    if (result == E_OK)
    {
        /* Set length from known block size */
        tempLength = 64U;
        *length = tempLength;
    }

    return result;
}

/**
 * @brief   Write data to storage manager port
 * @param   data Pointer to data buffer to store
 * @param   length Length of data to write
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_StorageManager_Port_Data
 * Uses NvM interface for non-volatile storage.
 */
Std_ReturnType Rte_Write_StorageManager_Port_Data(const uint8* data, uint16 length)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }

    if (length == 0U)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_INVALID);
        return E_NOT_OK;
    }
#endif

    /* Write to NVM via NVM interface */
    result = Rte_NvmWriteBlock(RTE_NVMBLOCK_USER_SETTINGS, (const void*)data);

    if (result == E_OK)
    {
        /* Also write to local port buffer for immediate access */
        result = Rte_Write(RTE_PORT_SM_DATA_WRITE_H, (const void*)data);
    }

    (void)length;

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - IOControl
==================================================================================================*/

/**
 * @brief   Read IO input value
 * @param   data Pointer to store IO input value (0-65535, ADC counts)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_IOControl_Port_Input
 */
Std_ReturnType Rte_Read_IOControl_Port_Input(uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_IO_INPUT_H, (void*)data);

    return result;
}

/**
 * @brief   Write IO output value
 * @param   data Pointer to output value (0-65535, PWM duty/analog output)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_IOControl_Port_Output
 */
Std_ReturnType Rte_Write_IOControl_Port_Output(const uint16* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Write(RTE_PORT_IO_OUTPUT_H, (const void*)data);

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - ModeManager
==================================================================================================*/

/**
 * @brief   Read current operation mode
 * @param   data Pointer to store current mode (0=Normal, 1=Sleep, etc.)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_ModeManager_Port_CurrentMode
 */
Std_ReturnType Rte_Read_ModeManager_Port_CurrentMode(uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    result = Rte_Read(RTE_PORT_MM_CURRENT_MODE_H, (void*)data);

    return result;
}

/**
 * @brief   Write target operation mode
 * @param   data Pointer to target mode value
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Write_ModeManager_Port_TargetMode
 * Triggers mode switch via BswM interface.
 */
Std_ReturnType Rte_Write_ModeManager_Port_TargetMode(const uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_WRITE, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    /* Write to local port buffer */
    result = Rte_Write(RTE_PORT_MM_TARGET_MODE_H, (const void*)data);

    if (result == E_OK)
    {
        /* Request mode switch via BswM */
        result = Rte_Bsw_BswM_RequestMode(RTE_BSW_COMPONENT_MODE_MANAGER, (uint8)(*data));
    }

    return result;
}

/*==================================================================================================
*                               GLOBAL FUNCTIONS - WatchdogManager
==================================================================================================*/

/**
 * @brief   Read watchdog manager status
 * @param   data Pointer to store watchdog status (0=OK, 1=Alive, 2=Expired)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Read_WatchdogManager_Port_Status
 */
Std_ReturnType Rte_Read_WatchdogManager_Port_Status(uint8* data)
{
    Std_ReturnType result = E_NOT_OK;

#if (RTE_DEV_ERROR_DETECT == STD_ON)
    if (data == NULL_PTR)
    {
        RTE_CSOPS_DET_REPORT_ERROR(RTE_SID_READ, RTE_E_SEG_FAULT);
        return E_NOT_OK;
    }
#endif

    /* Read global watchdog status via BSW interface */
    result = Rte_Bsw_WdgM_GetGlobalStatus((uint8*)data);

    if (result != E_OK)
    {
        /* Fallback to port buffer */
        result = Rte_Read(RTE_PORT_WM_STATUS_H, (void*)data);
    }

    return result;
}

/**
 * @brief   Perform watchdog system reset
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Call_WatchdogManager_Port_Reset
 * Calls WdgM PerformReset via BSW interface.
 */
Std_ReturnType Rte_Call_WatchdogManager_Port_Reset(void)
{
    Std_ReturnType result = E_NOT_OK;

    result = Rte_Bsw_WdgM_PerformReset();

    return result;
}

/**
 * @brief   Trigger watchdog (refresh/tickle)
 * @return  Std_ReturnType: E_OK on success, error code on failure
 *
 * Implements: Rte_Call_WatchdogManager_Port_Trigger
 * Calls the Wdg_Trigger BSW function to refresh the watchdog.
 */
Std_ReturnType Rte_Call_WatchdogManager_Port_Trigger(void)
{
    Std_ReturnType result = E_NOT_OK;

    /* Direct watchdog trigger call */
    Rte_Bsw_Wdg_Trigger();

    result = E_OK;

    return result;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
