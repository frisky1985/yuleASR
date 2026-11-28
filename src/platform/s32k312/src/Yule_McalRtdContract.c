/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Yule_McalRtdContract.c
 * @brief MCAL -> NXP S32K3 RTD adaptation contract table
 */

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "Yule_McalRtdContract.h"

#include <string.h>

/*==================================================================================================
*                                        GLOBAL CONSTANTS
==================================================================================================*/
const Yule_McalRtdModuleContractType Yule_McalRtdContract[YULE_MCAL_RTD_MODULE_COUNT] = {
    { "adc",    "Adc_Ip",        6U,  YULE_RTD_HEADERS_ONLY },
    { "can",    "Can_Ip",       14U,  YULE_RTD_HEADERS_ONLY },
    { "crypto", "Crypto_Ip",    24U,  YULE_RTD_ADAPTER_READY },
    { "dio",    "Dio_Ip",        4U,  YULE_RTD_HEADERS_ONLY },
    { "eep",    "Eep_Ip",        6U,  YULE_RTD_ADAPTER_READY },
    { "eth",    "Eth_Ip",       10U,  YULE_RTD_HEADERS_ONLY },
    { "flash",  "Fls_Ip",       14U,  YULE_RTD_ADAPTER_READY },
    { "fls",    "Fls_Ip",       12U,  YULE_RTD_ADAPTER_READY },
    { "gpt",    "Gpt_Ip",        8U,  YULE_RTD_HEADERS_ONLY },
    { "i2c",    "I2c_Ip",        8U,  YULE_RTD_HEADERS_ONLY },
    { "icu",    "Icu_Ip",        7U,  YULE_RTD_HEADERS_ONLY },
    { "lin",    "Lin_Ip",       10U,  YULE_RTD_HEADERS_ONLY },
    { "mcu",    "Mcu_Ip",       12U,  YULE_RTD_HEADERS_ONLY },
    { "ocu",    "Ocu_Ip",        8U,  YULE_RTD_HEADERS_ONLY },
    { "port",   "Port_Ip",       5U,  YULE_RTD_HEADERS_ONLY },
    { "pwm",    "Pwm_Ip",        6U,  YULE_RTD_HEADERS_ONLY },
    { "ramtst", "RamTst_Ip",     9U,  YULE_RTD_HEADERS_ONLY },
    { "spi",    "Spi_Ip",        9U,  YULE_RTD_HEADERS_ONLY },
    { "uart",   "Uart_Ip",      10U,  YULE_RTD_HEADERS_ONLY },
    { "wdg",    "Wdg_Ip",        5U,  YULE_RTD_HEADERS_ONLY }
};

/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
const Yule_McalRtdModuleContractType* Yule_McalRtd_GetModule(const char* moduleName)
{
    uint32 i;

    if (moduleName == NULL_PTR)
    {
        return NULL_PTR;
    }
    for (i = 0U; i < YULE_MCAL_RTD_MODULE_COUNT; i++)
    {
        if (strcmp(Yule_McalRtdContract[i].moduleName, moduleName) == 0)
        {
            return &Yule_McalRtdContract[i];
        }
    }
    return NULL_PTR;
}

boolean Yule_McalRtd_IsDriverAvailable(const char* moduleName)
{
    const Yule_McalRtdModuleContractType* module = Yule_McalRtd_GetModule(moduleName);

    return (boolean)((module != NULL_PTR) && (module->status == YULE_RTD_DRIVER_READY));
}

uint8 Yule_McalRtd_GetReadyModuleCount(void)
{
    uint32 i;
    uint8 count = 0U;

    for (i = 0U; i < YULE_MCAL_RTD_MODULE_COUNT; i++)
    {
        if (Yule_McalRtdContract[i].status == YULE_RTD_DRIVER_READY)
        {
            count++;
        }
    }
    return count;
}

const char* Yule_McalRtd_GetStatusString(Yule_McalRtdStatusType status)
{
    const char* text;

    switch (status)
    {
        case YULE_RTD_ADAPTER_READY:
            text = "adapter-ready";
            break;
        case YULE_RTD_DRIVER_READY:
            text = "driver-ready";
            break;
        case YULE_RTD_HEADERS_ONLY:
        default:
            text = "headers-only";
            break;
    }
    return text;
}

/*==================[end of file]===========================================*/
