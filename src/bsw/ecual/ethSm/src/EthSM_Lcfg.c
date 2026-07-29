/**
 * @file EthSM_Lcfg.c
 * @brief Ethernet State Manager (EthSM) Link-Time Configuration
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Ethernet State Manager (ETHSM)
 * Layer: ECU Abstraction Layer (ECUAL)
 * AUTOSAR Version: 4.4.0
 *
 * Description:
 * This file contains the link-time configuration tables for the Ethernet State Manager.
 * It defines the mapping between EthSM networks, Ethernet controllers, transceivers,
 * and TcpIp controllers.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "EthSM.h"
#include "EthSM_Cfg.h"
#include "EthIf.h"
#include "ComM.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ETHSM_LCFG_VENDOR_ID                    (0x01U)
#define ETHSM_LCFG_MODULE_ID                    (0x43U)
#define ETHSM_LCFG_AR_RELEASE_MAJOR_VERSION     (0x04U)
#define ETHSM_LCFG_AR_RELEASE_MINOR_VERSION     (0x04U)
#define ETHSM_LCFG_AR_RELEASE_REVISION_VERSION  (0x00U)
static const EthSM_TcpIpMappingType* EthSM_Lcfg_GetTcpIpMapping(EthSM_NetworkHandleType networkHandle);
static const EthSM_TrcvConfigType* EthSM_Lcfg_GetTrcvConfig(uint8 trcvIdx);
static const EthSM_CtrlConfigType* EthSM_Lcfg_GetCtrlConfig(uint8 ctrlIdx);
static const EthSM_NetworkConfigType* EthSM_Lcfg_GetNetworkConfig(EthSM_NetworkHandleType networkHandle);
#define ETHSM_LCFG_SW_MAJOR_VERSION             (0x01U)
#define ETHSM_LCFG_SW_MINOR_VERSION             (0x00U)
#define ETHSM_LCFG_SW_PATCH_VERSION             (0x00U)

/*==================================================================================================
*                                    NETWORK CONFIGURATION TABLE
==================================================================================================*/

/**
 * @brief Ethernet Network Configuration Type
 * @details Configuration for a single Ethernet network managed by EthSM
 */
typedef struct {
    EthSM_NetworkHandleType networkHandle;          /**< EthSM network handle */
    uint8 ctrlIdx;                                  /**< EthIf controller index */
    uint8 trcvIdx;                                  /**< EthIf transceiver index */
    uint8 tcpIpCtrlIdx;                             /**< TcpIp controller index */
    ComM_ChannelHandleType comMChannel;             /**< ComM channel handle */
    uint16 timeoutWaitTrcvLink;                     /**< Timeout for WAIT_TRCVLINK state (ms) */
    uint16 timeoutWaitOnline;                       /**< Timeout for WAIT_ONLINE state (ms) */
    boolean wakeUpSupport;                          /**< Wake-up support enabled */
    uint8 wakeUpSource;                             /**< EcuM wake-up source identifier */
    boolean wakeUpByBus;                            /**< Wake-up by bus enabled */
} EthSM_NetworkConfigType;

/**
 * @brief Controller Configuration Type
 * @details Configuration for Ethernet controller parameters
 */
typedef struct {
    uint8 ctrlIdx;                                  /**< Controller index */
    uint8 macAddress[6];                            /**< Default MAC address */
    uint16 mtu;                                     /**< Maximum Transmission Unit */
    boolean vlanSupport;                            /**< VLAN support enabled */
    uint16 vlanId;                                  /**< VLAN identifier */
} EthSM_CtrlConfigType;

/**
 * @brief Transceiver Configuration Type
 * @details Configuration for Ethernet transceiver parameters
 */
typedef struct {
    uint8 trcvIdx;                                  /**< Transceiver index */
    uint8 wakeUpMode;                               /**< Wake-up mode configuration */
    boolean autoNegotiation;                        /**< Auto-negotiation enabled */
    uint8 speed;                                    /**< Link speed (10/100/1000 Mbps) */
    uint8 duplexMode;                               /**< Duplex mode (half/full) */
} EthSM_TrcvConfigType;

/**
 * @brief TcpIp Controller Mapping Type
 * @details Maps EthSM networks to TcpIp controllers
 */
typedef struct {
    EthSM_NetworkHandleType networkHandle;          /**< EthSM network handle */
    uint8 tcpIpCtrlIdx;                             /**< TcpIp controller index */
    boolean dhcpEnabled;                            /**< DHCP enabled */
    uint32 staticIpAddress;                         /**< Static IP address */
    uint32 subnetMask;                              /**< Subnet mask */
    uint32 gatewayAddress;                          /**< Default gateway */
} EthSM_TcpIpMappingType;

