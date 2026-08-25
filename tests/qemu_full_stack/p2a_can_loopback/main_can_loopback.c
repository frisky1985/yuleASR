/*
 * main_can_loopback.c - C4: CAN Loopback Verification
 *
 * Scenarios:
 *   S4.1 CanWriteLoopback     - Can_Write triggers RxIndication callback (count==1)
 *   S4.2 ComSignalReceive     - Com signal value == 0x1234 after RX
 *   S4.3 RtePortRead          - RTE read port returns same value 0x1234
 *   S4.4 MultiFrameSequence   - 5 frames sent, 5 received
 *
 * CAN loopback mechanism: Can_Write stub immediately calls CanIf_RxIndication
 * with the same PDU, simulating an internal loopback without real hardware.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

/* ---------- minimal AUTOSAR types for standalone build ---------- */
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef uint8          Std_ReturnType;
#define E_OK     0U
#define E_NOT_OK 1U

typedef struct { uint8 *SduDataPtr; uint16 SduLength; } PduInfoType;
typedef uint16 PduIdType;
typedef uint8  Can_HwHandleType;

/* ---------- loopback state ---------- */
static volatile uint32 s_rx_count    = 0UL;
static volatile uint16 s_last_signal = 0U;
static volatile uint16 s_rte_port    = 0U;

/* ---------- CanIf_RxIndication stub (called from Can_Write loopback) --- */
void CanIf_RxIndication(Can_HwHandleType Hrh, uint32 CanId,
                        uint8 CanDlc, const uint8 *CanSduPtr)
{
    (void)Hrh; (void)CanId;
    s_rx_count++;
    if (CanDlc >= 2U)
    {
        /* big-endian signal: byte[0] high, byte[1] low */
        s_last_signal = (uint16)(((uint16)CanSduPtr[0] << 8U) | CanSduPtr[1]);
        /* simulate RTE write-port */
        s_rte_port = s_last_signal;
    }
}

/* ---------- Can_Write stub with built-in loopback ---------- */
Std_ReturnType Can_Write(Can_HwHandleType Hth, const PduInfoType *PduInfo)
{
    (void)Hth;
    if (PduInfo == NULL || PduInfo->SduDataPtr == NULL) { return E_NOT_OK; }
    /* loopback: immediately deliver as RX on handle 0 */
    CanIf_RxIndication(0U, 0x100UL, (uint8)PduInfo->SduLength, PduInfo->SduDataPtr);
    return E_OK;
}

static void vCanTask(void *pv)
{
    (void)pv;
    Uart_WriteString("=== C4 CAN Loopback ===\n");

    /* S4.1: CanWriteLoopback */
    {
        uint8 data[2] = {0x12U, 0x34U};
        PduInfoType pdu = { data, 2U };
        s_rx_count = 0UL;
        Std_ReturnType ret = Can_Write(0U, &pdu);
        Qemu_Assert(ret == E_OK,       "S4.1: Can_Write failed");
        Qemu_Assert(s_rx_count == 1UL, "S4.1: rx_count != 1");
        Uart_WriteString("S4.1 rx_count=");
        Uart_WriteDec(s_rx_count);
        Uart_WriteString("\n");
    }

    /* S4.2: ComSignalReceive */
    {
        Uart_WriteString("S4.2 last_signal=0x");
        Uart_WriteDec((uint32)s_last_signal);
        Uart_WriteString("\n");
        Qemu_Assert(s_last_signal == 0x1234U, "S4.2: signal != 0x1234");
    }

    /* S4.3: RtePortRead */
    {
        Uart_WriteString("S4.3 rte_port=0x");
        Uart_WriteDec((uint32)s_rte_port);
        Uart_WriteString("\n");
        Qemu_Assert(s_rte_port == 0x1234U, "S4.3: rte port != 0x1234");
    }

    /* S4.4: MultiFrameSequence - send 5 frames */
    {
        uint8 data[2];
        PduInfoType pdu = { data, 2U };
        s_rx_count = 0UL;
        for (uint32 i = 0UL; i < 5UL; i++)
        {
            data[0] = (uint8)(i + 1U);
            data[1] = (uint8)(i + 1U);
            Can_Write(0U, &pdu);
        }
        Uart_WriteString("S4.4 multi_rx_count=");
        Uart_WriteDec(s_rx_count);
        Uart_WriteString("\n");
        Qemu_Assert(s_rx_count == 5UL, "S4.4: not all 5 frames received");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("CAN_LOOPBACK_START\n");
    xTaskCreate(vCanTask, "CAN", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
