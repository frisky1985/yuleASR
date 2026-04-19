/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Example              : LED Blink
* Platform             : Generic Cortex-M
*
* Description          : Simple LED blink example using Dio and Gpt drivers
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-19
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "Mcu.h"
#include "Port.h"
#include "Dio.h"
#include "Gpt.h"

/* LED configuration */
#define LED_PORT        DioConf_DioPort_A
#define LED_CHANNEL     DioConf_DioChannel_A0
#define LED_PIN         0

/* Blink period in milliseconds */
#define BLINK_PERIOD_MS 500

/*==================================================================================================
*                                     GLOBAL VARIABLES
==================================================================================================*/
static volatile uint32 g_tickCount = 0;
static volatile boolean g_ledState = FALSE;

/*==================================================================================================
*                                     CALLBACK FUNCTIONS
==================================================================================================*/
void Gpt_Callback(void)
{
    g_tickCount++;
    
    /* Toggle LED every BLINK_PERIOD_MS */
    if (g_tickCount >= BLINK_PERIOD_MS)
    {
        g_tickCount = 0;
        g_ledState = !g_ledState;
        
        if (g_ledState)
        {
            Dio_WriteChannel(LED_CHANNEL, STD_HIGH);
        }
        else
        {
            Dio_WriteChannel(LED_CHANNEL, STD_LOW);
        }
    }
}

/*==================================================================================================
*                                     MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    Mcu_ConfigType mcuConfig;
    Port_ConfigType portConfig;
    Gpt_ConfigType gptConfig;
    
    /* Initialize Mcu */
    mcuConfig.ClockSettings = NULL;
    mcuConfig.RamSectorSettings = NULL;
    mcuConfig.NumClockSettings = 0;
    mcuConfig.NumRamSectors = 0;
    Mcu_Init(&mcuConfig);
    
    /* Initialize Port */
    portConfig.Pins = NULL;
    portConfig.NumPins = 0;
    Port_Init(&portConfig);
    
    /* Initialize Dio */
    Dio_Init(NULL);
    
    /* Initialize Gpt */
    gptConfig.Channels = NULL;
    gptConfig.NumChannels = 0;
    Gpt_Init(&gptConfig);
    
    /* Start Gpt timer with 1ms period */
    Gpt_StartTimer(GptConf_GptChannel_0, 1000);
    Gpt_EnableNotification(GptConf_GptChannel_0);
    
    /* Main loop */
    while (1)
    {
        /* Background tasks can be added here */
        Mcu_PerformReset();
    }
    
    return 0;
}
