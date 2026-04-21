/**
 * @file Dcm.h
 * @brief Diagnostic Communication Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Communication Manager (DCM)
 * Layer: Service Layer
 * Purpose: UDS (ISO 14229) and OBD-II diagnostic protocol implementation
 */

#ifndef DCM_H
#define DCM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dcm_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DCM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define DCM_MODULE_ID                   (0x29U) /* DCM Module ID */
#define DCM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DCM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DCM_AR_RELEASE_REVISION_VERSION (0x00U)
#define DCM_SW_MAJOR_VERSION            (0x01U)
#define DCM_SW_MINOR_VERSION            (0x00U)
#define DCM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DCM_SID_INIT                    (0x01U)
#define DCM_SID_START                   (0x02U)
#define DCM_SID_STOP                    (0x03U)
#define DCM_SID_GETVERSIONINFO          (0x04U)
#define DCM_SID_MAINFUNCTION            (0x05U)
#define DCM_SID_GETSECURITYLEVEL        (0x0DU)
#define DCM_SID_GETSESCTRLTYPE          (0x0EU)
#define DCM_SID_GETACTIVEDIAGNOSTIC     (0x0FU)
#define DCM_SID_SETACTIVEDIAGNOSTIC     (0x10U)
#define DCM_SID_RESETTODEFAULTSESSION   (0x11U)
#define DCM_SID_TRIGGERONEVENT          (0x12U)
#define DCM_SID_GETVIN                  (0x13U)
#define DCM_SID_GETPROXYINFO            (0x14U)
#define DCM_SID_SETPROXYINFO            (0x15U)
#define DCM_SID_TXCONFIRMATION          (0x40U)
#define DCM_SID_RXINDICATION            (0x42U)
#define DCM_SID_SVCHANDLER              (0x50U)

/* Service ID aliases used in Dcm.c */
#define DCM_SERVICE_ID_INIT             DCM_SID_INIT
#define DCM_SERVICE_ID_DEINIT           DCM_SID_DEINIT
#define DCM_SERVICE_ID_TXCONFIRMATION   DCM_SID_TXCONFIRMATION
#define DCM_SERVICE_ID_RXINDICATION     DCM_SID_RXINDICATION
#define DCM_SERVICE_ID_TRIGGERTRANSMIT  DCM_SID_TXCONFIRMATION
#define DCM_SERVICE_ID_GETVERSIONINFO   DCM_SID_GETVERSIONINFO

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DCM_E_INTERFACE_TIMEOUT         (0x01U)
#define DCM_E_INTERFACE_RETURN_VALUE    (0x02U)
#define DCM_E_INTERFACE_BUFFER_OVERFLOW (0x03U)
#define DCM_E_UNINIT                    (0x05U)
#define DCM_E_PARAM                     (0x06U)
#define DCM_E_PARAM_POINTER             (0x07U)
#define DCM_E_INIT_FAILED               (0x08U)
#define DCM_E_INVALID_VALUE             (0x09U)

/*==================================================================================================
*                                    UDS SERVICE IDs
==================================================================================================*/
#define DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL      (0x10U)
#define DCM_UDS_SID_ECU_RESET                       (0x11U)
#define DCM_UDS_SID_SECURITY_ACCESS                 (0x27U)
#define DCM_UDS_SID_COMMUNICATION_CONTROL           (0x28U)
#define DCM_UDS_SID_TESTER_PRESENT                  (0x3EU)
#define DCM_UDS_SID_ACCESS_TIMING_PARAMETER         (0x83U)
#define DCM_UDS_SID_SECURED_DATA_TRANSMISSION       (0x84U)
#define DCM_UDS_SID_CONTROL_DTC_SETTING             (0x85U)
#define DCM_UDS_SID_RESPONSE_ON_EVENT               (0x86U)
#define DCM_UDS_SID_LINK_CONTROL                    (0x87U)
#define DCM_UDS_SID_READ_DATA_BY_IDENTIFIER         (0x22U)
#define DCM_UDS_SID_READ_MEMORY_BY_ADDRESS          (0x23U)
#define DCM_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER (0x24U)
#define DCM_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER (0x2AU)
#define DCM_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER (0x2CU)
#define DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER        (0x2EU)
#define DCM_UDS_SID_WRITE_MEMORY_BY_ADDRESS         (0x3DU)
#define DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION    (0x14U)
#define DCM_UDS_SID_READ_DTC_INFORMATION            (0x19U)
#define DCM_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER (0x2FU)
#define DCM_UDS_SID_ROUTINE_CONTROL                 (0x31U)
#define DCM_UDS_SID_REQUEST_DOWNLOAD                (0x34U)
#define DCM_UDS_SID_REQUEST_UPLOAD                  (0x35U)
#define DCM_UDS_SID_TRANSFER_DATA                   (0x36U)
#define DCM_UDS_SID_REQUEST_TRANSFER_EXIT           (0x37U)
#define DCM_UDS_SID_REQUEST_FILE_TRANSFER           (0x38U)

