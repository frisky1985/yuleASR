/******************************************************************************
 * @file    port_cm7.c
 * @brief   Cortex-M7 Port Layer Implementation
 *
 * This file implements the architecture-specific port layer for Cortex-M7:
 * - Cache management (SCB_CleanInvalidateDCache)
 * - FPU context save/restore with double-precision support
 * - MPU configuration
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/

#include "port.h"

#if defined(PORT_ARCH_CM7)

/* ARM CMSIS Core headers */
#if defined(__has_include)
    #if __has_include("core_cm7.h")
        #include "core_cm7.h"
    #elif __has_include("cmsis_gcc.h")
        #include "cmsis_gcc.h"
    #else
        /* Minimal register definitions if CMSIS not available */
        #define __CM7_CMSIS_VERSION_MAIN  (0x05U)
        #define __CM7_CMSIS_VERSION_SUB   (0x00U)
        #define __CM7_CMSIS_VERSION       ((__CM7_CMSIS_VERSION_MAIN << 16U) | \
                                          __CM7_CMSIS_VERSION_SUB)
    #endif
#else
    #include "core_cm7.h"
#endif

/******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* Cache line size in bytes (32 bytes for CM7) */
#define PORT_CM7_CACHE_LINE_SIZE        32U

/* Cache set/way calculation for CM7 */
#define PORT_CM7_DCACHE_SETS            256U   /* 16KB D-Cache: 256 sets */
#define PORT_CM7_DCACHE_WAYS            4U     /* 4-way set associative */
#define PORT_CM7_ICACHE_SETS            256U   /* 16KB I-Cache: 256 sets */
#define PORT_CM7_ICACHE_WAYS            2U     /* 2-way set associative */

/* MPU Type register bits */
#define MPU_TYPE_IREGION_Pos           16U
#define MPU_TYPE_IREGION_Msk           (0xFFUL << MPU_TYPE_IREGION_Pos)
#define MPU_TYPE_DREGION_Pos           8U
#define MPU_TYPE_DREGION_Msk           (0xFFUL << MPU_TYPE_DREGION_Pos)
#define MPU_TYPE_SEPARATE_Pos          0U
#define MPU_TYPE_SEPARATE_Msk          (1UL)

/* MPU Control register bits */
#define MPU_CTRL_PRIVDEFENA_Pos        2U
#define MPU_CTRL_PRIVDEFENA_Msk        (1UL << MPU_CTRL_PRIVDEFENA_Pos)
#define MPU_CTRL_HFNMIENA_Pos          1U
#define MPU_CTRL_HFNMIENA_Msk          (1UL << MPU_CTRL_HFNMIENA_Pos)
#define MPU_CTRL_ENABLE_Pos            0U
#define MPU_CTRL_ENABLE_Msk            (1UL << MPU_CTRL_ENABLE_Pos)

/* MPU RBAR register bits */
#define MPU_RBAR_ADDR_Pos              5U
#define MPU_RBAR_ADDR_Msk              (0x7FFFFFFUL << MPU_RBAR_ADDR_Pos)
#define MPU_RBAR_VALID_Pos             4U
#define MPU_RBAR_VALID_Msk             (1UL << MPU_RBAR_VALID_Pos)
#define MPU_RBAR_REGION_Pos            0U
#define MPU_RBAR_REGION_Msk            (0xFUL)

/* MPU RASR register bits */
#define MPU_RASR_ATTRS_Pos             16U
#define MPU_RASR_ATTRS_Msk             (0xFFFFUL << MPU_RASR_ATTRS_Pos)
#define MPU_RASR_XN_Pos                28U
#define MPU_RASR_XN_Msk                (1UL << MPU_RASR_XN_Pos)
#define MPU_RASR_AP_Pos                24U
#define MPU_RASR_AP_Msk                (0x7UL << MPU_RASR_AP_Pos)
#define MPU_RASR_TEX_Pos               19U
#define MPU_RASR_TEX_Msk               (0x7UL << MPU_RASR_TEX_Pos)
#define MPU_RASR_S_Pos                 18U
#define MPU_RASR_S_Msk                 (1UL << MPU_RASR_S_Pos)
#define MPU_RASR_C_Pos                 17U
#define MPU_RASR_C_Msk                 (1UL << MPU_RASR_C_Pos)
#define MPU_RASR_B_Pos                 16U
#define MPU_RASR_B_Msk                 (1UL << MPU_RASR_B_Pos)
#define MPU_RASR_SRD_Pos               8U
#define MPU_RASR_SRD_Msk               (0xFFUL << MPU_RASR_SRD_Pos)
#define MPU_RASR_SIZE_Pos              1U
#define MPU_RASR_SIZE_Msk              (0x1FUL << MPU_RASR_SIZE_Pos)
#define MPU_RASR_ENABLE_Pos            0U
#define MPU_RASR_ENABLE_Msk            (1UL)

