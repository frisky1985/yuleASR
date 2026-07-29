/**
 * @file   Bcm_Wiper.c
 * @brief  BCM Wiper Control — Runnable implementations
 * @note   Template file. Fill in SWC logic after RTE generation.
 *
 * AutoSAR SWC: BCM_Wiper
 * Runnable Interfaces:
 *   - BCM_Wiper_Control()     : Control wiper speed/direction
 *
 * RTE APIs:
 *   - Rte_Read_BCM_Wiper_WiperSpeed_R_WiperSpeed(&speed)
 *   - Rte_Write_BCM_Wiper_WiperCtrl_P_WiperControl(&ctrl)
 */
#include "Rte_BCM_Wiper.h"

/* ─── Wiper Speed Settings ──────────────────────────────────────────────── */
#define WIPER_OFF           0U
#define WIPER_INTERMITTENT  1U
#define WIPER_LOW           2U
#define WIPER_HIGH          3U
#define WIPER_WASH          4U

/* ─── Wiper Control Commands ────────────────────────────────────────────── */
#define WIPER_CTRL_STOP     0U
#define WIPER_CTRL_LOW      1U
#define WIPER_CTRL_HIGH     2U
#define WIPER_CTRL_WASH     3U
#define WIPER_CTRL_PARK     4U

/* ==========================================================================
 *  Runnable: BCM_Wiper_Control
 *  ──────────────────────────────
 *  Period: 20 ms
 *  Reads wiper speed setting and drives wiper motor.
 * ========================================================================== */
void BCM_Wiper_Control(void)
{
    uint8 wiperSpeed = WIPER_OFF;
    uint8 wiperCtrl = WIPER_CTRL_STOP;
    Std_ReturnType ret;

    /* Read wiper speed setting */
    ret = Rte_Read_BCM_Wiper_WiperSpeed_R_WiperSpeed(&wiperSpeed);
    if (ret != RTE_E_OK)
    {
        /* No new data — keep current wiper state */
        return;
    }

    /* Map speed setting to control command */
    switch (wiperSpeed)
    {
        case WIPER_OFF:
            wiperCtrl = WIPER_CTRL_PARK;
            break;

        case WIPER_INTERMITTENT:
            wiperCtrl = WIPER_CTRL_LOW;
            /* In production: implement intermittent timing */
            break;

        case WIPER_LOW:
            wiperCtrl = WIPER_CTRL_LOW;
            break;

        case WIPER_HIGH:
            wiperCtrl = WIPER_CTRL_HIGH;
            break;

        case WIPER_WASH:
            wiperCtrl = WIPER_CTRL_WASH;
            break;

        default:
            wiperCtrl = WIPER_CTRL_STOP;
            break;
    }

    /* Write wiper control signal */
    (void)Rte_Write_BCM_Wiper_WiperCtrl_P_WiperControl(&wiperCtrl);
}
