/*******************************************************************************
* File: CanSm_Lcfg.c
* Description: Link-time configuration for CAN State Manager (CanSm)
*              AUTOSAR SWS CANStateManager 4.4.0 compliant
* This file contains:
*   - Controller configuration tables
*   - Transceiver configuration tables
*   - Network configuration tables
*   - Main CanSm configuration structure
*******************************************************************************/

/*******************************************************************************
* Includes
*******************************************************************************/
#include "CanSm.h"

/*******************************************************************************
* Configuration Constants
*******************************************************************************/

/* Number of networks configured */
#define CANSM_CFG_NETWORK_COUNT     2U

/* Network 0: CAN0 - Primary CAN network */
#define CANSM_NET0_CONTROLLER_COUNT 1U
#define CANSM_NET0_TRANSCEIVER_COUNT    1U

/* Network 1: CAN1 - Secondary CAN network */
#define CANSM_NET1_CONTROLLER_COUNT 1U
#define CANSM_NET1_TRANSCEIVER_COUNT    1U

/*******************************************************************************
* Memory Mapping
*******************************************************************************/
#define CANSM_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
* Controller Configurations
*******************************************************************************/

/* Network 0 Controllers (Primary CAN) */
static const CanSm_ControllerConfigType CanSm_Net0_Controllers[CANSM_NET0_CONTROLLER_COUNT] =
{
    {
        /* Controller 0 - Primary CAN controller */
        .ControllerId = 0U,                     /* CAN Controller 0 */
        .InitialMode = CANIF_CS_STOPPED         /* Start in STOPPED mode */
    }
};

/* Network 1 Controllers (Secondary CAN) */
static const CanSm_ControllerConfigType CanSm_Net1_Controllers[CANSM_NET1_CONTROLLER_COUNT] =
{
    {
        /* Controller 1 - Secondary CAN controller */
        .ControllerId = 1U,                     /* CAN Controller 1 */
        .InitialMode = CANIF_CS_STOPPED         /* Start in STOPPED mode */
    }
};

/*******************************************************************************
* Transceiver Configurations
*******************************************************************************/
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)

/* Network 0 Transceivers */
static const CanSm_TransceiverConfigType CanSm_Net0_Transceivers[CANSM_NET0_TRANSCEIVER_COUNT] =
{
    {
        /* Transceiver 0 - Primary CAN transceiver */
        .TransceiverId = 0U,                    /* CAN Transceiver 0 */
        .InitialMode = CANIF_TRCV_MODE_STANDBY, /* Start in STANDBY mode */
        .WakeupSupport = TRUE                   /* Wake-up supported */
    }
};

/* Network 1 Transceivers */
static const CanSm_TransceiverConfigType CanSm_Net1_Transceivers[CANSM_NET1_TRANSCEIVER_COUNT] =
{
    {
        /* Transceiver 1 - Secondary CAN transceiver */
        .TransceiverId = 1U,                    /* CAN Transceiver 1 */
        .InitialMode = CANIF_TRCV_MODE_STANDBY, /* Start in STANDBY mode */
        .WakeupSupport = TRUE                   /* Wake-up supported */
    }
};

#endif /* CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON */

/*******************************************************************************
* Network Configurations
*******************************************************************************/

