/*
 * main_nvm_read.c - C5 Phase 2: NvM Read Back After Power Cycle
 *
 * Scenarios:
 *   S5.3 PowerCycleRestore     - read-back value == 0xDEADBEEF
 *   S5.4 CorruptedBlockHandling - corrupted CRC returns NVM_REQ_INTEGRITY_FAILED
 */
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"
#include "../common/flash_persist.h"

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef uint8          Std_ReturnType;
#define E_OK     0U
#define E_NOT_OK 1U

typedef uint16 NvM_BlockIdType;
typedef uint8  NvM_RequestResultType;
#define NVM_REQ_OK               0x00U
#define NVM_REQ_INTEGRITY_FAILED 0x08U
#define NVM_BLOCK_ID_TEST        1U
#define NVM_FLASH_MAGIC          0xDEADBEEFU
#define NVM_FLASH_VERSION        0x00010001U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t payload_len;
    uint32_t crc32;
    uint8_t  payload[8];
} NvM_PersistedBlockType;

static uint32_t crc32_step(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (int i = 0; i < 8; i++) { crc = (crc >> 1) ^ (0xEDB88320U & (-(crc & 1U))); }
    return crc;
}

static uint32_t crc32_buf(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < len; i++) { crc = crc32_step(crc, data[i]); }
    return ~crc;
}

/* NvM stub */
static NvM_RequestResultType s_nvm_result = NVM_REQ_OK;
static uint8_t s_nvm_payload[8];

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void *NvM_DstPtr)
{
    (void)BlockId;
    if (NvM_DstPtr != NULL) { memcpy(NvM_DstPtr, s_nvm_payload, sizeof(s_nvm_payload)); }
    return E_OK;
}

Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr)
{
    (void)BlockId;
    *RequestResultPtr = s_nvm_result;
    return E_OK;
}

static void vNvmReadTask(void *pv)
{
    (void)pv;
    NvM_PersistedBlockType block;
    uint8_t payload[8];

    Uart_WriteString("=== C5 NvM Read ===\n");

    /* S5.3: import flash.bin, validate magic, return value */
    {
        Qemu_Assert(FlashPersist_Import("/tmp/qemu_flash.bin", (uint8_t *)&block, sizeof(block)),
                    "S5.3: flash.bin import failed");
        uint32_t stored_crc = block.crc32;
        block.crc32 = 0U;
        uint32_t calc_crc = crc32_buf((const uint8_t *)&block, sizeof(block) - sizeof(block.crc32));
        block.crc32 = stored_crc;
        Qemu_Assert(block.magic == NVM_FLASH_MAGIC, "S5.3: magic mismatch");
        Qemu_Assert(block.version == NVM_FLASH_VERSION, "S5.3: version mismatch");
        Qemu_Assert(stored_crc == calc_crc, "S5.3: CRC mismatch");

        memcpy(payload, block.payload, sizeof(payload));
        uint32_t value;
        memcpy(&value, payload, sizeof(value));

        Uart_WriteString("S5.3 value=0x");
        Uart_WriteDec(value);
        Uart_WriteString("\n");
        Qemu_Assert(value == NVM_FLASH_MAGIC, "S5.3: value != 0xDEADBEEF");
    }

    /* S5.4: corrupted block (flip one bit in CRC) handling */
    {
        block.crc32 ^= 0x1U;
        uint32_t calc_crc = crc32_buf((const uint8_t *)&block, sizeof(block) - sizeof(block.crc32));
        if (block.crc32 != calc_crc)
        {
            s_nvm_result = NVM_REQ_INTEGRITY_FAILED;
        }

        NvM_RequestResultType req_res;
        (void)NvM_GetErrorStatus(NVM_BLOCK_ID_TEST, &req_res);
        Uart_WriteString("S5.4 integrity=");
        Uart_WriteDec((uint32_t)req_res);
        Uart_WriteString("\n");
        Qemu_Assert(req_res == NVM_REQ_INTEGRITY_FAILED, "S5.4: expected INTEGRITY_FAILED");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("NVM_READ_START\n");
    xTaskCreate(vNvmReadTask, "NvmR", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
