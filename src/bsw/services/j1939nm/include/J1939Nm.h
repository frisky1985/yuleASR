/**
 * @file J1939Nm.h
 * @brief J1939 Network Management module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: J1939 Network Management (NM)
 * Layer: Service Layer
 * Module ID: 0x8D (J1939NM_MODULE_ID)
 */

#ifndef J1939NM_H
#define J1939NM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "J1939Nm_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define J1939NM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define J1939NM_MODULE_ID                   (0x8DU) /* J1939NM Module ID */
#define J1939NM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define J1939NM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define J1939NM_AR_RELEASE_REVISION_VERSION (0x00U)
#define J1939NM_SW_MAJOR_VERSION            (0x01U)
#define J1939NM_SW_MINOR_VERSION            (0x00U)
#define J1939NM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define J1939NM_SID_INIT                    (0x01U)
#define J1939NM_SID_DEINIT                  (0x02U)
#define J1939NM_SID_GET_VERSION_INFO        (0x03U)
#define J1939NM_SID_GET_STATE               (0x04U)
#define J1939NM_SID_GET_BUS_OFF_STATE       (0x05U)
#define J1939NM_SID_SET_BUS_OFF_STATE       (0x06U)
#define J1939NM_SID_GET_ADDRESS             (0x07U)
#define J1939NM_SID_SET_ADDRESS             (0x08U)
#define J1939NM_SID_GET_NAME                (0x09U)
#define J1939NM_SID_SET_NAME                (0x0AU)
#define J1939NM_SID_MAIN_FUNCTION           (0x0BU)
#define J1939NM_SID_BUS_OFF_CBK             (0x0CU)
#define J1939NM_SID_RX_INDICATION           (0x0DU)
#define J1939NM_SID_TX_CONFIRMATION         (0x0EU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define J1939NM_E_NOT_INITIALIZED           (0x01U)
#define J1939NM_E_INVALID_PARAMETER         (0x02U)
#define J1939NM_E_INVALID_POINTER           (0x03U)
#define J1939NM_E_INVALID_STATE             (0x04U)
#define J1939NM_E_INIT_FAILED               (0x05U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define J1939NM_PDU_ADDRESS_CLAIMED         (0x00EE00U) /* Address Claimed (0x00EE00) */
#define J1939NM_PDU_REQUEST_FOR_AC          (0x00EA00U) /* Request for Address Claimed (0x00EA00) */
#define J1939NM_PDU_CANNOT_CLAIM_ADDRESS    (0x00EEFFU) /* Cannot Claim Address (0x00EEFF with NULL address) */

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief J1939 NAME structure (64-bit)
 * 
 * NAME fields according to J1939-81:
 * - Identity Number: 21 bits
 * - Manufacturer Code: 11 bits
 * - ECU Instance: 3 bits
 * - Function Instance: 5 bits
 * - Function: 8 bits
 * - Reserved: 1 bit
 * - Vehicle System: 7 bits
 * - Vehicle System Instance: 4 bits
 * - Industry Group: 3 bits
 * - Arbitrary Address Capable: 1 bit
 */
typedef uint64 J1939Nm_NameType;

/**
 * @brief J1939 Address type (0-253 valid, 254 null, 255 global)
 */
typedef uint8 J1939Nm_AddressType;

/**
 * @brief J1939 NM Channel type
 */
typedef uint8 J1939Nm_ChannelType;

/**
 * @brief J1939 Node type
 */
typedef uint8 J1939Nm_NodeType;

/**
 * @brief J1939 NM State type
 */
typedef enum {
    J1939NM_STATE_UNINIT = 0,           /*!< Module not initialized */
    J1939NM_STATE_BUS_OFF,              /*!< Bus-off state */
    J1939NM_STATE_WAIT_FOR_AC,          /*!< Waiting for address claimed */
    J1939NM_STATE_AC_DELAY,             /*!< Address claim delay */
    J1939NM_STATE_NORMAL_OPERATION,     /*!< Normal operation */
    J1939NM_STATE_TX_AC,                /*!< Transmitting address claimed */
    J1939NM_STATE_TX_CANNOT_CLAIM       /*!< Cannot claim address */
} J1939Nm_StateType;

/**
 * @brief J1939 Address Claiming State type
 */
typedef enum {
    J1939NM_AC_STATE_IDLE = 0,          /*!< No address claiming in progress */
    J1939NM_AC_STATE_WAITING,           /*!< Waiting for address claimed response */
    J1939NM_AC_STATE_CLAIMED,           /*!< Address successfully claimed */
    J1939NM_AC_STATE_CONFLICT,          /*!< Address conflict detected */
    J1939NM_AC_STATE_CANNOT_CLAIM       /*!< Cannot claim address */
} J1939Nm_AcStateType;

/**
 * @brief J1939 NM Configuration type
 */
typedef struct {
    J1939Nm_ChannelType ChannelId;      /*!< Channel ID */
    J1939Nm_NodeType NodeId;            /*!< Node ID */
    J1939Nm_NameType Name;              /*!< J1939 NAME */
    J1939Nm_AddressType Address;        /*!< Default address */
    J1939Nm_AddressType PreferredAddress; /*!< Preferred address */
    boolean ArbitraryAddressCapable;    /*!< Arbitrary address capable */
    uint8 AcDelayMin;                   /*!< Min address claim delay (ms) */
    uint8 AcDelayMax;                   /*!< Max address claim delay (ms) */
    uint8 AcTimeout;                    /*!< Address claim timeout (ms) */
    uint8 BusOffRecoveryTime;           /*!< Bus-off recovery time (ms) */
} J1939Nm_ChannelConfigType;

