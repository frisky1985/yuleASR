/**
 * @file CryIf.h
 * @brief Crypto Interface Module - AutoSAR Service Layer
 * @version 1.0.0
 * @date 2026-05-01
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details Provides an AUTOSAR CRYIF module offering services for
 *          cryptographic operations abstraction between CSM and Crypto Driver.
 *          Following AutoSAR Classic Platform 4.x standard.
 */

#ifndef CRYIF_H
#define CRYIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CryIf_Types.h"
#include "CryIf_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CRYIF_VENDOR_ID                         (0x0055U)  /* YuleTech */
#define CRYIF_MODULE_ID                         (0x007CU)  /* CRYIF Module ID */
#define CRYIF_INSTANCE_ID                       (0x00U)

#define CRYIF_SW_MAJOR_VERSION                  (0x01U)
#define CRYIF_SW_MINOR_VERSION                  (0x00U)
#define CRYIF_SW_PATCH_VERSION                  (0x00U)

#define CRYIF_AR_RELEASE_MAJOR_VERSION          (0x04U)
#define CRYIF_AR_RELEASE_MINOR_VERSION          (0x04U)
#define CRYIF_AR_RELEASE_REVISION_VERSION       (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CRYIF_SID_INIT                          (0x01U)
#define CRYIF_SID_DEINIT                        (0x02U)
#define CRYIF_SID_GETVERSIONINFO                (0x03U)
#define CRYIF_SID_PROCESSJOB                    (0x04U)
#define CRYIF_SID_CANCELJOB                     (0x05U)
#define CRYIF_SID_KEYELEMENTSET                 (0x06U)
#define CRYIF_SID_KEYSETVALID                   (0x07U)
#define CRYIF_SID_KEYELEMENTGET                 (0x08U)
#define CRYIF_SID_KEYELEMENTCOPY                (0x09U)
#define CRYIF_SID_KEYCOPY                       (0x0AU)
#define CRYIF_SID_KEYELEMENTIDSGET              (0x0BU)
#define CRYIF_SID_RANDOMSEED                    (0x0CU)
#define CRYIF_SID_KEYGENERATE                   (0x0DU)
#define CRYIF_SID_KEYDERIVE                     (0x0EU)
#define CRYIF_SID_KEYEXCHANGECALCPUBVALUE       (0x0FU)
#define CRYIF_SID_KEYEXCHANGECALCSECRET         (0x10U)
#define CRYIF_SID_CERTIFICATEPARSE              (0x11U)
#define CRYIF_SID_CERTIFICATEVERIFY             (0x12U)
#define CRYIF_SID_CALLBACKNOTIFICATION          (0x13U)
#define CRYIF_SID_MAINFUNCTION                  (0x14U)

/*==================================================================================================
*                                    ERROR CODES
==================================================================================================*/
#define CRYIF_E_PARAM_POINTER                   (0x01U)
#define CRYIF_E_UNINIT                          (0x02U)
#define CRYIF_E_INIT_FAILED                     (0x03U)
#define CRYIF_E_PARAM_HANDLE                    (0x04U)
#define CRYIF_E_PARAM_VALUE                     (0x05U)
#define CRYIF_E_BUSY                            (0x06U)

/*==================================================================================================
*                                    API FUNCTION MACROS
==================================================================================================*/

#if (CRYIF_VERSION_INFO_API == STD_ON)
/**
 * @brief Gets the version information of the CryIf module
 * @param[out] versioninfo Pointer to version information structure
 */
#define CryIf_GetVersionInfo(versioninfo) \
    CryIf_GetVersionInfoInternal(versioninfo)
#endif

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------
*                                    LIFECYCLE FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00001 */
/**
 * @brief Initializes the Crypto Interface module
 * @param[in] configPtr Pointer to configuration structure
 * @return void
 * @serviceid CRYIF_SID_INIT
 */
extern void CryIf_Init(const CryIf_ConfigType* configPtr);

/** @req SWS_CryIf_00002 */
/**
 * @brief Deinitializes the Crypto Interface module
 * @param void
 * @return void
 * @serviceid CRYIF_SID_DEINIT
 */
extern void CryIf_DeInit(void);

/**
 * @brief Returns the version information of the CRYIF module
 * @param[out] versioninfo Pointer to version information structure
 * @return void
 * @serviceid CRYIF_SID_GETVERSIONINFO
 */
