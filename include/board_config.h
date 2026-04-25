/*
 * board_config.h
 * Board Configuration Header
 * Defines hardware-specific settings for different target platforms
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>

/*==============================================================================
 * Platform Selection
 * Define one of the following before including this header:
 *   - BOARD_S32G3 (NXP S32G3)
 *   - BOARD_AURIX_TC3XX (Infineon AURIX TC3xx)
 *   - BOARD_POSIX (Linux/Unix simulation)
 *============================================================================*/

#if !defined(BOARD_S32G3) && !defined(BOARD_AURIX_TC3XX) && !defined(BOARD_POSIX)
    #error "No board defined! Define BOARD_S32G3, BOARD_AURIX_TC3XX, or BOARD_POSIX"
#endif

/*==============================================================================
 * Common Board Information
 *============================================================================*/

typedef struct {
    const char *name;
    uint32_t cpu_freq_hz;
    uint32_t bus_freq_hz;
    uint32_t ptp_clock_hz;
    uint32_t sram_size;
    uint32_t ddr_size;
} board_info_t;

/*==============================================================================
 * NXP S32G3 Configuration
 *============================================================================*/
#ifdef BOARD_S32G3

    #define BOARD_NAME              "NXP S32G3"
    #define CPU_CORES               4
    #define CPU_FREQ_HZ             1000000000UL    /* 1 GHz */
    #define BUS_FREQ_HZ             500000000UL     /* 500 MHz */
    
    /* ENET Configuration */
    #define ENET_COUNT              3
    #define ENET0_BASE              0x40120000UL
    #define ENET1_BASE              0x40122000UL
    #define ENET2_BASE              0x40124000UL
    #define ENET_DEFAULT_PHY_ADDR   0
    
    /* PTP Configuration */
    #define PTP_CLOCK_FREQ_HZ       50000000UL      /* 50 MHz PTP clock */
    #define PTP_NS_PER_TICK         20
    
    /* Memory */
    #define SRAM_BASE               0x34000000UL
    #define SRAM_SIZE               0x00800000UL    /* 8 MB */
    #define DDR_BASE                0x80000000UL
    #define DDR_SIZE                0x80000000UL    /* 2 GB */
    
    /* DMA Configuration */
    #define DMA_BUFFER_SIZE         1536
    #define DMA_RX_DESCRIPTORS      16
    #define DMA_TX_DESCRIPTORS      16
    
    /* Interrupt Priorities */
    #define IRQ_PRIO_ENET0          10
    #define IRQ_PRIO_ENET1          10
    #define IRQ_PRIO_ENET2          10
    #define IRQ_PRIO_PTP            11

#endif /* BOARD_S32G3 */

/*==============================================================================
 * Infineon AURIX TC3xx Configuration
 *============================================================================*/
#ifdef BOARD_AURIX_TC3XX

    #define BOARD_NAME              "Infineon AURIX TC3xx"
    #define CPU_CORES               6
    #define CPU_FREQ_HZ             300000000UL     /* 300 MHz */
    #define BUS_FREQ_HZ             100000000UL     /* 100 MHz SPB */
    
    /* GETH Configuration */
    #define GETH_COUNT              2
    #define GETH0_BASE              0xF001D000UL
    #define GETH1_BASE              0xF0021000UL
    #define GETH_DEFAULT_PHY_ADDR   0
    
    /* PTP Configuration */
    #define PTP_CLOCK_FREQ_HZ       100000000UL     /* 100 MHz */
    #define PTP_NS_PER_TICK         10
    
    /* Memory */
    #define DSPR_BASE               0x70000000UL
    #define DSRAM_SIZE              0x00060000UL    /* 384 KB per core */
    #define LMU_SRAM_BASE           0xB0000000UL
    #define LMU_SRAM_SIZE           0x00080000UL    /* 512 KB */
    
    /* DMA Configuration */
    #define DMA_BUFFER_SIZE         1536
    #define DMA_RX_DESCRIPTORS      16
    #define DMA_TX_DESCRIPTORS      16
    
    /* Interrupt Priorities (AURIX SRN based) */
    #define IRQ_PRIO_GETH0          100
    #define IRQ_PRIO_GETH1          101
    #define IRQ_PRIO_PTP            102

#endif /* BOARD_AURIX_TC3XX */

/*==============================================================================
 * POSIX/Linux Simulation Configuration
 *============================================================================*/
