#include "qemu_assert.h"
#include "Uart_Cfg.h"

static void qemu_semihosting_exit(int code)
{
    volatile uint32_t params[2] = { 1U, (uint32_t)code };
    register uint32_t r0 __asm("r0") = 0x18U;
    register uint32_t r1 __asm("r1") = (uint32_t)params;
    __asm volatile("bkpt #0xAB" : : "r"(r0), "r"(r1) : "memory");
}

void Qemu_ReportPass(void)
{
    Uart_WriteString("\n" QEMU_PASS_MARKER "\n");
    qemu_semihosting_exit(0);
    for (;;) {}
}

void Qemu_ReportFail(const char *msg)
{
    Uart_WriteString("\n" QEMU_FAIL_MARKER ": ");
    if (msg != 0)
    {
        Uart_WriteString(msg);
    }
    Uart_WriteString("\n");
    qemu_semihosting_exit(1);
    for (;;) {}
}

void Qemu_Assert(bool cond, const char *msg)
{
    if (!cond)
    {
        Qemu_ReportFail(msg);
    }
}
