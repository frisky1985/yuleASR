/*
 * FiM_Cfg.h - Function Inhibition Manager Configuration
 */

#ifndef FIM_CFG_H
#define FIM_CFG_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define FIM_CFG_AR_RELEASE_MAJOR_VERSION    4u
#define FIM_CFG_AR_RELEASE_MINOR_VERSION    4u

/* Configuration Switches */
#define FIM_VERSION_INFO_API                STD_ON
#define FIM_DEV_ERROR_DETECT                STD_ON
#define FIM_EVENT_UPDATE_TRIGGERED_BY_DEM   STD_ON
#define FIM_FUNCTION_INHIBITION_AVAILABLE   STD_ON
#define FIM_EVENT_UPDATE_TRIGGER            STD_ON
#define FIM_MAIN_FUNCTION_CALLOUT_SUPPORTED STD_OFF

/* Maximum number of configured FIDs */
#define FIM_CFG_NUMBER_OF_FIDS              32u
#define FIM_CFG_MAX_FID                     (FIM_CFG_NUMBER_OF_FIDS - 1u)

/* Maximum number of configured Dem Events */
#define FIM_CFG_NUMBER_OF_EVENTS            64u
#define FIM_CFG_MAX_EVENT                   (FIM_CFG_NUMBER_OF_EVENTS - 1u)

/* Maximum number of inhibition configurations */
#define FIM_CFG_MAX_INHIBITION_CONFIG       128u

/* Maximum event-to-FID mappings */
#define FIM_CFG_MAX_EVENT_FID_MAP           96u

/* Invalid FID value */
#define FIM_INVALID_FID                     0xFFFFu

/* Invalid Event ID value */
#define FIM_INVALID_EVENT                   0xFFFFu

/*============================================================================
 * Inhibition Mask Definitions
 * Refer to AUTOSAR_SWS_FunctionInhibitionManager chapter 7.2.1
 *===========================================================================*/

/*
 * Inhibition mask: LAST_FAILED
 * Permission is set to FALSE if the event has reported a failure
 * since the last diagnostic session start (testFailed = TRUE in event status byte)
 */
#define FIM_LAST_FAILED                     0x01u

/*
 * Inhibition mask: NOT_TESTED
 * Permission is set to FALSE if the event is not yet tested
 * in this diagnostic session (tested = FALSE in event status byte)
 */
#define FIM_NOT_TESTED                      0x02u

/*
 * Inhibition mask: TESTED_FAULTY
 * Permission is set to FALSE if the event was tested and
 * reported a failure during this diagnostic session
 * (tested = TRUE AND testFailed = TRUE)
 */
#define FIM_TESTED_FAULTY                   0x04u

/*
 * Inhibition mask: TESTED_AND_FAILED
 * Alias for TESTED_FAULTY for backward compatibility
 */
#define FIM_TESTED_AND_FAILED               FIM_TESTED_FAULTY

/*
 * Combined mask: Inhibit if event has failed at any point
 * Combines LAST_FAILED and TESTED_FAULTY behavior
 */
#define FIM_INHIBIT_IF_FAILED               0x05u

/* Summary masks for common use cases */
#define FIM_MASK_NONE                       0x00u
#define FIM_MASK_ALL_FAILURES               0x07u

/*============================================================================
 * Dem Event Status Byte Bit Definitions (for reference)
 *===========================================================================*/
#define DEM_UDS_STATUS_TF                   0x01u  /* Test Failed */
#define DEM_UDS_STATUS_TFTOC                0x02u  /* Test Failed This Operation Cycle */
#define DEM_UDS_STATUS_PDTC                 0x04u  /* Pending DTC */
#define DEM_UDS_STATUS_CDTC                 0x08u  /* Confirmed DTC */
#define DEM_UDS_STATUS_TNCSLC               0x10u  /* Test Not Completed Since Last Clear */
#define DEM_UDS_STATUS_TFSLC                0x20u  /* Test Failed Since Last Clear */
#define DEM_UDS_STATUS_TNCTOC               0x40u  /* Test Not Completed This Operation Cycle */
#define DEM_UDS_STATUS_WIR                  0x80u  /* Warning Indicator Requested */

/*============================================================================
 * Configuration Data Types
 *===========================================================================*/

typedef uint16 FiM_InhibitionMaskConfigurationType;

typedef struct {
    FiM_FunctionIdType      Fid;
    uint16                  EventId;
    uint8                   InhibitionMask;
} FiM_InhibitionConfigType;

typedef struct {
    uint16                  EventId;
    FiM_FunctionIdType      Fid;
    uint8                   InhibitionMask;
} FiM_EventFidMappingType;

typedef struct {
    uint16                  EventId;
    boolean                 isConfigured;
} FiM_EventConfigType;

typedef struct {
    FiM_FunctionIdType      Fid;
    boolean                 isConfigured;
    boolean                 availability;
    boolean                 defaultPermission;
} FiM_FidConfigType;

/*============================================================================
 * Predefined FIDs (example configuration)
 *===========================================================================*/
#define FIM_FID_POWER_LIMIT                 0u
#define FIM_FID_TORQUE_LIMIT                1u
#define FIM_FID_SPEED_LIMIT                 2u
#define FIM_FID_EMISSION_CONTROL            3u
#define FIM_FID_START_STOP_FUNCTION         4u
#define FIM_FID_CRUISE_CONTROL              5u
#define FIM_FID_ADAS_FEATURES               6u
#define FIM_FID_THERMAL_MANAGEMENT          7u
#define FIM_FID_BATTERY_CHARGING            8u
#define FIM_FID_REGENERATIVE_BRAKING        9u
#define FIM_FID_USER_DEFINED_1              10u
#define FIM_FID_USER_DEFINED_2              11u

/*============================================================================
 * Predefined Event IDs (referencing Dem)
 *===========================================================================*/
#define FIM_EVENT_CATALYST_DAMAGE           0u
#define FIM_EVENT_MISFIRE                   1u
#define FIM_EVENT_FUEL_SYSTEM               2u
#define FIM_EVENT_O2_SENSOR                 3u
#define FIM_EVENT_BOOST_PRESSURE            4u
#define FIM_EVENT_COOLANT_TEMP              5u
#define FIM_EVENT_THROTTLE_POSITION         6u
#define FIM_EVENT_MAF_SENSOR                7u
#define FIM_EVENT_ENGINE_KNOCK              8u
#define FIM_EVENT_BATTERY_VOLTAGE           9u

/* Configuration constant for Lcfg */
#define FIM_CFG_CONST
#define FIM_CFG_CONST_ROOT

#endif /* FIM_CFG_H */