/* FPU CPACR bits */
#define CPACR_CP10_Pos                 (2U * 10)
#define CPACR_CP10_Msk                 (3UL << CPACR_CP10_Pos)
#define CPACR_CP11_Pos                 (2U * 11)
#define CPACR_CP11_Msk                 (3UL << CPACR_CP11_Pos)
#define CPACR_CP_FULL_ACCESS           (3UL)

/* FPU FPCCR bits */
#define FPCCR_ASPEN_Pos                31U
#define FPCCR_ASPEN_Msk                (1UL << FPCCR_ASPEN_Pos)
#define FPCCR_LSPEN_Pos                30U
#define FPCCR_LSPEN_Msk                (1UL << FPCCR_LSPEN_Pos)

/******************************************************************************
 * Private Functions - Register Access
 ******************************************************************************/

__attribute__((always_inline)) static inline void port_dsb(void)
{
    __asm volatile ("dsb" ::: "memory");
}

__attribute__((always_inline)) static inline void port_isb(void)
{
    __asm volatile ("isb" ::: "memory");
}

__attribute__((always_inline)) static inline void port_dmb(void)
{
    __asm volatile ("dmb" ::: "memory");
}

/******************************************************************************
 * Cache Operations Implementation
 ******************************************************************************/

/**
 * @brief Enable data cache
 */
void Port_Cache_EnableDCache(void)
{
    uint32_t ccsidr;
    uint32_t sets;
    uint32_t ways;

    /* Check if D-Cache is already enabled */
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U) {
        return;
    }

    /* Invalidate D-Cache before enabling */
    SCB->CSSELR = 0U;  /* Select D-Cache */
    port_dsb();
    ccsidr = SCB->CCSIDR;

    /* Get number of sets and ways */
    sets = ((ccsidr >> 13U) & 0x7FFFU) + 1U;
    ways = ((ccsidr >> 3U) & 0x3FFU) + 1U;

    /* Invalidate all sets and ways */
    for (uint32_t set = 0U; set < sets; set++) {
        for (uint32_t way = 0U; way < ways; way++) {
            uint32_t setway = ((way << 30U) | (set << 5U));
            SCB->DCISW = setway;
        }
    }
    port_dsb();

    /* Enable D-Cache */
    SCB->CCR |= SCB_CCR_DC_Msk;
    port_dsb();
    port_isb();
}

/**
 * @brief Disable data cache
 */
void Port_Cache_DisableDCache(void)
{
    uint32_t ccsidr;
    uint32_t sets;
    uint32_t ways;

    /* Check if D-Cache is enabled */
    if ((SCB->CCR & SCB_CCR_DC_Msk) == 0U) {
        return;
    }

    /* Clean and invalidate D-Cache before disabling */
    SCB->CSSELR = 0U;  /* Select D-Cache */
    port_dsb();
    ccsidr = SCB->CCSIDR;

    /* Get number of sets and ways */
    sets = ((ccsidr >> 13U) & 0x7FFFU) + 1U;
    ways = ((ccsidr >> 3U) & 0x3FFU) + 1U;

    /* Clean and invalidate all sets and ways */
    for (uint32_t set = 0U; set < sets; set++) {
        for (uint32_t way = 0U; way < ways; way++) {
            uint32_t setway = ((way << 30U) | (set << 5U));
            SCB->DCCISW = setway;
        }
    }
    port_dsb();

    /* Disable D-Cache */
    SCB->CCR &= ~SCB_CCR_DC_Msk;
    port_dsb();
    port_isb();
}

/**
 * @brief Clean data cache by address (recommended for DMA operations)
 */
void Port_Cache_CleanDCache_by_Addr(uint32_t *addr, int32_t size)
{
    if ((addr == NULL) || (size <= 0)) {
        return;
    }

    int32_t num_lines = (size + (PORT_CM7_CACHE_LINE_SIZE - 1)) / PORT_CM7_CACHE_LINE_SIZE;
    uint32_t aligned_addr = ((uint32_t)addr) & ~(PORT_CM7_CACHE_LINE_SIZE - 1U);

    /* Clean cache lines */
    for (int32_t i = 0; i < num_lines; i++) {
        SCB->DCCMVAC = aligned_addr + (i * PORT_CM7_CACHE_LINE_SIZE);
    }
    port_dsb();
}

