/**
 * @file Eth_Types.h
 * @brief Eth (Ethernet Driver) Types Definition
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Eth Module - Ethernet Hardware Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x11
 */

#ifndef ETH_TYPES_H
#define ETH_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*
 * Version Information
 *============================================================================*/
#define ETH_MAJOR_VERSION           1u
#define ETH_MINOR_VERSION           0u
#define ETH_PATCH_VERSION           0u

#define ETH_MODULE_NAME             "Eth"
#define ETH_VENDOR_ID               0x00u
#define ETH_MODULE_ID               0x11u    /* MCAL Module ID */
#define ETH_AR_MAJOR_VERSION        4u
#define ETH_AR_MINOR_VERSION        4u
#define ETH_AR_PATCH_VERSION        0u

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define ETH_MAX_CONTROLLERS         8u      /* Maximum Ethernet controllers */
#define ETH_MAX_RX_DESCRIPTORS      256u    /* Maximum RX descriptors */
#define ETH_MAX_TX_DESCRIPTORS      256u    /* Maximum TX descriptors */
#define ETH_MAX_MAC_ADDR_FILTERS    64u     /* Maximum MAC address filters */
#define ETH_MAX_VLAN_FILTERS        16u     /* Maximum VLAN filters */
#define ETH_MAX_PAYLOAD_SIZE        1522u   /* Maximum Ethernet payload (VLAN tagged) */
#define ETH_MIN_PAYLOAD_SIZE        46u     /* Minimum Ethernet payload */
#define ETH_FRAME_HEADER_SIZE       14u     /* Ethernet header size (without FCS) */
#define ETH_FCS_SIZE                4u      /* Frame Check Sequence size */
#define ETH_MAC_ADDR_SIZE           6u      /* MAC address size */

/* DMA buffer alignment */
#define ETH_DMA_ALIGNMENT           32u     /* DMA buffer alignment requirement */

/*============================================================================*
 * Error Codes (AUTOSAR Standard)
 *============================================================================*/
typedef enum {
    ETH_OK                      = 0x00u,    /* Operation successful */
    ETH_E_NOT_OK                = 0x01u,    /* General error */
    ETH_E_BUSY                  = 0x02u,    /* Controller busy */
    ETH_E_TIMEOUT               = 0x03u,    /* Operation timeout */
    ETH_E_NO_CTRL               = 0x04u,    /* Controller not available */
    ETH_E_INV_CTRL              = 0x05u,    /* Invalid controller index */
    ETH_E_INV_PARAM             = 0x06u,    /* Invalid parameter */
    ETH_E_INV_POINTER           = 0x07u,    /* Invalid pointer */
    ETH_E_INV_CONFIG            = 0x08u,    /* Invalid configuration */
    ETH_E_INV_MODE              = 0x09u,    /* Invalid controller mode */
    ETH_E_INV_MAC               = 0x0Au,    /* Invalid MAC address */
    ETH_E_INV_LEN               = 0x0Bu,    /* Invalid length */
    ETH_E_INV_FRAME             = 0x0Cu,    /* Invalid frame */
    ETH_E_TX_FAILED             = 0x0Du,    /* Transmission failed */
    ETH_E_RX_FAILED             = 0x0Eu,    /* Reception failed */
    ETH_E_DMA_ERROR             = 0x0Fu,    /* DMA error */
    ETH_E_PHY_ERROR             = 0x10u,    /* PHY error */
    ETH_E_BUFFER_OVERFLOW       = 0x11u,    /* Buffer overflow */
    ETH_E_BUFFER_UNDERFLOW      = 0x12u,    /* Buffer underflow */
    ETH_E_CRC_ERROR             = 0x13u,    /* CRC error */
    ETH_E_UNINIT                = 0x14u,    /* Module not initialized */
    ETH_E_ALREADY_INITIALIZED   = 0x15u,    /* Already initialized */
    ETH_E_HW_ERROR              = 0x16u     /* Hardware error */
} Eth_ErrorCode_t;

