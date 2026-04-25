/******************************************************************************
 * @file    port_cm33.c
 * @brief   Cortex-M33 Port Layer Implementation
 *
 * This file implements the architecture-specific port layer for Cortex-M33:
 * - TrustZone boundary configuration
 * - SAU (Security Attribution Unit) configuration
 * - IDAU (Implementation Defined Attribution Unit) support
 * - Secure and Non-secure context switching
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/

#include "port.h"

#if defined(PORT_ARCH_CM33) || defined(PORT_ARCH_CM33_NTZ)

/* ARM CMSIS Core headers */
#if defined(__has_include)
    #if __has_include("core_cm33.h")
        #include "core_cm33.h"
    #elif __has_include("cmsis_gcc.h")
        #include "cmsis_gcc.h"
    #else
        /* Minimal register definitions if CMSIS not available */
        #define __CM33_CMSIS_VERSION_MAIN  (0x05U)
        #define __CM33_CMSIS_VERSION_SUB   (0x00U)
        #define __CM33_CMSIS_VERSION       ((__CM33_CMSIS_VERSION_MAIN << 16U) | \
                                           __CM33_CMSIS_VERSION_SUB)
    #endif
#else
    #include "core_cm33.h"
#endif

/******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* SAU register definitions */
#define SAU_CTRL_ENABLE_Pos            0U
#define SAU_CTRL_ENABLE_Msk            (1UL << SAU_CTRL_ENABLE_Pos)
#define SAU_CTRL_ALLNS_Pos             1U
#define SAU_CTRL_ALLNS_Msk             (1UL << SAU_CTRL_ALLNS_Pos)

#define SAU_RNR_REGION_Pos             0U
#define SAU_RNR_REGION_Msk             (0xFFUL << SAU_RNR_REGION_Pos)

#define SAU_RBAR_BASE_Pos              5U
#define SAU_RBAR_BASE_Msk              (0x7FFFFFFUL << SAU_RBAR_BASE_Pos)

#define SAU_RLAR_LIMIT_Pos             5U
#define SAU_RLAR_LIMIT_Msk             (0x7FFFFFFUL << SAU_RLAR_LIMIT_Pos)
#define SAU_RLAR_NSC_Pos               1U
#define SAU_RLAR_NSC_Msk               (1UL << SAU_RLAR_NSC_Pos)
#define SAU_RLAR_ENABLE_Pos            0U
#define SAU_RLAR_ENABLE_Msk            (1UL)

/* IDAU register definitions */
#define IDAU_CTRL_INITSVTOR_Pos        0U
#define IDAU_CTRL_INITSVTOR_Msk        (1UL << IDAU_CTRL_INITSVTOR_Pos)
#define IDAU_CTRL_CPUWAIT_Pos          1U
#define IDAU_CTRL_CPUWAIT_Msk          (1UL << IDAU_CTRL_CPUWAIT_Pos)

/* TT instruction response bits */
#define TT_RESP_MREGION_Pos            0U
#define TT_RESP_MREGION_Msk            (0xFFUL << TT_RESP_MREGION_Pos)
#define TT_RESP_S_Pos                  22U
#define TT_RESP_S_Msk                  (1UL << TT_RESP_S_Pos)
#define TT_RESP_NSREQ_Pos              23U
#define TT_RESP_NSREQ_Msk              (1UL << TT_RESP_NSREQ_Pos)
#define TT_RESP_RW_Pos                 24U
#define TT_RESP_RW_Msk                 (1UL << TT_RESP_RW_Pos)

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
#define FPCCR_TS_Pos                   26U
#define FPCCR_TS_Msk                   (1UL << FPCCR_TS_Pos)
#define FPCCR_S_Pos                    2U
#define FPCCR_S_Msk                    (1UL << FPCCR_S_Pos)

/* MPU Type register bits */
#define MPU_TYPE_DREGION_Pos           8U
#define MPU_TYPE_DREGION_Msk           (0xFFUL << MPU_TYPE_DREGION_Pos)

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

/* Security state defines */
#define SECURITY_STATE_SECURE          0U
#define SECURITY_STATE_NONSECURE       1U

/* Maximum SAU regions */
#define PORT_SAU_MAX_REGIONS           8U

/******************************************************************************
 * Private Variables
 ******************************************************************************/