/* Service ID aliases used in Dcm.c */
#define DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL      DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL
#define DCM_SERVICE_ECU_RESET                       DCM_UDS_SID_ECU_RESET
#define DCM_SERVICE_SECURITY_ACCESS                 DCM_UDS_SID_SECURITY_ACCESS
#define DCM_SERVICE_TESTER_PRESENT                  DCM_UDS_SID_TESTER_PRESENT
#define DCM_SERVICE_READ_DATA_BY_IDENTIFIER         DCM_UDS_SID_READ_DATA_BY_IDENTIFIER
#define DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER        DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER
#define DCM_SERVICE_READ_DTC_INFORMATION            DCM_UDS_SID_READ_DTC_INFORMATION
#define DCM_SERVICE_CLEAR_DIAGNOSTIC_INFORMATION    DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION
#define DCM_SERVICE_ROUTINE_CONTROL                 DCM_UDS_SID_ROUTINE_CONTROL

/* NRC aliases used in Dcm.c */
#define DCM_E_SUBFUNCTION_NOT_SUPPORTED             DCM_E_SUBFUNCTIONNOTSUPPORTED
#define DCM_E_INCORRECT_MESSAGE_LENGTH              DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT
#define DCM_E_SERVICE_NOT_SUPPORTED                 DCM_E_SERVICENOTSUPPORTED
#define DCM_E_REQUEST_OUT_OF_RANGE                  DCM_E_REQUESTOUTOFRANGE
#define DCM_E_SECURITY_ACCESS_DENIED                DCM_E_SECURITYACCESSDENIED
#define DCM_E_CONDITIONS_NOT_CORRECT                DCM_E_CONDITIONSNOTCORRECT
#define DCM_E_RESPONSE_TOO_LONG                     DCM_E_RESPONSETOOLONG
#define DCM_E_INVALID_KEY                           DCM_E_INVALIDKEY
#define DCM_E_REQUIRED_TIME_DELAY_NOT_EXPIRED       DCM_E_REQUIREDTIMEDELAYNOTEXPIRED
#define DCM_E_EXCEED_NUMBER_OF_ATTEMPTS             DCM_E_EXCEEDNUMBEROFATTEMPTS

/*==================================================================================================
*                                    DCM SESSION TYPE
==================================================================================================*/
typedef enum {
    DCM_DEFAULT_SESSION = 0x01,
    DCM_PROGRAMMING_SESSION = 0x02,
    DCM_EXTENDED_DIAGNOSTIC_SESSION = 0x03,
    DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION = 0x04
} Dcm_SesCtrlType;

/*==================================================================================================
*                                    DCM SECURITY LEVEL TYPE
==================================================================================================*/
typedef uint8 Dcm_SecLevelType;

/*==================================================================================================
*                                    DCM PROTOCOL TYPE
==================================================================================================*/
typedef enum {
    DCM_OBD_ON_CAN = 0,
    DCM_OBD_ON_FLEXRAY,
    DCM_OBD_ON_IP,
    DCM_UDS_ON_CAN,
    DCM_UDS_ON_FLEXRAY,
    DCM_UDS_ON_IP,
    DCM_NO_PROTOCOL
} Dcm_ProtocolType;

/*==================================================================================================
*                                    DCM NEGATIVE RESPONSE CODE
==================================================================================================*/
typedef enum {
    DCM_E_POSITIVERESPONSE = 0x00,
    DCM_E_GENERALREJECT = 0x10,
    DCM_E_SERVICENOTSUPPORTED = 0x11,
    DCM_E_SUBFUNCTIONNOTSUPPORTED = 0x12,
    DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT = 0x13,
    DCM_E_RESPONSETOOLONG = 0x14,
    DCM_E_BUSYREPEATREQUEST = 0x21,
    DCM_E_CONDITIONSNOTCORRECT = 0x22,
    DCM_E_REQUESTSEQUENCEERROR = 0x24,
    DCM_E_NORESPONSEFROMSUBNETCOMPONENT = 0x25,
    DCM_E_FAILUREPREVENTSEXECUTIONOFREQUESTEDACTION = 0x26,
    DCM_E_REQUESTOUTOFRANGE = 0x31,
    DCM_E_SECURITYACCESSDENIED = 0x33,
    DCM_E_INVALIDKEY = 0x35,
    DCM_E_EXCEEDNUMBEROFATTEMPTS = 0x36,
    DCM_E_REQUIREDTIMEDELAYNOTEXPIRED = 0x37,
    DCM_E_UPLOADDOWNLOADNOTACCEPTED = 0x70,
    DCM_E_TRANSFERDATASUSPENDED = 0x71,
    DCM_E_GENERALPROGRAMMINGFAILURE = 0x72,
    DCM_E_WRONGBLOCKSEQUENCECOUNTER = 0x73,
    DCM_E_REQUESTCORRECTLYRECEIVEDRESPONSEPENDING = 0x78,
    DCM_E_SUBFUNCTIONNOTSUPPORTEDINACTIVESESSION = 0x7E,
    DCM_E_SERVICENOTSUPPORTEDINACTIVESESSION = 0x7F,
    DCM_E_RPMTOOHIGH = 0x81,
    DCM_E_RPMTOOLOW = 0x82,
    DCM_E_ENGINEISRUNNING = 0x83,
    DCM_E_ENGINEISNOTRUNNING = 0x84,
    DCM_E_ENGINERUNTIMETOOLOW = 0x85,
    DCM_E_TEMPERATURETOOHIGH = 0x86,
    DCM_E_TEMPERATURETOOLOW = 0x87,
    DCM_E_VEHICLESPEEDTOOHIGH = 0x88,
    DCM_E_VEHICLESPEEDTOOLOW = 0x89,
    DCM_E_THROTTLE_PEDALTOOHIGH = 0x8A,
    DCM_E_THROTTLE_PEDALTOOLOW = 0x8B,
    DCM_E_TRANSMISSIONRANGENOTINNEUTRAL = 0x8C,
    DCM_E_TRANSMISSIONRANGENOTINGEAR = 0x8D,
    DCM_E_BRAKESWITCH_NOTCLOSED = 0x8F,
    DCM_E_SHIFTERLEVERNOTINPARK = 0x90,
    DCM_E_TORQUECONVERTERCLUTCHLOCKED = 0x91,
    DCM_E_VOLTAGETOOHIGH = 0x92,
    DCM_E_VOLTAGETOOLOW = 0x93
} Dcm_NegativeResponseCodeType;