#if (CRYIF_VERSION_INFO_API == STD_ON)
/** @req SWS_CryIf_00003 */
extern void CryIf_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/*--------------------------------------------------------------------------------------------------
*                                    JOB MANAGEMENT FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00005 */
/**
 * @brief Processes a crypto job
 * @param[in] channelId Channel identifier
 * @param[in] job Pointer to job structure
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 *         CRYIF_E_BUSY: Channel busy
 * @serviceid CRYIF_SID_PROCESSJOB
 */
extern Std_ReturnType CryIf_ProcessJob(
    CryIf_ChannelIdType channelId,
    CryIf_JobType* job
);

/** @req SWS_CryIf_00006 */
/**
 * @brief Cancels a pending crypto job
 * @param[in] channelId Channel identifier
 * @param[in] job Pointer to job structure
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_CANCELJOB
 */
extern Std_ReturnType CryIf_CancelJob(
    CryIf_ChannelIdType channelId,
    CryIf_JobType* job
);

/*--------------------------------------------------------------------------------------------------
*                                    KEY MANAGEMENT FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00007 */
/**
 * @brief Sets a key element value
 * @param[in] cryIfKeyId CryIf key identifier
 * @param[in] keyElementId Key element identifier
 * @param[in] keyPtr Pointer to key data
 * @param[in] keyLength Length of key data
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYELEMENTSET
 */
extern Std_ReturnType CryIf_KeyElementSet(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);

/** @req SWS_CryIf_00008 */
/**
 * @brief Validates a key
 * @param[in] cryIfKeyId CryIf key identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYSETVALID
 */
extern Std_ReturnType CryIf_KeySetValid(CryIf_KeyIdType cryIfKeyId);

/** @req SWS_CryIf_00009 */
/**
 * @brief Gets a key element value
 * @param[in] cryIfKeyId CryIf key identifier
 * @param[in] keyElementId Key element identifier
 * @param[out] keyPtr Pointer to buffer for key data
 * @param[in,out] keyLengthPtr Pointer to buffer length / returns actual length
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYELEMENTGET
 */
extern Std_ReturnType CryIf_KeyElementGet(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr
);

/**
 * @brief Copies a key element from one key to another
 * @param[in] cryIfKeyId Source CryIf key identifier
 * @param[in] keyElementId Source key element identifier
 * @param[in] targetCryIfKeyId Target CryIf key identifier
 * @param[in] targetKeyElementId Target key element identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYELEMENTCOPY
 */
#if (CRYIF_KEY_ELEMENT_COPY_API == STD_ON)
/** @req SWS_CryIf_00010 */
extern Std_ReturnType CryIf_KeyElementCopy(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    CryIf_KeyIdType targetCryIfKeyId,
    CryIf_KeyElementIdType targetKeyElementId
);

/** @req SWS_CryIf_00011 */
/**
 * @brief Copies a key element with partial access
 * @param[in] cryIfKeyId Source CryIf key identifier
 * @param[in] keyElementId Source key element identifier
 * @param[in] keyElementSourceOffset Offset in source key element
 * @param[in] keyElementTargetOffset Offset in target key element
 * @param[in] keyElementCopyLength Length to copy
 * @param[in] targetCryIfKeyId Target CryIf key identifier
 * @param[in] targetKeyElementId Target key element identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 */
extern Std_ReturnType CryIf_KeyElementCopyPartial(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType keyElementId,
    uint32 keyElementSourceOffset,
    uint32 keyElementTargetOffset,
    uint32 keyElementCopyLength,
    CryIf_KeyIdType targetCryIfKeyId,
    CryIf_KeyElementIdType targetKeyElementId
);
#endif

/** @req SWS_CryIf_00012 */
/**
 * @brief Copies a key including all key elements
 * @param[in] cryIfKeyId Source CryIf key identifier
 * @param[in] targetCryIfKeyId Target CryIf key identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYCOPY
 */
extern Std_ReturnType CryIf_KeyCopy(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType targetCryIfKeyId
);

/** @req SWS_CryIf_00013 */
/**
 * @brief Gets the IDs of all key elements in a key
 * @param[in] cryIfKeyId CryIf key identifier
 * @param[out] keyElementIdsPtr Pointer to buffer for key element IDs
 * @param[out] keyElementIdsLengthPtr Pointer to buffer size / returns actual count
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYELEMENTIDSGET
 */
extern Std_ReturnType CryIf_KeyElementIdsGet(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyElementIdType* keyElementIdsPtr,
    uint32* keyElementIdsLengthPtr
);

