/**
 * @file Xcp.h
 * @brief XCP (Universal Measurement and Calibration Protocol) module
 *        following ASAM XCP 1.1 standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: XCP Protocol (ASAM XCP 1.1)
 * Layer: Service Layer
 * Purpose: Universal Measurement and Calibration Protocol
 * Supported Transport Layers: CAN, Ethernet (UDP/TCP), FlexRay
 */

#ifndef XCP_H
#define XCP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Xcp_Cfg.h"
#include "Xcp_MemMap.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define XCP_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define XCP_MODULE_ID                   (0xD0U) /* XCP Module ID (208 decimal) */
#define XCP_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define XCP_AR_RELEASE_MINOR_VERSION    (0x04U)
#define XCP_AR_RELEASE_REVISION_VERSION (0x00U)
#define XCP_SW_MAJOR_VERSION            (0x01U)
#define XCP_SW_MINOR_VERSION            (0x00U)
#define XCP_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define XCP_SID_INIT                    (0x00U)
#define XCP_SID_DEINIT                  (0x01U)
#define XCP_SID_GETVERSIONINFO          (0x02U)
#define XCP_SID_MAINFUNCTION            (0x03U)
#define XCP_SID_RXINDICATION            (0x04U)
#define XCP_SID_TXCONFIRMATION          (0x05U)
#define XCP_SID_TRIGGERTRANSMIT         (0x06U)
#define XCP_SID_SETTRANSMISSIONMODE     (0x07U)
#define XCP_SID_GETSESSIONSTATUS        (0x08U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define XCP_E_NO_ERROR                  (0x00U)
#define XCP_E_PARAM_POINTER             (0x01U)
#define XCP_E_PARAM_CHANNEL             (0x02U)
#define XCP_E_PARAM_DAQ                 (0x03U)
#define XCP_E_PARAM_STIM                (0x04U)
#define XCP_E_NOT_INITIALIZED           (0x05U)
#define XCP_E_INVALID_SEQUENCE          (0x06U)
#define XCP_E_PDU_LENGTH                (0x07U)
#define XCP_E_OUT_OF_RANGE              (0x08U)
#define XCP_E_BUSY                      (0x09U)

/*==================================================================================================
*                                    XCP PROTOCOL CONSTANTS
==================================================================================================*/
/* XCP Packet Identifier (PID) */ 
#define XCP_PID_RES                     (0xFFU)  /* Response */
#define XCP_PID_ERR                     (0xFEU)  /* Error */
#define XCP_PID_EV                      (0xFDU)  /* Event */
#define XCP_PID_SERV                    (0xFCU)  /* Service Request */
#define XCP_PID_DAQ                     (0xFBU)  /* DAQ */
#define XCP_PID_STIM                    (0xFAU)  /* Stimulation */

/* Standard Command Codes */
#define XCP_CMD_CONNECT                 (0xFFU)
#define XCP_CMD_DISCONNECT              (0xFEU)
#define XCP_CMD_GET_STATUS              (0xFDU)
#define XCP_CMD_SYNCH                   (0xFCU)
#define XCP_CMD_GET_COMM_MODE_INFO      (0xFBU)
#define XCP_CMD_GET_ID                  (0xFAU)
#define XCP_CMD_SET_REQUEST             (0xF9U)
#define XCP_CMD_GET_SEED                (0xF8U)
#define XCP_CMD_UNLOCK                  (0xF7U)
#define XCP_CMD_SET_MTA                 (0xF6U)
#define XCP_CMD_UPLOAD                  (0xF5U)
#define XCP_CMD_SHORT_UPLOAD            (0xF4U)
#define XCP_CMD_BUILD_CHECKSUM          (0xF3U)
#define XCP_CMD_TRANSPORT_LAYER_CMD     (0xF2U)
#define XCP_CMD_USER_CMD                (0xF1U)

