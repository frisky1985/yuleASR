/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Rte
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/* @req SHALL_RTE */


/**
 * @file Rte_SwcPortApi.c
 * @brief Implementations of the generated per-SWC port API symbols
 * @version 1.0.0
 * @date 2026-08-01
 *
 * These 36 symbols (Rte_Read_SWC_* / Rte_Write_SWC_* / Rte_Switch_SWC_*) were
 * declared extern in Rte.h and referenced by the SWC interface macros but had
 * no implementation anywhere in the tree, so every ASW component failed to
 * link. Each function forwards to the generic Rte_Read/Rte_Write/Rte_Switch
 * with the port handle derived from the component/port configuration.
 */

#include "Rte.h"
#include "Rte_Cfg.h"

/* Per-SWC data element types: connection DataLength must equal sizeof(<type>)
 * so Rte_Read/Rte_Write copy exactly the right number of bytes and never
 * over-read the sender or over-write the receiver. */
#include "Swc_EngineControl.h"
#include "Swc_VehicleDynamics.h"
#include "Swc_DiagnosticManager.h"
#include "Swc_CommunicationManager.h"
#include "Swc_StorageManager.h"
#include "Swc_IOControl.h"
#include "Swc_ModeManager.h"
#include "Swc_WatchdogManager.h"

#define RTE_START_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                  PORT CONNECTION TABLE
*==================================================================================================*/

typedef struct
{
    Rte_PortHandleType PortHandle;
    uint8              Direction;   /* 1 = Sender (Rte_Read), 2 = Receiver (Rte_Write) */
    uint16             DataLength;
} Rte_SwcPortConnectionType;

#define RTE_SWC_PORT_CONNECTION(componentId, portId, dir, len) \
    { (Rte_PortHandleType)(((uint16)(componentId) << 8U) | (uint16)(portId)), (dir), (len) }

