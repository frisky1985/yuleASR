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
 * @file Dcm_Obd.h
 * @brief DCM OBD-II Service interface
 */

#ifndef DCM_OBD_H
#define DCM_OBD_H

#include "Dcm.h"

/* OBD-II Service IDs */
#define DCM_OBD_SID_CURRENT_DATA            (0x41U) /* Service 0x01 response */
#define DCM_OBD_SID_STORED_DTCS             (0x43U) /* Service 0x03 response */
#define DCM_OBD_SID_CLEAR_DTCS              (0x44U) /* Service 0x04 response */
#define DCM_OBD_SID_VEHICLE_INFO            (0x49U) /* Service 0x09 response */

/* OBD-II PIDs for Service 0x01 */
#define DCM_OBD_PID_SUPPORTED_01_20         (0x00U)
#define DCM_OBD_PID_MONITOR_STATUS          (0x01U)
#define DCM_OBD_PID_FREEZE_DTC              (0x02U)
#define DCM_OBD_PID_FUEL_SYSTEM_STATUS      (0x03U)
#define DCM_OBD_PID_ENGINE_LOAD             (0x04U)
#define DCM_OBD_PID_ENGINE_COOLANT_TEMP     (0x05U)
#define DCM_OBD_PID_SHORT_TERM_FUEL_TRIM    (0x06U)
#define DCM_OBD_PID_LONG_TERM_FUEL_TRIM     (0x07U)
#define DCM_OBD_PID_INTAKE_MANIFOLD_PRESS   (0x0BU)
#define DCM_OBD_PID_ENGINE_RPM              (0x0CU)
#define DCM_OBD_PID_VEHICLE_SPEED           (0x0DU)
#define DCM_OBD_PID_TIMING_ADVANCE          (0x0EU)
#define DCM_OBD_PID_INTAKE_AIR_TEMP         (0x0FU)
#define DCM_OBD_PID_MAF_RATE                (0x10U)
#define DCM_OBD_PID_THROTTLE_POSITION       (0x11U)
#define DCM_OBD_PID_OBD_STANDARDS           (0x1CU)
#define DCM_OBD_PID_RUN_TIME_ENGINE_START   (0x1FU)

/* OBD-II Info Types for Service 0x09 */
#define DCM_OBD_INFO_VIN_COUNT              (0x01U)
#define DCM_OBD_INFO_VIN                    (0x02U)
#define DCM_OBD_INFO_CALIBRATION_ID_COUNT   (0x03U)
#define DCM_OBD_INFO_CALIBRATION_ID         (0x04U)
#define DCM_OBD_INFO_ECU_NAME_COUNT         (0x09U)
#define DCM_OBD_INFO_ECU_NAME               (0x0AU)

/* Function prototypes */
Std_ReturnType Dcm_ObdService01(Dcm_MsgContextType* MsgContext, Dcm_NegativeResponseCodeType* ErrorCode);
Std_ReturnType Dcm_ObdService03(Dcm_MsgContextType* MsgContext, Dcm_NegativeResponseCodeType* ErrorCode);
Std_ReturnType Dcm_ObdService04(Dcm_MsgContextType* MsgContext, Dcm_NegativeResponseCodeType* ErrorCode);
Std_ReturnType Dcm_ObdService09(Dcm_MsgContextType* MsgContext, Dcm_NegativeResponseCodeType* ErrorCode);

const uint8* Dcm_GetVIN(void);

#endif /* DCM_OBD_H */