/*============================================================================*
 * Controller Mode
 *============================================================================*/
typedef enum {
    ETH_MODE_UNINIT             = 0x00u,    /* Controller uninitialized */
    ETH_MODE_DOWN               = 0x01u,    /* Controller down (inactive) */
    ETH_MODE_ACTIVE             = 0x02u,    /* Controller active */
    ETH_MODE_HALTED             = 0x03u     /* Controller halted (for debugging) */
} Eth_ModeType;

/*============================================================================*
 * Duplex Mode
 *============================================================================*/
typedef enum {
    ETH_DUPLEX_HALF             = 0x00u,    /* Half duplex */
    ETH_DUPLEX_FULL             = 0x01u     /* Full duplex */
} Eth_DuplexModeType;

/*============================================================================*
 * Link Speed
 *============================================================================*/
typedef enum {
    ETH_SPEED_10M               = 0x00u,    /* 10 Mbps */
    ETH_SPEED_100M              = 0x01u,    /* 100 Mbps */
    ETH_SPEED_1000M             = 0x02u,    /* 1000 Mbps (1G) */
    ETH_SPEED_2500M             = 0x03u,    /* 2.5 Gbps */
    ETH_SPEED_5000M             = 0x04u,    /* 5 Gbps */
    ETH_SPEED_10000M            = 0x05u     /* 10 Gbps */
} Eth_SpeedType;

/*============================================================================*
 * PHY Interface Type
 *============================================================================*/
typedef enum {
    ETH_PHY_IF_MII              = 0x00u,    /* Media Independent Interface */
    ETH_PHY_IF_RMII             = 0x01u,    /* Reduced MII */
    ETH_PHY_IF_GMII             = 0x02u,    /* Gigabit MII */
    ETH_PHY_IF_RGMII            = 0x03u,    /* Reduced GMII */
    ETH_PHY_IF_SGMII            = 0x04u,    /* Serial GMII */
    ETH_PHY_IF_XGMII            = 0x05u,    /* 10 Gigabit MII */
    ETH_PHY_IF_USGMII           = 0x06u     /* Universal SGMII */
} Eth_PhyInterfaceType;

/*============================================================================*
 * PHY Chip Type
 *============================================================================*/
typedef enum {
    ETH_PHY_GENERIC             = 0x00u,    /* Generic PHY */
    ETH_PHY_TJA1101             = 0x01u,    /* NXP TJA1101 (100BASE-T1) */
    ETH_PHY_TJA1102             = 0x02u,    /* NXP TJA1102 (100BASE-T1 dual) */
    ETH_PHY_DP83825I            = 0x03u,    /* TI DP83825I */
    ETH_PHY_DP83848             = 0x04u,    /* TI DP83848 */
    ETH_PHY_KSZ8081             = 0x05u,    /* Microchip KSZ8081 */
    ETH_PHY_KSZ9031             = 0x06u,    /* Microchip KSZ9031 (RGMII) */
    ETH_PHY_RTL8211             = 0x07u,    /* Realtek RTL8211 */
    ETH_PHY_LAN8742             = 0x08u,    /* Microchip LAN8742 */
    ETH_PHY_VSC8211             = 0x09u,    /* Microsemi VSC8211 */
    ETH_PHY_BCM84891            = 0x0Au     /* Broadcom BCM84891 (10G) */
} Eth_PhyType;

/*============================================================================*
 * Hardware Type
 *============================================================================*/
typedef enum {
    ETH_HW_GENERIC              = 0x00u,    /* Generic/Emulator */
    ETH_HW_AURIX_TC3XX          = 0x01u,    /* Infineon Aurix TC3xx (GETH) */
    ETH_HW_S32G3                = 0x02u,    /* NXP S32G3 (ENET_QOS) */
    ETH_HW_S32K3                = 0x03u,    /* NXP S32K3 (ENET) */
    ETH_HW_STM32H7              = 0x04u,    /* STM32 H7 (ETH) */
    ETH_HW_STM32MP1             = 0x05u,    /* STM32MP1 (ETH) */
    ETH_HW_RH850                = 0x06u,    /* Renesas RH850 (ETHERC) */
    ETH_HW_POSIX                = 0xFFu     /* POSIX Simulation */
} Eth_HardwareType;