/**
 * @brief Invalidate data cache by address
 */
void Port_Cache_InvalidateDCache_by_Addr(uint32_t *addr, int32_t size)
{
    if ((addr == NULL) || (size <= 0)) {
        return;
    }

    int32_t num_lines = (size + (PORT_CM7_CACHE_LINE_SIZE - 1)) / PORT_CM7_CACHE_LINE_SIZE;
    uint32_t aligned_addr = ((uint32_t)addr) & ~(PORT_CM7_CACHE_LINE_SIZE - 1U);

    /* Invalidate cache lines */
    for (int32_t i = 0; i < num_lines; i++) {
        SCB->DCIMVAC = aligned_addr + (i * PORT_CM7_CACHE_LINE_SIZE);
    }
    port_dsb();
}

/**
 * @brief Clean and invalidate data cache by address (for DMA buffers)
 */
void Port_Cache_CleanInvalidateDCache_by_Addr(uint32_t *addr, int32_t size)
{
    if ((addr == NULL) || (size <= 0)) {
        return;
    }

    int32_t num_lines = (size + (PORT_CM7_CACHE_LINE_SIZE - 1)) / PORT_CM7_CACHE_LINE_SIZE;
    uint32_t aligned_addr = ((uint32_t)addr) & ~(PORT_CM7_CACHE_LINE_SIZE - 1U);

    /* Clean and invalidate cache lines */
    for (int32_t i = 0; i < num_lines; i++) {
        SCB->DCCIMVAC = aligned_addr + (i * PORT_CM7_CACHE_LINE_SIZE);
    }
    port_dsb();
}

/**
 * @brief Clean entire data cache
 */
void Port_Cache_CleanDCache(void)
{
    /* Clean all D-Cache by set/way */
    SCB->CSSELR = 0U;
    port_dsb();

    uint32_t ccsidr = SCB->CCSIDR;
    uint32_t sets = ((ccsidr >> 13U) & 0x7FFFU) + 1U;
    uint32_t ways = ((ccsidr >> 3U) & 0x3FFU) + 1U;

    for (uint32_t set = 0U; set < sets; set++) {
        for (uint32_t way = 0U; way < ways; way++) {
            uint32_t setway = ((way << 30U) | (set << 5U));
            SCB->DCCSW = setway;
        }
    }
    port_dsb();
}

/**
 * @brief Invalidate entire data cache
 */
void Port_Cache_InvalidateDCache(void)
{
    /* Invalidate all D-Cache by set/way */
    SCB->CSSELR = 0U;
    port_dsb();

    uint32_t ccsidr = SCB->CCSIDR;
    uint32_t sets = ((ccsidr >> 13U) & 0x7FFFU) + 1U;
    uint32_t ways = ((ccsidr >> 3U) & 0x3FFU) + 1U;

    for (uint32_t set = 0U; set < sets; set++) {
        for (uint32_t way = 0U; way < ways; way++) {
            uint32_t setway = ((way << 30U) | (set << 5U));
            SCB->DCISW = setway;
        }
    }
    port_dsb();
}

/**
 * @brief Clean and invalidate entire data cache
 */
void Port_Cache_CleanInvalidateDCache(void)
{
    /* Clean and invalidate all D-Cache by set/way */
    SCB->CSSELR = 0U;
    port_dsb();

    uint32_t ccsidr = SCB->CCSIDR;
    uint32_t sets = ((ccsidr >> 13U) & 0x7FFFU) + 1U;
    uint32_t ways = ((ccsidr >> 3U) & 0x3FFU) + 1U;

    for (uint32_t set = 0U; set < sets; set++) {
        for (uint32_t way = 0U; way < ways; way++) {
            uint32_t setway = ((way << 30U) | (set << 5U));
            SCB->DCCISW = setway;
        }
    }
    port_dsb();
}

/**
 * @brief Enable instruction cache
 */
void Port_Cache_EnableICache(void)
{
    /* Check if I-Cache is already enabled */
    if ((SCB->CCR & SCB_CCR_IC_Msk) != 0U) {
        return;
    }

    /* Invalidate I-Cache before enabling */
    SCB->ICIALLU = 0U;
    port_dsb();
    port_isb();

    /* Enable I-Cache */
    SCB->CCR |= SCB_CCR_IC_Msk;
    port_dsb();
    port_isb();
}

