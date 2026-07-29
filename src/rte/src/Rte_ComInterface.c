/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : Com, Rte
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (RTE Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "Com.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_COM_INSTANCE_ID             (0x00U)

/* COM Signal to RTE Data Element Mapping */
#define RTE_COM_MAP_SIZE                (16U)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* COM Signal Mapping Entry */
typedef struct
{
    uint16 ComSignalId;
    Rte_DataHandleType RteDataHandle;
    uint8 DataLength;
    boolean IsMapped;
} Rte_ComSignalMappingType;

/* COM Interface State */
typedef struct
{
    boolean IsInitialized;
    Rte_ComSignalMappingType SignalMap[RTE_COM_MAP_SIZE];
    uint8 NumMappedSignals;
} Rte_ComInterfaceStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Rte_ComInterfaceStateType Rte_ComInterfaceState;

#define RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC sint8 Rte_ComFindSignalMapping(uint16 comSignalId);
STATIC Std_ReturnType Rte_ComMapSignal(uint16 comSignalId, Rte_DataHandleType rteDataHandle, uint8 dataLength);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find COM signal mapping by COM signal ID
 */
STATIC sint8 Rte_ComFindSignalMapping(uint16 comSignalId)
{
    sint8 result = -1;
    uint8 i;

    for (i = 0U; i < Rte_ComInterfaceState.NumMappedSignals; i++)
    {
        if (Rte_ComInterfaceState.SignalMap[i].ComSignalId == comSignalId)
        {
            result = (sint8)i;
            break;
        }
    }

    return result;
}

/**
 * @brief   Map COM signal to RTE data handle
 */
STATIC Std_ReturnType Rte_ComMapSignal(uint16 comSignalId, Rte_DataHandleType rteDataHandle, uint8 dataLength)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_ComInterfaceState.NumMappedSignals < RTE_COM_MAP_SIZE)
    {
        uint8 index = Rte_ComInterfaceState.NumMappedSignals;
        Rte_ComInterfaceState.SignalMap[index].ComSignalId = comSignalId;
        Rte_ComInterfaceState.SignalMap[index].RteDataHandle = rteDataHandle;
        Rte_ComInterfaceState.SignalMap[index].DataLength = dataLength;
        Rte_ComInterfaceState.SignalMap[index].IsMapped = TRUE;
        Rte_ComInterfaceState.NumMappedSignals++;
        result = E_OK;
    }

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes RTE COM Interface
 */
void Rte_ComInterface_Init(void)
{
    uint8 i;

    Rte_ComInterfaceState.IsInitialized = FALSE;
    Rte_ComInterfaceState.NumMappedSignals = 0U;

    for (i = 0U; i < RTE_COM_MAP_SIZE; i++)
    {
        Rte_ComInterfaceState.SignalMap[i].ComSignalId = 0xFFFFU;
        Rte_ComInterfaceState.SignalMap[i].RteDataHandle = 0xFFFFU;
        Rte_ComInterfaceState.SignalMap[i].DataLength = 0U;
        Rte_ComInterfaceState.SignalMap[i].IsMapped = FALSE;
    }

    /* Initialize default signal mappings */
    /* Engine Signals */
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_ENGINE_RPM, RTE_DE_ENGINE_RPM, 2U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_VEHICLE_SPEED, RTE_DE_VEHICLE_SPEED, 2U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_ENGINE_TEMP, RTE_DE_ENGINE_TEMP, 1U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_BATTERY_VOLTAGE, RTE_DE_BATTERY_VOLTAGE, 2U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_GEAR_POSITION, RTE_DE_GEAR_POSITION, 1U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_ODOMETER, RTE_DE_ODOMETER, 4U);
    (void)Rte_ComMapSignal(RTE_COMSIGNAL_FUEL_LEVEL, RTE_DE_FUEL_LEVEL, 1U);

    Rte_ComInterfaceState.IsInitialized = TRUE;
}

/**
 * @brief   Sends data via COM signal
 */
Std_ReturnType Rte_ComSendSignal(uint16 comSignalId, const void* signalData)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_ComInterfaceState.IsInitialized && (signalData != NULL_PTR))
    {
        sint8 mapIndex = Rte_ComFindSignalMapping(comSignalId);

        if (mapIndex >= 0)
        {
            /* Send via COM */
            result = Com_SendSignal((Com_SignalIdType)comSignalId, signalData);
        }
    }

    return result;
}

/**
 * @brief   Receives data from COM signal
 */
Std_ReturnType Rte_ComReceiveSignal(uint16 comSignalId, void* signalData)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_ComInterfaceState.IsInitialized && (signalData != NULL_PTR))
    {
        sint8 mapIndex = Rte_ComFindSignalMapping(comSignalId);

        if (mapIndex >= 0)
        {
            /* Receive via COM */
            uint8 comResult = Com_ReceiveSignal((Com_SignalIdType)comSignalId, signalData);

            if (comResult == COM_SERVICE_OK)
            {
                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief   RTE COM callback for signal reception
 */
void Rte_ComCallbackRx(uint16 signalId, const void* data)
{
    if (Rte_ComInterfaceState.IsInitialized && (data != NULL_PTR))
    {
        sint8 mapIndex = Rte_ComFindSignalMapping(signalId);

        if (mapIndex >= 0)
        {
            Rte_DataHandleType dataHandle = Rte_ComInterfaceState.SignalMap[mapIndex].RteDataHandle;

            /* Notify RTE about received data */
            /* This would typically trigger a runnable or update a data element */
            (void)dataHandle;
        }
    }
}

/**
 * @brief   RTE COM callback for signal transmission confirmation
 */
void Rte_ComCallbackTx(uint16 signalId)
{
    if (Rte_ComInterfaceState.IsInitialized)
    {
        sint8 mapIndex = Rte_ComFindSignalMapping(signalId);

        if (mapIndex >= 0)
        {
            Rte_DataHandleType dataHandle = Rte_ComInterfaceState.SignalMap[mapIndex].RteDataHandle;

            /* Notify RTE about transmission completion */
            (void)dataHandle;
        }
    }
}

/**
 * @brief   Trigger COM IPDU send
 */
Std_ReturnType Rte_ComTriggerIPDUSend(PduIdType pduId)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_ComInterfaceState.IsInitialized)
    {
        result = Com_TriggerIPDUSend(pduId);
    }

    return result;
}

/**
 * @brief   Switch COM IPDU transmission mode
 */
void Rte_ComSwitchIpduTxMode(PduIdType pduId, boolean mode)
{
    if (Rte_ComInterfaceState.IsInitialized)
    {
        Com_SwitchIpduTxMode(pduId, mode);
    }
}

/**
 * @brief   Main function for COM interface processing
 */
void Rte_ComInterface_MainFunction(void)
{
    if (Rte_ComInterfaceState.IsInitialized)
    {
        /* Process COM main functions */
        Com_MainFunctionRx();
        Com_MainFunctionTx();
    }
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