STATIC const Rte_SwcPortConnectionType Rte_SwcPortConnections[] =
{
    /* ---- Receiver (R) ports: DataLength = sizeof(type read into) ---- */
    RTE_SWC_PORT_CONNECTION(0U, 0x05U, 1U, (uint16)sizeof(sint16)),                              /* COOLANT_TEMP_R       */
    RTE_SWC_PORT_CONNECTION(0U, 0x04U, 1U, (uint16)sizeof(uint16)),                              /* THROTTLE_POS_R       */
    RTE_SWC_PORT_CONNECTION(0U, 0x06U, 1U, (uint16)sizeof(uint16)),                              /* VEHICLE_SPEED_R      */
    RTE_SWC_PORT_CONNECTION(1U, 0x16U, 1U, (uint16)sizeof(sint16)),                              /* ACCEL_DATA_R         */
    RTE_SWC_PORT_CONNECTION(1U, 0x15U, 1U, (uint16)sizeof(sint16)),                              /* STEERING_ANGLE_R     */
    RTE_SWC_PORT_CONNECTION(1U, 0x14U, 1U, (uint16)sizeof(uint16)),                              /* WHEEL_SPEEDS_R       */
    RTE_SWC_PORT_CONNECTION(2U, 0x24U, 1U, (uint16)sizeof(Swc_DiagnosticRequestType)),           /* DIAG_REQUEST_R       */
    RTE_SWC_PORT_CONNECTION(3U, 0x33U, 1U, (uint16)sizeof(Swc_PduInfoType)),                     /* PDU_DATA_R           */
    RTE_SWC_PORT_CONNECTION(5U, 0x54U, 1U, (uint16)sizeof(Swc_AnalogIOValueType)),               /* ANALOG_INPUT_R       */
    RTE_SWC_PORT_CONNECTION(5U, 0x52U, 1U, (uint16)sizeof(Swc_DigitalIOValueType)),              /* DIGITAL_INPUT_R      */
    RTE_SWC_PORT_CONNECTION(5U, 0x56U, 1U, (uint16)sizeof(Swc_PwmIOValueType)),                  /* PWM_INPUT_R          */
    RTE_SWC_PORT_CONNECTION(6U, 0x63U, 1U, (uint16)sizeof(Swc_ModeTransitionRequestType)),       /* MODE_REQUEST_R       */
    RTE_SWC_PORT_CONNECTION(7U, 0x73U, 1U, (uint16)sizeof(uint8)),                               /* ALIVE_INDICATION_R   */

    /* ---- Sender (P) ports: DataLength = sizeof(type written) ---- */
    RTE_SWC_PORT_CONNECTION(0U, 0x01U, 2U, (uint16)sizeof(Swc_EngineStateType)),                 /* ENGINE_STATE_P       */
    RTE_SWC_PORT_CONNECTION(0U, 0x02U, 2U, (uint16)sizeof(Swc_EngineParametersType)),            /* ENGINE_PARAMS_P      */
    RTE_SWC_PORT_CONNECTION(0U, 0x03U, 2U, (uint16)sizeof(Swc_EngineControlOutputType)),         /* ENGINE_CONTROL_P     */
    RTE_SWC_PORT_CONNECTION(1U, 0x11U, 2U, (uint16)sizeof(Swc_VdcStateType)),                    /* VDC_STATE_P          */
    RTE_SWC_PORT_CONNECTION(1U, 0x12U, 2U, (uint16)sizeof(Swc_VehicleMotionType)),               /* MOTION_DATA_P        */
    RTE_SWC_PORT_CONNECTION(1U, 0x13U, 2U, (uint16)sizeof(Swc_VehicleDynamicsOutputType)),       /* VDC_OUTPUT_P         */
    RTE_SWC_PORT_CONNECTION(2U, 0x21U, 2U, (uint16)sizeof(Swc_DiagnosticSessionType)),           /* SESSION_P            */
    RTE_SWC_PORT_CONNECTION(2U, 0x22U, 2U, (uint16)sizeof(Swc_SecurityLevelType)),               /* SECURITY_P           */
    RTE_SWC_PORT_CONNECTION(2U, 0x23U, 2U, (uint16)sizeof(Swc_DtcStatusType)),                   /* DTC_STATUS_P         */
    RTE_SWC_PORT_CONNECTION(2U, 0x25U, 2U, (uint16)sizeof(Swc_DiagnosticResponseType)),          /* DIAG_RESPONSE_P      */
    RTE_SWC_PORT_CONNECTION(3U, 0x31U, 2U, (uint16)sizeof(Swc_CommStateType)),                   /* COMM_STATE_P         */
    RTE_SWC_PORT_CONNECTION(3U, 0x32U, 2U, (uint16)sizeof(Swc_SignalValueType)),                 /* SIGNAL_DATA_P        */
    RTE_SWC_PORT_CONNECTION(3U, 0x34U, 2U, (uint16)sizeof(Swc_PduInfoType)),                     /* PDU_DATA_P           */
    RTE_SWC_PORT_CONNECTION(4U, 0x42U, 2U, (uint16)sizeof(Swc_StorageBlockStatusType)),          /* BLOCK_STATUS_P       */
    RTE_SWC_PORT_CONNECTION(5U, 0x51U, 2U, (uint16)sizeof(Swc_IOStateType)),                     /* IO_STATE_P           */
    RTE_SWC_PORT_CONNECTION(5U, 0x53U, 2U, (uint16)sizeof(Swc_DigitalIOValueType)),              /* DIGITAL_OUTPUT_P     */
    RTE_SWC_PORT_CONNECTION(5U, 0x55U, 2U, (uint16)sizeof(Swc_AnalogIOValueType)),               /* ANALOG_OUTPUT_P      */
    RTE_SWC_PORT_CONNECTION(5U, 0x57U, 2U, (uint16)sizeof(Swc_PwmIOValueType)),                  /* PWM_OUTPUT_P         */
    RTE_SWC_PORT_CONNECTION(6U, 0x61U, 2U, (uint16)sizeof(Swc_SystemModeType)),                  /* SYSTEM_MODE_P        */
    RTE_SWC_PORT_CONNECTION(6U, 0x62U, 2U, (uint16)sizeof(Swc_SystemStateType)),                 /* SYSTEM_STATE_P       */
    RTE_SWC_PORT_CONNECTION(6U, 0x64U, 2U, (uint16)sizeof(Swc_ComponentModeType)),               /* MODE_NOTIFICATION_P  */
    RTE_SWC_PORT_CONNECTION(7U, 0x71U, 2U, (uint16)sizeof(Swc_WatchdogStatusType)),              /* WDG_STATUS_P         */
    RTE_SWC_PORT_CONNECTION(7U, 0x74U, 2U, (uint16)sizeof(uint8))                                /* WDG_TRIGGER_P        */
};

