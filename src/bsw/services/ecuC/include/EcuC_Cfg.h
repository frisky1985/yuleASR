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
 * @file EcuC_Cfg.h
 * @brief EcuC Configuration Header
 * @version 1.0.0
 * @date 2024-05-05
 */

#ifndef ECUC_CFG_H
#define ECUC_CFG_H

/*==================[Configuration Switches]================================*/

/* Development Error Detection */
#define ECUC_DEV_ERROR_DETECT              STD_ON

/* Version Info API */
#define ECUC_VERSION_INFO_API              STD_ON

/* Main Function Period [ms] */
#define ECUC_MAIN_FUNCTION_PERIOD_MS       10U

/*==================[General Configuration]=================================*/

/* Maximum number of PDUs */
#define ECUC_MAX_PDUS                      32U

/* Maximum number of Signals */
#define ECUC_MAX_SIGNALS                   128U

/* Maximum number of Routing Paths */
#define ECUC_MAX_ROUTING_PATHS             16U

/* Maximum PDU Length */
#define ECUC_MAX_PDU_LENGTH                256U

/* Maximum Signal Size in bits */
#define ECUC_MAX_SIGNAL_SIZE               64U

/*==================[Gateway Configuration]=================================*/

/* Gateway Operation Mode */
#define ECUC_GATEWAY_DIRECT                STD_ON
#define ECUC_GATEWAY_INDIRECT              STD_OFF

/* Signal Value Adaptation */
#define ECUC_SIGNAL_ADAPTATION_ENABLED     STD_ON

/*==================[Error Codes]===========================================*/

#ifndef ECUC_E_NO_ERROR
#define ECUC_E_NO_ERROR                    0x00U
#endif

#ifndef ECUC_E_PARAM_POINTER
#define ECUC_E_PARAM_POINTER               0x01U
#endif

/*==================[Version Information]===================================*/

#define ECUC_VENDOR_ID_VALUE               0x0001U
#define ECUC_MODULE_ID_VALUE               150U

#define ECUC_SW_MAJOR_VERSION_VALUE        1U
#define ECUC_SW_MINOR_VERSION_VALUE        0U
#define ECUC_SW_PATCH_VERSION_VALUE        0U

#endif /* ECUC_CFG_H */