/*============================================================================*
 * Interrupt Event Type
 *============================================================================*/
typedef enum {
    ETH_IRQ_NONE                = 0x0000u,
    ETH_IRQ_TX_COMPLETE         = 0x0001u,  /* TX complete */
    ETH_IRQ_RX_COMPLETE         = 0x0002u,  /* RX complete */
    ETH_IRQ_TX_ERROR            = 0x0004u,  /* TX error */
    ETH_IRQ_RX_ERROR            = 0x0008u,  /* RX error */
    ETH_IRQ_DMA_ERROR           = 0x0010u,  /* DMA error */
    ETH_IRQ_PHY_EVENT           = 0x0020u,  /* PHY event (link change) */
    ETH_IRQ_TIMESTAMP           = 0x0040u,  /* Timestamp capture */
    ETH_IRQ_BUS_ERROR           = 0x0080u,  /* Bus error */
    ETH_IRQ_WAKEUP              = 0x0100u   /* Wake-up event */
} Eth_IrqEventType;

/*============================================================================*
 * Frame Type
 *============================================================================*/
typedef enum {
    ETH_FRAMETYPE_DATA          = 0x0000u,  /* Standard data frame */
    ETH_FRAMETYPE_VLAN          = 0x8100u,  /* VLAN tagged frame */
    ETH_FRAMETYPE_AVB           = 0x88F7u,  /* AVB/TSN frame */
    ETH_FRAMETYPE_PTP           = 0x88F7u   /* PTP frame (same as AVB) */
} Eth_FrameType;

/*============================================================================*
 * MAC Address Type
 *============================================================================*/
typedef struct {
    uint8_t addr[ETH_MAC_ADDR_SIZE];
} Eth_MacAddrType;

/*============================================================================*
 * Frame Information
 *============================================================================*/
typedef struct {
    uint8_t* data;                          /* Frame data buffer */
    uint16_t len;                           /* Frame length (including header) */
    Eth_MacAddrType destAddr;               /* Destination MAC address */
    Eth_MacAddrType srcAddr;                /* Source MAC address */
    uint16_t type;                          /* EtherType */
    uint16_t vlanId;                        /* VLAN ID (0 if not VLAN tagged) */
    uint32_t timestamp;                     /* RX timestamp (if supported) */
    uint8_t priority;                       /* Frame priority */
    bool validFCS;                          /* FCS valid flag */
} Eth_FrameInfoType;

/*============================================================================*
 * DMA Descriptor Type (Generic)
 *============================================================================*/
typedef struct {
    volatile uint32_t status;               /* Status word */
    volatile uint32_t length;               /* Buffer length */
    volatile uint32_t buffer1;              /* Buffer 1 address */
    volatile uint32_t buffer2;              /* Buffer 2 address (next desc) */
    /* Software book-keeping */
    uint8_t* bufferAddr;                    /* Actual buffer address */
    uint16_t frameLen;                      /* Actual frame length */
    bool used;                              /* Descriptor used flag */
} Eth_DmaDescriptorType;

/*============================================================================*
 * Buffer Configuration
 *============================================================================*/
typedef struct {
    uint8_t* buffer;                        /* Buffer pointer */
    uint32_t size;                          /* Buffer size */
    uint32_t count;                         /* Number of buffers */
} Eth_BufferConfigType;

/*============================================================================*
 * Controller Configuration
 *============================================================================*/