#define RTE_SWC_NUM_PORT_CONNECTIONS (sizeof(Rte_SwcPortConnections) / sizeof(Rte_SwcPortConnections[0]))

/**
 * @brief Connect every configured SWC port so Rte_Read/Rte_Write succeed.
 *        Called from Rte_Start() to restore the RTE data path.
 */
void Rte_SwcPortApi_ConnectAllPorts(void)
{
    uint16 i;

    for (i = 0U; i < RTE_SWC_NUM_PORT_CONNECTIONS; i++)
    {
        (void)Rte_ConnectPort(Rte_SwcPortConnections[i].PortHandle,
                              Rte_SwcPortConnections[i].Direction,
                              Rte_SwcPortConnections[i].DataLength);
    }
}

/*==================================================================================================
*                                  PER-SWC PORT API
*==================================================================================================*/

/**
 * @brief Read COOLANT_TEMP_R
 */
Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_COOLANT_TEMP_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x05U))), data);
}

/**
 * @brief Read THROTTLE_POS_R
 */
Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_THROTTLE_POS_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x04U))), data);
}

/**
 * @brief Read VEHICLE_SPEED_R
 */
Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_VEHICLE_SPEED_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x06U))), data);
}

/**
 * @brief Read ACCEL_DATA_R
 */
Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_ACCEL_DATA_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x16U))), data);
}

/**
 * @brief Read STEERING_ANGLE_R
 */
Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_STEERING_ANGLE_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x15U))), data);
}

/**
 * @brief Read WHEEL_SPEEDS_R
 */
Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_WHEEL_SPEEDS_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x14U))), data);
}

/**
 * @brief Read DIAG_REQUEST_R
 */
Std_ReturnType Rte_Read_SWC_DIAGNOSTICMANAGER_PORT_DIAG_REQUEST_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(2U) << 8U) | (uint16)(0x24U))), data);
}

/**
 * @brief Read PDU_DATA_R
 */
Std_ReturnType Rte_Read_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(3U) << 8U) | (uint16)(0x33U))), data);
}

/**
 * @brief Read ANALOG_INPUT_R
 */
Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_ANALOG_INPUT_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x54U))), data);
}

/**
 * @brief Read DIGITAL_INPUT_R
 */
Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_DIGITAL_INPUT_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x52U))), data);
}

/**
 * @brief Read PWM_INPUT_R
 */
Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_PWM_INPUT_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x56U))), data);
}

/**
 * @brief Read MODE_REQUEST_R
 */
Std_ReturnType Rte_Read_SWC_MODEMANAGER_PORT_MODE_REQUEST_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(6U) << 8U) | (uint16)(0x63U))), data);
}

/**
 * @brief Read ALIVE_INDICATION_R
 */
Std_ReturnType Rte_Read_SWC_WATCHDOGMANAGER_PORT_ALIVE_INDICATION_R(void* data)
{
    return Rte_Read(((Rte_PortHandleType)(((uint16)(7U) << 8U) | (uint16)(0x73U))), data);
}

