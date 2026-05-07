/*******************************************************************************
* File: CanSm_Cfg.h
* Description: Configuration header for CAN State Manager (CanSm)
*              AUTOSAR SWS CANStateManager 4.4.0 compliant
*******************************************************************************/

#ifndef CANSM_CFG_H
#define CANSM_CFG_H

/*******************************************************************************
* Version Information
*******************************************************************************/
#define CANSM_CFG_MAJOR_VERSION     4U
#define CANSM_CFG_MINOR_VERSION     4U
#define CANSM_CFG_PATCH_VERSION     0U

/*******************************************************************************
* Pre-compile Configuration Parameters
*******************************************************************************/

/* Enables/Disables the version info API */
#define CANSM_VERSION_INFO_API      STD_ON

/* Enables/Disables Development Error Detection */
#define CANSM_DEV_ERROR_DETECT      STD_ON

/* Enables/Disables Transceiver Switch Off support */
#define CANSM_TRANSCEIVER_SWITCH_OFF    STD_ON

/* Enables/Disables Wake-up support */
#define CANSM_WAKEUP_SUPPORT        STD_ON

/* Enables/Disables Bus-Off Recovery support */
#define CANSM_BUSOFF_RECOVERY       STD_ON

/* Enables/Disables Tx Timeout Exception handling */
#define CANSM_TX_TIMEOUT_EXCEPTION  STD_ON

/*******************************************************************************
* Bus-Off Recovery Timing Parameters (in seconds)
*******************************************************************************/

/* Time to wait before requesting STARTED mode after bus-off detection */
/* Range: 0.001s to 65.535s (AUTOSAR specifies typically 0.1s) */
#define CANSM_T_RESTART_DEFAULT     0.1f

/* Time to wait before allowing another bus-off recovery attempt */
/* Range: 0.001s to 65.535s (AUTOSAR specifies typically 1.0s) */
#define CANSM_T_RECOVERY_DEFAULT    1.0f

/* Maximum number of bus-off recovery attempts before declaring failure */
#define CANSM_BUSOFF_MAX_RETRIES    10U

/*******************************************************************************
* Main Function Period (in seconds)
*******************************************************************************/
#define CANSM_MAIN_FUNCTION_PERIOD  0.01f  /* 10ms typical */

/*******************************************************************************
* Network Configuration Counts
*******************************************************************************/
/* Number of configured CAN networks - to be defined by configuration tool */
#ifndef CANSM_NETWORK_COUNT
#define CANSM_NETWORK_COUNT         2U
#endif

/* Maximum number of CAN controllers per network */
#ifndef CANSM_MAX_CONTROLLERS_PER_NETWORK
#define CANSM_MAX_CONTROLLERS_PER_NETWORK   1U
#endif

/* Maximum number of transceivers per network */
#ifndef CANSM_MAX_TRANSCEIVERS_PER_NETWORK
#define CANSM_MAX_TRANSCEIVERS_PER_NETWORK  1U
#endif

/*******************************************************************************
* Development Error Codes (AUTOSAR Specified)
*******************************************************************************/
#define CANSM_E_PARAM_POINTER       0x01U   /* API called with NULL pointer */
#define CANSM_E_PARAM_INVALID_NETWORK   0x02U   /* Invalid network handle */
#define CANSM_E_PARAM_INVALID_CONTROLLER    0x03U   /* Invalid controller */
#define CANSM_E_PARAM_INVALID_TRCV  0x04U   /* Invalid transceiver */
#define CANSM_E_PARAM_INVALID_MODE  0x05U   /* Invalid communication mode */
#define CANSM_E_UNINIT              0x06U   /* API called before Init */
#define CANSM_E_NOT_IN_NO_COM       0x07U   /* RequestComMode called, not in NO_COM */
#define CANSM_E_INVALID_COMM_REQUEST    0x08U   /* Invalid communication request */
#define CANSM_E_MODE_REQUEST_TIMEOUT    0x09U   /* Mode request timeout */
#define CANSM_E_INVALID_BUSOFF      0x0AU   /* Invalid bus-off notification */

/*******************************************************************************
* Service IDs for Error Reporting
*******************************************************************************/
#define CANSM_SID_INIT                          0x00U
#define CANSM_SID_DEINIT                        0x01U
#define CANSM_SID_REQUESTCOMMODE                0x02U
#define CANSM_SID_GETCURRENTCOMMODE             0x03U
#define CANSM_SID_CONTROLLERBUSOFF              0x04U
#define CANSM_SID_MAINFUNCTION                  0x05U
#define CANSM_SID_GETVERSIONINFO                0x06U
#define CANSM_SID_CONTROLLERMODEINDICATION      0x07U
#define CANSM_SID_TRANSMITTIMEOUTEXCEPTION      0x08U
#define CANSM_SID_TXTIMEOUTEXCEPTION            0x08U
#define CANSM_SID_TRCVMODEINDICATION            0x09U
#define CANSM_SID_STARTWAKEUPSOURCE             0x0AU
#define CANSM_SID_STOPWAKEUPSOURCE              0x0BU
#define CANSM_SID_CHECKWAKEUP                     0x0CU
#define CANSM_SID_CONTROLLERWAKEUP                0x0DU

/*******************************************************************************
* Module ID for CanSm
*******************************************************************************/
#define CANSM_MODULE_ID             0x0EU

/*******************************************************************************
* Instance ID (for single instance modules)
*******************************************************************************/
#define CANSM_INSTANCE_ID           0x00U

/*******************************************************************************
* Configuration Variants
*******************************************************************************/
#define CANSM_CONFIG_VARIANT_PRECOMPILE     1U
#define CANSM_CONFIG_VARIANT_LINKTIME       2U
#define CANSM_CONFIG_VARIANT_POSTBUILD      3U

#ifndef CANSM_CONFIGURATION_VARIANT
#define CANSM_CONFIGURATION_VARIANT         CANSM_CONFIG_VARIANT_LINKTIME
#endif

#endif /* CANSM_CFG_H */