#if PORT_HAS_TRUSTZONE
/* SAU state tracking */
static volatile uint32_t port_sau_region_count = PORT_SAU_MAX_REGIONS;
static volatile uint8_t port_current_security_state = SECURITY_STATE_SECURE;

/* NSC (Non-Secure Callable) region tracking */
static struct {
    uint32_t base;
    uint32_t limit;
    bool enabled;
} port_nsc_region = { 0U, 0U, false };
#endif

static volatile uint32_t port_mpu_region_count = 0U;

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

#if PORT_HAS_TRUSTZONE
/**
 * @brief Execute SG (Secure Gateway) instruction
 * Used to transition from Non-secure to Secure state
 */
__attribute__((always_inline)) static inline void port_sg_instruction(void)
{
    __asm volatile ("sg" ::: "memory");
}

/**
 * @brief Test Target instruction - check memory security attributes
 */
__attribute__((always_inline)) static inline uint32_t port_tt_instruction(uint32_t addr)
{
    uint32_t result;
    __asm volatile ("tt %0, %1" : "=r" (result) : "r" (addr));
    return result;
}

/**
 * @brief Test Target Non-secure instruction
 */
__attribute__((always_inline)) static inline uint32_t port_ttt_instruction(uint32_t addr)
{
    uint32_t result;
    __asm volatile ("ttt %0, %1" : "=r" (result) : "r" (addr));
    return result;
}
#endif /* PORT_HAS_TRUSTZONE */

/******************************************************************************
 * TrustZone Implementation
 ******************************************************************************/

#if PORT_HAS_TRUSTZONE

/**
 * @brief Initialize TrustZone support for CM33
 */
void Port_TrustZone_Init(void)
{
    /* Initialize IDAU first (affects initial security state) */
    Port_IDAU_Init();

    /* Initialize SAU */
    Port_SAU_Init();

    /* Determine current security state */
    port_current_security_state = Port_TrustZone_GetSecurityState();
}

/**
 * @brief Get current security state
 * @return PORT_SECURITY_SECURE or PORT_SECURITY_NONSECURE
 */
uint32_t Port_TrustZone_GetSecurityState(void)
{
    uint32_t value;
    /* Read special register for security state */
    __asm volatile ("mrs %0, control" : "=r" (value));

    /* Check CONTROL register bit 0 for security state on exception return */
    /* Or use TT on known secure address */
    uint32_t tt_resp = port_tt_instruction((uint32_t)Port_TrustZone_GetSecurityState);

    if ((tt_resp & TT_RESP_S_Msk) != 0U) {
        port_current_security_state = SECURITY_STATE_SECURE;
        return PORT_SECURITY_SECURE;
    } else {
        port_current_security_state = SECURITY_STATE_NONSECURE;
        return PORT_SECURITY_NONSECURE;
    }
}

/**
 * @brief Switch to Non-secure state
 * @param ns_entry Non-secure entry point
 */
void Port_TrustZone_SwitchToNonSecure(uint32_t ns_entry)
{
    /* Configure Non-secure VTOR (if in secure state) */
    if (Port_TrustZone_GetSecurityState() == PORT_SECURITY_SECURE) {
        /* Set Non-secure VTOR */
        uint32_t ns_vtor = ns_entry & 0xFFFFFF80U;  /* Aligned to 128 bytes */

        /* Write to NS_VTOR register */
        __asm volatile ("msr vtor_ns, %0" : : "r" (ns_vtor));
        port_isb();

        /* Prepare for Non-secure entry */
        /* Set LR to return to Non-secure handler */
        __asm volatile ("mov lr, %0" : : "r" (0xFFFFFFBCU));  /* EXC_RETURN for NS thread mode */

        /* Branch to Non-secure code */
        __asm volatile (
            "bxns %0"
            :
            : "r" (ns_entry)
        );
    }
}

/**
 * @brief Initialize SAU (Security Attribution Unit)
 */
void Port_SAU_Init(void)
{
    /* Disable SAU before configuration */
    Port_SAU_Disable();

    /* Clear all SAU regions */
    for (uint32_t i = 0U; i < PORT_SAU_MAX_REGIONS; i++) {
        SAU->RNR = i;
        SAU->RBAR = 0U;
        SAU->RLAR = 0U;
    }
    port_dsb();

    /* Default configuration: All Non-Secure regions must be explicitly defined */
    /* Do not set ALLNS bit - keep strict security */
}

/**
 * @brief Configure SAU region
 * @param region Region number (0-7)
 * @param config Region configuration
 * @return true if successful
 */
