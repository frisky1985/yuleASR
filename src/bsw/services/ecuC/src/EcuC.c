/** @file EcuC.c
 *  @brief ECU Configuration implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_ECUConfiguration.pdf
 */

#include "EcuC.h"
#include "Det.h"

#define ECUC_SID_INIT               0x00U
#define ECUC_SID_DEINIT             0x01U
#define ECUC_SID_GET_CONFIG         0x02U
#define ECUC_SID_SET_CONFIG         0x03U
#define ECUC_SID_GET_VERSION_INFO   0x04U

#define ECUC_E_PARAM_POINTER        0x10U
#define ECUC_E_UNINIT               0x20U
#define ECUC_E_PARAM_CONFIG         0x30U
#define ECUC_E_READ_ONLY            0x40U

typedef enum { ECUC_UNINIT = 0, ECUC_INIT } EcuC_StateType;

typedef struct {
    EcuC_StateType  state;
    uint8           variant;
    EcuC_ConfigType activeConfig;
    const EcuC_ConfigType* configPtr;
} EcuC_InternalType;

static EcuC_InternalType EcuC_State = {
    ECUC_UNINIT, 0U,
    {0,0,0,0,0,0,0,0},
    NULL_PTR
};

void EcuC_Init(const EcuC_ConfigType* ConfigPtr)
{
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_INIT, ECUC_E_PARAM_POINTER);
        return;
    }
#endif
    EcuC_State.configPtr = ConfigPtr;
    EcuC_State.activeConfig = *ConfigPtr;
    EcuC_State.variant = 1U;
    EcuC_State.state = ECUC_INIT;
}

void EcuC_DeInit(void)
{
    EcuC_State.state = ECUC_UNINIT;
    EcuC_State.configPtr = NULL_PTR;
}

Std_ReturnType EcuC_GetConfigValue(uint16 ConfigId, uint32* Value)
{
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (EcuC_State.state == ECUC_UNINIT) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_GET_CONFIG, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == Value) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_GET_CONFIG, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    switch (ConfigId) {
        case ECUC_CONFIG_ID_CORE_FREQ:     *Value = EcuC_State.activeConfig.CoreFrequency; break;
        case ECUC_CONFIG_ID_BUS_FREQ:      *Value = EcuC_State.activeConfig.BusFrequency; break;
        case ECUC_CONFIG_ID_RAM_SIZE:      *Value = EcuC_State.activeConfig.RamSize; break;
        case ECUC_CONFIG_ID_FLASH_SIZE:    *Value = EcuC_State.activeConfig.FlashSize; break;
        case ECUC_CONFIG_ID_EEPROM_SIZE:   *Value = EcuC_State.activeConfig.EepromSize; break;
        case ECUC_CONFIG_ID_CAN_BAUD:      *Value = EcuC_State.activeConfig.CanBaudrate; break;
        case ECUC_CONFIG_ID_LIN_BAUD:      *Value = EcuC_State.activeConfig.LinBaudrate; break;
        default: return E_NOT_OK;
    }
    return E_OK;
}

Std_ReturnType EcuC_SetConfigValue(uint16 ConfigId, uint32 Value)
{
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (EcuC_State.state == ECUC_UNINIT) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_SET_CONFIG, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    switch (ConfigId) {
        case ECUC_CONFIG_ID_CORE_FREQ:     EcuC_State.activeConfig.CoreFrequency = Value; break;
        case ECUC_CONFIG_ID_BUS_FREQ:      EcuC_State.activeConfig.BusFrequency = Value; break;
        case ECUC_CONFIG_ID_RAM_SIZE:      EcuC_State.activeConfig.RamSize = Value; break;
        case ECUC_CONFIG_ID_FLASH_SIZE:    EcuC_State.activeConfig.FlashSize = Value; break;
        case ECUC_CONFIG_ID_EEPROM_SIZE:   EcuC_State.activeConfig.EepromSize = Value; break;
        case ECUC_CONFIG_ID_CAN_BAUD:      EcuC_State.activeConfig.CanBaudrate = Value; break;
        case ECUC_CONFIG_ID_LIN_BAUD:      EcuC_State.activeConfig.LinBaudrate = Value; break;
        default: return E_NOT_OK;
    }
    return E_OK;
}

void EcuC_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_GET_VERSION_INFO, ECUC_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = ECUC_VENDOR_ID;
    versioninfo->moduleID = ECUC_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}