/**
 * @brief Disable instruction cache
 */
void Port_Cache_DisableICache(void)
{
    /* Check if I-Cache is enabled */
    if ((SCB->CCR & SCB_CCR_IC_Msk) == 0U) {
        return;
    }

    /* Disable I-Cache */
    SCB->CCR &= ~SCB_CCR_IC_Msk;
    port_dsb();
    port_isb();

    /* Invalidate I-Cache after disabling */
    SCB->ICIALLU = 0U;
    port_dsb();
    port_isb();
}

/**
 * @brief Invalidate instruction cache
 */
void Port_Cache_InvalidateICache(void)
{
    SCB->ICIALLU = 0U;
    port_dsb();
    port_isb();
}

/******************************************************************************
 * FPU Implementation (Double Precision)
 ******************************************************************************/

/**
 * @brief Initialize FPU context management for CM7
 */
void Port_FPU_Init(void)
{
    /* Enable FPU coprocessor access (CP10 and CP11) */
    SCB->CPACR |= ((CPACR_CP_FULL_ACCESS << CPACR_CP10_Pos) |
                   (CPACR_CP_FULL_ACCESS << CPACR_CP11_Pos));
    port_dsb();
    port_isb();

    /* Configure lazy FP context save (recommended for RTOS) */
    FPU->FPCCR &= ~(FPCCR_ASPEN_Msk | FPCCR_LSPEN_Msk);

    #if defined(FPU_USE_LAZY_STATE_PRESERVATION)
        /* Enable lazy state preservation */
        FPU->FPCCR |= FPCCR_LSPEN_Msk;
    #else
        /* Enable automatic state preservation (safest for RTOS) */
        FPU->FPCCR |= FPCCR_ASPEN_Msk;
    #endif

    port_dsb();
    port_isb();
}

/**
 * @brief Enable FPU
 */
void Port_FPU_Enable(void)
{
    SCB->CPACR |= ((CPACR_CP_FULL_ACCESS << CPACR_CP10_Pos) |
                   (CPACR_CP_FULL_ACCESS << CPACR_CP11_Pos));
    port_dsb();
    port_isb();
}

/**
 * @brief Disable FPU
 */
void Port_FPU_Disable(void)
{
    SCB->CPACR &= ~((CPACR_CP_FULL_ACCESS << CPACR_CP10_Pos) |
                    (CPACR_CP_FULL_ACCESS << CPACR_CP11_Pos));
    port_dsb();
    port_isb();
}

/**
 * @brief Check if FPU is enabled
 */
bool Port_FPU_IsEnabled(void)
{
    uint32_t cpacr = SCB->CPACR;
    uint32_t cp10 = (cpacr >> CPACR_CP10_Pos) & 0x3UL;
    uint32_t cp11 = (cpacr >> CPACR_CP11_Pos) & 0x3UL;
    return ((cp10 == CPACR_CP_FULL_ACCESS) && (cp11 == CPACR_CP_FULL_ACCESS));
}

/**
 * @brief Get FPU type (CM7 has double precision)
 */
int32_t Port_FPU_GetType(void)
{
    /* CM7 has double-precision FPU (FPv5-D16) */
    if ((FPU->MVFR0 & FPU_MVFR0_FPSP_Msk) != 0U) {
        if ((FPU->MVFR0 & FPU_MVFR0_FPDP_Msk) != 0U) {
            return 1;  /* Double precision */
        }
        return 0;  /* Single precision only */
    }
    return -1;  /* No FPU */
}

/**
 * @brief Set lazy FPU state preservation
 */
void Port_FPU_SetLazyState(bool enable)
{
    if (enable) {
        FPU->FPCCR &= ~FPCCR_ASPEN_Msk;
        FPU->FPCCR |= FPCCR_LSPEN_Msk;
    } else {
        FPU->FPCCR &= ~FPCCR_LSPEN_Msk;
        FPU->FPCCR |= FPCCR_ASPEN_Msk;
    }
    port_dsb();
}

/******************************************************************************
 * MPU Implementation
 ******************************************************************************/

static volatile uint32_t port_mpu_region_count = 0U;

/**
 * @brief Initialize MPU
 */
void Port_MPU_Init(void)
{
    /* Get number of supported regions from MPU_TYPE register */
    port_mpu_region_count = ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos);

    /* Disable MPU before configuration */
    Port_MPU_Disable();

    /* Clear all regions */
    for (uint32_t i = 0U; i < port_mpu_region_count; i++) {
        MPU->RNR = i;
        MPU->RBAR = 0U;
        MPU->RASR = 0U;
    }
    port_dsb();
}