bool Port_SAU_ConfigRegion(uint32_t region, const Port_SAU_RegionCfgType *config)
{
    if ((config == NULL) || (region >= PORT_SAU_MAX_REGIONS)) {
        return false;
    }

    /* Validate addresses are aligned to 32 bytes */
    if (((config->baseAddr & 0x1FU) != 0U) || ((config->limitAddr & 0x1FU) != 0U)) {
        return false;
    }

    /* Ensure base < limit */
    if (config->baseAddr >= config->limitAddr) {
        return false;
    }

    /* Select region */
    SAU->RNR = region;

    /* Configure base address */
    SAU->RBAR = (config->baseAddr & SAU_RBAR_BASE_Msk);

    /* Configure limit address and attributes */
    uint32_t rlar = (config->limitAddr & SAU_RLAR_LIMIT_Msk);

    /* Non-Secure Callable attribute */
    if (config->nsc) {
        rlar |= SAU_RLAR_NSC_Msk;
    }

    /* Enable region */
    if (config->enable) {
        rlar |= SAU_RLAR_ENABLE_Msk;
    }

    SAU->RLAR = rlar;

    port_dsb();
    port_isb();

    return true;
}

/**
 * @brief Enable SAU
 */
void Port_SAU_Enable(void)
{
    SAU->CTRL |= SAU_CTRL_ENABLE_Msk;
    port_dsb();
    port_isb();
}

/**
 * @brief Disable SAU
 */
void Port_SAU_Disable(void)
{
    SAU->CTRL &= ~SAU_CTRL_ENABLE_Msk;
    port_dsb();
    port_isb();
}

/**
 * @brief Configure IDAU (Implementation Defined Attribution Unit)
 * Note: IDAU is typically read-only or pre-configured in hardware
 */
void Port_IDAU_Init(void)
{
    /* CM33 IDAU is typically implemented in hardware */
    /* Read IDAU configuration to understand current setup */

    /* For STM32L5/U5 series, IDAU divides memory map:
     * 0x0000_0000 - 0x0FFF_FFFF: Secure alias
     * 0x1000_0000 - 0x1FFF_FFFF: Non-secure alias
     * 0x2000_0000 - 0x2FFF_FFFF: Secure alias
     * etc.
     */

    /* Check IDAU type register for region count (if available) */
    port_dsb();
}

/**
 * @brief Secure context save (called on secure context switch)
 */
void Port_TrustZone_SaveSecureContext(void)
{
    /* Save secure context if needed */
    /* This is called before switching to Non-secure */

    /* Clear secure registers that shouldn't leak to NS */
    /* Push any secure data to stack */

    port_dsb();
}

/**
 * @brief Secure context restore (called on secure context switch)
 */
void Port_TrustZone_RestoreSecureContext(void)
{
    /* Restore secure context after returning from NS */

    /* Reload secure registers */
    /* Pop secure data from stack */

    port_dsb();
}

/**
 * @brief Configure Non-secure callable region
 * @param base Base address of SG veneers
 * @param size Size of region
 */
void Port_TrustZone_ConfigNSCRegion(uint32_t base, uint32_t size)
{
    if (size == 0U) {
        port_nsc_region.enabled = false;
        return;
    }

    port_nsc_region.base = base & ~0x1FU;  /* Align to 32 bytes */
    port_nsc_region.limit = port_nsc_region.base + ((size + 31U) & ~31U) - 1U;
    port_nsc_region.enabled = true;

    /* Configure as SAU region with NSC attribute */
    Port_SAU_RegionCfgType nsc_config = {
        .baseAddr = port_nsc_region.base,
        .limitAddr = port_nsc_region.limit,
        .nsc = 1U,      /* Non-Secure Callable */
        .enable = true
    };

    /* Use last SAU region for NSC */
    Port_SAU_ConfigRegion(PORT_SAU_MAX_REGIONS - 1U, &nsc_config);
}

/**
 * @brief Validate if address is in secure memory
 * @param addr Address to check
 * @return true if secure, false if non-secure
 */
bool Port_TrustZone_IsSecureAddress(uint32_t addr)
{
    uint32_t tt_resp = port_tt_instruction(addr);
    return ((tt_resp & TT_RESP_S_Msk) != 0U);
}

#endif /* PORT_HAS_TRUSTZONE */

