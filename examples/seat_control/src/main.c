/**
 * @file main.c
 * @brief S32K312 Seat Control Demo — Main entry point
 * @version 1.0.0
 * @date 2026-07-12
 *
 * BSW 9-stage initialization followed by application start.
 * Main loop runs at approximately 10ms per cycle.
 *
 * Target: NXP S32K312 (Cortex-M7 @ 80MHz)
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Mcu.h"
#include "Port.h"
#include "Gpt.h"
#include "Dio.h"
#include "Pwm.h"
#include "Adc.h"
#include "Can.h"
#include "Lin.h"
#include "Fls.h"

#include "Mcu_Cfg.h"
#include "Port_Cfg.h"
#include "Gpt_Cfg.h"
#include "Dio_Cfg.h"
#include "Pwm_Cfg.h"
#include "Adc_Cfg.h"
#include "Can_Cfg.h"
#include "Lin_Cfg.h"
#include "Fls_Cfg.h"
#include "SeatControl.h"

/*==================================================================================================
 * External configuration references (defined in BSW .c config files)
 *==================================================================================================*/
extern const Mcu_ConfigType  Mcu_Config;
extern const Port_ConfigType Port_Config;
extern const Gpt_ConfigType  Gpt_Config;
extern const Dio_ConfigType  Dio_Config;
extern const Pwm_ConfigType  Pwm_Config;
extern const Adc_ConfigType  Adc_Config;
extern const Can_ConfigType  Can_Config;
extern const Lin_ConfigType  Lin_Config;
extern const Fls_ConfigType  Fls_Config;

/*==================================================================================================
 * Private function prototypes
 *==================================================================================================*/
static void Delay_Approx10ms(void);

/*==================================================================================================
 * Main Entry Point
 *==================================================================================================*/
int main(void)
{
    /*========================================================================
     * BSW 9-Stage Initialization (AUTOSAR-compliant sequence)
     *======================================================================*/

    /* Stage 1: MCU — clock configuration, lockstep, WDT */
    Mcu_Init(&Mcu_Config);
    Mcu_DistributePllClock();

    /* Stage 2: PORT — pin multiplexing and electrical characteristics */
    Port_Init(&Port_Config);

    /* Stage 3: GPT — timer channels (1ms, 10ms, 100ms) */
    Gpt_Init(&Gpt_Config);

    /* Stage 4: DIO — digital I/O for switches, LEDs, motor relays */
    Dio_Init(&Dio_Config);

    /* Stage 5: PWM — motor and heater PWM outputs */
    Pwm_Init(&Pwm_Config);

    /* Stage 6: ADC — position sensor analog inputs */
    Adc_Init(&Adc_Config);

    /* Stage 7: CAN — vehicle bus communication */
    Can_Init(&Can_Config);

    /* Stage 8: LIN — local switch panel communication */
    Lin_Init(&Lin_Config);

    /* Stage 9: Flash — NVM for seat position memory */
    Fls_Init(&Fls_Config);

    /*========================================================================
     * Application Initialization
     *======================================================================*/
    SeatControl_Init();

    /*========================================================================
     * Main Loop (~10ms cycle)
     *======================================================================*/
    while (1U)
    {
        SeatControl_MainFunction();

        /* Approximate 10ms delay (busy-wait for demo).
         * In production, this would be replaced by a GPT interrupt
         * or a real-time OS scheduler tick. */
        Delay_Approx10ms();
    }

    /* Should never reach here */
    /* return 0; */
}

/*==================================================================================================
 * Delay — approximate 10ms busy-wait
 *==================================================================================================*/
static void Delay_Approx10ms(void)
{
    /* Calibrated for 80MHz Cortex-M7:
     * Each loop iteration ≈ 3 cycles (load+cmp+bne)
     * ~80000000 * 0.01 / 3 ≈ 266666 iterations for 10ms
     * Using 200000 for safety margin */
    volatile uint32 count;
    for (count = 0U; count < 200000U; count++)
    {
        /* Wait */
    }
}