/**
 * @brief Enable MPU
 */
void Port_MPU_Enable(bool privDefault)
{
    uint32_t ctrl = MPU_CTRL_ENABLE_Msk;

    if (privDefault) {
        ctrl |= MPU_CTRL_PRIVDEFENA_Msk;
    }

    /* Enable MPU in privileged mode */
    MPU->CTRL = ctrl;
    port_dsb();
    port_isb();
}

/**
 * @brief Disable MPU
 */
void Port_MPU_Disable(void)
{
    MPU->CTRL = 0U;
    port_dsb();
    port_isb();
}

/**
 * @brief Configure MPU region
 */
bool Port_MPU_ConfigRegion(uint32_t region, const Port_MPU_RegionCfgType *config)
{
    if ((config == NULL) || (region >= port_mpu_region_count)) {
        return false;
    }

    /* Validate region size is power of 2 and minimum 32 bytes */
    if ((config->size < 32U) || ((config->size & (config->size - 1U)) != 0U)) {
        return false;
    }

    /* Calculate region size encoding */
    uint32_t size_encoding = 0U;
    uint32_t size = config->size;
    while (size > 1U) {
        size >>= 1U;
        size_encoding++;
    }

    /* Build RASR register */
    uint32_t rasr = 0U;

    /* Attributes: TEX, C, B, S */
    switch (config->attributes) {
        case PORT_MPU_ATTR_STRONGLY_ORDERED:
            /* TEX=000, C=0, B=0, S=1 */
            rasr |= (0x0U << MPU_RASR_TEX_Pos);
            break;
        case PORT_MPU_ATTR_DEVICE:
            /* TEX=000, C=0, B=1, S=1 */
            rasr |= (1U << MPU_RASR_B_Pos);
            break;
        case PORT_MPU_ATTR_NORMAL_WT:
            /* TEX=000, C=1, B=0, S=1 */
            rasr |= (1U << MPU_RASR_C_Pos);
            rasr |= (1U << MPU_RASR_S_Pos);
            break;
        case PORT_MPU_ATTR_NORMAL_WB:
            /* TEX=001, C=1, B=1, S=1 */
            rasr |= (1U << MPU_RASR_TEX_Pos);
            rasr |= (1U << MPU_RASR_C_Pos);
            rasr |= (1U << MPU_RASR_B_Pos);
            rasr |= (1U << MPU_RASR_S_Pos);
            break;
        case PORT_MPU_ATTR_NORMAL_NONCACHE:
            /* TEX=001, C=0, B=0, S=1 */
            rasr |= (1U << MPU_RASR_TEX_Pos);
            rasr |= (1U << MPU_RASR_S_Pos);
            break;
        default:
            /* Default to strongly ordered */
            break;
    }

    /* Access permissions */
    rasr |= ((config->accessPermission << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk);

    /* Execute Never */
    if (config->executeNever) {
        rasr |= MPU_RASR_XN_Msk;
    }

    /* Sub-region disable */
    rasr |= ((config->subRegionDisable << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk);

    /* Region size */
    rasr |= ((size_encoding << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk);

    /* Enable bit */
    if (config->enable) {
        rasr |= MPU_RASR_ENABLE_Msk;
    }

    /* Write region configuration */
    MPU->RNR = region;
    MPU->RBAR = (config->baseAddr & MPU_RBAR_ADDR_Msk);
    MPU->RASR = rasr;

    port_dsb();
    port_isb();

    return true;
}

/**
 * @brief Enable MPU region
 */
void Port_MPU_EnableRegion(uint32_t region)
{
    if (region < port_mpu_region_count) {
        MPU->RNR = region;
        MPU->RASR |= MPU_RASR_ENABLE_Msk;
        port_dsb();
    }
}

/**
 * @brief Disable MPU region
 */
void Port_MPU_DisableRegion(uint32_t region)
{
    if (region < port_mpu_region_count) {
        MPU->RNR = region;
        MPU->RASR &= ~MPU_RASR_ENABLE_Msk;
        port_dsb();
    }
}

/**
 * @brief Get current MPU configuration
 */
bool Port_MPU_GetRegionConfig(uint32_t region, Port_MPU_RegionCfgType *config)
{
    if ((config == NULL) || (region >= port_mpu_region_count)) {
        return false;
    }

    MPU->RNR = region;

    uint32_t rbar = MPU->RBAR;
    uint32_t rasr = MPU->RASR;

    config->baseAddr = rbar & MPU_RBAR_ADDR_Msk;
    config->enable = ((rasr & MPU_RASR_ENABLE_Msk) != 0U);
    config->executeNever = ((rasr & MPU_RASR_XN_Msk) != 0U);
    config->accessPermission = (rasr & MPU_RASR_AP_Msk) >> MPU_RASR_AP_Pos;
    config->subRegionDisable = (rasr & MPU_RASR_SRD_Msk) >> MPU_RASR_SRD_Pos;

    /* Decode size */
    uint32_t size_encoding = (rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos;
    config->size = 1U << size_encoding;

    /* Decode attributes (simplified) */
    uint32_t tex = (rasr & MPU_RASR_TEX_Msk) >> MPU_RASR_TEX_Pos;
    uint32_t c = (rasr & MPU_RASR_C_Msk) >> MPU_RASR_C_Pos;
    uint32_t b = (rasr & MPU_RASR_B_Msk) >> MPU_RASR_B_Pos;

    if (tex == 0U) {
        if (c == 0U && b == 0U) {
            config->attributes = PORT_MPU_ATTR_STRONGLY_ORDERED;
        } else if (c == 0U && b == 1U) {
            config->attributes = PORT_MPU_ATTR_DEVICE;
        } else if (c == 1U && b == 0U) {
            config->attributes = PORT_MPU_ATTR_NORMAL_WT;
        } else {
            config->attributes = PORT_MPU_ATTR_NORMAL_WB;
        }
    } else {
        config->attributes = PORT_MPU_ATTR_NORMAL_NONCACHE;
    }

    return true;
}

/******************************************************************************
 * System Control Implementation
 ******************************************************************************/

/**
 * @brief Enable interrupts
 */
void Port_System_EnableIRQ(void)
{
    __enable_irq();
}

/**
 * @brief Disable interrupts
 */
void Port_System_DisableIRQ(void)
{
    __disable_irq();
}

/**
 * @brief Check if interrupts are enabled
 */
bool Port_System_IRQEnabled(void)
{
    uint32_t primask = __get_PRIMASK();
    return (primask == 0U);
}

/**
 * @brief Data Synchronization Barrier
 */
void Port_System_DSB(void)
{
    port_dsb();
}

/**
 * @brief Instruction Synchronization Barrier
 */
void Port_System_ISB(void)
{
    port_isb();
}

/**
 * @brief Data Memory Barrier
 */
void Port_System_DMB(void)
{
    port_dmb();
}

/**
 * @brief System reset
 */
void Port_System_Reset(void)
{
    port_dsb();
    NVIC_SystemReset();
}

/**
 * @brief Get unique device ID (STM32 format)
 */
uint32_t Port_System_GetUniqueID(uint8_t *id_buf)
{
    if (id_buf == NULL) {
        return 0U;
    }

    /* STM32 unique ID at 0x1FF0F420 (96 bits / 12 bytes) */
    const uint32_t *uid_ptr = (const uint32_t *)0x1FF0F420U;

    for (uint32_t i = 0U; i < 3U; i++) {
        uint32_t word = uid_ptr[i];
        id_buf[i * 4U + 0U] = (uint8_t)(word & 0xFFU);
        id_buf[i * 4U + 1U] = (uint8_t)((word >> 8U) & 0xFFU);
        id_buf[i * 4U + 2U] = (uint8_t)((word >> 16U) & 0xFFU);
        id_buf[i * 4U + 3U] = (uint8_t)((word >> 24U) & 0xFFU);
    }

    return 12U;  /* 12 bytes written */
}

/******************************************************************************
 * Architecture Initialization
 ******************************************************************************/

/**
 * @brief Initialize architecture-specific features for CM7
 */
void Port_Arch_Init(void)
{
    /* Enable FPU if present */
    Port_FPU_Init();

    /* Initialize MPU */
    Port_MPU_Init();

    /* Configure cache (can be enabled later by application) */
    /* Port_Cache_EnableDCache(); */
    /* Port_Cache_EnableICache(); */
}

/**
 * @brief Get architecture type
 */
Port_ArchType Port_Arch_GetType(void)
{
    return PORT_ARCH_CM7;
}

/**
 * @brief Get architecture name
 */
const char* Port_Arch_GetName(void)
{
    return "Cortex-M7";
}

#endif /* PORT_ARCH_CM7 */
