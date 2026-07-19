/**
 * @file EthIf.h
 * @brief Ethernet Interface - AUTOSAR ECUAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_EthernetInterface.pdf
 */

#ifndef ETHIF_H
#define ETHIF_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "EthIf_Cfg.h"

#define ETHIF_MODULE_ID             0x70U
#define ETHIF_VENDOR_ID             0x0055U
#define ETHIF_MAX_CONTROLLERS       4U
#define ETHIF_MAX_VLANS             4U

#define ETH_MODE_DOWN               0x00U
#define ETH_MODE_ACTIVE             0x01U
#define ETH_MODE_SLEEP              0x02U

typedef enum {
    ETHIF_CS_STOPPED = 0,
    ETHIF_CS_STARTED,
    ETHIF_CS_SLEEP
} EthIf_ControllerMode;

typedef struct {
    uint8  MacAddress[6];
    uint16 EtherType;
    uint8* SduPtr;
    uint16 SduLength;
} EthIf_PduType;

typedef void (*EthIf_RxCallback)(uint8 ControllerId, const EthIf_PduType* PduInfoPtr);

typedef enum {
    ETHIF_FILTER_MAC = 0,
    ETHIF_FILTER_ETHERTYPE,
    ETHIF_FILTER_VLAN
} EthIf_FilterType;

typedef struct {
    EthIf_FilterType FilterType;
    uint8 ControllerId;
    uint8 MacAddress[6];
    uint16 EtherType;
    uint16 VlanId;
    EthIf_RxCallback RxCallback;
} EthIf_RxFilterType;

typedef struct {
    uint8 CtrlIdx;
    uint8 VlanId;
    uint8 CtrlMode;
    uint8 MacAddress[6];
    uint32 ControllerHandle;
} EthIf_ControllerConfigType;

typedef struct {
    uint16 VlanId;
    uint8  Priority;
} EthIf_VlanConfigType;

typedef struct {
    uint8 NumControllers;
    const EthIf_ControllerConfigType* Controllers;
    uint8 NumVlans;
    const EthIf_VlanConfigType* Vlans;
    uint8 NumRxFilters;
    const EthIf_RxFilterType* RxFilters;
} EthIf_ConfigType;

void EthIf_Init(const EthIf_ConfigType* ConfigPtr);
void EthIf_DeInit(void);
Std_ReturnType EthIf_Transmit(uint8 ControllerId, uint32 BufferHandle, const EthIf_PduType* PduInfoPtr);
Std_ReturnType EthIf_SetControllerMode(uint8 ControllerId, EthIf_ControllerMode Mode);
EthIf_ControllerMode EthIf_GetControllerMode(uint8 ControllerId);
void EthIf_RxIndication(uint8 ControllerId, const EthIf_PduType* PduInfoPtr);
void EthIf_TxConfirmation(uint8 ControllerId, uint32 BufferHandle);
void EthIf_MainFunction(void);
void EthIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* ETHIF_H */