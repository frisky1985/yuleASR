/**
 * @file   Bcm_Door.c
 * @brief  BCM Door Control — Runnable implementations
 * @note   Template file. Fill in SWC logic after RTE generation.
 *
 * AutoSAR SWC: BCM_Door
 * Runnable Interfaces:
 *   - BCM_Door_Monitor()     : Monitor door status sensors
 *   - BCM_Door_ProcessLock() : Process lock/unlock commands
 *
 * RTE APIs:
 *   - Rte_Read_BCM_Door_DoorStatus_R_DoorStatus(&status)
 *   - Rte_Write_BCM_Door_DoorLock_P_LockCommand(&cmd)
 *   - Rte_Read_BCM_Door_LightSwitch_R_LightSwitch(&sw)
 */
#include "Rte_BCM_Door.h"

/* ─── Door Status Enum ──────────────────────────────────────────────────── */
#define DOOR_STATUS_CLOSED      0U
#define DOOR_STATUS_OPEN        1U
#define DOOR_STATUS_AJAR        2U
#define DOOR_STATUS_ERROR       0xFFU

/* ─── Lock State Enum ───────────────────────────────────────────────────── */
#define LOCK_STATE_UNLOCKED     0U
#define LOCK_STATE_LOCKED       1U
#define LOCK_STATE_SUPERLOCKED  2U

/* ─── Local State ───────────────────────────────────────────────────────── */
STATIC uint8 BcmDoor_CurrentLockState = LOCK_STATE_UNLOCKED;

/* ==========================================================================
 *  Runnable: BCM_Door_Monitor
 *  ────────────────────────────
 *  Period: 50 ms
 *  Reads door status sensor and determines lock actions.
 * ========================================================================== */
void BCM_Door_Monitor(void)
{
    uint8 doorStatus = DOOR_STATUS_CLOSED;
    uint8 lightSwitch = 0U;
    Std_ReturnType ret;

    /* Read door status from sensor */
    ret = Rte_Read_BCM_Door_DoorStatus_R_DoorStatus(&doorStatus);
    if (ret != RTE_E_OK)
    {
        /* No new data — use last known state */
        return;
    }

    /* Read light switch for auto-lock integration */
    (void)Rte_Read_BCM_Door_LightSwitch_R_LightSwitch(&lightSwitch);

    /* Logic example: auto-lock when door closed + ignition on */
    if (doorStatus == DOOR_STATUS_CLOSED)
    {
        if (BcmDoor_CurrentLockState == LOCK_STATE_UNLOCKED)
        {
            /* Auto-lock after delay (simplified) */
            uint8 lockCmd = LOCK_STATE_LOCKED;
            (void)Rte_Write_BCM_Door_DoorLock_P_LockCommand(&lockCmd);
            BcmDoor_CurrentLockState = LOCK_STATE_LOCKED;
        }
    }
    else if (doorStatus == DOOR_STATUS_OPEN)
    {
        /* Unlock when door opened */
        if (BcmDoor_CurrentLockState != LOCK_STATE_UNLOCKED)
        {
            uint8 unlockCmd = LOCK_STATE_UNLOCKED;
            (void)Rte_Write_BCM_Door_DoorLock_P_LockCommand(&unlockCmd);
            BcmDoor_CurrentLockState = LOCK_STATE_UNLOCKED;
        }
    }
}

/* ==========================================================================
 *  Runnable: BCM_Door_ProcessLock
 *  ────────────────────────────────
 *  Period: 10 ms
 *  Processes explicit lock/unlock commands from other SWCs.
 * ========================================================================== */
void BCM_Door_ProcessLock(void)
{
    /* TODO: Implement lock command processing */
    /* Reads lock requests from CAN/LIN and drives lock actuator */
    /* Rte_Write_BCM_Door_DoorLock_P_LockCommand(&lockCmd); */
}
