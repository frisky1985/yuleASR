/*==================================================================================================
 * SecOc_Lcfg.c - SecOC link-time configuration
 *
 * Provides the SecOC configuration structures per SecOc.h (current API).
 * Profiles and PDUs are declared in the module's configuration header
 * (SecOc_Cfg.h / SecOC_Cfg.h).
 *================================================================================================*/

#include "SecOc.h"
#include "SecOc_Cfg.h"

/*==================================================================================================
 *                                    SECURITY PROFILES
 *==================================================================================================*/
/* Authenticator lengths: 4/8 bytes with 32-bit freshness, per SecOC spec */
static const SecOC_AuthBuildConfigType SecOc_Profiles[] = {
    {
        .algorithm      = SECOC_AES_MAC,
        .authInfoLength = SECOC_AUTH_LENGTH_4,
        .dataId         = 0x01U
    },
    {
        .algorithm      = SECOC_AES_MAC,
        .authInfoLength = SECOC_AUTH_LENGTH_8,
        .dataId         = 0x02U
    }
};

/*==================================================================================================
 *                                    PDU CONFIGURATIONS
 *==================================================================================================*/
static const SecOC_PduConfigType SecOc_TxPdus[SECOC_NUM_TX_PDUS] = {
    {
        .pduId              = 0U,
        .lowerLayerPduId    = 0U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[0],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 0U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_4 + 1U
    },
    {
        .pduId              = 1U,
        .lowerLayerPduId    = 1U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[1],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 1U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_8 + 1U
    },
    {
        .pduId              = 2U,
        .lowerLayerPduId    = 2U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[0],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 2U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_4 + 1U
    },
    {
        .pduId              = 3U,
        .lowerLayerPduId    = 3U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[1],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 3U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_8 + 1U
    }
};

static const SecOC_PduConfigType SecOc_RxPdus[SECOC_NUM_RX_PDUS] = {
    {
        .pduId              = 0U,
        .lowerLayerPduId    = 0U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[0],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 0U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_4 + 1U
    },
    {
        .pduId              = 1U,
        .lowerLayerPduId    = 1U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[1],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 1U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_8 + 1U
    },
    {
        .pduId              = 2U,
        .lowerLayerPduId    = 2U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[0],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 2U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_4 + 1U
    },
    {
        .pduId              = 3U,
        .lowerLayerPduId    = 3U,
        .pduType            = SECOC_IFPDU,
        .authConfig         = SecOc_Profiles[1],
        .freshnessConfig    = { .type = SECOC_COUNTER, .freshnessValueId = 3U,
                                .freshnessValueLength = 32U,
                                .freshnessValueTxLength = SECOC_FRESHNESS_LENGTH_4 },
        .useCryptographicPdu = FALSE,
        .dataToAuthOffset   = 0U,
        .dataToAuthLength   = 8U,
        .authPduLength      = 8U + SECOC_AUTH_LENGTH_8 + 1U
    }
};

/*==================================================================================================
 *                                    CONFIGURATION
 *==================================================================================================*/
const SecOC_ConfigType SecOc_Config = {
    .txPduConfigs           = SecOc_TxPdus,
    .numTxPdus              = SECOC_NUM_TX_PDUS,
    .rxPduConfigs           = SecOc_RxPdus,
    .numRxPdus              = SECOC_NUM_RX_PDUS,
    .mainFunctionPeriodRx   = 10U,
    .mainFunctionPeriodTx   = 10U,
    .devErrorDetect         = TRUE,
    .versionInfoApi         = TRUE,
    .overrideStatusAllowed  = FALSE
};