#ifdef BOARD_POSIX

    #define BOARD_NAME              "POSIX/Linux"
    #define CPU_CORES               1
    #define CPU_FREQ_HZ             0               /* Not applicable */
    #define BUS_FREQ_HZ             0
    
    /* Virtual Ethernet */
    #define VIRT_ENET_COUNT         1
    #define VIRT_ENET_DEFAULT_IF    "eth0"
    
    /* PTP Configuration (software timestamping) */
    #define PTP_CLOCK_FREQ_HZ       1000000000UL    /* 1 GHz (nanosecond resolution) */
    
    /* Memory */
    #define SIMULATED_SRAM_SIZE     0x00800000UL    /* 8 MB simulated */
    
    /* DMA (simulated) */
    #define DMA_BUFFER_SIZE         1536
    #define DMA_RX_DESCRIPTORS      16
    #define DMA_TX_DESCRIPTORS      16

#endif /* BOARD_POSIX */

/*==============================================================================
 * Ethernet MAC Configuration
 *============================================================================*/

/* MAC Address Configuration */
#ifndef MAC_ADDR_0
    #define MAC_ADDR_0              0x00
    #define MAC_ADDR_1              0x04
    #define MAC_ADDR_2              0x9F
    #define MAC_ADDR_3              0x00
    #define MAC_ADDR_4              0x00
    #define MAC_ADDR_5              0x01
#endif

/* Ethernet Features */
#define ETH_ENABLE_JUMBO_FRAMES     0
#define ETH_ENABLE_VLAN_TAGGING     1
#define ETH_ENABLE_FLOW_CONTROL     1
#define ETH_ENABLE_CHECKSUM_OFFLOAD 1

/* TSN Configuration */
#define TSN_ENABLE_GPTP             1
#define TSN_ENABLE_TAS              1
#define TSN_ENABLE_CBS              1
#define TSN_ENABLE_FRAME_PREEMPTION 0

/*==============================================================================
 * PHY Configuration
 *============================================================================*/

/* PHY Type Selection */
typedef enum {
    PHY_TYPE_NONE = 0,
    PHY_TYPE_TJA1101,       /* Single 100BASE-T1 */
    PHY_TYPE_TJA1102,       /* Dual 100BASE-T1 */
    PHY_TYPE_KSZ8081,       /* 10/100 Ethernet PHY */
    PHY_TYPE_KSZ9031,       /* Gigabit Ethernet PHY */
    PHY_TYPE_RTL8211,       /* Gigabit Ethernet PHY */
    PHY_TYPE_GENERIC        /* Generic MII PHY */
} phy_type_t;

/* Default PHY Configuration */
#ifndef PHY_TYPE
    #if defined(BOARD_AURIX_TC3XX)
        #define PHY_TYPE        PHY_TYPE_TJA1101
    #elif defined(BOARD_S32G3)
        #define PHY_TYPE        PHY_TYPE_KSZ9031
    #else
        #define PHY_TYPE        PHY_TYPE_GENERIC
    #endif
#endif

#ifndef PHY_ADDRESS
    #define PHY_ADDRESS         0
#endif

#ifndef PHY_SPEED
    #define PHY_SPEED           100     /* Mbps */
#endif

#ifndef PHY_DUPLEX
    #define PHY_DUPLEX          1       /* 1=Full, 0=Half */
#endif

/*==============================================================================
 * Software Configuration
 *============================================================================*/

/* Buffer Pool Sizes */
#define ETH_RX_BUFFER_POOL_SIZE     (DMA_RX_DESCRIPTORS * 2)
#define ETH_TX_BUFFER_POOL_SIZE     (DMA_TX_DESCRIPTORS * 2)

/* Maximum Frame Size */
#define ETH_MAX_FRAME_SIZE          1522
#define ETH_MAX_JUMBO_FRAME_SIZE    9018

/* Timeouts */
#define ETH_LINK_TIMEOUT_MS         5000
#define ETH_AUTONEG_TIMEOUT_MS      5000
#define ETH_RESET_TIMEOUT_MS        1000

/* Debug Options */
#define ETH_DEBUG_LEVEL             2   /* 0=None, 1=Error, 2=Warning, 3=Info */

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* Board initialization */
int board_init(void);
int board_deinit(void);
const board_info_t *board_get_info(void);

/* Clock functions */
uint32_t board_get_cpu_freq(void);
uint32_t board_get_bus_freq(void);
void board_udelay(uint32_t us);

/* Interrupt functions */
int board_irq_enable(uint32_t irq);
int board_irq_disable(uint32_t irq);
int board_irq_set_priority(uint32_t irq, uint32_t priority);

/* Memory functions */
void *board_alloc_dma_buffer(size_t size);
void board_free_dma_buffer(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