/*==================================================================================================
*                                    CONFIGURATION CONSTANTS
==================================================================================================*/

/**
 * @brief Default MAC Address for Network 0 Controller
 * @note Format: {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
 */
#define ETHSM_CTRL_0_MAC_ADDR       {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U}

/**
 * @brief Default MAC Address for Network 1 Controller
 * @note Format: {0x00, 0x11, 0x22, 0x33, 0x44, 0x56}
 */
#define ETHSM_CTRL_1_MAC_ADDR       {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x56U}

/**
 * @brief Default MTU for Ethernet controllers
 */
#define ETHSM_DEFAULT_MTU           (1500U)

/**
 * @brief Default VLAN ID
 */
#define ETHSM_DEFAULT_VLAN_ID       (1U)

/**
 * @brief Static IP Address for Network 0 (192.168.1.100)
 */
#define ETHSM_NETWORK_0_IP_ADDR     (0xC0A80164U)   /* 192.168.1.100 */

/**
 * @brief Static Subnet Mask for Network 0 (255.255.255.0)
 */
#define ETHSM_NETWORK_0_SUBNET      (0xFFFFFF00U)   /* 255.255.255.0 */

/**
 * @brief Default Gateway for Network 0 (192.168.1.1)
 */
#define ETHSM_NETWORK_0_GATEWAY     (0xC0A80101U)   /* 192.168.1.1 */

/**
 * @brief Static IP Address for Network 1 (192.168.2.100)
 */
#define ETHSM_NETWORK_1_IP_ADDR     (0xC0A80264U)   /* 192.168.2.100 */

/**
 * @brief Static Subnet Mask for Network 1 (255.255.255.0)
 */
#define ETHSM_NETWORK_1_SUBNET      (0xFFFFFF00U)   /* 255.255.255.0 */

/**
 * @brief Default Gateway for Network 1 (192.168.2.1)
 */
#define ETHSM_NETWORK_1_GATEWAY     (0xC0A80201U)   /* 192.168.2.1 */

/*==================================================================================================
*                                    NETWORK CONFIGURATION TABLE
==================================================================================================*/

/**
 * @brief EthSM Network Configuration Table
 * @details Contains configuration for all Ethernet networks managed by EthSM
 * @note This table is indexed by network handle (0..ETHSM_MAX_NETWORKS-1)
 */
static const EthSM_NetworkConfigType EthSM_NetworkConfig[ETHSM_MAX_NETWORKS] =
{
    /* Network 0 - Primary Ethernet Network */
    {
        /* networkHandle */       ETHSM_NETWORK_0,
        /* ctrlIdx */             ETHSM_CTRL_IDX_NETWORK_0,
        /* trcvIdx */             ETHSM_TRCV_IDX_NETWORK_0,
        /* tcpIpCtrlIdx */        ETHSM_TCPIP_CTRL_IDX_NETWORK_0,
        /* comMChannel */         (ComM_ChannelHandleType)0U,
        /* timeoutWaitTrcvLink */ ETHSM_TIMEOUT_WAIT_TRCVLINK,
        /* timeoutWaitOnline */   ETHSM_TIMEOUT_WAIT_ONLINE,
#if (ETHSM_WAKEUP_SUPPORT == STD_ON)
        /* wakeUpSupport */       TRUE,
        /* wakeUpSource */        ETHSM_WAKEUP_SOURCE_NETWORK_0,
        /* wakeUpByBus */         (ETHSM_WAKEUP_BY_BUS_NETWORK_0 == STD_ON) ? TRUE : FALSE
#else
        /* wakeUpSupport */       FALSE,
        /* wakeUpSource */        0U,
        /* wakeUpByBus */         FALSE
#endif
    },
#if (ETHSM_MAX_NETWORKS > 1)
    /* Network 1 - Secondary Ethernet Network (if configured) */
    {
        /* networkHandle */       ETHSM_NETWORK_1,
        /* ctrlIdx */             ETHSM_CTRL_IDX_NETWORK_1,
        /* trcvIdx */             ETHSM_TRCV_IDX_NETWORK_1,
        /* tcpIpCtrlIdx */        ETHSM_TCPIP_CTRL_IDX_NETWORK_1,
        /* comMChannel */         (ComM_ChannelHandleType)1U,
        /* timeoutWaitTrcvLink */ ETHSM_TIMEOUT_WAIT_TRCVLINK,
        /* timeoutWaitOnline */   ETHSM_TIMEOUT_WAIT_ONLINE,
#if (ETHSM_WAKEUP_SUPPORT == STD_ON)
        /* wakeUpSupport */       TRUE,
        /* wakeUpSource */        ETHSM_WAKEUP_SOURCE_NETWORK_1,
        /* wakeUpByBus */         (ETHSM_WAKEUP_BY_BUS_NETWORK_1 == STD_ON) ? TRUE : FALSE
#else
        /* wakeUpSupport */       FALSE,
        /* wakeUpSource */        0U,
        /* wakeUpByBus */         FALSE
#endif
    }
#endif
};