/* Calibration Commands */
#define XCP_CMD_DOWNLOAD                (0xF0U)
#define XCP_CMD_DOWNLOAD_NEXT           (0xEFU)
#define XCP_CMD_DOWNLOAD_MAX            (0xEEU)
#define XCP_CMD_SHORT_DOWNLOAD          (0xEDU)
#define XCP_CMD_MODIFY_BITS             (0xECU)

/* Page Switching Commands */
#define XCP_CMD_SET_CAL_PAGE            (0xEBU)
#define XCP_CMD_GET_CAL_PAGE            (0xEAU)
#define XCP_CMD_COPY_CAL_PAGE           (0xE9U)

/* DAQ Commands */
#define XCP_CMD_CLEAR_DAQ_LIST          (0xE3U)
#define XCP_CMD_SET_DAQ_PTR             (0xE2U)
#define XCP_CMD_WRITE_DAQ               (0xE1U)
#define XCP_CMD_SET_DAQ_LIST_MODE       (0xE0U)
#define XCP_CMD_GET_DAQ_LIST_MODE       (0xDFU)
#define XCP_CMD_START_STOP_DAQ_LIST     (0xDEU)
#define XCP_CMD_START_STOP_SYNCH        (0xDDU)
#define XCP_CMD_GET_DAQ_CLOCK           (0xDCU)
#define XCP_CMD_READ_DAQ                (0xDBU)
#define XCP_CMD_GET_DAQ_PROCESSOR_INFO  (0xDAU)
#define XCP_CMD_GET_DAQ_RESOLUTION_INFO (0xD9U)
#define XCP_CMD_GET_DAQ_LIST_INFO       (0xD8U)
#define XCP_CMD_GET_DAQ_EVENT_INFO      (0xD7U)
#define XCP_CMD_FREE_DAQ                (0xD6U)
#define XCP_CMD_ALLOC_DAQ               (0xD5U)
#define XCP_CMD_ALLOC_ODT               (0xD4U)
#define XCP_CMD_ALLOC_ODT_ENTRY         (0xD3U)

/* Stimulation Commands */
#define XCP_CMD_PROGRAM_START           (0xD2U)
#define XCP_CMD_PROGRAM_CLEAR           (0xD1U)
#define XCP_CMD_PROGRAM                 (0xD0U)
#define XCP_CMD_PROGRAM_RESET           (0xCFU)
#define XCP_CMD_PROGRAM_NEXT            (0xCEU)
#define XCP_CMD_PROGRAM_MAX             (0xCDU)
#define XCP_CMD_PROGRAM_VERIFY          (0xCCU)

/* Non-Volatile Memory Programming Commands */
#define XCP_CMD_WRITE_DAQ_MULTIPLE      (0xCBU)
#define XCP_CMD_WRITE_DAQ_ENTRY         (0xCAU)
#define XCP_CMD_READ_DAQ_ENTRY          (0xC9U)

/* Time Synchronization Commands */
#define XCP_CMD_TIME_CORRELATION_PROPERTIES (0xC8U)

/* Error Codes */
#define XCP_ERR_CMD_SYNCH               (0x00U)
#define XCP_ERR_CMD_BUSY                (0x10U)
#define XCP_ERR_DAQ_ACTIVE              (0x11U)
#define XCP_ERR_PGM_ACTIVE              (0x12U)
#define XCP_ERR_CMD_UNKNOWN             (0x20U)
#define XCP_ERR_CMD_SYNTAX              (0x21U)
#define XCP_ERR_OUT_OF_RANGE            (0x22U)
#define XCP_ERR_WRITE_PROTECTED         (0x23U)
#define XCP_ERR_ACCESS_DENIED           (0x24U)
#define XCP_ERR_ACCESS_LOCKED           (0x25U)
#define XCP_ERR_PAGE_NOT_VALID          (0x26U)
#define XCP_ERR_MODE_NOT_VALID          (0x27U)
#define XCP_ERR_SEGMENT_NOT_VALID       (0x28U)
#define XCP_ERR_SEQUENCE                (0x29U)
#define XCP_ERR_DAQ_CONFIG              (0x2AU)
#define XCP_ERR_MEMORY_OVERFLOW         (0x30U)
#define XCP_ERR_GENERIC                 (0x31U)
#define XCP_ERR_VERIFY                  (0x32U)
#define XCP_ERR_RESOURCE_TEMPORARY_NOT_ACCESSIBLE (0x33U)