typedef struct {
    uint8_t controllerIdx;                  /* Controller index */
    Eth_HardwareType hwType;                /* Hardware type */
    uint32_t baseAddress;                   /* MAC base address */
    uint32_t dmaBaseAddress;                /* DMA base address (if separate) */
    
    /* MAC address */
    Eth_MacAddrType macAddr;                /* Default MAC address */
    
    /* PHY configuration */
    Eth_PhyType phyType;                    /* PHY chip type */
    Eth_PhyInterfaceType phyInterface;      /* PHY interface type */
    uint8_t phyAddress;                     /* PHY MDIO address (0-31) */
    uint32_t mdcClock;                      /* MDC clock frequency (Hz) */
    uint32_t mdioTimeout;                   /* MDIO operation timeout (us) */
    
    /* Link configuration */
    Eth_SpeedType speed;                    /* Link speed */
    Eth_DuplexModeType duplex;              /* Duplex mode */
    bool autoNegotiation;                   /* Auto-negotiation enable */
    bool loopback;                          /* Loopback mode enable */
    
    /* DMA configuration */
    uint16_t rxDescCount;                   /* Number of RX descriptors */
    uint16_t txDescCount;                   /* Number of TX descriptors */
    uint16_t rxBufferSize;                  /* RX buffer size (bytes) */
    uint16_t txBufferSize;                  /* TX buffer size (bytes) */
    
    /* Interrupt configuration */
    uint32_t interruptMask;                 /* Enabled interrupts (Eth_IrqEventType) */
    uint8_t irqPriority;                    /* Interrupt priority */
    uint8_t irqVector;                      /* Interrupt vector number */
    
    /* Flow control */
    bool flowControlEnabled;                /* Flow control enable */
    uint16_t pauseTime;                     /* Pause time quanta */
    
    /* Hardware-specific configuration */
    void* hwConfig;                         /* Hardware-specific config pointer */
} Eth_ControllerConfigType;

/*============================================================================*
 * General Configuration
 *============================================================================*/
typedef struct {
    uint8_t maxControllers;                 /* Maximum number of controllers */
    uint32_t mainFunctionPeriod;            /* Main function call period (ms) */
    bool devErrorDetect;                    /* Development error detection */
    bool versionInfoApi;                    /* Version info API enable */
    bool globalTimeSupport;                 /* Global time (PTP) support */
    bool timestampSupport;                  /* Timestamp support */
    bool dmaSwBufferEnabled;                /* DMA software buffer management */
} Eth_GeneralConfigType;

/*============================================================================*
 * Runtime Configuration
 *============================================================================*/
typedef struct {
    const Eth_GeneralConfigType* general;   /* General configuration */
    const Eth_ControllerConfigType* controllers; /* Controller config array */
    uint8_t controllerCount;                /* Number of controllers */
} Eth_ConfigType;

/*============================================================================*
 * Controller State
 *============================================================================*/
typedef struct {
    uint8_t ctrlIdx;                        /* Controller index */
    Eth_ModeType mode;                      /* Current mode */
    bool linkUp;                            /* Link status */
    Eth_SpeedType currentSpeed;             /* Current link speed */
    Eth_DuplexModeType currentDuplex;       /* Current duplex mode */
    
    /* DMA state */
    Eth_DmaDescriptorType* rxDesc;          /* RX descriptor ring */
    Eth_DmaDescriptorType* txDesc;          /* TX descriptor ring */
    uint16_t rxDescHead;                    /* RX descriptor head */
    uint16_t rxDescTail;                    /* RX descriptor tail */
    uint16_t txDescHead;                    /* TX descriptor head */
    uint16_t txDescTail;                    /* TX descriptor tail */
    
    /* Statistics */
    uint32_t txFrames;                      /* Transmitted frames */
    uint32_t rxFrames;                      /* Received frames */
    uint32_t txErrors;                      /* TX errors */
    uint32_t rxErrors;                      /* RX errors */
    uint32_t crcErrors;                     /* CRC errors */
    uint32_t bufferOverflows;               /* Buffer overflow count */
    
    /* PHY state */
    uint16_t phyStatus;                     /* PHY status register cache */
    uint32_t lastLinkCheck;                 /* Last link check timestamp */
    
    /* Interrupt state */
    uint32_t pendingIrqs;                   /* Pending interrupts */
} Eth_ControllerStateType;

