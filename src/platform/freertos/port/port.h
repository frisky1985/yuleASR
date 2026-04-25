/******************************************************************************
 * @file    port.h
 * @brief   FreeRTOS Platform Abstraction Layer - Architecture Support
 *
 * This header defines the unified interface for architecture-specific
 * configurations for Cortex-M4F, Cortex-M7, Cortex-M33, and Cortex-M33-NTZ.
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/

#ifndef FREERTOS_PORT_H
#define FREERTOS_PORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Architecture Type Definitions
 ******************************************************************************/

typedef enum {
    PORT_ARCH_CM4F = 0,      /* Cortex-M4F with FPU */
    PORT_ARCH_CM7,           /* Cortex-M7 with double-precision FPU, Cache */
    PORT_ARCH_CM33,          /* Cortex-M33 with TrustZone, single-precision FPU */
    PORT_ARCH_CM33_NTZ,      /* Cortex-M33 Non-TrustZone (single-precision FPU) */
    PORT_ARCH_UNKNOWN
} Port_ArchType;

/******************************************************************************
 * Architecture Detection (Set by CMake/build system)
 ******************************************************************************/

/* Architecture is defined by build system: -DPORT_ARCH_CM7, -DPORT_ARCH_CM33, etc. */
#if defined(PORT_ARCH_CM7)
    #define PORT_ARCH_TYPE              PORT_ARCH_CM7
    #define PORT_HAS_FPU_DOUBLE         1
    #define PORT_HAS_CACHE              1
    #define PORT_HAS_MPU                1
    #define PORT_HAS_TRUSTZONE          0
#elif defined(PORT_ARCH_CM33)
    #define PORT_ARCH_TYPE              PORT_ARCH_CM33
    #define PORT_HAS_FPU_SINGLE         1
    #define PORT_HAS_CACHE              0
    #define PORT_HAS_MPU                1
    #define PORT_HAS_TRUSTZONE          1
#elif defined(PORT_ARCH_CM33_NTZ)
    #define PORT_ARCH_TYPE              PORT_ARCH_CM33_NTZ
    #define PORT_HAS_FPU_SINGLE         1
    #define PORT_HAS_CACHE              0
    #define PORT_HAS_MPU                1
    #define PORT_HAS_TRUSTZONE          0
#elif defined(PORT_ARCH_CM4F)
    #define PORT_ARCH_TYPE              PORT_ARCH_CM4F
    #define PORT_HAS_FPU_SINGLE         1
    #define PORT_HAS_CACHE              0
    #define PORT_HAS_MPU                1
    #define PORT_HAS_TRUSTZONE          0
#else
    #define PORT_ARCH_TYPE              PORT_ARCH_UNKNOWN
    #define PORT_HAS_FPU_SINGLE         0
    #define PORT_HAS_FPU_DOUBLE         0
    #define PORT_HAS_CACHE              0
    #define PORT_HAS_MPU                0
    #define PORT_HAS_TRUSTZONE          0
    #warning "Unknown or undefined architecture type. Define PORT_ARCH_CM7, PORT_ARCH_CM33, etc."
#endif

/******************************************************************************
 * Feature Detection Helpers
 ******************************************************************************/

#ifndef PORT_HAS_FPU_SINGLE
    #define PORT_HAS_FPU_SINGLE         0
#endif

#ifndef PORT_HAS_FPU_DOUBLE
    #define PORT_HAS_FPU_DOUBLE         0
#endif

#define PORT_HAS_FPU                    (PORT_HAS_FPU_SINGLE || PORT_HAS_FPU_DOUBLE)

/******************************************************************************
 * Cache Operations (CM7 specific)
 ******************************************************************************/

#if PORT_HAS_CACHE

/**
 * @brief Enable data cache
 */
void Port_Cache_EnableDCache(void);

/**
 * @brief Disable data cache
 */
void Port_Cache_DisableDCache(void);

/**
 * @brief Clean data cache by address
 * @param addr Start address
 * @param size Size in bytes
 */
void Port_Cache_CleanDCache_by_Addr(uint32_t *addr, int32_t size);

/**
 * @brief Invalidate data cache by address
 * @param addr Start address
 * @param size Size in bytes
 */
void Port_Cache_InvalidateDCache_by_Addr(uint32_t *addr, int32_t size);

/**
 * @brief Clean and invalidate data cache by address
 * @param addr Start address
 * @param size Size in bytes
 */
void Port_Cache_CleanInvalidateDCache_by_Addr(uint32_t *addr, int32_t size);

/**
 * @brief Clean entire data cache
 */
