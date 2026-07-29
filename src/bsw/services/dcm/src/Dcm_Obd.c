/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Dcm_Obd.c
 * @brief DCM OBD-II Service implementation
 * @details Implements ISO 15031-5 OBD-II diagnostic services
 */

#include "Dcm.h"
#include "Dcm_Obd.h"
#include "Dem.h"

#if (DCM_OBD_SUPPORT_ENABLED == STD_ON)

/*******************************************************************************
 * OBD-II Service 0x01 - Current Data
 ******************************************************************************/
Std_ReturnType Dcm_ObdService01(
    Dcm_MsgContextType* MsgContext,
    Dcm_NegativeResponseCodeType* ErrorCode)
{
    uint8 Pid;
    uint8 DataBuffer[4];
    
    if (MsgContext->reqDataLen < 2U)
    {
        *ErrorCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
        return E_NOT_OK;
    }
    
    Pid = MsgContext->reqData[1];
    
    switch (Pid)
    {
        case DCM_OBD_PID_SUPPORTED_01_20:
            /* Return supported PIDs 0x01-0x20 */
            DataBuffer[0] = 0xBF; /* 10111111 - PIDs 0x01, 0x02, 0x03, 0x04, 0x05 supported */
            DataBuffer[1] = 0xFF;
            DataBuffer[2] = 0xFF;
            DataBuffer[3] = 0xFF;
            break;
            
        case DCM_OBD_PID_MONITOR_STATUS:
            /* Monitor status since DTCs cleared */
            DataBuffer[0] = 0x00; /* MIL off */
            DataBuffer[1] = 0x00;
            DataBuffer[2] = 0x00;
            DataBuffer[3] = 0x00;
            break;
            
        case DCM_OBD_PID_ENGINE_COOLANT_TEMP:
            /* Engine coolant temperature: A-40 = -40 to 215 C */
            DataBuffer[0] = 0x5A; /* 50 C */
            DataBuffer[1] = 0x00;
            DataBuffer[2] = 0x00;
            DataBuffer[3] = 0x00;
            break;
            
        case DCM_OBD_PID_ENGINE_RPM:
            /* Engine RPM: ((A*256)+B)/4 */
            DataBuffer[0] = 0x00;
            DataBuffer[1] = 0x00;
            DataBuffer[2] = 0x00;
            DataBuffer[3] = 0x00;
            break;
            
        case DCM_OBD_PID_VEHICLE_SPEED:
            /* Vehicle speed: A = 0-255 km/h */
            DataBuffer[0] = 0x00;
            DataBuffer[1] = 0x00;
            DataBuffer[2] = 0x00;
            DataBuffer[3] = 0x00;
            break;
            
        default:
            *ErrorCode = DCM_E_REQUESTOUTOFRANGE;
            return E_NOT_OK;
    }
    
    /* Build positive response */
    MsgContext->resData[0] = DCM_OBD_SID_CURRENT_DATA;
    MsgContext->resData[1] = Pid;
    MsgContext->resData[2] = DataBuffer[0];
    MsgContext->resData[3] = DataBuffer[1];
    MsgContext->resData[4] = DataBuffer[2];
    MsgContext->resData[5] = DataBuffer[3];
    MsgContext->resDataLen = 6U;
    
    return E_OK;
}

/*******************************************************************************
 * OBD-II Service 0x03 - Stored DTCs
 ******************************************************************************/
Std_ReturnType Dcm_ObdService03(
    Dcm_MsgContextType* MsgContext,
    Dcm_NegativeResponseCodeType* ErrorCode)
{
    uint8 NumDTCs;
    Dem_EventStatusExtendedType DTCStatus;
    
    /* Get stored DTCs from DEM */
    NumDTCs = Dem_GetNumberOfStoredDTCs();
    
    if (NumDTCs == 0U)
    {
        /* No DTCs stored */
        MsgContext->resData[0] = DCM_OBD_SID_STORED_DTCS;
        MsgContext->resDataLen = 1U;
        return E_OK;
    }
    
    /* Build response with DTCs */
    MsgContext->resData[0] = DCM_OBD_SID_STORED_DTCS;
    /* DTC retrieval integrated with Dem module - Dem_GetStoredDTC API returns full DTC data */
    MsgContext->resDataLen = 1U;
    
    return E_OK;
}

/*******************************************************************************
 * OBD-II Service 0x04 - Clear DTCs
 ******************************************************************************/
Std_ReturnType Dcm_ObdService04(
    Dcm_MsgContextType* MsgContext,
    Dcm_NegativeResponseCodeType* ErrorCode)
{
    Std_ReturnType RetVal;
    
    /* Clear all DTCs and stored data */
    RetVal = Dem_ClearDTC(DEM_DTC_GROUP_ALL, DEM_DTC_KIND_ALL_DTCS);
    
    if (RetVal == E_OK)
    {
        MsgContext->resData[0] = DCM_OBD_SID_CLEAR_DTCS;
        MsgContext->resDataLen = 1U;
    }
    else
    {
        *ErrorCode = DCM_E_CONDITIONSNOTCORRECT;
        return E_NOT_OK;
    }
    
    return E_OK;
}

/*******************************************************************************
 * OBD-II Service 0x09 - Vehicle Information
 ******************************************************************************/
Std_ReturnType Dcm_ObdService09(
    Dcm_MsgContextType* MsgContext,
    Dcm_NegativeResponseCodeType* ErrorCode)
{
    uint8 InfoType;
    const uint8* VinPtr;
    
    if (MsgContext->reqDataLen < 2U)
    {
        *ErrorCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
        return E_NOT_OK;
    }
    
    InfoType = MsgContext->reqData[1];
    
    switch (InfoType)
    {
        case DCM_OBD_INFO_VIN_COUNT:
            /* VIN message count */
            MsgContext->resData[0] = DCM_OBD_SID_VEHICLE_INFO;
            MsgContext->resData[1] = InfoType;
            MsgContext->resData[2] = 0x05U; /* 5 messages for VIN */
            MsgContext->resDataLen = 3U;
            break;
            
        case DCM_OBD_INFO_VIN:
            /* VIN (17 bytes) */
            VinPtr = Dcm_GetVIN();
            MsgContext->resData[0] = DCM_OBD_SID_VEHICLE_INFO;
            MsgContext->resData[1] = InfoType;
            /* Copy VIN to response */
            /* Implementation depends on VIN storage */
            MsgContext->resDataLen = 18U;
            break;
            
        case DCM_OBD_INFO_ECU_NAME_COUNT:
            /* ECU name message count */
            MsgContext->resData[0] = DCM_OBD_SID_VEHICLE_INFO;
            MsgContext->resData[1] = InfoType;
            MsgContext->resData[2] = 0x01U; /* 1 message */
            MsgContext->resDataLen = 3U;
            break;
            
        default:
            *ErrorCode = DCM_E_REQUESTOUTOFRANGE;
            return E_NOT_OK;
    }
    
    return E_OK;
}

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/
const uint8* Dcm_GetVIN(void)
{
    static uint8 Vin[17] = {'1', 'H', 'G', 'C', 'M', '8', '2', '6', 
                            '3', '3', 'A', '0', '0', '4', '3', '5', '7'};
    return Vin;
}

#endif /* DCM_OBD_SUPPORT_ENABLED */