/*==================================================================================================
*                                    XCP TRANSPORT LAYER TYPES
==================================================================================================*/
typedef enum {
    XCP_TRANSPORT_CAN = 0,
    XCP_TRANSPORT_ETH_UDP,
    XCP_TRANSPORT_ETH_TCP,
    XCP_TRANSPORT_FLEXRAY,
    XCP_TRANSPORT_USB,
    XCP_TRANSPORT_LIN
} Xcp_TransportLayerType;

/*==================================================================================================
*                                    XCP CONNECTION STATE TYPE
==================================================================================================*/
typedef enum {
    XCP_STATE_DISCONNECTED = 0,
    XCP_STATE_CONNECTED
} Xcp_ConnectionStateType;

/*==================================================================================================
*                                    XCP SESSION STATUS TYPE
==================================================================================================*/
#define XCP_SESSION_RESUME              (0x01U)
#define XCP_SESSION_DAQ_RUNNING         (0x02U)
#define XCP_SESSION_CLEAR_DAQ_REQUEST   (0x04U)
#define XCP_SESSION_STORE_DAQ_REQUEST   (0x08U)
#define XCP_SESSION_STORE_CAL_REQUEST   (0x10U)
#define XCP_SESSION_PGM_RUNNING         (0x40U)

typedef uint8 Xcp_SessionStatusType;

/*==================================================================================================
*                                    XCP RESOURCE PROTECTION TYPE
==================================================================================================*/
#define XCP_RESOURCE_CAL_PAG            (0x01U)
#define XCP_RESOURCE_DAQ                (0x04U)
#define XCP_RESOURCE_STIM               (0x08U)
#define XCP_RESOURCE_PGM                (0x10U)

typedef uint8 Xcp_ResourceProtectionType;

/*==================================================================================================
*                                    XCP DAQ STATE TYPE
==================================================================================================*/
typedef enum {
    XCP_DAQ_STATE_STOPPED = 0,
    XCP_DAQ_STATE_RUNNING
} Xcp_DaqStateType;

/*==================================================================================================
*                                    XCP DAQ MODE TYPE
==================================================================================================*/
#define XCP_DAQ_MODE_SELECTED           (0x01U)
#define XCP_DAQ_MODE_STIM               (0x02U)
#define XCP_DAQ_MODE_DTO_CTR            (0x04U)
#define XCP_DAQ_MODE_TIMESTAMP          (0x10U)
#define XCP_DAQ_MODE_PID_OFF            (0x20U)
#define XCP_DAQ_MODE_RUNNING            (0x40U)
#define XCP_DAQ_MODE_RESUME             (0x80U)

/*==================================================================================================
*                                    XCP PGM MODE TYPE
==================================================================================================*/
typedef enum {
    XCP_PGM_STATE_IDLE = 0,
    XCP_PGM_STATE_STARTED,
    XCP_PGM_STATE_PROGRAMMING
} Xcp_PgmStateType;

/*==================================================================================================
*                                    XCP COMMUNICATION MODE TYPE
==================================================================================================*/
typedef enum {
    XCP_COMM_MODE_BASIC = 0,
    XCP_COMM_MODE_OPTIONAL,
    XCP_COMM_MODE_BLOCK
} Xcp_CommModeType;

/*==================================================================================================
*                                    XCP ADDRESS EXTENSION TYPE
==================================================================================================*/
typedef uint8 Xcp_AddressExtensionType;

/*==================================================================================================
*                                    XCP MTA (Memory Transfer Address) TYPE
==================================================================================================*/
typedef struct {
    uint32 Address;
    Xcp_AddressExtensionType Extension;
} Xcp_MtaType;

