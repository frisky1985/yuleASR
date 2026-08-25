/*
 * secoc_crypto_stub.c - C8: AES-128-CMAC stub for SecOC loopback verification
 *
 * Provides:
 *   Csm_MacGenerate / Csm_MacVerify
 *   FvM_GetTxFreshnessValue / FvM_GetRxFreshnessValue / FvM_UpdateCounter
 */
#include <string.h>
#include "Uart_Cfg.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef uint8 Std_ReturnType;
#define E_OK 0U
#define E_NOT_OK 1U

typedef uint16 Crypto_JobType;
#define SECOC_CMAC_LEN 4U
#define SECOC_FV_LEN   4U

static const uint8_t s_key[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static uint32_t s_tx_fv = 0x00000001U;
static uint32_t s_rx_fv = 0x00000001U;

/* Very simple AES-like substitution; NOT cryptographically secure, only for test.
 * We use a keyed XOR-substitution MAC truncated to 4 bytes. */
static void mac_generate(const uint8_t *data, uint16_t len, uint32_t fv, uint8_t *mac4)
{
    uint8_t state[16];
    memcpy(state, s_key, 16);
    state[0] ^= (uint8_t)(fv >> 24);
    state[1] ^= (uint8_t)(fv >> 16);
    state[2] ^= (uint8_t)(fv >> 8);
    state[3] ^= (uint8_t)(fv);

    for (uint16_t i = 0; i < len; i++) {
        state[i % 16] ^= data[i];
        state[i % 16] = (uint8_t)((state[i % 16] * 31 + 17) & 0xFFU);
    }
    for (int r = 0; r < 4; r++) {
        for (int j = 0; j < 16; j++) {
            state[j] ^= s_key[j];
            state[j] = (uint8_t)((state[j] * 31 + 17) & 0xFFU);
        }
    }
    memcpy(mac4, state, 4);
}

/* CSM stubs */
Std_ReturnType Csm_MacGenerate(uint32_t jobId, uint8_t mode, const uint8_t *dataPtr, uint32_t dataLength,
                               uint8_t *macPtr, uint32_t *macLengthPtr)
{
    (void)jobId; (void)mode;
    if (dataPtr == NULL || macPtr == NULL || macLengthPtr == NULL) return E_NOT_OK;
    if (*macLengthPtr < SECOC_CMAC_LEN) return E_NOT_OK;
    mac_generate(dataPtr, (uint16_t)dataLength, s_tx_fv, macPtr);
    *macLengthPtr = SECOC_CMAC_LEN;
    return E_OK;
}

Std_ReturnType Csm_MacVerify(uint32_t jobId, uint8_t mode, const uint8_t *dataPtr, uint32_t dataLength,
                             const uint8_t *macPtr, uint32_t macLength, uint8_t *verifyPtr)
{
    (void)jobId; (void)mode; (void)macLength;
    if (dataPtr == NULL || macPtr == NULL || verifyPtr == NULL) return E_NOT_OK;
    uint8_t expected[4];
    mac_generate(dataPtr, (uint16_t)dataLength, s_rx_fv, expected);
    *verifyPtr = (memcmp(expected, macPtr, 4) == 0) ? 0x01U : 0x00U;
    return E_OK;
}

/* FVM stubs */
Std_ReturnType FvM_GetTxFreshnessValue(uint16_t id, uint8_t *freshnessValue, uint32_t *len)
{
    (void)id;
    if (freshnessValue == NULL || len == NULL) return E_NOT_OK;
    freshnessValue[0] = (uint8_t)(s_tx_fv >> 24);
    freshnessValue[1] = (uint8_t)(s_tx_fv >> 16);
    freshnessValue[2] = (uint8_t)(s_tx_fv >> 8);
    freshnessValue[3] = (uint8_t)(s_tx_fv);
    *len = SECOC_FV_LEN;
    return E_OK;
}

Std_ReturnType FvM_GetRxFreshnessValue(uint16_t id, const uint8_t *truncFreshnessValue,
                                       uint8_t *freshnessValue, uint32_t *len)
{
    (void)id; (void)truncFreshnessValue;
    if (freshnessValue == NULL || len == NULL) return E_NOT_OK;
    freshnessValue[0] = (uint8_t)(s_rx_fv >> 24);
    freshnessValue[1] = (uint8_t)(s_rx_fv >> 16);
    freshnessValue[2] = (uint8_t)(s_rx_fv >> 8);
    freshnessValue[3] = (uint8_t)(s_rx_fv);
    *len = SECOC_FV_LEN;
    return E_OK;
}

Std_ReturnType FvM_UpdateCounter(uint16_t id)
{
    (void)id;
    s_tx_fv++;
    s_rx_fv++;
    return E_OK;
}

void SecocCrypto_ResetFv(void)
{
    s_tx_fv = 0x00000001U;
    s_rx_fv = 0x00000001U;
}

uint32_t SecocCrypto_GetTxFv(void) { return s_tx_fv; }
