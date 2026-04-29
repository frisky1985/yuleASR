/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: SecOC.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_SecureOnboardCommunication.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Secure Onboard Communication module
 *==================================================================================================
 */

#ifndef SECOC_H
#define SECOC_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "SecOC_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define SECOC_VENDOR_ID                   (100u)
#define SECOC_MODULE_ID                   (150u)
#define SECOC_INSTANCE_ID                 (0u)

#define SECOC_AR_RELEASE_MAJOR_VERSION    (4u)
#define SECOC_AR_RELEASE_MINOR_VERSION    (7u)
#define SECOC_AR_RELEASE_REVISION_VERSION (0u)

#define SECOC_SW_MAJOR_VERSION            (1u)
#define SECOC_SW_MINOR_VERSION            (0u)
#define SECOC_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((SECOC_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (SECOC_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of SecOC.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define SECOC_SID_INIT                          (0x01u)
#define SECOC_SID_DEINIT                        (0x02u)
#define SECOC_SID_GETVERSIONINFO                (0x03u)
#define SECOC_SID_TRANSMIT                      (0x41u)
#define SECOC_SID_IFTRANSMIT                    (0x42u)
#define SECOC_SID_IFRXINDICATION                (0x43u)
#define SECOC_SID_TPTRANSMIT                    (0x44u)
#define SECOC_SID_TPRXINDICATION                (0x45u)
#define SECOC_SID_COPYTXDATA                    (0x46u)
#define SECOC_SID_COPYRXDATA                    (0x47u)
#define SECOC_SID_STARTOFRECEPTION              (0x48u)
#define SECOC_SID_VERIFYSTATUSOVERRIDE          (0x81u)
#define SECOC_SID_RXINDICATION                  (0x82u)
#define SECOC_SID_TXCONFIRMATION                (0x83u)
#define SECOC_SID_MAINFUNCTIONRX                (0x91u)
#define SECOC_SID_MAINFUNCTIONTX                (0x92u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define SECOC_E_PARAM_POINTER                   (0x01u)  /* API called with NULL pointer */
#define SECOC_E_INVALID_PDU_SDU_ID              (0x02u)  /* Invalid PDU/SDU ID */
#define SECOC_E_INVALID_PARAMETER               (0x03u)  /* Invalid parameter */
#define SECOC_E_UNINIT                          (0x04u)  /* API called before initialization */
#define SECOC_E_ALREADY_INITIALIZED             (0x05u)  /* Multiple initialization call */
#define SECOC_E_CRYPTO_FAILURE                  (0x06u)  /* Crypto operation failed */

/* Runtime error codes */
#define SECOC_E_CRYPTO_AUTH_FAILED              (0x01u)  /* Authentication verification failed */
#define SECOC_E_FRESHNESS_FAILURE               (0x02u)  /* Freshness value verification failed */
#define SECOC_E_SEC_PAYLOAD_ERROR               (0x03u)  /* Secured payload error */
#define SECOC_E_BUSY                            (0x04u)  /* Module busy */

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief SecOC verification result type
 */
typedef enum {
    SECOC_VERIFICATIONSUCCESS = 0,          /* Verification successful */
    SECOC_VERIFICATIONFAILURE,              /* Verification failed */
    SECOC_FRESHNESSFAILURE,                 /* Freshness verification failed */
    SECOC_AUTHENTICATIONBUILDFAILURE,       /* Authentication build failed */
    SECOC_NO_VERIFICATION                   /* No verification performed */
} SecOC_VerificationResultType;

/**
 * @brief SecOC verification status type
 */
typedef enum {
    SECOC_UNVERIFIED = 0,                   /* Not verified yet */
    SECOC_VERIFICATIONSUCCESS_STATUS,       /* Verification succeeded */
    SECOC_VERIFICATIONFAILURE_STATUS,       /* Verification failed */
    SECOC_VERIFICATIONOVERRIDE              /* Verification overridden */
} SecOC_VerificationStatusType;

/**
 * @brief SecOC authentication algorithm type
 */
typedef enum {
    SECOC_AES_MAC = 0,                      /* AES-CMAC */
    SECOC_HMAC_SHA256,                      /* HMAC-SHA256 */
    SECOC_HMAC_SHA512                       /* HMAC-SHA512 */
} SecOC_AuthAlgorithmType;

/**
 * @brief SecOC freshness value type
 */
typedef enum {
    SECOC_COUNTER = 0,                      /* Counter-based freshness */
    SECOC_TIMESTAMP                         /* Timestamp-based freshness */
} SecOC_FreshnessValueType;

/**
 * @brief SecOC PDU type
 */
typedef enum {
    SECOC_IFPDU = 0,                        /* Interface PDU */
    SECOC_TPPDU                             /* Transport Protocol PDU */
} SecOC_PduType;

/**
 * @brief SecOC freshness value ID type
 */
typedef uint16 SecOC_FreshnessValueIdType;

/**
 * @brief SecOC authentication build configuration
 */
typedef struct {
    SecOC_AuthAlgorithmType algorithm;
    uint8 authInfoLength;                   /* Length of authentication info in bytes */
    uint8 dataId;                           /* Data identifier */
} SecOC_AuthBuildConfigType;

/**
 * @brief SecOC freshness value configuration
 */
typedef struct {
    SecOC_FreshnessValueType type;
    SecOC_FreshnessValueIdType freshnessValueId;
    uint8 freshnessValueLength;             /* Total freshness length in bits */
    uint8 freshnessValueTxLength;           /* Freshness length transmitted in bits */
} SecOC_FreshnessValueConfigType;