#if (CRYIF_KEY_VALID_CHECK_API == STD_ON)
/** @req SWS_CryIf_00014 */
/**
 * @brief Checks if a key is valid
 * @param[in] cryIfKeyId CryIf key identifier
 * @return E_OK: Key is valid
 *         E_NOT_OK: Key is not valid
 */
extern Std_ReturnType CryIf_KeyValidCheck(CryIf_KeyIdType cryIfKeyId);
#endif

/*--------------------------------------------------------------------------------------------------
*                                    CRYPTOGRAPHIC FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00015 */
/**
 * @brief Seeds the random number generator
 * @param[in] cryIfKeyId CryIf key identifier for seed
 * @param[in] seedPtr Pointer to seed data
 * @param[in] seedLength Length of seed data
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_RANDOMSEED
 */
extern Std_ReturnType CryIf_RandomSeed(
    CryIf_KeyIdType cryIfKeyId,
    const uint8* seedPtr,
    uint32 seedLength
);

/** @req SWS_CryIf_00016 */
/**
 * @brief Generates a new key
 * @param[in] cryIfKeyId CryIf key identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYGENERATE
 */
extern Std_ReturnType CryIf_KeyGenerate(CryIf_KeyIdType cryIfKeyId);

/** @req SWS_CryIf_00017 */
/**
 * @brief Derives a key from another key
 * @param[in] cryIfKeyId Source CryIf key identifier
 * @param[in] targetCryIfKeyId Target CryIf key identifier
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYDERIVE
 */
extern Std_ReturnType CryIf_KeyDerive(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType targetCryIfKeyId
);

/** @req SWS_CryIf_00018 */
/**
 * @brief Calculates the public value for key exchange
 * @param[in] cryIfKeyId CryIf key identifier
 * @param[out] publicValuePtr Pointer to buffer for public value
 * @param[in,out] publicValueLengthPtr Pointer to buffer size / returns actual length
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYEXCHANGECALCPUBVALUE
 */
extern Std_ReturnType CryIf_KeyExchangeCalcPubValue(
    CryIf_KeyIdType cryIfKeyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr
);

/** @req SWS_CryIf_00019 */
/**
 * @brief Calculates the shared secret for key exchange
 * @param[in] cryIfKeyId CryIf key identifier
 * @param[in] partnerPublicValuePtr Pointer to partner's public value
 * @param[in] partnerPublicValueLength Length of partner's public value
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_KEYEXCHANGECALCSECRET
 */
extern Std_ReturnType CryIf_KeyExchangeCalcSecret(
    CryIf_KeyIdType cryIfKeyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength
);

/*--------------------------------------------------------------------------------------------------
*                                    CERTIFICATE FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00020 */
/**
 * @brief Parses a certificate
 * @param[in] cryIfKeyId CryIf key identifier for certificate
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_CERTIFICATEPARSE
 */
extern Std_ReturnType CryIf_CertificateParse(CryIf_KeyIdType cryIfKeyId);

/** @req SWS_CryIf_00021 */
/**
 * @brief Verifies a certificate
 * @param[in] cryIfKeyId CryIf key identifier for certificate to verify
 * @param[in] verifyCryIfKeyId CryIf key identifier for verification key
 * @return E_OK: Request successful
 *         E_NOT_OK: Request failed
 * @serviceid CRYIF_SID_CERTIFICATEVERIFY
 */
extern Std_ReturnType CryIf_CertificateVerify(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType verifyCryIfKeyId
);

/*--------------------------------------------------------------------------------------------------
*                                    CALLBACK FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00022 */
/**
 * @brief Callback notification from crypto driver
 * @param[in] channelId Channel identifier
 * @param[in] job Pointer to job structure
 * @param[in] result Result of the operation
 * @return void
 * @serviceid CRYIF_SID_CALLBACKNOTIFICATION
 */
extern void CryIf_CallbackNotification(
    CryIf_ChannelIdType channelId,
    CryIf_JobType* job,
    CryIf_ResultType result
);

/*--------------------------------------------------------------------------------------------------
*                                    SCHEDULING FUNCTIONS
--------------------------------------------------------------------------------------------------*/

/** @req SWS_CryIf_00004 */
/**
 * @brief Main function for processing async operations
 * @param void
 * @return void
 * @serviceid CRYIF_SID_MAINFUNCTION
 */
extern void CryIf_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* CRYIF_H */