void Port_Cache_CleanDCache(void);

/**
 * @brief Invalidate entire data cache
 */
void Port_Cache_InvalidateDCache(void);

/**
 * @brief Clean and invalidate entire data cache
 */
void Port_Cache_CleanInvalidateDCache(void);

/**
 * @brief Enable instruction cache
 */
void Port_Cache_EnableICache(void);

/**
 * @brief Disable instruction cache
 */
void Port_Cache_DisableICache(void);

/**
 * @brief Invalidate instruction cache
 */
void Port_Cache_InvalidateICache(void);

#else /* !PORT_HAS_CACHE */

/* Stub implementations for architectures without cache */
static inline void Port_Cache_EnableDCache(void) { }
static inline void Port_Cache_DisableDCache(void) { }
static inline void Port_Cache_CleanDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
static inline void Port_Cache_InvalidateDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
static inline void Port_Cache_CleanInvalidateDCache_by_Addr(uint32_t *addr, int32_t size) { (void)addr; (void)size; }
static inline void Port_Cache_CleanDCache(void) { }
static inline void Port_Cache_InvalidateDCache(void) { }
static inline void Port_Cache_CleanInvalidateDCache(void) { }
static inline void Port_Cache_EnableICache(void) { }
static inline void Port_Cache_DisableICache(void) { }
static inline void Port_Cache_InvalidateICache(void) { }

#endif /* PORT_HAS_CACHE */

/******************************************************************************
 * FPU Context Management
 ******************************************************************************/

#if PORT_HAS_FPU

/**
 * @brief Initialize FPU context management
 */
void Port_FPU_Init(void);

/**
 * @brief Enable FPU
 */
void Port_FPU_Enable(void);

/**
 * @brief Disable FPU
 */
void Port_FPU_Disable(void);

/**
 * @brief Check if FPU is enabled
 * @return true if enabled, false otherwise
 */
bool Port_FPU_IsEnabled(void);

/**
 * @brief Get FPU type
 * @return 0=Single, 1=Double precision, -1=None
 */
int32_t Port_FPU_GetType(void);

/**
 * @brief Lazy FPU context save/restore control
 * @param enable true to enable lazy stacking, false for immediate
 */
void Port_FPU_SetLazyState(bool enable);

#else /* !PORT_HAS_FPU */

static inline void Port_FPU_Init(void) { }
static inline void Port_FPU_Enable(void) { }
static inline void Port_FPU_Disable(void) { }
static inline bool Port_FPU_IsEnabled(void) { return false; }
static inline int32_t Port_FPU_GetType(void) { return -1; }
static inline void Port_FPU_SetLazyState(bool enable) { (void)enable; }

#endif /* PORT_HAS_FPU */

/******************************************************************************
 * MPU Configuration
 ******************************************************************************/

#if PORT_HAS_MPU

/* MPU Region configuration structure */
typedef struct {
    uint32_t baseAddr;          /* Region base address */
    uint32_t size;              /* Region size (must be power of 2) */
    uint8_t  subRegionDisable;  /* Sub-region disable bits */
    uint8_t  attributes;        /* TEX, C, B, S attributes */
    uint8_t  accessPermission;  /* AP field */
    uint8_t  executeNever;      /* XN bit */
    bool     enable;            /* Region enable */
} Port_MPU_RegionCfgType;

/* MPU region numbers */
#define PORT_MPU_REGION_0       0U
#define PORT_MPU_REGION_1       1U
#define PORT_MPU_REGION_2       2U
#define PORT_MPU_REGION_3       3U
#define PORT_MPU_REGION_4       4U
#define PORT_MPU_REGION_5       5U
#define PORT_MPU_REGION_6       6U
#define PORT_MPU_REGION_7       7U
#define PORT_MPU_REGION_8       8U
#define PORT_MPU_REGION_9       9U
#define PORT_MPU_REGION_10      10U
#define PORT_MPU_REGION_11      11U
#define PORT_MPU_REGION_12      12U
#define PORT_MPU_REGION_13      13U
#define PORT_MPU_REGION_14      14U
#define PORT_MPU_REGION_15      15U

