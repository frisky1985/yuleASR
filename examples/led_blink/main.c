/**
 * @file main.c
 * @brief LED Blink Example
 * @version 1.0.0
 * 
 * This example demonstrates basic usage of DIO and GPT drivers
 * to create a simple LED blinking application.
 */

#include "Dio.h"
#include "Gpt.h"
#include "Mcu.h"

#define LED_PIN     DioConf_DioChannel_LED
#define DELAY_MS    500

int main(void)
{
    /* Initialize MCU */
    Mcu_Init(&Mcu_Config);
    
    /* Initialize GPT */
    Gpt_Init(&Gpt_Config);
    
    /* Initialize DIO */
    Dio_Init(&Dio_Config);
    
    /* Main loop */
    while (1)
    {
        /* Turn LED on */
        Dio_WriteChannel(LED_PIN, STD_HIGH);
        Gpt_StartTimer(DELAY_MS);
        
        /* Turn LED off */
        Dio_WriteChannel(LED_PIN, STD_LOW);
        Gpt_StartTimer(DELAY_MS);
    }
    
    return 0;
}