/*============================================================================*
 * Module State
 *============================================================================*/
typedef struct {
    Eth_ModeType moduleState;               /* Module state */
    const Eth_ConfigType* config;           /* Current configuration */
    Eth_ControllerStateType* ctrlState;     /* Controller states */
    uint8_t initializedCtrlCount;           /* Number of initialized controllers */
    bool initialized;                       /* Module initialized flag */
} Eth_ModuleStateType;

/*============================================================================*
 * Callback Types
 *============================================================================*/
typedef void (*Eth_TxConfirmationCallback_t)(
    uint8_t ctrlIdx,
    uint8_t bufIdx,
    Eth_ErrorCode_t result
);

typedef void (*Eth_RxIndicationCallback_t)(
    uint8_t ctrlIdx,
    const Eth_FrameInfoType* frameInfo
);

typedef void (*Eth_LinkStateChangeCallback_t)(
    uint8_t ctrlIdx,
    bool linkUp
);

typedef void (*Eth_ErrorCallback_t)(
    uint8_t ctrlIdx,
    Eth_IrqEventType errorType,
    uint32_t errorInfo
);

/*============================================================================*
 * Hardware Interface (Abstract Layer)
 *============================================================================*/
typedef struct {
    /* Initialize controller hardware */
    Eth_ErrorCode_t (*Init)(
        uint8_t ctrlIdx,
        const Eth_ControllerConfigType* config
    );
    
    /* Deinitialize controller hardware */
    Eth_ErrorCode_t (*Deinit)(uint8_t ctrlIdx);
    
    /* Set controller mode */
    Eth_ErrorCode_t (*SetMode)(uint8_t ctrlIdx, Eth_ModeType mode);
    
    /* Get controller mode */
    Eth_ModeType (*GetMode)(uint8_t ctrlIdx);
    
    /* Set MAC address */
    Eth_ErrorCode_t (*SetMacAddr)(
        uint8_t ctrlIdx,
        const Eth_MacAddrType* macAddr
    );
    
    /* Get MAC address */
    Eth_ErrorCode_t (*GetMacAddr)(
        uint8_t ctrlIdx,
        Eth_MacAddrType* macAddr
    );
    
    /* Transmit frame */
    Eth_ErrorCode_t (*Transmit)(
        uint8_t ctrlIdx,
        const uint8_t* data,
        uint16_t len,
        uint8_t* bufIdx
    );
    
    /* Receive frame */
    Eth_ErrorCode_t (*Receive)(
        uint8_t ctrlIdx,
        uint8_t* data,
        uint16_t* len,
        Eth_FrameInfoType* frameInfo
    );
    
    /* Get TX buffer */
    Eth_ErrorCode_t (*GetTxBuffer)(
        uint8_t ctrlIdx,
        uint16_t len,
        uint8_t** buf,
        uint8_t* bufIdx
    );
    
    /* Transmit TX buffer */
    Eth_ErrorCode_t (*TransmitTxBuffer)(
        uint8_t ctrlIdx,
        uint8_t bufIdx,
        uint16_t len
    );
    
    /* Check if TX is ready */
    bool (*IsTxReady)(uint8_t ctrlIdx);
    
    /* Get RX pending count */
    uint16_t (*GetRxPendingCount)(uint8_t ctrlIdx);
    
    /* Read PHY register (MDIO) */
    Eth_ErrorCode_t (*ReadPhy)(
        uint8_t ctrlIdx,
        uint8_t regAddr,
        uint16_t* regVal
    );
    
    /* Write PHY register (MDIO) */
    Eth_ErrorCode_t (*WritePhy)(
        uint8_t ctrlIdx,
        uint8_t regAddr,
        uint16_t regVal
    );
    
    /* Get link state */
    bool (*GetLinkState)(uint8_t ctrlIdx);
    
    /* Get baud rate */
    Eth_SpeedType (*GetBaudRate)(uint8_t ctrlIdx);
    
    /* Get duplex mode */
    Eth_DuplexModeType (*GetDuplexMode)(uint8_t ctrlIdx);
    
    /* Enable/Disable interrupts */
    void (*EnableIrq)(uint8_t ctrlIdx, uint32_t irqMask);
    void (*DisableIrq)(uint8_t ctrlIdx, uint32_t irqMask);
    
    /* Process interrupt */
    void (*ProcessIrq)(uint8_t ctrlIdx, Eth_IrqEventType* event);
    
    /* Clear interrupt flag */
    void (*ClearIrq)(uint8_t ctrlIdx, Eth_IrqEventType event);
    
    /* Main function (cyclic processing) */
    void (*MainFunction)(uint8_t ctrlIdx);
    
    /* Update PHY configuration */
    Eth_ErrorCode_t (*UpdatePhyConfig)(uint8_t ctrlIdx);
} Eth_HwInterfaceType;