/* Network Configuration Table */
static const CanSm_NetworkConfigType CanSm_NetworkConfigs[CANSM_CFG_NETWORK_COUNT] =
{
    /* Network 0: Primary CAN Network */
    {
        .NetworkId = 0U,                        /* Network handle 0 */
        .ControllerCount = CANSM_NET0_CONTROLLER_COUNT,
        .ControllerRefs = &CanSm_Net0_Controllers[0],
        
        #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
        .TransceiverCount = CANSM_NET0_TRANSCEIVER_COUNT,
        .TransceiverRefs = &CanSm_Net0_Transceivers[0],
        #endif
        
        /* Bus-off Recovery Timing Parameters */
        /* T_RESTART: Time to wait before restarting after bus-off (0.1s = 100ms) */
        .TRestart = CANSM_T_RESTART_DEFAULT,
        
        /* T_RECOVERY: Time to wait before allowing next retry (1.0s = 1000ms) */
        .TRecovery = CANSM_T_RECOVERY_DEFAULT,
        
        /* Maximum retry attempts before declaring bus-off failure */
        .BusOffMaxRetries = CANSM_BUSOFF_MAX_RETRIES
    },
    
    /* Network 1: Secondary CAN Network */
    {
        .NetworkId = 1U,                        /* Network handle 1 */
        .ControllerCount = CANSM_NET1_CONTROLLER_COUNT,
        .ControllerRefs = &CanSm_Net1_Controllers[0],
        
        #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
        .TransceiverCount = CANSM_NET1_TRANSCEIVER_COUNT,
        .TransceiverRefs = &CanSm_Net1_Transceivers[0],
        #endif
        
        /* Bus-off Recovery Timing Parameters */
        /* T_RESTART: Time to wait before restarting after bus-off (0.1s = 100ms) */
        .TRestart = CANSM_T_RESTART_DEFAULT,
        
        /* T_RECOVERY: Time to wait before allowing next retry (1.0s = 1000ms) */
        .TRecovery = CANSM_T_RECOVERY_DEFAULT,
        
        /* Maximum retry attempts before declaring bus-off failure */
        .BusOffMaxRetries = CANSM_BUSOFF_MAX_RETRIES
    }
};

/*******************************************************************************
* Main Configuration Structure
*******************************************************************************/

/* CanSm Configuration Structure - Exported to CanSm.c */
const CanSm_ConfigType CanSm_Config =
{
    .NetworkCount = CANSM_CFG_NETWORK_COUNT,
    .Networks = &CanSm_NetworkConfigs[0]
};

#define CANSM_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
* Post-Build Configuration (if required)
*******************************************************************************/
#if (CANSM_CONFIGURATION_VARIANT == CANSM_CONFIG_VARIANT_POSTBUILD)

#define CANSM_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* Post-build configuration pointer - initialized by EcuM */
static const CanSm_ConfigType* CanSm_PostBuildConfigPtr = NULL_PTR;

#define CANSM_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
* Name: CanSm_GetPostBuildConfig
* Description: Returns pointer to post-build configuration
* Parameters: None
* Returns: Pointer to CanSm_ConfigType
*******************************************************************************/
const CanSm_ConfigType* CanSm_GetPostBuildConfig(void)
{
    return CanSm_PostBuildConfigPtr;
}

/*******************************************************************************
* Name: CanSm_SetPostBuildConfig
* Description: Sets the post-build configuration pointer (called by EcuM)
* Parameters: ConfigPtr - Pointer to post-build configuration
* Returns: None
*******************************************************************************/
void CanSm_SetPostBuildConfig(const CanSm_ConfigType* ConfigPtr)
{
    CanSm_PostBuildConfigPtr = ConfigPtr;
}

#endif /* CANSM_CONFIGURATION_VARIANT == CANSM_CONFIG_VARIANT_POSTBUILD */

/*******************************************************************************
* Configuration Validation Checks
*******************************************************************************/

/* Validate network count matches configured value */
#if (CANSM_CFG_NETWORK_COUNT != CANSM_NETWORK_COUNT)
    #error "CANSM_CFG_NETWORK_COUNT does not match CANSM_NETWORK_COUNT in CanSm_Cfg.h"
#endif

/* Validate controller limits */
#if (CANSM_NET0_CONTROLLER_COUNT > CANSM_MAX_CONTROLLERS_PER_NETWORK)
    #error "Network 0: Controller count exceeds maximum"
#endif

#if (CANSM_NET1_CONTROLLER_COUNT > CANSM_MAX_CONTROLLERS_PER_NETWORK)
    #error "Network 1: Controller count exceeds maximum"
#endif

/* Validate transceiver limits */
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
#if (CANSM_NET0_TRANSCEIVER_COUNT > CANSM_MAX_TRANSCEIVERS_PER_NETWORK)
    #error "Network 0: Transceiver count exceeds maximum"
#endif

#if (CANSM_NET1_TRANSCEIVER_COUNT > CANSM_MAX_TRANSCEIVERS_PER_NETWORK)
    #error "Network 1: Transceiver count exceeds maximum"
#endif
#endif

/*******************************************************************************
* End of File
*******************************************************************************/
