#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

#include "Std_Types.h"
#include <stdint.h>
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
    uint32_t anti_rollback_counter;
    uint8_t  reserved[32];
    uint32_t crc32;
} Boot_InfoBlock;

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