/*==================================================================================================
*                                    XCP ODT ENTRY TYPE
==================================================================================================*/
typedef struct {
    uint32 BitOffset;      /* Bit offset for bit access */
    uint32 EleLength;      /* Element length in bits */
    uint8 AddrExt;         /* Address extension */
    uint32 Addr;           /* Address */
    boolean IsValid;
} Xcp_OdtEntryType;

/*==================================================================================================
*                                    XCP ODT TYPE
==================================================================================================*/
typedef struct {
    Xcp_OdtEntryType* Entries;
    uint8 NumEntries;
    uint8 NextOdt;         /* Next ODT for this DAQ list */
    uint8 FirstPid;        /* First PID for this ODT */
} Xcp_OdtType;

/*==================================================================================================
*                                    XCP DAQ LIST TYPE
==================================================================================================*/
typedef struct {
    uint16 ListNumber;
    Xcp_OdtType* OdtList;
    uint8 NumOdts;
    uint8 Mode;
    uint16 Prescaler;
    uint16 EventChannel;
    uint8 Priority;
    Xcp_DaqStateType State;
    boolean IsAllocated;
    uint32 CurrentTimestamp;
    uint16 CurrentOdt;
} Xcp_DaqListType;

/*==================================================================================================
*                                    XCP EVENT CHANNEL TYPE
==================================================================================================*/
typedef enum {
    XCP_EVENT_CYCLE_1MS = 0,
    XCP_EVENT_CYCLE_10MS,
    XCP_EVENT_CYCLE_100MS,
    XCP_EVENT_TRIGGERED
} Xcp_EventChannelType;

/*==================================================================================================
*                                    XCP DTO (Data Transfer Object) TYPE
==================================================================================================*/
typedef struct {
    uint8 IdentificationField;
    uint8 Data[XCP_MAX_DTO_SIZE];
} Xcp_DtoType;

/*==================================================================================================
*                                    XCP CTO (Command Transfer Object) TYPE
==================================================================================================*/
typedef struct {
    uint8 Data[XCP_MAX_CTO_SIZE];
    uint8 Length;
} Xcp_CtoType;

/*==================================================================================================
*                                    XCP SESSION CONFIGURATION TYPE
==================================================================================================*/
typedef struct {
    boolean CalPageSupported;
    boolean DaqSupported;
    boolean StimSupported;
    boolean PgmSupported;
    uint8 MaxCto;
    uint16 MaxDto;
    uint16 MaxDaq;
    uint16 MaxEventChannels;
    uint8 MinDaq;
    uint8 DaqKeyByte;
    uint8 PgmKeyByte;
    uint16 DaqListSize;
    uint8 OdtCount;
    uint8 OdtEntryCount;
} Xcp_SessionConfigType;

/*==================================================================================================
*                                    XCP CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    Xcp_TransportLayerType TransportLayer;
    uint8 Priority;
    boolean IsActive;
} Xcp_ChannelConfigType;

/*==================================================================================================
*                                    XCP CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Xcp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
    const Xcp_SessionConfigType* SessionConfig;
    Xcp_DaqListType* DaqLists;
    uint8 NumDaqLists;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean BlockDownloadSupported;
    boolean InterleavedModeSupported;
    uint16 MainFunctionPeriod;
} Xcp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define XCP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Xcp_ConfigType Xcp_Config;

#define XCP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CALLBACK FUNCTION TYPES
==================================================================================================*/
typedef void (*Xcp_CmdProcessorType)(const uint8* Data, uint8 Length);
typedef Std_ReturnType (*Xcp_SeedKeyCallbackType)(uint8 Resource, uint8* Seed, uint8* Key);

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define XCP_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_Xcp_00001 */
/**
 * @brief Initializes the XCP module
 * @param ConfigPtr Pointer to configuration structure
 */
void Xcp_Init(const Xcp_ConfigType* ConfigPtr);

/** @req SWS_Xcp_00002 */
/**
 * @brief Deinitializes the XCP module
 */
void Xcp_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (XCP_VERSION_INFO_API == STD_ON)
/** @req SWS_Xcp_00003 */
void Xcp_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @req SWS_Xcp_00004 */
/**
 * @brief Main function for cyclic processing
 */