/*==================================================================================================
*                                    CONTROLLER CONFIGURATION TABLE
==================================================================================================*/

/**
 * @brief EthSM Controller Configuration Table
 * @details Contains configuration for all Ethernet controllers
 * @note This table is indexed by controller index
 */
static const EthSM_CtrlConfigType EthSM_CtrlConfig[ETHSM_MAX_NETWORKS] =
{
    /* Controller 0 */
    {
        /* ctrlIdx */         ETHSM_CTRL_IDX_NETWORK_0,
        /* macAddress */      ETHSM_CTRL_0_MAC_ADDR,
        /* mtu */             ETHSM_DEFAULT_MTU,
        /* vlanSupport */     FALSE,
        /* vlanId */          ETHSM_DEFAULT_VLAN_ID
    },
#if (ETHSM_MAX_NETWORKS > 1)
    /* Controller 1 */
    {
        /* ctrlIdx */         ETHSM_CTRL_IDX_NETWORK_1,
        /* macAddress */      ETHSM_CTRL_1_MAC_ADDR,
        /* mtu */             ETHSM_DEFAULT_MTU,
        /* vlanSupport */     FALSE,
        /* vlanId */          ETHSM_DEFAULT_VLAN_ID
    }
#endif
};

/*==================================================================================================
*                                    TRANSCEIVER CONFIGURATION TABLE
==================================================================================================*/

/**
 * @brief EthSM Transceiver Configuration Table
 * @details Contains configuration for all Ethernet transceivers
 * @note This table is indexed by transceiver index
 */
static const EthSM_TrcvConfigType EthSM_TrcvConfig[ETHSM_MAX_NETWORKS] =
{
    /* Transceiver 0 */
    {
        /* trcvIdx */         ETHSM_TRCV_IDX_NETWORK_0,
        /* wakeUpMode */      1U,         /* Enabled */
        /* autoNegotiation */ TRUE,       /* Auto-negotiation enabled */
        /* speed */           1U,         /* 100 Mbps */
        /* duplexMode */      1U          /* Full duplex */
    },
#if (ETHSM_MAX_NETWORKS > 1)
    /* Transceiver 1 */
    {
        /* trcvIdx */         ETHSM_TRCV_IDX_NETWORK_1,
        /* wakeUpMode */      1U,         /* Enabled */
        /* autoNegotiation */ TRUE,       /* Auto-negotiation enabled */
        /* speed */           1U,         /* 100 Mbps */
        /* duplexMode */      1U          /* Full duplex */
    }
#endif
};

/*==================================================================================================
*                                    TCPIP MAPPING TABLE
==================================================================================================*/

/**
 * @brief EthSM to TcpIp Mapping Table
 * @details Contains mapping between EthSM networks and TcpIp controllers
 * @note This table is indexed by network handle
 */
static const EthSM_TcpIpMappingType EthSM_TcpIpMapping[ETHSM_MAX_NETWORKS] =
{
    /* Network 0 to TcpIp Controller 0 */
    {
        /* networkHandle */   ETHSM_NETWORK_0,
        /* tcpIpCtrlIdx */    ETHSM_TCPIP_CTRL_IDX_NETWORK_0,
        /* dhcpEnabled */     FALSE,      /* Static IP configuration */
        /* staticIpAddress */ ETHSM_NETWORK_0_IP_ADDR,
        /* subnetMask */      ETHSM_NETWORK_0_SUBNET,
        /* gatewayAddress */  ETHSM_NETWORK_0_GATEWAY
    },
#if (ETHSM_MAX_NETWORKS > 1)
    /* Network 1 to TcpIp Controller 1 */
    {
        /* networkHandle */   ETHSM_NETWORK_1,
        /* tcpIpCtrlIdx */    ETHSM_TCPIP_CTRL_IDX_NETWORK_1,
        /* dhcpEnabled */     FALSE,      /* Static IP configuration */
        /* staticIpAddress */ ETHSM_NETWORK_1_IP_ADDR,
        /* subnetMask */      ETHSM_NETWORK_1_SUBNET,
        /* gatewayAddress */  ETHSM_NETWORK_1_GATEWAY
    }
#endif
};

/*==================================================================================================
*                                    GLOBAL CONFIGURATION STRUCTURE
==================================================================================================*/

/**
 * @brief EthSM Global Configuration Type
 * @details Contains all configuration references
 */
