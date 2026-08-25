/*
 * Can_Qemu_Lcfg.c - C4: Minimal CAN configuration for QEMU loopback
 *
 * 1 CAN controller, 1 HTH, 4 PDU - sufficient for loopback testing.
 * Compiled only when QEMU_CAN_LOOPBACK is defined.
 */
#ifdef QEMU_CAN_LOOPBACK

#include "Can.h"
#include "Can_Cfg.h"

#define CAN_QEMU_NUM_CONTROLLERS  1U
#define CAN_QEMU_NUM_HOH          4U

static const Can_BaudrateConfigType Can_QemuBaudrate = {
    .Prescaler     = 1U,
    .SyncJumpWidth = 1U,
    .PhaseSeg1     = 6U,
    .PhaseSeg2     = 3U,
    .PropSeg       = 2U
};

static const Can_HardwareObjectType Can_QemuHardwareObjects[CAN_QEMU_NUM_HOH] = {
    { .Hoh = 0U, .HohType = CAN_HOH_TYPE_RECEIVE, .IdType = CAN_ID_TYPE_STANDARD,
      .FirstId = 0x100U, .LastId = 0x100U, .ObjectId = 0U, .Filtering = FALSE },
    { .Hoh = 1U, .HohType = CAN_HOH_TYPE_RECEIVE, .IdType = CAN_ID_TYPE_STANDARD,
      .FirstId = 0x101U, .LastId = 0x101U, .ObjectId = 1U, .Filtering = FALSE },
    { .Hoh = 2U, .HohType = CAN_HOH_TYPE_TRANSMIT, .IdType = CAN_ID_TYPE_STANDARD,
      .FirstId = 0x100U, .LastId = 0x100U, .ObjectId = 2U, .Filtering = FALSE },
    { .Hoh = 3U, .HohType = CAN_HOH_TYPE_TRANSMIT, .IdType = CAN_ID_TYPE_STANDARD,
      .FirstId = 0x101U, .LastId = 0x101U, .ObjectId = 3U, .Filtering = FALSE }
};

static const Can_ControllerConfigType Can_QemuControllerConfig = {
    .ControllerId       = 0U,
    .BaseAddress        = 0U,
    .BaudrateConfigs    = &Can_QemuBaudrate,
    .NumBaudrateConfigs = 1U,
    .HardwareObjects    = Can_QemuHardwareObjects,
    .NumHardwareObjects = CAN_QEMU_NUM_HOH,
    .RxProcessing       = CAN_PROCESSING_POLLING,
    .TxProcessing       = CAN_PROCESSING_POLLING,
    .BusOffProcessing   = FALSE,
    .WakeupProcessing   = FALSE,
    .WakeupSupport      = FALSE,
    .DefaultBaudrateIndex = 0U
};

const Can_ConfigType Can_QemuConfig = {
    .Controllers     = &Can_QemuControllerConfig,
    .NumControllers  = CAN_QEMU_NUM_CONTROLLERS,
    .DevErrorDetect  = TRUE,
    .VersionInfoApi  = FALSE
};

#endif /* QEMU_CAN_LOOPBACK */