/**
 * @brief Write ENGINE_STATE_P
 */
Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_STATE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x01U))), data);
}

/**
 * @brief Write ENGINE_PARAMS_P
 */
Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_PARAMS_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x02U))), data);
}

/**
 * @brief Write ENGINE_CONTROL_P
 */
Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_CONTROL_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(0U) << 8U) | (uint16)(0x03U))), data);
}

/**
 * @brief Write VDC_STATE_P
 */
Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_STATE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x11U))), data);
}

/**
 * @brief Write MOTION_DATA_P
 */
Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_MOTION_DATA_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x12U))), data);
}

/**
 * @brief Write VDC_OUTPUT_P
 */
Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_OUTPUT_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(1U) << 8U) | (uint16)(0x13U))), data);
}

/**
 * @brief Write SESSION_P
 */
Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SESSION_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(2U) << 8U) | (uint16)(0x21U))), data);
}

/**
 * @brief Write SECURITY_P
 */
Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SECURITY_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(2U) << 8U) | (uint16)(0x22U))), data);
}

/**
 * @brief Write DTC_STATUS_P
 */
Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DTC_STATUS_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(2U) << 8U) | (uint16)(0x23U))), data);
}

/**
 * @brief Write DIAG_RESPONSE_P
 */
Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DIAG_RESPONSE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(2U) << 8U) | (uint16)(0x25U))), data);
}

/**
 * @brief Write COMM_STATE_P
 */
Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_COMM_STATE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(3U) << 8U) | (uint16)(0x31U))), data);
}

/**
 * @brief Write SIGNAL_DATA_P
 */
Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_SIGNAL_DATA_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(3U) << 8U) | (uint16)(0x32U))), data);
}

/**
 * @brief Write PDU_DATA_P
 */
Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(3U) << 8U) | (uint16)(0x34U))), data);
}

/**
 * @brief Write BLOCK_STATUS_P
 */
Std_ReturnType Rte_Write_SWC_STORAGEMANAGER_PORT_BLOCK_STATUS_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(4U) << 8U) | (uint16)(0x42U))), data);
}

/**
 * @brief Write IO_STATE_P
 */
Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_IO_STATE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x51U))), data);
}

/**
 * @brief Write DIGITAL_OUTPUT_P
 */
Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_DIGITAL_OUTPUT_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x53U))), data);
}

/**
 * @brief Write ANALOG_OUTPUT_P
 */
Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_ANALOG_OUTPUT_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x55U))), data);
}

/**
 * @brief Write PWM_OUTPUT_P
 */
Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_PWM_OUTPUT_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(5U) << 8U) | (uint16)(0x57U))), data);
}

/**
 * @brief Write SYSTEM_MODE_P
 */
Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_MODE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(6U) << 8U) | (uint16)(0x61U))), data);
}

/**
 * @brief Write SYSTEM_STATE_P
 */
Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_STATE_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(6U) << 8U) | (uint16)(0x62U))), data);
}

/**
 * @brief Write MODE_NOTIFICATION_P
 */
Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_MODE_NOTIFICATION_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(6U) << 8U) | (uint16)(0x64U))), data);
}

/**
 * @brief Write WDG_STATUS_P
 */
Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_STATUS_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(7U) << 8U) | (uint16)(0x71U))), data);
}

/**
 * @brief Write WDG_TRIGGER_P
 */
Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_TRIGGER_P(const void* data)
{
    return Rte_Write(((Rte_PortHandleType)(((uint16)(7U) << 8U) | (uint16)(0x74U))), data);
}

/**
 * @brief Mode switch for EngineControl mode port
 */
Std_ReturnType Rte_Switch_SWC_ENGINECONTROL_PORT_MODE_P(uint32 data)
{
    return Rte_Switch((Rte_ModeHandleType)0x00U, data);
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
*==================================================================================================*/
