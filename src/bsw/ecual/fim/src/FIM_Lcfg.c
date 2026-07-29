/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*
 * FiM_Lcfg.c - Function Inhibition Manager Link-time Configuration
 * 
 * This file contains the configuration tables for FIDs, functions,
 * events, and inhibition mappings.
 */

#include "FiM.h"
#include "FiM_Cfg.h"

/*============================================================================
 * Memory Section Definitions
 *===========================================================================*/
#define FIM_START_SEC_CONST_UNSPECIFIED
#include "FiM_MemMap.h"

/*============================================================================
 * Configuration Constants
 *===========================================================================*/

/* Total number of configured FIDs */
FIM_CFG_CONST uint16 FiM_NumFids = FIM_CFG_NUMBER_OF_FIDS;

/* Total number of configured Dem Events */
FIM_CFG_CONST uint16 FiM_NumEvents = FIM_CFG_NUMBER_OF_EVENTS;

/*============================================================================
 * FID Configuration Table
 * 
 * Defines each FID's default behavior and availability settings
 *===========================================================================*/
FIM_CFG_CONST FiM_FidConfigType FiM_FidConfigTable[FIM_CFG_NUMBER_OF_FIDS] = {
    /* FID 0: Power Limit - Inhibited by catalyst or fuel system faults */
    {
        /* Fid */               FIM_FID_POWER_LIMIT,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 1: Torque Limit - Inhibited by engine knock or misfire */
    {
        /* Fid */               FIM_FID_TORQUE_LIMIT,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 2: Speed Limit - Inhibited by critical sensor faults */
    {
        /* Fid */               FIM_FID_SPEED_LIMIT,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 3: Emission Control - Inhibited by any emission-related fault */
    {
        /* Fid */               FIM_FID_EMISSION_CONTROL,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 4: Start-Stop Function - Inhibited by battery or temperature faults */
    {
        /* Fid */               FIM_FID_START_STOP_FUNCTION,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 5: Cruise Control - Inhibited by vehicle speed or brake faults */
    {
        /* Fid */               FIM_FID_CRUISE_CONTROL,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 6: ADAS Features - Inhibited by sensor faults */
    {
        /* Fid */               FIM_FID_ADAS_FEATURES,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 7: Thermal Management - Inhibited by coolant temperature faults */
    {
        /* Fid */               FIM_FID_THERMAL_MANAGEMENT,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 8: Battery Charging - Inhibited by voltage or charging faults */
    {
        /* Fid */               FIM_FID_BATTERY_CHARGING,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FID 9: Regenerative Braking - Inhibited by brake or battery faults */
    {
        /* Fid */               FIM_FID_REGENERATIVE_BRAKING,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FIDs 10-11: User Defined */
    {
        /* Fid */               FIM_FID_USER_DEFINED_1,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    {
        /* Fid */               FIM_FID_USER_DEFINED_2,
        /* isConfigured */      TRUE,
        /* availability */      TRUE,
        /* defaultPermission */ TRUE
    },
    /* FIDs 12-31: Reserved/Unused */
    { 12u, FALSE, FALSE, FALSE },
    { 13u, FALSE, FALSE, FALSE },
    { 14u, FALSE, FALSE, FALSE },
    { 15u, FALSE, FALSE, FALSE },
    { 16u, FALSE, FALSE, FALSE },
    { 17u, FALSE, FALSE, FALSE },
    { 18u, FALSE, FALSE, FALSE },
    { 19u, FALSE, FALSE, FALSE },
    { 20u, FALSE, FALSE, FALSE },
    { 21u, FALSE, FALSE, FALSE },
    { 22u, FALSE, FALSE, FALSE },
    { 23u, FALSE, FALSE, FALSE },
    { 24u, FALSE, FALSE, FALSE },
    { 25u, FALSE, FALSE, FALSE },
    { 26u, FALSE, FALSE, FALSE },
    { 27u, FALSE, FALSE, FALSE },
    { 28u, FALSE, FALSE, FALSE },
    { 29u, FALSE, FALSE, FALSE },
    { 30u, FALSE, FALSE, FALSE },
    { 31u, FALSE, FALSE, FALSE }
};

/*============================================================================
 * Event Configuration Table
 * 
 * Maps configured Dem Event IDs to FiM internal handling
 *===========================================================================*/
FIM_CFG_CONST FiM_EventConfigType FiM_EventConfigTable[FIM_CFG_NUMBER_OF_EVENTS] = {
    /* Event 0: Catalyst Damage */
    {
        /* EventId */       FIM_EVENT_CATALYST_DAMAGE,
        /* isConfigured */  TRUE
    },
    /* Event 1: Misfire */
    {
        /* EventId */       FIM_EVENT_MISFIRE,
        /* isConfigured */  TRUE
    },
    /* Event 2: Fuel System */
    {
        /* EventId */       FIM_EVENT_FUEL_SYSTEM,
        /* isConfigured */  TRUE
    },
    /* Event 3: O2 Sensor */
    {
        /* EventId */       FIM_EVENT_O2_SENSOR,
        /* isConfigured */  TRUE
    },
    /* Event 4: Boost Pressure */
    {
        /* EventId */       FIM_EVENT_BOOST_PRESSURE,
        /* isConfigured */  TRUE
    },
    /* Event 5: Coolant Temperature */
    {
        /* EventId */       FIM_EVENT_COOLANT_TEMP,
        /* isConfigured */  TRUE
    },
    /* Event 6: Throttle Position */
    {
        /* EventId */       FIM_EVENT_THROTTLE_POSITION,
        /* isConfigured */  TRUE
    },
    /* Event 7: MAF Sensor */
    {
        /* EventId */       FIM_EVENT_MAF_SENSOR,
        /* isConfigured */  TRUE
    },
    /* Event 8: Engine Knock */
    {
        /* EventId */       FIM_EVENT_ENGINE_KNOCK,
        /* isConfigured */  TRUE
    },
    /* Event 9: Battery Voltage */
    {
        /* EventId */       FIM_EVENT_BATTERY_VOLTAGE,
        /* isConfigured */  TRUE
    },
    /* Events 10-63: Reserved/Unused */
    { 10u, FALSE }, { 11u, FALSE }, { 12u, FALSE }, { 13u, FALSE },
    { 14u, FALSE }, { 15u, FALSE }, { 16u, FALSE }, { 17u, FALSE },
    { 18u, FALSE }, { 19u, FALSE }, { 20u, FALSE }, { 21u, FALSE },
    { 22u, FALSE }, { 23u, FALSE }, { 24u, FALSE }, { 25u, FALSE },
    { 26u, FALSE }, { 27u, FALSE }, { 28u, FALSE }, { 29u, FALSE },
    { 30u, FALSE }, { 31u, FALSE }, { 32u, FALSE }, { 33u, FALSE },
    { 34u, FALSE }, { 35u, FALSE }, { 36u, FALSE }, { 37u, FALSE },
    { 38u, FALSE }, { 39u, FALSE }, { 40u, FALSE }, { 41u, FALSE },
    { 42u, FALSE }, { 43u, FALSE }, { 44u, FALSE }, { 45u, FALSE },
    { 46u, FALSE }, { 47u, FALSE }, { 48u, FALSE }, { 49u, FALSE },
    { 50u, FALSE }, { 51u, FALSE }, { 52u, FALSE }, { 53u, FALSE },
    { 54u, FALSE }, { 55u, FALSE }, { 56u, FALSE }, { 57u, FALSE },
    { 58u, FALSE }, { 59u, FALSE }, { 60u, FALSE }, { 61u, FALSE },
    { 62u, FALSE }, { 63u, FALSE }
};

/*============================================================================
 * Inhibition Configuration Table
 * 
 * Maps FIDs to events with inhibition masks
 * Multiple events can inhibit a single FID
 *===========================================================================*/
FIM_CFG_CONST uint16 FiM_NumInhibitionConfigs = 20u;

FIM_CFG_CONST FiM_InhibitionConfigType FiM_InhibitionConfigTable[20] = {
    /* Power Limit (FID 0) - Inhibited by catalyst damage, fuel system, or O2 sensor */
    {
        /* Fid */               FIM_FID_POWER_LIMIT,
        /* EventId */           FIM_EVENT_CATALYST_DAMAGE,
        /* InhibitionMask */    FIM_LAST_FAILED
    },
    {
        /* Fid */               FIM_FID_POWER_LIMIT,
        /* EventId */           FIM_EVENT_FUEL_SYSTEM,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    {
        /* Fid */               FIM_FID_POWER_LIMIT,
        /* EventId */           FIM_EVENT_O2_SENSOR,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* Torque Limit (FID 1) - Inhibited by misfire, engine knock, or boost pressure */
    {
        /* Fid */               FIM_FID_TORQUE_LIMIT,
        /* EventId */           FIM_EVENT_MISFIRE,
        /* InhibitionMask */    FIM_INHIBIT_IF_FAILED
    },
    {
        /* Fid */               FIM_FID_TORQUE_LIMIT,
        /* EventId */           FIM_EVENT_ENGINE_KNOCK,
        /* InhibitionMask */    FIM_LAST_FAILED
    },
    {
        /* Fid */               FIM_FID_TORQUE_LIMIT,
        /* EventId */           FIM_EVENT_BOOST_PRESSURE,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* Speed Limit (FID 2) - Inhibited by critical sensor faults */
    {
        /* Fid */               FIM_FID_SPEED_LIMIT,
        /* EventId */           FIM_EVENT_THROTTLE_POSITION,
        /* InhibitionMask */    FIM_NOT_TESTED
    },
    {
        /* Fid */               FIM_FID_SPEED_LIMIT,
        /* EventId */           FIM_EVENT_MAF_SENSOR,
        /* InhibitionMask */    FIM_NOT_TESTED
    },
    {
        /* Fid */               FIM_FID_SPEED_LIMIT,
        /* EventId */           FIM_EVENT_BOOST_PRESSURE,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* Emission Control (FID 3) - Inhibited by any emission-related fault */
    {
        /* Fid */               FIM_FID_EMISSION_CONTROL,
        /* EventId */           FIM_EVENT_CATALYST_DAMAGE,
        /* InhibitionMask */    FIM_LAST_FAILED
    },
    {
        /* Fid */               FIM_FID_EMISSION_CONTROL,
        /* EventId */           FIM_EVENT_O2_SENSOR,
        /* InhibitionMask */    FIM_LAST_FAILED
    },
    {
        /* Fid */               FIM_FID_EMISSION_CONTROL,
        /* EventId */           FIM_EVENT_FUEL_SYSTEM,
        /* InhibitionMask */    FIM_LAST_FAILED
    },
    {
        /* Fid */               FIM_FID_EMISSION_CONTROL,
        /* EventId */           FIM_EVENT_MISFIRE,
        /* InhibitionMask */    FIM_INHIBIT_IF_FAILED
    },
    
    /* Start-Stop Function (FID 4) - Inhibited by battery or temperature */
    {
        /* Fid */               FIM_FID_START_STOP_FUNCTION,
        /* EventId */           FIM_EVENT_BATTERY_VOLTAGE,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    {
        /* Fid */               FIM_FID_START_STOP_FUNCTION,
        /* EventId */           FIM_EVENT_COOLANT_TEMP,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* Cruise Control (FID 5) - Inhibited by sensor or speed-related faults */
    {
        /* Fid */               FIM_FID_CRUISE_CONTROL,
        /* EventId */           FIM_EVENT_THROTTLE_POSITION,
        /* InhibitionMask */    FIM_NOT_TESTED
    },
    {
        /* Fid */               FIM_FID_CRUISE_CONTROL,
        /* EventId */           FIM_EVENT_MAF_SENSOR,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* ADAS Features (FID 6) - Inhibited by sensor faults */
    {
        /* Fid */               FIM_FID_ADAS_FEATURES,
        /* EventId */           FIM_EVENT_THROTTLE_POSITION,
        /* InhibitionMask */    FIM_NOT_TESTED
    },
    {
        /* Fid */               FIM_FID_ADAS_FEATURES,
        /* EventId */           FIM_EVENT_MAF_SENSOR,
        /* InhibitionMask */    FIM_NOT_TESTED
    },
    
    /* Thermal Management (FID 7) - Inhibited by coolant temperature fault */
    {
        /* Fid */               FIM_FID_THERMAL_MANAGEMENT,
        /* EventId */           FIM_EVENT_COOLANT_TEMP,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    },
    
    /* Battery Charging (FID 8) - Inhibited by battery voltage fault */
    {
        /* Fid */               FIM_FID_BATTERY_CHARGING,
        /* EventId */           FIM_EVENT_BATTERY_VOLTAGE,
        /* InhibitionMask */    FIM_TESTED_FAULTY
    }
};

/*============================================================================
 * Event-to-FID Mapping Table
 * 
 * Reverse lookup for events to affected FIDs
 * Used for efficient recalculation when events change
 *===========================================================================*/
FIM_CFG_CONST uint16 FiM_NumEventFidMappings = 24u;

FIM_CFG_CONST FiM_EventFidMappingType FiM_EventFidMapTable[24] = {
    /* Catalyst Damage Event affects FIDs */
    { FIM_EVENT_CATALYST_DAMAGE,    FIM_FID_POWER_LIMIT,        FIM_LAST_FAILED },
    { FIM_EVENT_CATALYST_DAMAGE,    FIM_FID_EMISSION_CONTROL,   FIM_LAST_FAILED },
    
    /* Misfire Event affects FIDs */
    { FIM_EVENT_MISFIRE,            FIM_FID_TORQUE_LIMIT,       FIM_INHIBIT_IF_FAILED },
    { FIM_EVENT_MISFIRE,            FIM_FID_EMISSION_CONTROL,   FIM_INHIBIT_IF_FAILED },
    
    /* Fuel System Event affects FIDs */
    { FIM_EVENT_FUEL_SYSTEM,        FIM_FID_POWER_LIMIT,        FIM_TESTED_FAULTY },
    { FIM_EVENT_FUEL_SYSTEM,        FIM_FID_EMISSION_CONTROL,   FIM_LAST_FAILED },
    
    /* O2 Sensor Event affects FIDs */
    { FIM_EVENT_O2_SENSOR,          FIM_FID_POWER_LIMIT,        FIM_TESTED_FAULTY },
    { FIM_EVENT_O2_SENSOR,          FIM_FID_EMISSION_CONTROL,   FIM_LAST_FAILED },
    
    /* Boost Pressure Event affects FIDs */
    { FIM_EVENT_BOOST_PRESSURE,     FIM_FID_TORQUE_LIMIT,       FIM_TESTED_FAULTY },
    { FIM_EVENT_BOOST_PRESSURE,     FIM_FID_SPEED_LIMIT,        FIM_TESTED_FAULTY },
    
    /* Coolant Temperature Event affects FIDs */
    { FIM_EVENT_COOLANT_TEMP,       FIM_FID_START_STOP_FUNCTION, FIM_TESTED_FAULTY },
    { FIM_EVENT_COOLANT_TEMP,       FIM_FID_THERMAL_MANAGEMENT,  FIM_TESTED_FAULTY },
    
    /* Throttle Position Event affects FIDs */
    { FIM_EVENT_THROTTLE_POSITION,  FIM_FID_SPEED_LIMIT,        FIM_NOT_TESTED },
    { FIM_EVENT_THROTTLE_POSITION,  FIM_FID_CRUISE_CONTROL,     FIM_NOT_TESTED },
    { FIM_EVENT_THROTTLE_POSITION,  FIM_FID_ADAS_FEATURES,      FIM_NOT_TESTED },
    
    /* MAF Sensor Event affects FIDs */
    { FIM_EVENT_MAF_SENSOR,         FIM_FID_SPEED_LIMIT,        FIM_NOT_TESTED },
    { FIM_EVENT_MAF_SENSOR,         FIM_FID_CRUISE_CONTROL,     FIM_TESTED_FAULTY },
    { FIM_EVENT_MAF_SENSOR,         FIM_FID_ADAS_FEATURES,      FIM_NOT_TESTED },
    
    /* Engine Knock Event affects FIDs */
    { FIM_EVENT_ENGINE_KNOCK,       FIM_FID_TORQUE_LIMIT,       FIM_LAST_FAILED },
    
    /* Battery Voltage Event affects FIDs */
    { FIM_EVENT_BATTERY_VOLTAGE,    FIM_FID_START_STOP_FUNCTION, FIM_TESTED_FAULTY },
    { FIM_EVENT_BATTERY_VOLTAGE,    FIM_FID_BATTERY_CHARGING,    FIM_TESTED_FAULTY }
};

/*============================================================================
 * Inhibition Mask Summary Configuration
 * 
 * Quick lookup table for event inhibition calculations
 *===========================================================================*/
FIM_CFG_CONST uint8 FiM_EventFidInhibitionMask[FIM_CFG_NUMBER_OF_EVENTS] = {
    /* Event 0: Catalyst Damage */
    FIM_LAST_FAILED,
    /* Event 1: Misfire */
    FIM_INHIBIT_IF_FAILED,
    /* Event 2: Fuel System */
    FIM_TESTED_FAULTY,
    /* Event 3: O2 Sensor */
    FIM_TESTED_FAULTY,
    /* Event 4: Boost Pressure */
    FIM_TESTED_FAULTY,
    /* Event 5: Coolant Temperature */
    FIM_TESTED_FAULTY,
    /* Event 6: Throttle Position */
    FIM_NOT_TESTED,
    /* Event 7: MAF Sensor */
    FIM_NOT_TESTED,
    /* Event 8: Engine Knock */
    FIM_LAST_FAILED,
    /* Event 9: Battery Voltage */
    FIM_TESTED_FAULTY,
    /* Events 10-63: Not configured */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0
};

#define FIM_STOP_SEC_CONST_UNSPECIFIED
#include "FiM_MemMap.h"

/*============================================================================
 * Symbolic Name Definitions
 * 
 * These provide symbolic names for configuration tools
 *===========================================================================*/

/* FID Symbolic Names */
#define FimConf_FimFid_PowerLimit               FIM_FID_POWER_LIMIT
#define FimConf_FimFid_TorqueLimit              FIM_FID_TORQUE_LIMIT
#define FimConf_FimFid_SpeedLimit               FIM_FID_SPEED_LIMIT
#define FimConf_FimFid_EmissionControl          FIM_FID_EMISSION_CONTROL
#define FimConf_FimFid_StartStopFunction        FIM_FID_START_STOP_FUNCTION
#define FimConf_FimFid_CruiseControl            FIM_FID_CRUISE_CONTROL
#define FimConf_FimFid_ADASFeatures             FIM_FID_ADAS_FEATURES
#define FimConf_FimFid_ThermalManagement        FIM_FID_THERMAL_MANAGEMENT
#define FimConf_FimFid_BatteryCharging          FIM_FID_BATTERY_CHARGING
#define FimConf_FimFid_RegenerativeBraking      FIM_FID_REGENERATIVE_BRAKING

/* Event Symbolic Names */
#define FimConf_DemEventParameter_CatalystDamage    FIM_EVENT_CATALYST_DAMAGE
#define FimConf_DemEventParameter_Misfire           FIM_EVENT_MISFIRE
#define FimConf_DemEventParameter_FuelSystem        FIM_EVENT_FUEL_SYSTEM
#define FimConf_DemEventParameter_O2Sensor          FIM_EVENT_O2_SENSOR
#define FimConf_DemEventParameter_BoostPressure     FIM_EVENT_BOOST_PRESSURE
#define FimConf_DemEventParameter_CoolantTemp       FIM_EVENT_COOLANT_TEMP
#define FimConf_DemEventParameter_ThrottlePosition  FIM_EVENT_THROTTLE_POSITION
#define FimConf_DemEventParameter_MafSensor         FIM_EVENT_MAF_SENSOR
#define FimConf_DemEventParameter_EngineKnock       FIM_EVENT_ENGINE_KNOCK
#define FimConf_DemEventParameter_BatteryVoltage    FIM_EVENT_BATTERY_VOLTAGE
