/**
 * @file   Bcm_Light.c
 * @brief  BCM Light Control — Runnable implementations
 * @note   Template file. Fill in SWC logic after RTE generation.
 *
 * AutoSAR SWC: BCM_Light
 * Runnable Interfaces:
 *   - BCM_Light_Control()     : Control lights based on switch input
 *
 * RTE APIs:
 *   - Rte_Read_BCM_Light_LightSwitch_R_LightSwitch(&sw)
 *   - Rte_Write_BCM_Light_LightOutput_P_LightOutput(&output)
 */
#include "Rte_BCM_Light.h"

/* ─── Light Switch Positions ────────────────────────────────────────────── */
#define LIGHT_SWITCH_OFF        0U
#define LIGHT_SWITCH_PARKING    1U
#define LIGHT_SWITCH_LOW_BEAM   2U
#define LIGHT_SWITCH_HIGH_BEAM  3U
#define LIGHT_SWITCH_AUTO       4U

/* ─── Light Output Bitmask ──────────────────────────────────────────────── */
#define LIGHT_OUTPUT_NONE       0x0000U
#define LIGHT_OUTPUT_PARKING    0x0001U
#define LIGHT_OUTPUT_LOW_BEAM   0x0002U
#define LIGHT_OUTPUT_HIGH_BEAM  0x0004U
#define LIGHT_OUTPUT_FOG_FRONT  0x0008U
#define LIGHT_OUTPUT_FOG_REAR   0x0010U
#define LIGHT_OUTPUT_DRL        0x0020U
#define LIGHT_OUTPUT_TURN_L    0x0040U
#define LIGHT_OUTPUT_TURN_R    0x0080U
#define LIGHT_OUTPUT_BRAKE     0x0100U
#define LIGHT_OUTPUT_REVERSE   0x0200U
#define LIGHT_OUTPUT_INTERIOR  0x0400U

/* ==========================================================================
 *  Runnable: BCM_Light_Control
 *  ──────────────────────────────
 *  Period: 20 ms
 *  Reads light switch and drives light outputs.
 * ========================================================================== */
void BCM_Light_Control(void)
{
    uint8 lightSwitch = LIGHT_SWITCH_OFF;
    uint16 lightOutput = LIGHT_OUTPUT_NONE;
    Std_ReturnType ret;

    /* Read light switch position */
    ret = Rte_Read_BCM_Light_LightSwitch_R_LightSwitch(&lightSwitch);
    if (ret != RTE_E_OK)
    {
        /* Keep last output state on error */
        return;
    }

    /* Map switch position to light output */
    switch (lightSwitch)
    {
        case LIGHT_SWITCH_OFF:
            lightOutput = LIGHT_OUTPUT_NONE;
            break;

        case LIGHT_SWITCH_PARKING:
            lightOutput = LIGHT_OUTPUT_PARKING | LIGHT_OUTPUT_DRL;
            break;

        case LIGHT_SWITCH_LOW_BEAM:
            lightOutput = LIGHT_OUTPUT_LOW_BEAM | LIGHT_OUTPUT_PARKING
                        | LIGHT_OUTPUT_DRL;
            break;

        case LIGHT_SWITCH_HIGH_BEAM:
            lightOutput = LIGHT_OUTPUT_HIGH_BEAM | LIGHT_OUTPUT_LOW_BEAM
                        | LIGHT_OUTPUT_PARKING;
            break;

        case LIGHT_SWITCH_AUTO:
        default:
            /* Auto mode: logic depends on ambient light sensor */
            lightOutput = LIGHT_OUTPUT_DRL; /* Default to DRL */
            break;
    }

    /* Write light output to actuator */
    (void)Rte_Write_BCM_Light_LightOutput_P_LightOutput(&lightOutput);
}
