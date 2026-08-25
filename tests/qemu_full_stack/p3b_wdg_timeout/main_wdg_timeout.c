/*
 * main_wdg_timeout.c - C10: QEMU WDG Timeout Reset Verification
 *
 * Scenarios:
 *   S10.1 NormalFeeding    - feed WDG for 100 cycles, no timeout
 *   S10.2 TimeoutReset     - stop feeding, WDG counter expires
 *   S10.3 WdgMSupervision  - miss checkpoints, WdgM detects failed supervision
 *   S10.4 WdgMModeSwitch   - SetMode(STOPPED), WDG enters OFF mode
 */
#include "FreeRTOS.h"
#include "task.h"
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

extern void     Wdg_Init(const void *cfg);
extern void     Wdg_SetMode(uint8_t mode);
extern void     Wdg_SetTriggerCondition(uint32_t ticks);
extern void     WdgM_Init(void);
extern void     WdgM_CheckpointReached(uint32_t checkpoint);
extern void     WdgM_UpdateAliveSupervision(void);
extern uint8_t  WdgM_GetGlobalStatus(void);
extern void     WdgM_SetMode(uint8_t mode);
extern uint32_t WdgStub_GetTimeoutCount(void);
extern uint8_t  WdgStub_GetOffModeCalled(void);
extern uint8_t  WdgStub_GetSupervisionStatus(void);
extern uint32_t WdgStub_GetMissedCount(void);
extern void     WdgStub_Tick(void);
extern void     WdgStub_SetCheckpointTarget(uint32_t target);
extern void     WdgStub_ResetTimeoutCount(void);

#define WDGM_GLOBAL_MODE_OK       0U
#define WDGM_GLOBAL_MODE_STOPPED  2U
#define WDGM_SUP_OK      0U
#define WDGM_SUP_FAILED  1U

static void vWdgTask(void *pv)
{
    (void)pv;
    Uart_WriteString("=== C10 WDG Timeout ===\n");

    /* S10.1: Normal feeding — 100 cycles, no timeout */
    {
        Wdg_Init(NULL);
        WdgM_Init();
        WdgStub_SetCheckpointTarget(5U);
        WdgStub_ResetTimeoutCount();

        for (int i = 0; i < 100; i++)
        {
            WdgM_CheckpointReached(0U);
            WdgM_UpdateAliveSupervision();
            WdgStub_Tick();
        }
        uint32_t to_cnt = WdgStub_GetTimeoutCount();
        Uart_WriteString("S10.1 timeout_count=");
        Uart_WriteDec(to_cnt);
        Uart_WriteString("\n");
        Qemu_Assert(to_cnt == 0U, "S10.1: unexpected WDG timeout during normal feeding");
    }

    /* S10.2: Timeout reset — stop feeding, WDG expires */
    {
        Wdg_Init(NULL);
        Wdg_SetTriggerCondition(50U);
        WdgStub_ResetTimeoutCount();

        for (int i = 0; i < 60; i++)
        {
            WdgStub_Tick();
        }
        uint32_t to_cnt = WdgStub_GetTimeoutCount();
        Uart_WriteString("S10.2 timeout_count=");
        Uart_WriteDec(to_cnt);
        Uart_WriteString("\n");
        Qemu_Assert(to_cnt > 0U, "S10.2: WDG did not timeout after feeding stopped");
    }

    /* S10.3: WdgM supervision — miss checkpoints, supervision fails */
    {
        Wdg_Init(NULL);
        WdgM_Init();
        WdgStub_SetCheckpointTarget(10U);

        for (int i = 0; i < 5; i++)
        {
            WdgM_UpdateAliveSupervision();
        }
        uint8_t sup = WdgStub_GetSupervisionStatus();
        uint32_t missed = WdgStub_GetMissedCount();
        Uart_WriteString("S10.3 sup_status=");
        Uart_WriteDec((uint32_t)sup);
        Uart_WriteString(" missed=");
        Uart_WriteDec(missed);
        Uart_WriteString("\n");
        Qemu_Assert(sup == WDGM_SUP_FAILED, "S10.3: WdgM supervision not failed");
        Qemu_Assert(missed >= 2U, "S10.3: missed count too low");
    }

    /* S10.4: WdgM mode switch — SetMode(STOPPED), WDG enters OFF */
    {
        WdgM_SetMode(WDGM_GLOBAL_MODE_STOPPED);
        uint8_t off_called = WdgStub_GetOffModeCalled();
        Uart_WriteString("S10.4 off_mode_called=");
        Uart_WriteDec((uint32_t)off_called);
        Uart_WriteString("\n");
        Qemu_Assert(off_called == 1U, "S10.4: WDG OFF mode not called");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("WDG_TIMEOUT_START\n");
    xTaskCreate(vWdgTask, "Wdg", 512, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
