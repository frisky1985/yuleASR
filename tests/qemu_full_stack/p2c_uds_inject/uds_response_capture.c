/*
 * uds_response_capture.c - C6: Capture Dcm responses by intercepting Can_Write
 *
 * Provides:
 *   - UdsCapture_Init()      install the capture hook
 *   - UdsCapture_GetResponse() returns pointer to last captured response
 *   - UdsCapture_GetLength()   returns captured response length
 *   - Can_Write() override that copies outgoing PDU into buffer and loops back
 */
#include <string.h>
#include "Uart_Cfg.h"

#define UDS_CAPTURE_MAX 64U

static volatile uint8_t  s_capture_buf[UDS_CAPTURE_MAX];
static volatile uint8_t  s_capture_len = 0U;
static volatile uint8_t  s_ready       = 0U;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef uint8 Std_ReturnType;
#define E_OK 0U
#define E_NOT_OK 1U

typedef struct {
    uint8 *SduDataPtr;
    uint16 SduLength;
} PduInfoType;

typedef uint8 Can_HwHandleType;
typedef uint32 Can_IdType;

/* Original Can_Write is replaced by this capture version in the test image. */
Std_ReturnType Can_Write(Can_HwHandleType Hth, const PduInfoType *PduInfo)
{
    (void)Hth;
    if (PduInfo == NULL || PduInfo->SduDataPtr == NULL) return E_NOT_OK;

    uint16 len = PduInfo->SduLength;
    if (len > UDS_CAPTURE_MAX) len = UDS_CAPTURE_MAX;

    for (uint16 i = 0U; i < len; i++) {
        s_capture_buf[i] = PduInfo->SduDataPtr[i];
    }
    s_capture_len = (uint8_t)len;
    s_ready       = 1U;

    Uart_WriteString("[CAPTURE] len=");
    Uart_WriteDec((uint32_t)len);
    Uart_WriteString("\n");
    return E_OK;
}

void UdsCapture_Init(void)
{
    s_capture_len = 0U;
    s_ready       = 0U;
}

const volatile uint8_t *UdsCapture_GetResponse(uint8_t *out_len)
{
    *out_len = s_capture_len;
    return s_capture_buf;
}

uint8_t UdsCapture_IsReady(void)
{
    return s_ready;
}

/* Stub for CanIf_RxIndication used by other parts if needed. */
void CanIf_RxIndication(Can_HwHandleType Hrh, Can_IdType CanId, uint8 CanDlc, const uint8 *CanSduPtr)
{
    (void)Hrh; (void)CanId; (void)CanDlc; (void)CanSduPtr;
}
