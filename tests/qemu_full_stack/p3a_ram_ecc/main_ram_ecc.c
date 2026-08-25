/*
 * main_ram_ecc.c - C9: QEMU RAM ECC Fault Injection Verification
 *
 * Scenarios:
 *   S9.1 SecFaultCorrected  - 1-bit flip: RamSafety corrects, state stays OK
 *   S9.2 DedFaultFatal      - 2-bit flip: EnterSafeState called, state = FAILED
 *   S9.3 SafeRamWriteProtect - March C- verification of protected region
 *   S9.4 SecThresholdDegrade - 5+ SEC errors: count exceeds threshold (5)
 */
#include "FreeRTOS.h"
#include "task.h"
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

/* From ram_ecc_fault_inject.c */
extern void FaultInject_RegisterEccCallbacks(void (*sec)(uint32_t), void (*ded)(uint32_t));
extern void FaultInject_FlipBit1(uint8_t *addr, uint8_t bitPos);
extern void FaultInject_FlipBit2(uint8_t *addr, uint8_t pos1, uint8_t pos2);

extern void     RamSafety_HandleSingleBitError(uint32_t addr);
extern void     RamSafety_HandleDoubleBitError(uint32_t addr);
extern void     RamSafety_EnterSafeState(void);
extern uint32_t RamSafety_GetErrorCount(void);
extern uint32_t RamSafety_GetCorrectedCount(void);
extern uint8_t  RamSafety_GetState(void);
extern uint32_t RamSafety_GetSafeStateCalls(void);
extern void     RamSafety_ResetStats(void);
extern uint8_t  RamSafety_VerifyRegion(const uint8_t *start, uint32_t len);

#define RAMSAFETY_OK      0U
#define RAMSAFETY_FAILED  1U
#define SEC_THRESHOLD     5U

static uint8_t s_test_buf[64];

static void vRamEccTask(void *pv)
{
    (void)pv;
    Uart_WriteString("=== C9 RAM ECC ===\n");

    FaultInject_RegisterEccCallbacks(RamSafety_HandleSingleBitError,
                                     RamSafety_HandleDoubleBitError);

    /* S9.1: SEC fault corrected */
    {
        RamSafety_ResetStats();
        s_test_buf[0] = 0xAAU;
        FaultInject_FlipBit1(&s_test_buf[0], 2U);
        Uart_WriteString("S9.1 corrected=");
        Uart_WriteDec(RamSafety_GetCorrectedCount());
        Uart_WriteString(" state=");
        Uart_WriteDec((uint32_t)RamSafety_GetState());
        Uart_WriteString("\n");
        Qemu_Assert(RamSafety_GetCorrectedCount() == 1U, "S9.1: SEC not corrected");
        Qemu_Assert(RamSafety_GetState() == RAMSAFETY_OK, "S9.1: state not OK after SEC");
    }

    /* S9.2: DED fault fatal */
    {
        RamSafety_ResetStats();
        s_test_buf[1] = 0x55U;
        FaultInject_FlipBit2(&s_test_buf[1], 0U, 4U);
        Uart_WriteString("S9.2 safe_state=");
        Uart_WriteDec(RamSafety_GetSafeStateCalls());
        Uart_WriteString(" state=");
        Uart_WriteDec((uint32_t)RamSafety_GetState());
        Uart_WriteString("\n");
        Qemu_Assert(RamSafety_GetSafeStateCalls() >= 1U, "S9.2: safe state not entered");
        Qemu_Assert(RamSafety_GetState() == RAMSAFETY_FAILED, "S9.2: state not FAILED after DED");
    }

    /* S9.3: SafeRAM write protection (March C- verification) */
    {
        RamSafety_ResetStats();
        for (int i = 0; i < 64; i++) s_test_buf[i] = (uint8_t)(i & 0xFF);
        uint8_t result = RamSafety_VerifyRegion(s_test_buf, 64U);
        Uart_WriteString("S9.3 verify=");
        Uart_WriteDec((uint32_t)result);
        Uart_WriteString("\n");
        Qemu_Assert(result == 0U, "S9.3: March C- verification failed");
    }

    /* S9.4: SEC threshold degrade */
    {
        RamSafety_ResetStats();
        for (uint8_t i = 0; i < SEC_THRESHOLD + 1U; i++)
        {
            s_test_buf[i & 0x3FU] = 0xFFU;
            FaultInject_FlipBit1(&s_test_buf[i & 0x3FU], i & 7U);
        }
        uint32_t err_cnt = RamSafety_GetErrorCount();
        Uart_WriteString("S9.4 error_count=");
        Uart_WriteDec(err_cnt);
        Uart_WriteString("\n");
        Qemu_Assert(err_cnt > SEC_THRESHOLD, "S9.4: error count did not exceed threshold");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("RAM_ECC_START\n");
    xTaskCreate(vRamEccTask, "RamEcc", 512, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
