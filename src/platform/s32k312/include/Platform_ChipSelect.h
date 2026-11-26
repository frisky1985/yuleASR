/*==================================================================================================
 * Platform_ChipSelect.h - S32K3 family chip selection
 *
 * Selects the target chip within the NXP S32K3 family. Each chip variant
 * has its own set of NXP RTD register definition headers.
 *
 * Supported chips (via YULE_TARGET_CHIP CMake option):
 *   S32K312  (default) - S32K31x: Cortex-M7, 4MB Flash, 512KB SRAM
 *   S32K314            - S32K31x: Cortex-M7, 4MB Flash, 512KB SRAM
 *   S32K322            - S32K32x: Cortex-M7, 2MB Flash, 256KB SRAM
 *   S32K324            - S32K32x: Cortex-M7, 4MB Flash, 512KB SRAM
 *   S32K328            - S32K32x: Cortex-M7, 8MB Flash, 1MB SRAM
 *   S32K344            - S32K34x: Cortex-M7, 4MB Flash, 512KB SRAM
 *   S32K348            - S32K34x: Cortex-M7, 8MB Flash, 1MB SRAM
 *
 * Usage:
 *   cmake -DYULE_TARGET_CHIP=S32K312 -DYULE_USE_NXP_RTD=ON ..
 *================================================================================================*/
#ifndef PLATFORM_CHIPSELECT_H
#define PLATFORM_CHIPSELECT_H

/* Default target chip */
#if !defined(YULE_TARGET_CHIP)
#define YULE_TARGET_CHIP_S32K312
#endif

/* Chip family identification */
#if defined(YULE_TARGET_CHIP_S32K312) || defined(YULE_TARGET_CHIP_S32K314)
#define YULE_CHIP_FAMILY_S32K31X
#elif defined(YULE_TARGET_CHIP_S32K322) || defined(YULE_TARGET_CHIP_S32K324) || defined(YULE_TARGET_CHIP_S32K328)
#define YULE_CHIP_FAMILY_S32K32X
#elif defined(YULE_TARGET_CHIP_S32K344) || defined(YULE_TARGET_CHIP_S32K348)
#define YULE_CHIP_FAMILY_S32K34X
#endif

/* Chip-specific definitions */
#if defined(YULE_TARGET_CHIP_S32K312)
#define PLATFORM_CHIP_NAME    "S32K312"
#define PLATFORM_FLASH_SIZE   (4U * 1024U * 1024U)   /* 4 MB */
#define PLATFORM_SRAM_SIZE    (512U * 1024U)          /* 512 KB */
#define PLATFORM_CAN_COUNT    6U
#define PLATFORM_ADC_COUNT    2U
#define PLATFORM_LPSPI_COUNT  4U
#define PLATFORM_LPUART_COUNT 8U
#elif defined(YULE_TARGET_CHIP_S32K314)
#define PLATFORM_CHIP_NAME    "S32K314"
#define PLATFORM_FLASH_SIZE   (4U * 1024U * 1024U)
#define PLATFORM_SRAM_SIZE    (512U * 1024U)
#define PLATFORM_CAN_COUNT    6U
#define PLATFORM_ADC_COUNT    2U
#define PLATFORM_LPSPI_COUNT  4U
#define PLATFORM_LPUART_COUNT 8U
#elif defined(YULE_TARGET_CHIP_S32K344)
#define PLATFORM_CHIP_NAME    "S32K344"
#define PLATFORM_FLASH_SIZE   (4U * 1024U * 1024U)
#define PLATFORM_SRAM_SIZE    (512U * 1024U)
#define PLATFORM_CAN_COUNT    8U
#define PLATFORM_ADC_COUNT    3U
#define PLATFORM_LPSPI_COUNT  6U
#define PLATFORM_LPUART_COUNT 16U
#else
#define PLATFORM_CHIP_NAME    "S32K312"  /* fallback */
#define PLATFORM_FLASH_SIZE   (4U * 1024U * 1024U)
#define PLATFORM_SRAM_SIZE    (512U * 1024U)
#define PLATFORM_CAN_COUNT    6U
#define PLATFORM_ADC_COUNT    2U
#define PLATFORM_LPSPI_COUNT  4U
#define PLATFORM_LPUART_COUNT 8U
#endif

#endif /* PLATFORM_CHIPSELECT_H */
