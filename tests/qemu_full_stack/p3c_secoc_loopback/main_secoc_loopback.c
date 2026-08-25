/*
 * main_secoc_loopback.c - C8: SecOC Loopback Verification
 */
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

#define SECOC_CMAC_LEN 4U
#define SECOC_FV_LEN   4U
#define SECOC_PDU_LEN  10U

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef uint8 Std_ReturnType;
#define E_OK 0U
#define E_NOT_OK 1U

extern Std_ReturnType Csm_MacGenerate(uint32_t jobId, uint8_t mode, const uint8_t *dataPtr, uint32_t dataLength,
                                      uint8_t *macPtr, uint32_t *macLengthPtr);
extern Std_ReturnType Csm_MacVerify(uint32_t jobId, uint8_t mode, const uint8_t *dataPtr, uint32_t dataLength,
                                    const uint8_t *macPtr, uint32_t macLength, uint8_t *verifyPtr);
extern Std_ReturnType FvM_GetTxFreshnessValue(uint16_t id, uint8_t *freshnessValue, uint32_t *len);
extern Std_ReturnType FvM_GetRxFreshnessValue(uint16_t id, const uint8_t *truncFreshnessValue,
                                              uint8_t *freshnessValue, uint32_t *len);
extern Std_ReturnType FvM_UpdateCounter(uint16_t id);
extern void SecocCrypto_ResetFv(void);
extern uint32_t SecocCrypto_GetTxFv(void);

static uint32_t s_accepted = 0UL;
static uint32_t s_rejected = 0UL;

/**
 * @brief Build a secured PDU for loopback testing
 * @req SWS_SecOC_00010
 */
static void secoc_build_pdu(uint8_t data0, uint8_t data1, uint8_t *pdu, uint8_t tamper)
{
    uint8_t data[2] = { data0, data1 };
    uint8_t mac[SECOC_CMAC_LEN];
    uint32_t macLen = SECOC_CMAC_LEN;
    uint8_t fv[SECOC_FV_LEN];
    uint32_t fvLen = SECOC_FV_LEN;

    (void)Csm_MacGenerate(0U, 0U, data, 2U, mac, &macLen);
    (void)FvM_GetTxFreshnessValue(0U, fv, &fvLen);

    pdu[0] = data0;
    pdu[1] = data1;
    pdu[2] = fv[0];
    pdu[3] = fv[1];
    pdu[4] = fv[2];
    pdu[5] = fv[3];
    pdu[6] = mac[0];
    pdu[7] = mac[1];
    pdu[8] = mac[2];
    pdu[9] = mac[3];

    if (tamper) { pdu[7] ^= 0xFFU; }
}

/**
 * @brief Receive and verify a secured PDU for loopback testing
 * @req SWS_SecOC_00011
 */
static void secoc_receive(const uint8_t *pdu)
{
    uint8_t data[2] = { pdu[0], pdu[1] };
    uint8_t rx_mac[SECOC_CMAC_LEN] = { pdu[6], pdu[7], pdu[8], pdu[9] };
    uint8_t verify = 0U;

    (void)Csm_MacVerify(0U, 0U, data, 2U, rx_mac, SECOC_CMAC_LEN, &verify);

    if (verify == 0x01U) {
        s_accepted++;
    } else {
        s_rejected++;
    }
}

/**
 * @brief SecOC loopback verification test task
 * @req SWS_SecOC_00010
 * @req SWS_SecOC_00011
 * @req SWS_SecOC_00020
 * @req SWS_SecOC_00021
 */
static void vSecocTask(void *pv)
{
    (void)pv;
    uint8_t pdu[SECOC_PDU_LEN];

    Uart_WriteString("=== C8 SecOC Loopback ===\n");
    SecocCrypto_ResetFv();

    /* S8.1: legitimate PDU accepted */
    uint8_t saved_pdu[SECOC_PDU_LEN];
    {
        secoc_build_pdu(0x12U, 0x34U, pdu, 0U);
        memcpy(saved_pdu, pdu, SECOC_PDU_LEN);
        secoc_receive(pdu);
        FvM_UpdateCounter(0U);
        Uart_WriteString("S8.1 accepted=");
        Uart_WriteDec(s_accepted);
        Uart_WriteString("\n");
        Qemu_Assert(s_accepted == 1UL, "S8.1: legitimate PDU not accepted");
    }

    /* S8.2: tampered PDU rejected */
    {
        secoc_build_pdu(0x12U, 0x34U, pdu, 1U);
        secoc_receive(pdu);
        Uart_WriteString("S8.2 rejected=");
        Uart_WriteDec(s_rejected);
        Uart_WriteString("\n");
        Qemu_Assert(s_rejected == 1UL, "S8.2: tampered PDU not rejected");
    }

    /* S8.3: replay rejected (re-send S8.1 PDU after FV advanced) */
    {
        secoc_receive(saved_pdu);
        Uart_WriteString("S8.3 replay rejected=");
        Uart_WriteDec(s_rejected);
        Uart_WriteString("\n");
        Qemu_Assert(s_rejected >= 2UL, "S8.3: replay not rejected");
    }

    /* S8.4: 10-frame alternating sequence */
    {
        s_accepted = 0UL; s_rejected = 0UL;
        for (uint32_t i = 0; i < 10; i++) {
            secoc_build_pdu((uint8_t)i, (uint8_t)(i + 1U), pdu, (i & 1U));
            secoc_receive(pdu);
            if ((i & 1U) == 0U) { (void)FvM_UpdateCounter(0U); }
        }
        Uart_WriteString("S8.4 accepted=");
        Uart_WriteDec(s_accepted);
        Uart_WriteString(" rejected=");
        Uart_WriteDec(s_rejected);
        Uart_WriteString("\n");
        Qemu_Assert(s_accepted == 5UL && s_rejected == 5UL, "S8.4: 10-frame counts wrong");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("SECOC_LOOPBACK_START\n");
    xTaskCreate(vSecocTask, "SecOC", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
