#ifndef BOOT_CFG_H
#define BOOT_CFG_H

#include "Std_Types.h"

/* === Flash Memory Map (S32K312 default — edit when porting) === */
#define BOOT_FLASH_BASE             0x00000000UL

#define BOOT_PBL_ADDR               0x00000000UL
#define BOOT_PBL_SIZE               0x00001000UL    /* 4KB */

#define BOOT_SBL_ADDR               0x00002000UL
#define BOOT_SBL_SIZE               0x00010000UL    /* 64KB */

#define BOOT_APP_SLOT_A_ADDR        0x00012000UL
#define BOOT_APP_SLOT_A_SIZE        0x000EE000UL

#define BOOT_APP_SLOT_B_ADDR        0x00100000UL
#define BOOT_APP_SLOT_B_SIZE        0x000E0000UL

#define BOOT_BIB_ADDR               0x001E0000UL
#define BOOT_BIB_SIZE               0x00010000UL

#define BOOT_NVM_ADDR               0x001F0000UL
#define BOOT_NVM_SIZE               0x00008000UL

#define BOOT_WDG_TIMEOUT_MS         5000U
#define BOOT_MAX_RETRIES            3U
#define BOOT_VERIFY_TIMEOUT_MS      10000U
#define BOOT_MAX_BOOT_ATTEMPTS      5U

#define BOOT_HASH_SIZE              32U
#define BOOT_SIGNATURE_SIZE         64U

#define BOOT_HSM_KEY_SLOT_PBL       0U
#define BOOT_HSM_KEY_SLOT_SBL       1U
#define BOOT_HSM_KEY_SLOT_APP       2U

#ifndef BOOT_DEBUG_ENABLE
#define BOOT_DEBUG_ENABLE           0U
#endif

#if BOOT_DEBUG_ENABLE
#include "SchM.h"
#define BOOT_TRACE(fmt, ...)  SchM_Log(0, fmt, ##__VA_ARGS__)
#else
#define BOOT_TRACE(fmt, ...)
#endif

#endif /* BOOT_CFG_H */