/*==================================================================================================
*                                    DCM DID CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 DID;
    uint16 DataLength;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*ReadDataFnc)(uint8* Data);
    Std_ReturnType (*WriteDataFnc)(const uint8* Data, uint16 DataLength);
} Dcm_DIDConfigType;

/*==================================================================================================
*                                    DCM RID CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 RID;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*StartFnc)(const uint8* RequestData, uint16 RequestDataLength, uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*StopFnc)(const uint8* RequestData, uint16 RequestDataLength, uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*RequestResultFnc)(uint8* ResponseData, uint16* ResponseDataLength);
} Dcm_RIDConfigType;

/*==================================================================================================
*                                    DCM MSG CONTEXT TYPE
==================================================================================================*/
typedef struct {
    uint8* reqData;
    uint8* resData;
    uint16 reqDataLen;
    uint16 resDataLen;
    uint16 resMaxDataLen;
    PduIdType dcmRxPduId;
    Dcm_NegativeResponseCodeType nrc;
    uint8 msgAddInfo;
    uint8 msgContextState;
} Dcm_MsgContextType;

/*==================================================================================================
*                                    DCM CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 NumProtocols;
    uint8 NumConnections;
    uint8 NumRxPduIds;
    uint8 NumTxPduIds;
    uint8 NumSessions;
    uint8 NumSecurityLevels;
    uint8 NumServices;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean RespondAllRequest;
    boolean DcmTaskTime;
} Dcm_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DCM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Dcm_ConfigType Dcm_Config;

#define DCM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DCM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Diagnostic Communication Manager
 * @param ConfigPtr Pointer to configuration structure
 */
void Dcm_Init(const Dcm_ConfigType* ConfigPtr);

/**
 * @brief Starts the DCM
 */
void Dcm_Start(void);

/**
 * @brief Stops the DCM
 */
void Dcm_Stop(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Deinitializes the DCM module
 */
void Dcm_DeInit(void);

/**
 * @brief Gets current security level
 * @param SecLevel Pointer to store security level
 * @return Result of operation
 */
Std_ReturnType Dcm_GetSecurityLevel(uint8* SecLevel);

/**
 * @brief Gets current session control type
 * @param SesCtrlType Pointer to store session type
 * @return Result of operation
 */
Std_ReturnType Dcm_GetSesCtrlType(uint8* SesCtrlType);

/**
 * @brief Gets active diagnostic status
 * @return Active diagnostic flag
 */
boolean Dcm_GetActiveDiagnostic(void);

/**
 * @brief Sets active diagnostic status
 * @param active Active status
 */
void Dcm_SetActiveDiagnostic(boolean active);

/**
 * @brief Resets to default session
 */
void Dcm_ResetToDefaultSession(void);

/**
 * @brief Triggers on event
 * @param RoeEventId Event ID
 * @param OpStatus Operation status
 * @return Result of operation
 */
Std_ReturnType Dcm_TriggerOnEvent(uint8 RoeEventId, uint8 OpStatus);

/**
 * @brief Tx confirmation callback
 * @param TxPduId PDU ID
 * @param result Result of transmission
 */
void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Rx indication callback
 * @param RxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Trigger transmit callback
 * @param TxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/**
 * @brief Main function for periodic processing
 */
void Dcm_MainFunction(void);

#define DCM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DCM_H */
