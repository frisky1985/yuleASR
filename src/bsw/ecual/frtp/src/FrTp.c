/**
 * @file FrTp.c
 * @brief FlexRay Transport Protocol module - Main implementation
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: FlexRay Transport Protocol (FRTP)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Private.h"
#include "Det.h"
#include "PduR.h"

/* Version check */
#if (FRTP_AR_RELEASE_MAJOR_VERSION != 4U)
    #error "FrTp: AR major version mismatch"
#endif

#if (FRTP_AR_RELEASE_MINOR_VERSION != 4U)
    #error "FrTp: AR minor version mismatch"
#endif

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/
#define FRTP_UNINIT                     (0U)
#define FRTP_INIT                       (1U)

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define FRTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint8 FrTp_InitState = FRTP_UNINIT;

#define FRTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    EXTERNAL VARIABLES
==================================================================================================*/
#define FRTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

FrTp_RuntimeType FrTp_Runtime;

#define FRTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static Std_ReturnType FrTp_ValidateConfig(const FrTp_ConfigType* config);
static void FrTp_InitConnections(void);

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the FrTp module
 * @param CfgPtr Pointer to configuration structure
 */
void FrTp_Init(const FrTp_ConfigType* CfgPtr)
{
    uint8 connIdx;

    /* Check for NULL pointer if development error detection is enabled */
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (CfgPtr == NULL_PTR)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_INIT, FRTP_E_PARAM_POINTER);
        return;
    }

    if (FrTp_InitState == FRTP_INIT)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_INIT, FRTP_E_ALREADY_INITIALIZED);
        return;
    }
#endif

    /* Validate configuration */
    if (FrTp_ValidateConfig(CfgPtr) != E_OK)
    {
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
        FRTP_DET_REPORT_ERROR(FRTP_SID_INIT, FRTP_E_PARAM_CONFIG);
#endif
        return;
    }

    /* Initialize runtime data */
    FrTp_Runtime.initialized = FALSE;
    FrTp_Runtime.activeConnections = 0U;

    /* Initialize all connections */
    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        FrTp_ResetConnection(connIdx);
    }

    /* Mark module as initialized */
    FrTp_Runtime.initialized = TRUE;
    FrTp_InitState = FRTP_INIT;
}

/**
 * @brief Deinitializes the FrTp module
 */
void FrTp_DeInit(void)
{
    uint8 connIdx;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_InitState != FRTP_INIT)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_INIT, FRTP_E_UNINIT);
        return;
    }
#endif

    /* Reset all connections */
    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        FrTp_ResetConnection(connIdx);
    }

    /* Mark module as uninitialized */
    FrTp_Runtime.initialized = FALSE;
    FrTp_Runtime.activeConnections = 0U;
    FrTp_InitState = FRTP_UNINIT;
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void FrTp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_GETVERSIONINFO, FRTP_E_PARAM_POINTER);
        return;
    }
#endif

#if (FRTP_VERSION_INFO_API == STD_ON)
    versioninfo->vendorID = FRTP_VENDOR_ID;
    versioninfo->moduleID = FRTP_MODULE_ID;
    versioninfo->sw_major_version = FRTP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FRTP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FRTP_SW_PATCH_VERSION;
#endif
}

/**
 * @brief Changes a TP parameter
 * @param id PDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return E_OK if changed, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
    FrTp_ConnectionIdxType connIdx;
    FrTp_ConnectionRuntimeType* runtime;
    Std_ReturnType result = E_NOT_OK;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_InitState != FRTP_INIT)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_CHANGEPARAMETER, FRTP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Find connection by Tx PDU ID */
    connIdx = FrTp_FindConnectionByTxPdu(id);
    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
        connIdx = FrTp_FindConnectionByRxPdu(id);
    }

    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        runtime = FrTp_GetConnectionRuntime(connIdx);

        if (runtime != NULL_PTR)
        {
            switch (parameter)
            {
                case TP_STMIN:
                    /* STmin value is 8-bit */
                    if (value <= 0xFFU)
                    {
                        runtime->stMin = (uint8)value;
                        result = E_OK;
                    }
                    break;

                case TP_BS:
                    /* Block size is 8-bit */
                    if (value <= 0xFFU)
                    {
                        runtime->blockSize = (uint8)value;
                        result = E_OK;
                    }
                    break;

                case TP_BC:
                    /* Not supported for FrTp */
                default:
                    break;
            }
        }
    }

    return result;
}

/**
 * @brief Main function for periodic processing
 */
void FrTp_MainFunction(void)
{
    uint8 connIdx;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_InitState != FRTP_INIT)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_MAINFUNCTION, FRTP_E_UNINIT);
        return;
    }
#endif

    /* Update all timers */
    FrTp_UpdateTimers();

    /* Process each connection */
    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        FrTp_ConnectionRuntimeType* runtime = FrTp_GetConnectionRuntime(connIdx);

        if (runtime == NULL_PTR)
        {
            continue;
        }

        /* Check for timeouts */
        if (FrTp_IsTimerExpired(runtime))
        {
            /* Handle timeout based on state */
            switch (runtime->state)
            {
                case FRTP_STATE_TX_WAIT_FC:
                    /* N_Bs timeout - retry or abort */
                    runtime->retryCount++;
                    if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
                    {
                        /* Abort transmission */
                        PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
                        FrTp_ResetConnection(connIdx);
                    }
                    else
                    {
                        /* Retry sending FF */
                        runtime->state = FRTP_STATE_TX_STARTING;
                    }
                    break;

                case FRTP_STATE_TX_WAIT_CONFIRM:
                    /* N_As timeout - retry or abort */
                    runtime->retryCount++;
                    if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
                    {
                        PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
                        FrTp_ResetConnection(connIdx);
                    }
                    break;

                case FRTP_STATE_RX_WAIT_CF:
                    /* N_Cr timeout */
                    FrTp_ResetConnection(connIdx);
                    break;

                default:
                    /* Other states - just reset */
                    FrTp_ResetConnection(connIdx);
                    break;
            }
        }

        /* Process active connections */
        if (runtime->state != FRTP_STATE_IDLE)
        {
            /* Call TX state machine for TX states */
            if (FrTp_IsTxState(runtime->state))
            {
                extern void FrTp_TxStateMachine(FrTp_ConnectionIdxType connIdx);
                FrTp_TxStateMachine(connIdx);
            }
            /* Call RX state machine for RX states */
            else if (FrTp_IsRxState(runtime->state))
            {
                extern void FrTp_RxStateMachine(FrTp_ConnectionIdxType connIdx);
                FrTp_RxStateMachine(connIdx);
            }
            else
            {
                /* Unknown state - reset connection */
                FrTp_ResetConnection(connIdx);
            }
        }
    }
}

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Validates the configuration structure
 * @param config Pointer to configuration structure
 * @return E_OK if valid, E_NOT_OK otherwise
 */
static Std_ReturnType FrTp_ValidateConfig(const FrTp_ConfigType* config)
{
    Std_ReturnType result = E_OK;

    if (config == NULL_PTR)
    {
        result = E_NOT_OK;
    }
    else if (config->numConnections > FRTP_MAX_CONNECTIONS)
    {
        result = E_NOT_OK;
    }
    else if (config->connections == NULL_PTR)
    {
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief Initializes all connections
 */
static void FrTp_InitConnections(void)
{
    uint8 connIdx;

    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        FrTp_ResetConnection(connIdx);
    }
}

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"