/* MPU region sizes (encoding for RASR register) */
#define PORT_MPU_REGION_SIZE_32B        0x04U
#define PORT_MPU_REGION_SIZE_64B        0x05U
#define PORT_MPU_REGION_SIZE_128B       0x06U
#define PORT_MPU_REGION_SIZE_256B       0x07U
#define PORT_MPU_REGION_SIZE_512B       0x08U
#define PORT_MPU_REGION_SIZE_1KB        0x09U
#define PORT_MPU_REGION_SIZE_2KB        0x0AU
#define PORT_MPU_REGION_SIZE_4KB        0x0BU
#define PORT_MPU_REGION_SIZE_8KB        0x0CU
#define PORT_MPU_REGION_SIZE_16KB       0x0DU
#define PORT_MPU_REGION_SIZE_32KB       0x0EU
#define PORT_MPU_REGION_SIZE_64KB       0x0FU
#define PORT_MPU_REGION_SIZE_128KB      0x10U
#define PORT_MPU_REGION_SIZE_256KB      0x11U
#define PORT_MPU_REGION_SIZE_512KB      0x12U
#define PORT_MPU_REGION_SIZE_1MB        0x13U
#define PORT_MPU_REGION_SIZE_2MB        0x14U
#define PORT_MPU_REGION_SIZE_4MB        0x15U
#define PORT_MPU_REGION_SIZE_8MB        0x16U
#define PORT_MPU_REGION_SIZE_16MB       0x17U
#define PORT_MPU_REGION_SIZE_32MB       0x18U
#define PORT_MPU_REGION_SIZE_64MB       0x19U
#define PORT_MPU_REGION_SIZE_128MB      0x1AU
#define PORT_MPU_REGION_SIZE_256MB      0x1BU
#define PORT_MPU_REGION_SIZE_512MB      0x1CU
#define PORT_MPU_REGION_SIZE_1GB        0x1DU
#define PORT_MPU_REGION_SIZE_2GB        0x1EU
#define PORT_MPU_REGION_SIZE_4GB        0x1FU

/* MPU access permissions */
#define PORT_MPU_AP_NO_ACCESS           0x00U  /* No access */
#define PORT_MPU_AP_PRIV_RW             0x01U  /* Privileged RW, User no access */
#define PORT_MPU_AP_PRIV_RW_USER_RO     0x02U  /* Privileged RW, User RO */
#define PORT_MPU_AP_PRIV_RW_USER_RW     0x03U  /* Privileged RW, User RW */
#define PORT_MPU_AP_PRIV_RO             0x05U  /* Privileged RO, User no access */
#define PORT_MPU_AP_PRIV_RO_USER_RO     0x06U  /* Privileged RO, User RO */

/* Memory attributes */
#define PORT_MPU_ATTR_STRONGLY_ORDERED  0x00U  /* Strongly ordered, shareable */
#define PORT_MPU_ATTR_DEVICE            0x01U  /* Device, shareable */
#define PORT_MPU_ATTR_NORMAL_WT         0x02U  /* Normal, Write-through */
#define PORT_MPU_ATTR_NORMAL_WB         0x03U  /* Normal, Write-back */
#define PORT_MPU_ATTR_NORMAL_NONCACHE   0x04U  /* Normal, Non-cacheable */

/**
 * @brief Initialize MPU
 */
void Port_MPU_Init(void);

/**
 * @brief Enable MPU
 * @param privDefault Use default memory map for privileged access
 */
void Port_MPU_Enable(bool privDefault);

/**
 * @brief Disable MPU
 */
void Port_MPU_Disable(void);

/**
 * @brief Configure MPU region
 * @param region Region number (0-15)
 * @param config Region configuration
 * @return true if successful, false otherwise
 */
bool Port_MPU_ConfigRegion(uint32_t region, const Port_MPU_RegionCfgType *config);

/**
 * @brief Enable MPU region
 * @param region Region number
 */
void Port_MPU_EnableRegion(uint32_t region);

/**
 * @brief Disable MPU region
 * @param region Region number
 */
void Port_MPU_DisableRegion(uint32_t region);

/**
 * @brief Get current MPU configuration
 * @param region Region number
 * @param config Output configuration structure
 * @return true if region is valid and active
 */
bool Port_MPU_GetRegionConfig(uint32_t region, Port_MPU_RegionCfgType *config);

#else /* !PORT_HAS_MPU */

static inline void Port_MPU_Init(void) { }
static inline void Port_MPU_Enable(bool privDefault) { (void)privDefault; }
static inline void Port_MPU_Disable(void) { }

#endif /* PORT_HAS_MPU */

/******************************************************************************
 * TrustZone Support (CM33 specific)
 ******************************************************************************/

#if PORT_HAS_TRUSTZONE

/* Security states */
#define PORT_SECURITY_SECURE        0U
#define PORT_SECURITY_NONSECURE     1U