typedef struct {
    const EthSM_NetworkConfigType* networkConfig;   /**< Pointer to network config table */
    const EthSM_CtrlConfigType* ctrlConfig;         /**< Pointer to controller config table */
    const EthSM_TrcvConfigType* trcvConfig;         /**< Pointer to transceiver config table */
    const EthSM_TcpIpMappingType* tcpIpMapping;     /**< Pointer to TcpIp mapping table */
    uint8 maxNetworks;                              /**< Maximum number of networks */
} EthSM_GlobalConfigType;

/**
 * @brief EthSM Global Configuration
 * @details Main configuration structure passed to EthSM_Init
 */
static const EthSM_GlobalConfigType EthSM_GlobalConfig =
{
    /* networkConfig */   EthSM_NetworkConfig,
    /* ctrlConfig */      EthSM_CtrlConfig,
    /* trcvConfig */      EthSM_TrcvConfig,
    /* tcpIpMapping */    EthSM_TcpIpMapping,
    /* maxNetworks */     ETHSM_MAX_NETWORKS
};

/*==================================================================================================
*                                    EXTERNAL REFERENCES
==================================================================================================*/

/**
 * @brief Public configuration pointer for EthSM_Init
 * @details This symbol is referenced by EthSM_Init to access configuration
 */
const EthSM_ConfigType* const EthSM_Config = (const EthSM_ConfigType*)&EthSM_GlobalConfig;

/*==================================================================================================
*                                    GETTER FUNCTIONS FOR CONFIGURATION
==================================================================================================*/

/**
 * @brief Gets network configuration for a given network handle
 * @param networkHandle EthSM network handle
 * @return Pointer to network configuration, or NULL_PTR if invalid handle
 */
static const EthSM_NetworkConfigType* EthSM_Lcfg_GetNetworkConfig(EthSM_NetworkHandleType networkHandle)
{
    const EthSM_NetworkConfigType* config = NULL_PTR;

    if (networkHandle < ETHSM_MAX_NETWORKS)
    {
        config = &EthSM_NetworkConfig[networkHandle];
    }

    return config;
}

/**
 * @brief Gets controller configuration for a given controller index
 * @param ctrlIdx Controller index
 * @return Pointer to controller configuration, or NULL_PTR if invalid index
 */
static const EthSM_CtrlConfigType* EthSM_Lcfg_GetCtrlConfig(uint8 ctrlIdx)
{
    const EthSM_CtrlConfigType* config = NULL_PTR;

    if (ctrlIdx < ETHSM_MAX_NETWORKS)
    {
        config = &EthSM_CtrlConfig[ctrlIdx];
    }

    return config;
}

/**
 * @brief Gets transceiver configuration for a given transceiver index
 * @param trcvIdx Transceiver index
 * @return Pointer to transceiver configuration, or NULL_PTR if invalid index
 */
static const EthSM_TrcvConfigType* EthSM_Lcfg_GetTrcvConfig(uint8 trcvIdx)
{
    const EthSM_TrcvConfigType* config = NULL_PTR;

    if (trcvIdx < ETHSM_MAX_NETWORKS)
    {
        config = &EthSM_TrcvConfig[trcvIdx];
    }

    return config;
}

/**
 * @brief Gets TcpIp mapping for a given network handle
 * @param networkHandle EthSM network handle
 * @return Pointer to TcpIp mapping, or NULL_PTR if invalid handle
 */
static const EthSM_TcpIpMappingType* EthSM_Lcfg_GetTcpIpMapping(EthSM_NetworkHandleType networkHandle)
{
    const EthSM_TcpIpMappingType* mapping = NULL_PTR;

    if (networkHandle < ETHSM_MAX_NETWORKS)
    {
        mapping = &EthSM_TcpIpMapping[networkHandle];
    }

    return mapping;
}

/*==================================================================================================
*                                    VERSION CHECK
==================================================================================================*/

/* Check if source and header file are of the same version */
#if (ETHSM_LCFG_AR_RELEASE_MAJOR_VERSION != ETHSM_AR_RELEASE_MAJOR_VERSION)
#error "EthSM_Lcfg.c: AR major version mismatch with EthSM.h"
#endif

#if (ETHSM_LCFG_AR_RELEASE_MINOR_VERSION != ETHSM_AR_RELEASE_MINOR_VERSION)
#error "EthSM_Lcfg.c: AR minor version mismatch with EthSM.h"
#endif

#if (ETHSM_LCFG_SW_MAJOR_VERSION != ETHSM_SW_MAJOR_VERSION)
#error "EthSM_Lcfg.c: SW major version mismatch with EthSM.h"
#endif

#if (ETHSM_LCFG_SW_MINOR_VERSION != ETHSM_SW_MINOR_VERSION)
#error "EthSM_Lcfg.c: SW minor version mismatch with EthSM.h"
#endif
