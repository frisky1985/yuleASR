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
 * @file FiM_Cfg.h
 * @brief Function Inhibition Manager configuration header
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FIM_CFG_H
#define FIM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define FIM_DEV_ERROR_DETECT                    (STD_ON)
#define FIM_VERSION_INFO_API                    (STD_ON)
#define FIM_INHIBITION_CONFIGURATION_SUPPORTED  (STD_ON)

/*==================================================================================================
*                                    FUNCTION CONFIGURATION
==================================================================================================*/
#define FIM_NUM_FUNCTIONS                       (32U)
#define FIM_NUM_EVENTS_PER_FUNCTION             (8U)
#define FIM_NUM_SUMMARY_EVENTS                  (16U)

/*==================================================================================================
*                                    FUNCTION IDs
==================================================================================================*/
#define FIM_FID_MIN                             ((FiM_FunctionIdType)1U)
#define FIM_FID_MAX                             ((FiM_FunctionIdType)31U)
#define FIM_FID_INVALID                         ((FiM_FunctionIdType)0U)

/*==================================================================================================
*                                    SUMMARY EVENT IDs
==================================================================================================*/
#define FIM_SUMMARY_EVENT_ID_MIN                ((FiM_SummaryEventIdType)1U)
#define FIM_SUMMARY_EVENT_ID_MAX                ((FiM_SummaryEventIdType)15U)
#define FIM_SUMMARY_EVENT_ID_INVALID            ((FiM_SummaryEventIdType)0U)

/*==================================================================================================
*                                    INHIBITION CONFIGURATION MASKS
==================================================================================================*/
#define FIM_INHIBIT_NONE                        (0x00U)
#define FIM_INHIBIT_IF_TEST_FAILED              (DEM_UDS_STATUS_TF)
#define FIM_INHIBIT_IF_TEST_FAILED_TOC          (DEM_UDS_STATUS_TFTOC)
#define FIM_INHIBIT_IF_PENDING                  (DEM_UDS_STATUS_PDTC)
#define FIM_INHIBIT_IF_CONFIRMED                (DEM_UDS_STATUS_CDTC)
#define FIM_INHIBIT_IF_TEST_NOT_COMPLETED_TOC   (DEM_UDS_STATUS_TNCTOC)
#define FIM_INHIBIT_IF_TEST_NOT_COMPLETED_SLC   (DEM_UDS_STATUS_TNCSLC)
#define FIM_INHIBIT_IF_TEST_FAILED_SLC          (DEM_UDS_STATUS_TFSLC)
#define FIM_INHIBIT_IF_WARNING_INDICATOR        (DEM_UDS_STATUS_WIR)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define FIM_MAIN_FUNCTION_PERIOD_MS             (10U)

/*==================================================================================================
*                                    DEFAULT PERMISSIONS
==================================================================================================*/
#define FIM_DEFAULT_PERMISSION                  (FIM_PERMISSION_ALLOWED)
#define FIM_DEFAULT_AVAILABILITY                (TRUE)

#endif /* FIM_CFG_H */