/*============================================================================*
 * PHY Register Addresses (IEEE 802.3 Standard)
 *============================================================================*/
#define ETH_PHY_REG_BMCR            0x00u    /* Basic Mode Control Register */
#define ETH_PHY_REG_BMSR            0x01u    /* Basic Mode Status Register */
#define ETH_PHY_REG_PHYID1          0x02u    /* PHY Identifier 1 */
#define ETH_PHY_REG_PHYID2          0x03u    /* PHY Identifier 2 */
#define ETH_PHY_REG_ANAR            0x04u    /* Auto-Negotiation Advertisement */
#define ETH_PHY_REG_ANLPAR          0x05u    /* Auto-Negotiation Link Partner Ability */
#define ETH_PHY_REG_ANER            0x06u    /* Auto-Negotiation Expansion */
#define ETH_PHY_REG_ANNPTR          0x07u    /* Auto-Negotiation Next Page TX */

/* BMCR bits */
#define ETH_PHY_BMCR_RESET          0x8000u
#define ETH_PHY_BMCR_LOOPBACK       0x4000u
#define ETH_PHY_BMCR_SPEED_100M     0x2000u
#define ETH_PHY_BMCR_AN_ENABLE      0x1000u
#define ETH_PHY_BMCR_POWER_DOWN     0x0800u
#define ETH_PHY_BMCR_ISOLATE        0x0400u
#define ETH_PHY_BMCR_RESTART_AN     0x0200u
#define ETH_PHY_BMCR_DUPLEX_FULL    0x0100u
#define ETH_PHY_BMCR_SPEED_1000M    0x0040u

/* BMSR bits */
#define ETH_PHY_BMSR_100BASE_T4     0x8000u
#define ETH_PHY_BMSR_100BASE_TX_FD  0x4000u
#define ETH_PHY_BMSR_100BASE_TX_HD  0x2000u
#define ETH_PHY_BMSR_10BASE_T_FD    0x1000u
#define ETH_PHY_BMSR_10BASE_T_HD    0x0800u
#define ETH_PHY_BMSR_MF_PREAMB      0x0040u
#define ETH_PHY_BMSR_AN_COMPLETE    0x0020u
#define ETH_PHY_BMSR_REMOTE_FAULT   0x0010u
#define ETH_PHY_BMSR_AN_CAPABLE     0x0008u
#define ETH_PHY_BMSR_LINK_STATUS    0x0004u
#define ETH_PHY_BMSR_JABBER_DETECT  0x0002u
#define ETH_PHY_BMSR_EXT_CAPABLE    0x0001u

#ifdef __cplusplus
}
#endif

#endif /* ETH_TYPES_H */
