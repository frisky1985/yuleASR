#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

#include "Std_Types.h"
#include <stdint.h>
#include <stddef.h>
#include "Boot_Cfg.h"

#define BOOT_AR_RELEASE_MAJOR_VERSION   4
#define BOOT_AR_RELEASE_MINOR_VERSION   0
#define BOOT_AR_RELEASE_REVISION_VERSION 3

#define BOOT_SW_MAJOR_VERSION           1
#define BOOT_SW_MINOR_VERSION           0
#define BOOT_SW_PATCH_VERSION           0

typedef enum {
    BOOT_OK              = 0x00U,
    BOOT_E_NOT_INIT      = 0x01U,
    BOOT_E_PARAM         = 0x02U,
    BOOT_E_SIGNATURE     = 0x10U,
    BOOT_E_HASH          = 0x11U,
    BOOT_E_VERSION       = 0x12U,
    BOOT_E_HEADER_CRC    = 0x13U,
    BOOT_E_FLASH_ERASE   = 0x20U,
    BOOT_E_FLASH_WRITE   = 0x21U,
    BOOT_E_FLASH_READ    = 0x22U,
    BOOT_E_FLASH_PROTECT = 0x23U,
    BOOT_E_HSM_INIT      = 0x30U,
    BOOT_E_HSM_VERIFY    = 0x31U,
    BOOT_E_TIMEOUT       = 0x40U,
    BOOT_E_CONFIRM_PENDING = 0x50U,  /* 用户确认未完成 (等待确认/未请求) */
    BOOT_E_CONFIRM_DENIED  = 0x51U,  /* 用户拒绝升级 */
    BOOT_E_GENERAL       = 0xFFU
} Boot_Result;

typedef enum {
    BOOT_IMAGE_NONE      = 0x00U,
    BOOT_IMAGE_PBL       = 0x01U,
    BOOT_IMAGE_SBL       = 0x02U,
    BOOT_IMAGE_APP       = 0x03U
} Boot_ImageType;

typedef enum {
    BOOT_STAGE_ROM       = 0x00U,
    BOOT_STAGE_PBL       = 0x01U,
    BOOT_STAGE_SBL       = 0x02U,
    BOOT_STAGE_APP       = 0x03U,
    BOOT_STAGE_RECOVERY  = 0xFFU
} Boot_Stage;

typedef struct {
    uint32_t magic;
    uint32_t pbl_version;
    uint32_t sbl_version;
    uint32_t app_version;
    uint32_t boot_count;
    uint32_t max_boot_attempts;
    uint8_t  status;
    /* 抗回滚 (RS-OTA-01 / P1-4 延后递增):
     * anti_rollback_counter = 已确认地板; pending_counter/pending_boot_count
     * = 待确认升级状态 (新版本成功启动 N 次后提交)。
     * 字段占用原 reserved[32] 前 12 字节 (reserved 缩为 [24]),
     * 结构总尺寸与 crc32 偏移不变 (68B / 64) → 旧 BIB CRC 仍可校验,
     * 旧 reserved 字节被读为 pending 时由 bib_pending_valid 拦截。 */
    uint32_t anti_rollback_counter;
    uint32_t pending_counter;        /* 待确认版本 (0 = 无) */
    uint32_t pending_boot_count;     /* 新版本已成功启动次数 */
    /* 掉电保护 (RS-OTA-06): 升级状态, 复用原 reserved 首字节
     * (reserved 缩为 [23]), 结构总尺寸与 crc32 偏移不变 → 旧 BIB 兼容读
     * (旧 reserved 字节为 0 = IDLE)。 */
    uint8_t  update_state;
    uint8_t  reserved[23];
    uint32_t crc32;
} Boot_InfoBlock;

/* P2-3 (2026-08-13): 布局不变量编译期固化 — 旧 BIB CRC/兼容读依赖
 * crc32 位于偏移 64 (sizeof 68), update_state 复用 reserved 首字节 (偏移 40)。
 * 结构变更若破坏这些偏移, 编译期直接报错而非运行时静默破坏。 */
#ifdef __STDC_VERSION__
#if (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(Boot_InfoBlock) == 68U, "Boot_InfoBlock size changed (breaks old BIB compat)");
_Static_assert(offsetof(Boot_InfoBlock, crc32) == 64U, "Boot_InfoBlock crc32 offset changed");
_Static_assert(offsetof(Boot_InfoBlock, update_state) == 40U, "Boot_InfoBlock update_state offset changed");
#endif
#endif

/* BIB 升级状态枚举 (RS-OTA-06 掉电保护状态机)
 * - IDLE: 无进行中的升级 (默认/全部成功后的终态)
 * - DOWNLOADING: Prepare 已落盘、擦除/写入进行中 (中途掉电 → 回退活动槽)
 * - PENDING: Finalize 校验通过, 进入候选槽验证/确认流程
 * 旧 BIB 无此字段 (reserved=0), 读为 IDLE, 天然兼容 */
typedef enum {
    BOOT_UPDATE_IDLE        = 0x00U,
    BOOT_UPDATE_DOWNLOADING = 0x01U,
    BOOT_UPDATE_PENDING     = 0x02U
} Boot_UpdateState;

#define BOOT_IMAGE_MAGIC  0x314C4259UL

typedef struct {
    uint32_t magic;
    uint32_t header_crc;
    uint32_t image_type;
    uint32_t version;
    uint32_t payload_size;
    uint8_t  hash[32];
    uint8_t  reserved[12];
} Boot_ImageHeader;

typedef struct {
    uint8_t  signature[64];
    uint32_t signature_algo;
    uint32_t signing_time;
    uint8_t  reserved[56];
} Boot_ImageTrailer;

typedef struct {
    Boot_ImageType target;
    uint32_t       target_addr;
    Boot_Result    last_error;
    uint8_t        attempt;
} Boot_Decision;

typedef struct {
    const uint8_t *data;
    uint16_t       length;
} Boot_PubKey;

#endif /* BOOT_TYPES_H */