/**
 * @brief SecOC PDU configuration
 */
typedef struct {
    PduIdType pduId;                        /* PDU identifier */
    PduIdType lowerLayerPduId;              /* Lower layer PDU ID */
    SecOC_PduType pduType;                  /* IF or TP PDU */
    SecOC_AuthBuildConfigType authConfig;
    SecOC_FreshnessValueConfigType freshnessConfig;
    boolean useCryptographicPdu;            /* Use separate crypto PDU */
    uint8 dataToAuthOffset;                 /* Offset for data to authenticate */
    uint8 dataToAuthLength;                 /* Length of data to authenticate */
    uint16 authPduLength;                   /* Length of authenticated PDU */
} SecOC_PduConfigType;

/**
 * @brief SecOC configuration
 */
typedef struct {
    const SecOC_PduConfigType* txPduConfigs;
    uint16 numTxPdus;
    const SecOC_PduConfigType* rxPduConfigs;
    uint16 numRxPdus;
    uint16 mainFunctionPeriodRx;
    uint16 mainFunctionPeriodTx;
    boolean devErrorDetect;
    boolean versionInfoApi;
    boolean overrideStatusAllowed;
} SecOC_ConfigType;

/**
 * @brief SecOC authentication PDU info
 */
typedef struct {
    uint8* authPduDataPtr;
    PduLengthType authPduLength;
    uint8* cryptoPduDataPtr;
    PduLengthType cryptoPduLength;
} SecOC_AuthPduInfoType;

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define SECOC_MAX_AUTH_INFO_LEN             (32u)   /* Maximum authentication info length */
#define SECOC_MAX_FRESHNESS_LEN             (8u)    /* Maximum freshness value length in bytes */

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

extern boolean SecOC_Initialized;
extern const SecOC_ConfigType* SecOC_ConfigPtr;

#define SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

/*==================================================================================================
 *                                     API DECLARATIONS
 *==================================================================================================*/
#define SECOC_START_SEC_CODE
#include "SecOC_MemMap.h"

/**
 * @brief Initializes the SecOC module
 * @param configPtr Pointer to configuration structure
 * @return None
 * @req SWS_SecOC_00001
 */
extern void SecOC_Init(const SecOC_ConfigType* configPtr);

/**
 * @brief Deinitializes the SecOC module
 * @return None
 * @req SWS_SecOC_00002
 */
extern void SecOC_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_SecOC_00003
 */
#if (SECOC_VERSION_INFO_API == STD_ON)
extern void SecOC_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Transmits a secured PDU
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 * @req SWS_SecOC_00041
 */
extern Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Receive indication callback for secured PDUs
 * @param RxPduId PDU that was received
 * @param PduInfoPtr Pointer to PDU info
 * @return None
 * @req SWS_SecOC_00043
 */
extern void SecOC_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Overrides verification status for a PDU
 * @param PduId PDU ID
 * @param status New verification status
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_SecOC_00081
 */
extern Std_ReturnType SecOC_VerifyStatusOverride(PduIdType PduId, 
                                                  SecOC_VerificationStatusType status);

/**
 * @brief Gets current verification status for a PDU
 * @param PduId PDU ID
 * @return Current verification status
 * @req SWS_SecOC_00084
 */
extern SecOC_VerificationStatusType SecOC_GetVerificationStatus(PduIdType PduId);

/**
 * @brief Gets verification result for a PDU
 * @param PduId PDU ID
 * @param resultPtr Pointer to store verification result
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_SecOC_00085
 */
extern Std_ReturnType SecOC_GetVerificationResult(PduIdType PduId, 
                                                   SecOC_VerificationResultType* resultPtr);

/**
 * @brief Copy transmit data callback for TP PDUs
 * @param TxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @param RetryInfoPtr Pointer to retry info
 * @param AvailableDataPtr Pointer to available data
 * @return BufReq_ReturnType
 * @req SWS_SecOC_00046
 */
extern BufReq_ReturnType SecOC_CopyTxData(PduIdType TxPduId, 
                                           PduInfoType* PduInfoPtr,
                                           RetryInfoType* RetryInfoPtr,
                                           PduLengthType* AvailableDataPtr);

/**
 * @brief Copy receive data callback for TP PDUs
 * @param RxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @param RxBufferSizePtr Pointer to receive buffer size
 * @return BufReq_ReturnType
 * @req SWS_SecOC_00047
 */
extern BufReq_ReturnType SecOC_CopyRxData(PduIdType RxPduId,
                                           const PduInfoType* PduInfoPtr,
                                           PduLengthType* RxBufferSizePtr);

/**
 * @brief Start of reception callback for TP PDUs
 * @param RxPduId PDU ID
 * @param TpSduLength TP SDU length
 * @param RxBufferSizePtr Pointer to receive buffer size
 * @return BufReq_ReturnType
 * @req SWS_SecOC_00048
 */
extern BufReq_ReturnType SecOC_StartOfReception(PduIdType RxPduId,
                                                 PduLengthType TpSduLength,
                                                 PduLengthType* RxBufferSizePtr);

/**
 * @brief Tx confirmation callback
 * @param TxPduId PDU that was transmitted
 * @param result Transmission result
 * @return None
 * @req SWS_SecOC_00083
 */
extern void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Main function for RX processing
 * @return None
 * @req SWS_SecOC_00091
 */
extern void SecOC_MainFunctionRx(void);

/**
 * @brief Main function for TX processing
 * @return None
 * @req SWS_SecOC_00092
 */
extern void SecOC_MainFunctionTx(void);

#define SECOC_STOP_SEC_CODE
#include "SecOC_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* SECOC_H */
