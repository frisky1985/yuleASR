/**
 * @file DoCan.h
 * @brief Diagnostic over CAN module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic over CAN (DoCan)
 * Layer: Service Layer
 */

#ifndef DOCAN_H
#define DOCAN_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "DoCan_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DOCAN_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define DOCAN_MODULE_ID                 (0x4DU) /* DOCAN Module ID */
#define DOCAN_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define DOCAN_AR_RELEASE_MINOR_VERSION  (0x04U)
#define DOCAN_AR_RELEASE_REVISION_VERSION (0x00U)
#define DOCAN_SW_MAJOR_VERSION          (0x01U)
#define DOCAN_SW_MINOR_VERSION          (0x00U)
#define DOCAN_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DOCAN_SID_INIT                  (0x01U)
#define DOCAN_SID_DEINIT                (0x02U)
#define DOCAN_SID_GETVERSIONINFO        (0x03U)
#define DOCAN_SID_TRANSMIT              (0x04U)
#define DOCAN_SID_RXINDICATION          (0x05U)
#define DOCAN_SID_TXCONFIRMATION        (0x06U)
#define DOCAN_SID_MAINFUNCTION          (0x07U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DOCAN_E_PARAM_POINTER           (0x01U)
#define DOCAN_E_PARAM_CONFIG            (0x02U)
#define DOCAN_E_UNINIT                  (0x03U)
#define DOCAN_E_INVALID_PDU_ID          (0x04U)
#define DOCAN_E_INVALID_CHANNEL         (0x05U)
#define DOCAN_E_TX_FAILED               (0x06U)

/*==================================================================================================
*                                    DOCAN STATE TYPE
==================================================================================================*/
typedef enum {
    DOCAN_STATE_UNINIT = 0,
    DOCAN_STATE_INIT
} DoCan_StateType;

/*==================================================================================================
*                                    DOCAN CHANNEL TYPE
==================================================================================================*/
typedef enum {
    DOCAN_CHANNEL_PHYSICAL = 0,
    DOCAN_CHANNEL_FUNCTIONAL
} DoCan_ChannelType;

/*==================================================================================================
*                                    DOCAN CHANNEL STATE TYPE
==================================================================================================*/
typedef enum {
    DOCAN_CHANNEL_IDLE = 0,
    DOCAN_CHANNEL_TX_IN_PROGRESS,
    DOCAN_CHANNEL_RX_IN_PROGRESS
} DoCan_ChannelStateType;

/*==================================================================================================
*                                    DOCAN PDU MAPPING TYPE
==================================================================================================*/
typedef struct {
    PduIdType DoCanPduId;
    PduIdType CanTpPduId;
    DoCan_ChannelType ChannelType;
    boolean TxConfirmationEnabled;
    boolean RxIndicationEnabled;
} DoCan_PduMappingType;

/*==================================================================================================
*                                    DOCAN CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 ChannelId;
    DoCan_ChannelType ChannelType;
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint16 TimeoutMs;
} DoCan_ChannelConfigType;

/*==================================================================================================
*                                    DOCAN CONFIG TYPE
==================================================================================================*/
typedef struct {
    const DoCan_PduMappingType* PduMappings;
    uint8 NumPduMappings;
    const DoCan_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} DoCan_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DOCAN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const DoCan_ConfigType DoCan_Config;

#define DOCAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DOCAN_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the DoCan module
 * @param ConfigPtr Pointer to configuration structure
 */
void DoCan_Init(const DoCan_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the DoCan module
 */
void DoCan_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void DoCan_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Transmits a diagnostic message via CanTp
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType DoCan_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Receive indication from CanTp
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void DoCan_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Transmit confirmation from CanTp
 * @param TxPduId PDU that was transmitted
 * @param result Transmission result
 */
void DoCan_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Main function for periodic processing
 */
void DoCan_MainFunction(void);

#define DOCAN_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DOCAN_H */
