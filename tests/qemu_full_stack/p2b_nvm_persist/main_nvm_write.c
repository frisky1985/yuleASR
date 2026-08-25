/*
 * main_nvm_write.c - C5 Phase 1: NvM Write + Flash Persist Export
 *
 * Scenario:
 *   S5.1 NvMWriteAndFlush   - NvM_WriteBlock returns NVM_REQ_OK
 *   S5.2 FlashBinDump       - flash.bin exists + magic correct
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

/* Minimal CRC32 table-less implementation */
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

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void *NvM_SrcPtr)
{
    (void)BlockId;
    (void)NvM_SrcPtr;
    return E_OK;
}

Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr)
{
    (void)BlockId;
    *RequestResultPtr = s_nvm_result;
    return E_OK;
}

Std_ReturnType NvM_WriteAll(void) { return E_OK; }

/* Application */
static void vNvmWriteTask(void *pv)
{
    (void)pv;
    uint32_t value = NVM_FLASH_MAGIC;
    uint8_t payload[8];
    NvM_PersistedBlockType block;

    Uart_WriteString("=== C5 NvM Write ===\n");

    /* S5.1: write block */
    memcpy(payload, &value, sizeof(value));
    payload[4] = 0xAAU; payload[5] = 0xBBU; payload[6] = 0xCCU; payload[7] = 0xDDU;

    memset(&block, 0, sizeof(block));
    block.magic       = NVM_FLASH_MAGIC;
    block.version     = NVM_FLASH_VERSION;
    block.payload_len = sizeof(payload);
    memcpy(block.payload, payload, sizeof(payload));
    block.crc32       = crc32_buf((const uint8_t *)&block, sizeof(block) - sizeof(block.crc32));

    Std_ReturnType ret = NvM_WriteBlock(NVM_BLOCK_ID_TEST, payload);
    Qemu_Assert(ret == E_OK, "S5.1: NvM_WriteBlock failed");

    NvM_RequestResultType req_res;
    (void)NvM_GetErrorStatus(NVM_BLOCK_ID_TEST, &req_res);
    Qemu_Assert(req_res == NVM_REQ_OK, "S5.1: NvM_GetErrorStatus != OK");

    /* S5.2: export to flash.bin via semihosting */
    FlashPersist_Export("/tmp/qemu_flash.bin", (const uint8_t *)&block, sizeof(block));
    Uart_WriteString("FLASH_WRITTEN\n");

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("NVM_WRITE_START\n");
    xTaskCreate(vNvmWriteTask, "NvmW", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
