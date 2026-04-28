/*
 * Com_Cfg.h
 * AUTOSAR COM Module - Configuration Header
 */

#ifndef COM_CFG_H
#define COM_CFG_H

/*==================[Pre-compile Configuration]============================*/

/* Development Error Detection */
#define COM_DEV_ERROR_DETECT                STD_ON

/* Version Info API */
#define COM_VERSION_INFO_API                STD_ON

/* Enable Signal Group Array API */
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   STD_ON

/* Enable MD for TMC Always/None */
#define COM_ENABLE_MDT_FOR_CYCLIC_TRANSMISSION STD_OFF

/* Optimization for supported platforms */
#define COM_OPTIMIZE_FOR_SIZE               STD_OFF
#define COM_OPTIMIZE_FOR_SPEED              STD_ON

/*==================[Configuration Constants]===============================*/

/* Maximum number of elements */
#define COM_MAX_SIGNALS                     128u
#define COM_MAX_SIGNAL_GROUPS               32u
#define COM_MAX_IPDUS                       64u
#define COM_MAX_IPDU_GROUPS                 16u

/* Maximum buffer sizes */
#define COM_MAX_IPDU_LENGTH                 64u
#define COM_MAX_SHADOW_BUFFER_SIZE          256u

/*==================[Symbolic Names]========================================*/

/* IPdu Group IDs */
#define ComConf_ComIPduGroup_EngineGroup    0u
#define ComConf_ComIPduGroup_ChassisGroup   1u
#define ComConf_ComIPduGroup_BodyGroup      2u

/* IPdu IDs */
#define ComConf_ComIPdu_EngineData          0u
#define ComConf_ComIPdu_EngineStatus        1u
#define ComConf_ComIPdu_VehicleSpeed        2u
#define ComConf_ComIPdu_GearPosition        3u

/* Signal IDs */
#define ComConf_ComSignal_EngineSpeed       0u
#define ComConf_ComSignal_EngineTemp        1u
#define ComConf_ComSignal_VehicleSpeed      2u
#define ComConf_ComSignal_GearPosition      3u
#define ComConf_ComSignal_AcceleratorPos    4u

/* Signal Group IDs */
#define ComConf_ComSignalGroup_EngineInfo   0u

/*==================[External Declarations]=================================*/

/* Configuration structure declared in Com_Lcfg.c */
extern const Com_ConfigType ComConfig;

#endif /* COM_CFG_H */