/**
 * @brief J1939 NM Configuration type
 */
typedef struct {
    const J1939Nm_ChannelConfigType* ChannelConfig;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean NodeDetectionEnabled;
    boolean NodeMonitoringEnabled;
} J1939Nm_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define J1939NM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const J1939Nm_ConfigType J1939Nm_Config;

#define J1939NM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define J1939NM_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_J1939Nm_00001 */
/**
 * @brief Initializes the J1939 Network Management module
 * @param ConfigPtr Pointer to configuration structure
 */
extern void J1939Nm_Init(const J1939Nm_ConfigType* ConfigPtr);

/** @req SWS_J1939Nm_00002 */
/**
 * @brief Deinitializes the J1939 Network Management module
 */
extern void J1939Nm_DeInit(void);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
#if (J1939NM_VERSION_INFO_API == STD_ON)
/** @req SWS_J1939Nm_00003 */
extern void J1939Nm_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/** @req SWS_J1939Nm_00005 */
/**
 * @brief Gets the current NM state for a channel
 * @param Channel Channel ID
 * @param State Pointer to store state
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_GetState(J1939Nm_ChannelType Channel, J1939Nm_StateType* State);

/** @req SWS_J1939Nm_00006 */
/**
 * @brief Gets the bus-off state for a channel
 * @param Channel Channel ID
 * @param BusOffState Pointer to store bus-off state
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_GetBusOffState(J1939Nm_ChannelType Channel, boolean* BusOffState);

/** @req SWS_J1939Nm_00007 */
/**
 * @brief Sets the bus-off state for a channel
 * @param Channel Channel ID
 * @param BusOffState Bus-off state to set
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_SetBusOffState(J1939Nm_ChannelType Channel, boolean BusOffState);

/** @req SWS_J1939Nm_00008 */
/**
 * @brief Gets the current address for a channel
 * @param Channel Channel ID
 * @param Address Pointer to store address
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_GetAddress(J1939Nm_ChannelType Channel, J1939Nm_AddressType* Address);

/** @req SWS_J1939Nm_00009 */
/**
 * @brief Sets the address for a channel
 * @param Channel Channel ID
 * @param Address Address to set
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_SetAddress(J1939Nm_ChannelType Channel, J1939Nm_AddressType Address);

/** @req SWS_J1939Nm_00010 */
/**
 * @brief Gets the NAME for a channel
 * @param Channel Channel ID
 * @param Name Pointer to store NAME
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_GetName(J1939Nm_ChannelType Channel, J1939Nm_NameType* Name);

/** @req SWS_J1939Nm_00011 */
/**
 * @brief Sets the NAME for a channel
 * @param Channel Channel ID
 * @param Name NAME to set
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_SetName(J1939Nm_ChannelType Channel, J1939Nm_NameType Name);

/** @req SWS_J1939Nm_00004 */
/**
 * @brief Main function for J1939NM (to be called periodically)
 */
extern void J1939Nm_MainFunction(void);

/** @req SWS_J1939Nm_00012 */
/**
 * @brief Bus-off callback from CanIf
 * @param Channel Channel ID
 */
extern void J1939Nm_BusOffCbk(J1939Nm_ChannelType Channel);

/** @req SWS_J1939Nm_00013 */
/**
 * @brief RxIndication callback from CanIf
 * @param Channel Channel ID
 * @param CanId CAN ID of received message
 * @param Data Pointer to received data
 * @param DataLength Length of received data
 */
extern void J1939Nm_RxIndication(
    J1939Nm_ChannelType Channel,
    uint32 CanId,
    const uint8* Data,
    uint8 DataLength
);

/** @req SWS_J1939Nm_00014 */
/**
 * @brief TxConfirmation callback from CanIf
 * @param Channel Channel ID
 * @param TxPduId PDU ID
 * @param result Transmission result
 */
extern void J1939Nm_TxConfirmation(J1939Nm_ChannelType Channel, PduIdType TxPduId, Std_ReturnType result);

/** @req SWS_J1939Nm_00015 */
/**
 * @brief Requests transmission of Address Claimed message
 * @param Channel Channel ID
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_RequestAddressClaimed(J1939Nm_ChannelType Channel);

/** @req SWS_J1939Nm_00016 */
/**
 * @brief Requests transmission of Cannot Claim Address message
 * @param Channel Channel ID
 * @return Result of operation
 */
extern Std_ReturnType J1939Nm_RequestCannotClaimAddress(J1939Nm_ChannelType Channel);

/** @req SWS_J1939Nm_00017 */
/**
 * @brief Handles address conflict detection
 * @param Channel Channel ID
 * @param ReceivedName Received NAME
 * @param ReceivedAddress Received address
 */
extern void J1939Nm_HandleAddressConflict(
    J1939Nm_ChannelType Channel,
    J1939Nm_NameType ReceivedName,
    J1939Nm_AddressType ReceivedAddress
);

#define J1939NM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* J1939NM_H */
