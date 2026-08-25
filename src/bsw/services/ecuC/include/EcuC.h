/**
 * @file EcuC.h
 * @brief ECU Configuration - AUTOSAR Services Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_ECUConfiguration.pdf
 */

#ifndef ECUC_H
#define ECUC_H

#include "Std_Types.h"
#include "EcuC_Cfg.h"
#include "ComStack_Types.h"

#define ECUC_MODULE_ID              0x13U
#define ECUC_VENDOR_ID              0x0055U

#define ECUC_CONFIG_ID_CORE_FREQ    0x01U
#define ECUC_CONFIG_ID_BUS_FREQ     0x02U
#define ECUC_CONFIG_ID_RAM_SIZE     0x03U
#define ECUC_CONFIG_ID_FLASH_SIZE   0x04U
#define ECUC_CONFIG_ID_EEPROM_SIZE  0x05U
#define ECUC_CONFIG_ID_CAN_BAUD     0x06U
#define ECUC_CONFIG_ID_LIN_BAUD     0x07U

/* Signal Transfer Properties */
#define ECUC_SIGNAL_TRIGGERED               0x01U
#define ECUC_SIGNAL_TRIGGERED_ON_CHANGE     0x02U

/* Signal Direction */
#define ECUC_SEND                   0x01U
#define ECUC_RECEIVE                0x02U

/* Container Types */
#define ECUC_CONTAINER_ECU          0x01U
#define ECUC_CONTAINER_PDU          0x02U
#define ECUC_CONTAINER_SIGNAL       0x03U

typedef struct {
    uint16 SignalId;
    uint16 SignalSize;
    uint16 SignalStartBit;
    uint8  SignalBitOrder;
    uint8  TransferProperty;
    uint8  Direction;
    uint16 RelatedPduId;
} EcuC_SignalConfigType;

typedef struct {
    uint16 PduId;
    uint16 PduLength;
    uint16 SignalCount;
    const EcuC_SignalConfigType* Signals;
} EcuC_PduConfigType;

typedef struct {
    uint16 ContainerId;
    uint8  ContainerType;
    uint16 NumPdus;
    const uint16* PduRefs;
} EcuC_ContainerType;

typedef struct {
    uint16 SourcePduId;
    uint16 DestinationPduId;
    uint8  SignalCount;
    const uint16* SignalMapping;
} EcuC_RoutingPathType;

typedef struct {
    uint32 CoreFrequency;
    uint32 BusFrequency;
    uint32 RamSize;
    uint32 FlashSize;
    uint32 EepromSize;
    uint32 CanBaudrate;
    uint32 LinBaudrate;
    uint32 SpiFrequency;
    /* Extended config for Lcfg */
    uint16 PduCount;
    uint16 SignalCount;
    uint16 RoutingPathCount;
    const EcuC_PduConfigType* Pdus;
    const EcuC_SignalConfigType* Signals;
    const EcuC_RoutingPathType* RoutingPaths;
} EcuC_ConfigType;

/** @req SWS_EcuC_00001 */
void EcuC_Init(const EcuC_ConfigType* ConfigPtr);
/** @req SWS_EcuC_00002 */
void EcuC_DeInit(void);
/** @req SWS_EcuC_00004 */
Std_ReturnType EcuC_GetConfigValue(uint16 ConfigId, uint32* Value);
/** @req SWS_EcuC_00005 */
Std_ReturnType EcuC_SetConfigValue(uint16 ConfigId, uint32 Value);
/** @req SWS_EcuC_00003 */
void EcuC_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* ECUC_H */