/* SAU region configuration */
typedef struct {
    uint32_t baseAddr;          /* Region base address */
    uint32_t limitAddr;         /* Region limit address (inclusive) */
    uint8_t  nsc;               /* Non-secure callable */
    bool     enable;            /* Region enable */
} Port_SAU_RegionCfgType;

/**
 * @brief Initialize TrustZone support
 */
void Port_TrustZone_Init(void);

/**
 * @brief Get current security state
 * @return PORT_SECURITY_SECURE or PORT_SECURITY_NONSECURE
 */
uint32_t Port_TrustZone_GetSecurityState(void);

/**
 * @brief Switch to Non-secure state
 * @param ns_entry Non-secure entry point
 */
void Port_TrustZone_SwitchToNonSecure(uint32_t ns_entry);

/**
 * @brief Initialize SAU (Security Attribution Unit)
 */
void Port_SAU_Init(void);

/**
 * @brief Configure SAU region
 * @param region Region number (0-7)
 * @param config Region configuration
 * @return true if successful
 */
bool Port_SAU_ConfigRegion(uint32_t region, const Port_SAU_RegionCfgType *config);

/**
 * @brief Enable SAU
 */
void Port_SAU_Enable(void);

/**
 * @brief Disable SAU
 */
void Port_SAU_Disable(void);

/**
 * @brief Configure IDAU (Implementation Defined Attribution Unit)
 */
void Port_IDAU_Init(void);

/**
 * @brief Secure context save (called on secure context switch)
 */
void Port_TrustZone_SaveSecureContext(void);

/**
 * @brief Secure context restore (called on secure context switch)
 */
void Port_TrustZone_RestoreSecureContext(void);

/**
 * @brief Configure Non-secure callable region
 * @param base Base address of SG veneers
 * @param size Size of region
 */
void Port_TrustZone_ConfigNSCRegion(uint32_t base, uint32_t size);

/**
 * @brief Validate if address is in secure memory
 * @param addr Address to check
 * @return true if secure, false if non-secure
 */
bool Port_TrustZone_IsSecureAddress(uint32_t addr);

#else /* !PORT_HAS_TRUSTZONE */

static inline void Port_TrustZone_Init(void) { }
static inline uint32_t Port_TrustZone_GetSecurityState(void) { return PORT_SECURITY_SECURE; }
static inline void Port_TrustZone_SwitchToNonSecure(uint32_t ns_entry) { (void)ns_entry; }
static inline void Port_SAU_Init(void) { }
static inline bool Port_SAU_ConfigRegion(uint32_t region, const void *config) { (void)region; (void)config; return false; }
static inline void Port_SAU_Enable(void) { }
static inline void Port_SAU_Disable(void) { }
static inline void Port_IDAU_Init(void) { }
static inline void Port_TrustZone_SaveSecureContext(void) { }
static inline void Port_TrustZone_RestoreSecureContext(void) { }
static inline void Port_TrustZone_ConfigNSCRegion(uint32_t base, uint32_t size) { (void)base; (void)size; }
static inline bool Port_TrustZone_IsSecureAddress(uint32_t addr) { (void)addr; return true; }

#endif /* PORT_HAS_TRUSTZONE */

/******************************************************************************
 * Architecture Initialization
 ******************************************************************************/

/**
 * @brief Initialize architecture-specific features
 * Called early during system startup
 */
void Port_Arch_Init(void);

/**
 * @brief Get current architecture type
 * @return Architecture type enum value
 */
Port_ArchType Port_Arch_GetType(void);

/**
 * @brief Get architecture name string
 * @return Human-readable architecture name
 */
const char* Port_Arch_GetName(void);

/******************************************************************************
 * System Control
 ******************************************************************************/

/**
 * @brief Enable interrupts globally
 */
void Port_System_EnableIRQ(void);

/**
 * @brief Disable interrupts globally
 */
void Port_System_DisableIRQ(void);

/**
 * @brief Get interrupt state
 * @return true if interrupts are enabled
 */
bool Port_System_IRQEnabled(void);

/**
 * @brief Data Synchronization Barrier
 */
void Port_System_DSB(void);

/**
 * @brief Instruction Synchronization Barrier
 */
void Port_System_ISB(void);

/**
 * @brief Data Memory Barrier
 */
void Port_System_DMB(void);

/**
 * @brief System reset
 */
void Port_System_Reset(void);

/**
 * @brief Get unique device ID
 * @param id_buf Buffer to store ID (at least 12 bytes for STM32)
 * @return Number of bytes written
 */
uint32_t Port_System_GetUniqueID(uint8_t *id_buf);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_PORT_H */