void Xcp_MainFunction(void);

/** @req SWS_Xcp_00005 */
/**
 * @brief Rx Indication callback from lower layer
 * @param XcpChannelId Channel ID
 * @param XcpPduId PDU ID
 * @param XcpRxPduPtr Pointer to received PDU data
 */
void Xcp_RxIndication(uint8 XcpChannelId, PduIdType XcpPduId, const PduInfoType* XcpRxPduPtr);

/** @req SWS_Xcp_00006 */
/**
 * @brief Tx Confirmation callback from lower layer
 * @param XcpChannelId Channel ID
 * @param XcpTxPduId PDU ID
 */
void Xcp_TxConfirmation(uint8 XcpChannelId, PduIdType XcpTxPduId);

/** @req SWS_Xcp_00007 */
/**
 * @brief Trigger transmit callback from lower layer
 * @param XcpChannelId Channel ID
 * @param XcpTxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType Xcp_TriggerTransmit(uint8 XcpChannelId, PduIdType XcpTxPduId, PduInfoType* PduInfoPtr);

/** @req SWS_Xcp_00008 */
/**
 * @brief Sets transmission mode
 * @param XcpChannelId Channel ID
 * @param Mode Transmission mode
 */
void Xcp_SetTransmissionMode(uint8 XcpChannelId, uint8 Mode);

/**
 * @brief Gets current session status
 * @return Session status
 */
Xcp_SessionStatusType Xcp_GetSessionStatus(void);

/** @req SWS_Xcp_00009 */
/**
 * @brief Processes received XCP command
 * @param ChannelId Channel ID
 * @param Data Command data
 * @param Length Data length
 */