/******************************************************************************
 * FPU Implementation (Single Precision for CM33)
 ******************************************************************************/

#if PORT_HAS_FPU

/**
 * @brief Initialize FPU context management for CM33
 */
void Port_FPU_Init(void)
{
    /* Enable FPU coprocessor access (CP10 and CP11) */
    SCB->CPACR |= ((CPACR_CP_FULL_ACCESS << CPACR_CP10_Pos) |
                   (CPACR_CP_FULL_ACCESS << CPACR_CP11_Pos));
    port_dsb();
    port_isb();

    /* Configure FP context save for TrustZone if enabled */
    #if PORT_HAS_TRUSTZONE
        /* Enable FP security state extension */
        FPU->FPCCR &= ~FPCCR_TS_Msk;  /* Disable TS (use separate for NS) */
        FPU->FPCCR &= ~FPCCR_S_Msk;   /* Ensure FP is treated as non-secure */
    #endif

    /* Configure lazy FP context save */
    FPU->FPCCR &= ~(FPCCR_ASPEN_Msk | FPCCR_LSPEN_Msk);

    #if defined(FPU_USE_LAZY_STATE_PRESERVATION)
        /* Enable lazy state preservation */
        FPU->FPCCR |= FPCCR_LSPEN_Msk;
    #else
        /* Enable automatic state preservation */
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
 * @brief Get FPU type (CM33 has single precision only)
 */
int32_t Port_FPU_GetType(void)
{
    /* CM33 has single-precision FPU (FPv5-SP) */
    if ((FPU->MVFR0 & FPU_MVFR0_FPSP_Msk) != 0U) {
        /* Check if double precision is available */
        if ((FPU->MVFR0 & FPU_MVFR0_FPDP_Msk) != 0U) {
            return 1;  /* Double precision (shouldn't happen on CM33) */
        }
        return 0;  /* Single precision */
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

#else /* !PORT_HAS_FPU */

void Port_FPU_Init(void) { }
void Port_FPU_Enable(void) { }
void Port_FPU_Disable(void) { }
bool Port_FPU_IsEnabled(void) { return false; }
int32_t Port_FPU_GetType(void) { return -1; }
void Port_FPU_SetLazyState(bool enable) { (void)enable; }

#endif /* PORT_HAS_FPU */

/******************************************************************************
 * Cache Operations (CM33 has no cache - stubs)
 ******************************************************************************/

void Port_Cache_EnableDCache(void) { }
void Port_Cache_DisableDCache(void) { }
void Port_Cache_CleanDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
void Port_Cache_InvalidateDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
void Port_Cache_CleanInvalidateDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
void Port_Cache_CleanDCache(void) { }
void Port_Cache_InvalidateDCache(void) { }
void Port_Cache_CleanInvalidateDCache(void) { }
void Port_Cache_EnableICache(void) { }
void Port_Cache_DisableICache(void) { }
void Port_Cache_InvalidateICache(void) { }

/******************************************************************************
 * MPU Implementation
 ******************************************************************************/

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
 * @brief Get unique device ID (STM32 format for L5/U5)
 */
uint32_t Port_System_GetUniqueID(uint8_t *id_buf)
{
    if (id_buf == NULL) {
        return 0U;
    }

    /* STM32L5/U5 unique ID at 0x0BFA0700 (96 bits / 12 bytes) */
    const uint32_t *uid_ptr = (const uint32_t *)0x0BFA0700U;

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
 * @brief Initialize architecture-specific features for CM33
 */
void Port_Arch_Init(void)
{
    #if PORT_HAS_TRUSTZONE
        /* Initialize TrustZone first */
        Port_TrustZone_Init();
    #endif

    /* Enable FPU if present */
    Port_FPU_Init();

    /* Initialize MPU */
    Port_MPU_Init();
}

/**
 * @brief Get architecture type
 */
Port_ArchType Port_Arch_GetType(void)
{
    #if defined(PORT_ARCH_CM33)
        return PORT_ARCH_CM33;
    #else
        return PORT_ARCH_CM33_NTZ;
    #endif
}

/**
 * @brief Get architecture name
 */
const char* Port_Arch_GetName(void)
{
    #if defined(PORT_ARCH_CM33)
        return "Cortex-M33 (TrustZone)";
    #else
        return "Cortex-M33 (Non-TrustZone)";
    #endif
}

#endif /* PORT_ARCH_CM33 || PORT_ARCH_CM33_NTZ */
