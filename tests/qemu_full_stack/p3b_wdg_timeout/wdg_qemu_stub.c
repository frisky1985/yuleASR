/*
 * wdg_qemu_stub.c - C10: QEMU WDG hardware stub (CMSDK Watchdog simulation)
 *
 * All state consolidated in a single struct to prevent compiler from
 * splitting volatile statics across .data and .bss sections.
 */
#include <stdint.h>
#include "Uart_Cfg.h"

#define WDGIF_OFF_MODE      0U
#define WDGIF_RUN_MODE      1U

#define WDGM_GLOBAL_MODE_OK       0U
#define WDGM_GLOBAL_MODE_DEGRADED 1U
#define WDGM_GLOBAL_MODE_STOPPED  2U

#define WDGM_SUP_OK      0U
#define WDGM_SUP_FAILED  1U

static volatile struct {
    uint8_t  wdg_mode;
    uint32_t wdg_counter;
    uint32_t wdg_timeout_cnt;
    uint32_t wdg_trigger;
    uint8_t  wdgm_mode;
    uint8_t  wdgm_sup;
    uint32_t checkpoint_cnt;
    uint32_t checkpoint_tgt;
    uint32_t missed;
    uint8_t  off_mode_called;
} s = {
    .wdg_mode = WDGIF_OFF_MODE,
    .wdg_counter = 0U,
    .wdg_timeout_cnt = 0U,
    .wdg_trigger = 1000U,
    .wdgm_mode = WDGM_GLOBAL_MODE_OK,
    .wdgm_sup = WDGM_SUP_OK,
    .checkpoint_cnt = 0U,
    .checkpoint_tgt = 10U,
    .missed = 0U,
    .off_mode_called = 0U,
};

__attribute__((noinline)) void Wdg_Init(const void *cfg)
{
    (void)cfg;
    s.wdg_mode = WDGIF_RUN_MODE;
    s.wdg_counter = s.wdg_trigger;
}

__attribute__((noinline)) void Wdg_SetMode(uint8_t mode)
{
    s.wdg_mode = mode;
    if (mode == WDGIF_OFF_MODE) { s.off_mode_called = 1U; }
}

__attribute__((noinline)) void Wdg_SetTriggerCondition(uint32_t ticks)
{
    s.wdg_counter = ticks;
    s.wdg_trigger = ticks;
}

__attribute__((noinline)) void WdgM_Init(void)
{
    s.wdgm_mode = WDGM_GLOBAL_MODE_OK;
    s.wdgm_sup  = WDGM_SUP_OK;
    s.checkpoint_cnt = 0U;
    s.missed = 0U;
}

__attribute__((noinline)) void WdgM_CheckpointReached(uint32_t checkpoint)
{
    (void)checkpoint;
    s.checkpoint_cnt++;
}

__attribute__((noinline)) void WdgM_UpdateAliveSupervision(void)
{
    if (s.checkpoint_cnt >= s.checkpoint_tgt)
    {
        s.wdgm_sup = WDGM_SUP_OK;
        if (s.wdg_mode == WDGIF_RUN_MODE)
        {
            Wdg_SetTriggerCondition(s.wdg_trigger);
        }
    }
    else
    {
        s.missed++;
        if (s.missed >= 2U) { s.wdgm_sup = WDGM_SUP_FAILED; }
    }
    s.checkpoint_cnt = 0U;
}

uint8_t WdgM_GetGlobalStatus(void) { return s.wdgm_mode; }

void WdgM_SetMode(uint8_t mode)
{
    s.wdgm_mode = mode;
    if (mode == WDGM_GLOBAL_MODE_STOPPED)
    {
        Wdg_SetMode(WDGIF_OFF_MODE);
    }
}

uint32_t WdgStub_GetTimeoutCount(void) { return s.wdg_timeout_cnt; }
uint32_t WdgStub_GetCounter(void) { return s.wdg_counter; }
uint32_t WdgStub_GetCheckpointTarget(void) { return s.checkpoint_tgt; }
uint32_t WdgStub_GetCheckpointCount(void) { return s.checkpoint_cnt; }
uint8_t  WdgStub_GetOffModeCalled(void) { return s.off_mode_called; }
uint8_t  WdgStub_GetSupervisionStatus(void) { return s.wdgm_sup; }
uint32_t WdgStub_GetMissedCount(void) { return s.missed; }

void WdgStub_Tick(void)
{
    if (s.wdg_mode != WDGIF_RUN_MODE) return;
    if (s.wdg_counter > 0U) { s.wdg_counter--; }
    if (s.wdg_counter == 0U) { s.wdg_timeout_cnt++; }
}

void WdgStub_SetCheckpointTarget(uint32_t target) { s.checkpoint_tgt = target; }
void WdgStub_ResetTimeoutCount(void) { s.wdg_timeout_cnt = 0U; }
