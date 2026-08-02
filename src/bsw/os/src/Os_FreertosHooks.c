/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : FreeRTOS-Kernel V11.1.0
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Os_FreertosHooks.c
 * @brief FreeRTOS required hook implementations for static allocation
 * @version 1.0.0
 * @date 2026-08-01
 *
 * FreeRTOSConfig.h enables configSUPPORT_STATIC_ALLOCATION, so the kernel
 * requires the application to provide memory buffers for the Idle task and
 * the software-timer task. Without these two hooks the link fails with
 * undefined references to vApplicationGetIdleTaskMemory /
 * vApplicationGetTimerTaskMemory.
 */

#include "FreeRTOS.h"
#include "task.h"

/*==================================================================================================
*                                  IDLE TASK MEMORY
*==================================================================================================*/

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleTaskStackBuffer[configMINIMAL_STACK_SIZE];

/**
 * @brief Provide memory for the FreeRTOS Idle task (static allocation)
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *puxIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = xIdleTaskStackBuffer;
    *puxIdleTaskStackSize = (configSTACK_DEPTH_TYPE)configMINIMAL_STACK_SIZE;
}

/*==================================================================================================
*                                  TIMER TASK MEMORY
*==================================================================================================*/

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t  xTimerTaskStackBuffer[configTIMER_TASK_STACK_DEPTH];

/**
 * @brief Provide memory for the FreeRTOS software-timer task (static allocation)
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *puxTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = xTimerTaskStackBuffer;
    *puxTimerTaskStackSize = (configSTACK_DEPTH_TYPE)configTIMER_TASK_STACK_DEPTH;
}

/*==================================================================================================
*                                  ASSERT HOOK
*==================================================================================================*/

/**
 * @brief Called when configASSERT fails - trap for debug.
 *        Only referenced when configASSERT is redirected to this hook.
 */
void vAssertCalled(const char *pcFile, uint32_t ulLine)
{
    (void)pcFile;
    (void)ulLine;
    for (;;)
    {
        /* Trap */
    }
}

/*==================================================================================================
*                                       END OF FILE
*==================================================================================================*/
