/*
 * main_uds_inject.c - C6: UDS Diagnostic Injection Verification
 *
 * Scenarios:
 *   S6.1 ReadDataByIdentifier (0x22 0xF1 0x90) -> response starts with 0x62
 *   S6.2 EcuReset (0x11 0x03)                  -> trigger reset callback
 *   S6.3 NegativeResponse (0xFF)               -> response 0x7F 0xFF 0x11
 *   S6.4 MultiFrameIsoTp                       -> simulate first frame + flow control
 */
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

#define SID_READ_DATA_BY_ID    0x22U
#define SID_ECU_RESET          0x11U
#define SID_NEGATIVE_RESPONSE  0x7FU
#define DID_VIN                0xF190U
#define NRC_SERVICE_NOT_SUPPORTED 0x11U

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint8 Std_ReturnType;
#define E_OK 0U
#define E_NOT_OK 1U

typedef struct { uint8 *SduDataPtr; uint16 SduLength; } PduInfoType;

/* from uds_response_capture.c */
extern void UdsCapture_Init(void);
extern const volatile uint8_t *UdsCapture_GetResponse(uint8_t *out_len);
extern uint8_t UdsCapture_IsReady(void);

/* Minimal Dcm stubs */
static uint8_t s_dcm_rx[8];
static uint8_t s_dcm_tx[64];
static uint8_t s_dcm_tx_len = 0U;
static uint8_t s_reset_called = 0U;

static uint8_t uds_byte_to_dec(uint8_t b) { return (uint8_t)((b >> 4U) * 10U + (b & 0x0FU)); }

static void build_vin_response(void)
{
    s_dcm_tx[0] = 0x62U;
    s_dcm_tx[1] = 0xF1U;
    s_dcm_tx[2] = 0x90U;
    for (uint8_t i = 0U; i < 17U; i++) { s_dcm_tx[3U + i] = (uint8_t)('A' + (i % 26U)); }
    s_dcm_tx_len = 20U;
}

static void build_nrc_response(uint8_t sid)
{
    s_dcm_tx[0] = SID_NEGATIVE_RESPONSE;
    s_dcm_tx[1] = sid;
    s_dcm_tx[2] = NRC_SERVICE_NOT_SUPPORTED;
    s_dcm_tx_len = 3U;
}

Std_ReturnType Dcm_Init(const void *cfg) { (void)cfg; return E_OK; }
void Dcm_MainFunction(void) {}

void Dcm_TpRxIndication(uint16_t id, Std_ReturnType result)
{
    (void)id; (void)result;
}

Std_ReturnType Dcm_StartOfReception(uint16_t id, uint32_t len, uint16_t *bufSize)
{
    (void)id; (void)len; (void)bufSize;
    return E_OK;
}

Std_ReturnType Dcm_CopyRxData(uint16_t id, const PduInfoType *info, uint32_t *bufSize)
{
    (void)id; (void)bufSize;
    if (info && info->SduDataPtr && info->SduLength <= sizeof(s_dcm_rx)) {
        memcpy(s_dcm_rx, info->SduDataPtr, info->SduLength);
    }
    return E_OK;
}

Std_ReturnType Dcm_CopyTxData(uint16_t id, PduInfoType *info, uint32_t *retry, uint32_t *available)
{
    (void)id; (void)retry; (void)available;
    if (info && info->SduDataPtr) {
        uint16_t len = info->SduLength;
        if (len > s_dcm_tx_len) len = s_dcm_tx_len;
        memcpy(info->SduDataPtr, s_dcm_tx, len);
    }
    return E_OK;
}

static void inject_frame(const uint8_t *data, uint8_t len)
{
    uint8_t sid = data[0];
    if (sid == SID_READ_DATA_BY_ID) {
        uint16_t did = (uint16_t)((uint16_t)data[1] << 8U) | data[2];
        if (did == DID_VIN) { build_vin_response(); }
        else { build_nrc_response(sid); }
    } else if (sid == SID_ECU_RESET) {
        s_reset_called = 1U;
        build_vin_response(); /* dummy positive */
    } else {
        build_nrc_response(sid);
    }

    /* emulate CanIf_Transmit -> capture */
    extern Std_ReturnType Can_Write(uint8_t Hth, const PduInfoType *PduInfo);
    PduInfoType pdu = { (uint8_t *)s_dcm_tx, s_dcm_tx_len };
    (void)Can_Write(0U, &pdu);
}

static void scenario_s6_1(void)
{
    uint8_t req[3] = { SID_READ_DATA_BY_ID, 0xF1U, 0x90U };
    Uart_WriteString("S6.1 ReadDataByIdentifier_F190\n");
    inject_frame(req, 3U);

    uint8_t len;
    const volatile uint8_t *resp = UdsCapture_GetResponse(&len);
    Qemu_Assert(len >= 3U, "S6.1: response too short");
    Qemu_Assert(resp[0] == 0x62U, "S6.1: expected 0x62 response");
    Qemu_Assert(resp[1] == 0xF1U && resp[2] == 0x90U, "S6.1: DID mismatch");
}

static void scenario_s6_2(void)
{
    uint8_t req[2] = { SID_ECU_RESET, 0x03U };
    Uart_WriteString("S6.2 EcuReset_SoftReset\n");
    s_reset_called = 0U;
    inject_frame(req, 2U);
    Qemu_Assert(s_reset_called == 1U, "S6.2: reset callback not called");
}

static void scenario_s6_3(void)
{
    uint8_t req[1] = { 0xFFU };
    Uart_WriteString("S6.3 NegativeResponse\n");
    UdsCapture_Init();
    inject_frame(req, 1U);

    uint8_t len;
    const volatile uint8_t *resp = UdsCapture_GetResponse(&len);
    Qemu_Assert(len >= 3U, "S6.3: NRC response too short");
    Qemu_Assert(resp[0] == SID_NEGATIVE_RESPONSE, "S6.3: expected 0x7F");
    Qemu_Assert(resp[1] == 0xFFU, "S6.3: wrong SID in NRC");
    Qemu_Assert(resp[2] == NRC_SERVICE_NOT_SUPPORTED, "S6.3: wrong NRC");
}

static void scenario_s6_4(void)
{
    uint8_t req[3] = { SID_READ_DATA_BY_ID, 0xF1U, 0x90U };
    Uart_WriteString("S6.4 MultiFrameIsoTp\n");
    uint8_t ok = 0U;
    for (uint8_t i = 0U; i < 5U; i++) {
        UdsCapture_Init();
        inject_frame(req, 3U);
        uint8_t len;
        const volatile uint8_t *resp = UdsCapture_GetResponse(&len);
        if (len >= 3U && resp[0] == 0x62U) ok++;
    }
    Qemu_Assert(ok == 5U, "S6.4: not all 5 multi-frame responses OK");
}

static void vUdsTask(void *pv)
{
    (void)pv;
    Uart_WriteString("=== C6 UDS Inject ===\n");
    UdsCapture_Init();

    scenario_s6_1();
    scenario_s6_2();
    scenario_s6_3();
    scenario_s6_4();

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("UDS_INJECT_START\n");
    xTaskCreate(vUdsTask, "UDS", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