void Xcp_ProcessCommand(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00010 */
/**
 * @brief Sends response packet
 * @param ChannelId Channel ID
 * @param Data Response data
 * @param Length Data length
 */
void Xcp_SendResponse(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00011 */
/**
 * @brief Sends error packet
 * @param ChannelId Channel ID
 * @param ErrorCode Error code
 * @param ErrorInfo Additional error info
 */
void Xcp_SendError(uint8 ChannelId, uint8 ErrorCode, uint8 ErrorInfo);

/** @req SWS_Xcp_00012 */
/**
 * @brief Sends event packet
 * @param ChannelId Channel ID
 * @param EventCode Event code
 * @param Data Event data
 * @param Length Data length
 */
void Xcp_SendEvent(uint8 ChannelId, uint8 EventCode, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00013 */
/**
 * @brief Handles Connect command
 * @param ChannelId Channel ID
 * @param Data Command data
 * @param Length Data length
 */
void Xcp_CmdConnect(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00014 */
/**
 * @brief Handles Disconnect command
 * @param ChannelId Channel ID
 */
void Xcp_CmdDisconnect(uint8 ChannelId);

/** @req SWS_Xcp_00015 */
/**
 * @brief Handles GetStatus command
 * @param ChannelId Channel ID
 */
void Xcp_CmdGetStatus(uint8 ChannelId);

/** @req SWS_Xcp_00016 */
/**
 * @brief Handles GetCommModeInfo command
 * @param ChannelId Channel ID
 */
void Xcp_CmdGetCommModeInfo(uint8 ChannelId);

/** @req SWS_Xcp_00017 */
/**
 * @brief Handles GetID command
 * @param ChannelId Channel ID
 * @param Data Command data
 * @param Length Data length
 */
void Xcp_CmdGetId(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00018 */
/**
 * @brief Handles SetMTA command
 * @param ChannelId Channel ID
 * @param Data Command data
 */
void Xcp_CmdSetMta(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00019 */
/**
 * @brief Handles Upload command
 * @param ChannelId Channel ID
 * @param Data Command data
 */
void Xcp_CmdUpload(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00020 */
/**
 * @brief Handles ShortUpload command
 * @param ChannelId Channel ID
 * @param Data Command data
 */
void Xcp_CmdShortUpload(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00021 */
/**
 * @brief Handles Download command
 * @param ChannelId Channel ID
 * @param Data Command data
 * @param Length Data length
 */
void Xcp_CmdDownload(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00022 */
/**
 * @brief Handles GetSeed command
 * @param ChannelId Channel ID
 * @param Data Command data
 */
void Xcp_CmdGetSeed(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00023 */
/**
 * @brief Handles Unlock command
 * @param ChannelId Channel ID
 * @param Data Command data
 * @param Length Data length
 */
void Xcp_CmdUnlock(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00024 */
/**
 * @brief DAQ Functions
 */
void Xcp_CmdClearDaqList(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00025 */
void Xcp_CmdSetDaqPtr(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00026 */
void Xcp_CmdWriteDaq(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00027 */
void Xcp_CmdSetDaqListMode(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00028 */
void Xcp_CmdGetDaqListMode(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00029 */
void Xcp_CmdStartStopDaqList(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00030 */
void Xcp_CmdStartStopSynch(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00031 */
void Xcp_CmdGetDaqProcessorInfo(uint8 ChannelId);
/** @req SWS_Xcp_00032 */
void Xcp_CmdGetDaqResolutionInfo(uint8 ChannelId);
/** @req SWS_Xcp_00033 */
void Xcp_CmdGetDaqListInfo(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00034 */
void Xcp_CmdFreeDaq(uint8 ChannelId);
/** @req SWS_Xcp_00035 */
void Xcp_CmdAllocDaq(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00036 */
void Xcp_CmdAllocOdt(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00037 */
void Xcp_CmdAllocOdtEntry(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00038 */
/**
 * @brief PGM Functions
 */
void Xcp_CmdProgramStart(uint8 ChannelId);
/** @req SWS_Xcp_00039 */
void Xcp_CmdProgramClear(uint8 ChannelId, const uint8* Data);
/** @req SWS_Xcp_00040 */
void Xcp_CmdProgram(uint8 ChannelId, const uint8* Data, uint8 Length);
/** @req SWS_Xcp_00041 */
void Xcp_CmdProgramReset(uint8 ChannelId);
/** @req SWS_Xcp_00042 */
void Xcp_CmdProgramVerify(uint8 ChannelId, const uint8* Data);

/** @req SWS_Xcp_00043 */
/**
 * @brief DAQ Processing Functions
 */
void Xcp_DaqProcessor(void);
/** @req SWS_Xcp_00044 */
void Xcp_DaqSample(uint16 DaqListIdx);
/** @req SWS_Xcp_00045 */
void Xcp_DaqTransmit(uint16 DaqListIdx, uint8 OdtIdx);

/** @req SWS_Xcp_00046 */
/**
 * @brief STIM Processing Functions
 */
void Xcp_StimProcessor(uint8 ChannelId, const uint8* Data, uint8 Length);

/** @req SWS_Xcp_00047 */
/**
 * @brief Memory Access Functions
 * @param Addr Memory address
 * @param Ext Address extension
 * @param Data Data buffer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType Xcp_ReadMemory(uint32 Addr, uint8 Ext, uint8* Data, uint32 Length);
/** @req SWS_Xcp_00048 */
Std_ReturnType Xcp_WriteMemory(uint32 Addr, uint8 Ext, const uint8* Data, uint32 Length);

/** @req SWS_Xcp_00049 */
/**
 * @brief Set resource protection
 * @param Resource Resource type
 * @param Protected Protection state
 */
void Xcp_SetResourceProtection(uint8 Resource, boolean Protected);

/** @req SWS_Xcp_00050 */
/**
 * @brief Check resource protection
 * @param Resource Resource type
 * @return TRUE if protected, FALSE otherwise
 */
boolean Xcp_IsResourceProtected(uint8 Resource);

/** @req SWS_Xcp_00051 */
/**
 * @brief Unlock resource
 * @param Resource Resource type
 * @return Result of operation
 */
Std_ReturnType Xcp_UnlockResource(uint8 Resource);

#define XCP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* XCP_H */
