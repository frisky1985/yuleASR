import { layers, moduleDetails } from './modules';

export interface SearchIndexEntry {
  id: string;
  title: string;
  content: string;
  path: string;
  category: '模块' | 'API' | '场景' | '类型' | '配置' | '错误码' | '文档';
}

export const searchIndex: SearchIndexEntry[] = [];

let entryId = 0;
function addEntry(entry: Omit<SearchIndexEntry, 'id'>) {
  searchIndex.push({ id: String(entryId++), ...entry });
}

/* ── Layers & Modules ── */
for (const layer of layers) {
  addEntry({
    title: layer.name,
    content: layer.description,
    path: '/modules',
    category: '模块',
  });

  for (const mod of layer.modules) {
    const path = `/module/${layer.id}/${mod.id}`;

    addEntry({
      title: mod.name,
      content: mod.description,
      path,
      category: '模块',
    });

    const detail = moduleDetails[mod.id];
    if (!detail) continue;

    /* Overview */
    addEntry({
      title: `${mod.name} 功能概述`,
      content: detail.overview,
      path,
      category: '文档',
    });

    /* APIs */
    for (const apiGroup of detail.apis) {
      for (const item of apiGroup.items) {
        addEntry({
          title: item.name,
          content: `${apiGroup.category}：${item.description}${item.calledBy ? `（调用方：${item.calledBy}）` : ''}`,
          path,
          category: 'API',
        });
      }
    }

    /* Data Types */
    for (const dt of detail.dataTypes) {
      addEntry({
        title: dt.name,
        content: `${dt.name} 定义：${dt.definition}`,
        path,
        category: '类型',
      });
    }

    /* Scenarios */
    for (const sc of detail.scenarios) {
      addEntry({
        title: sc.title,
        content: `${sc.description} 流程：${sc.flow.join(' ')} 预期结果：${sc.expected}`,
        path,
        category: '场景',
      });
    }

    /* Config Params */
    for (const cp of detail.configParams) {
      addEntry({
        title: cp.name,
        content: `默认值：${cp.defaultValue}。${cp.description}`,
        path,
        category: '配置',
      });
    }

    /* Error Codes */
    if (detail.errorCodes) {
      for (const ec of detail.errorCodes) {
        addEntry({
          title: ec.name,
          content: `值：${ec.value}。${ec.description}`,
          path,
          category: '错误码',
        });
      }
    }
  }
}

/* ── OpenSpec Markdown Documents ── */

const specDocs: { title: string; path: string; sections: { heading: string; content: string }[] }[] = [
  {
    title: 'PduR Specification',
    path: '/module/services/pdur',
    sections: [
      { heading: 'PduR Module Overview', content: 'PduR is a core Service Layer module responsible for routing I-PDUs between upper-layer modules and lower-layer communication interface modules. It does not process bus protocols; it only performs routing decisions and dispatches PDUs based on statically configured routing tables.' },
      { heading: 'PduR Key Responsibilities', content: 'Route transmit requests from upper layers to lower layers. Route receive indications from lower layers to upper layers. Route transmit confirmations from lower layers back to upper layers. Support multicast, FIFO-based deferred routing, and routing path group enable/disable.' },
      { heading: 'PduR Core Lifecycle APIs', content: 'PduR_Init initializes the module. PduR_DeInit deinitializes. PduR_GetVersionInfo retrieves version information.' },
      { heading: 'PduR Upper-Layer Transmit APIs', content: 'PduR_ComTransmit routes COM transmit requests. PduR_DcmTransmit routes DCM transmit requests.' },
      { heading: 'PduR Lower-Layer Callback APIs', content: 'PduR_CanIfRxIndication routes CAN receive indications. PduR_CanIfTxConfirmation routes CAN transmit confirmations. PduR_CanIfTriggerTransmit requests PDU data for trigger transmit.' },
      { heading: 'PduR Data Types', content: 'PduR_StateType, PduR_ReturnType, PduR_RoutingPathType with DIRECT FIFO GATEWAY, PduR_DestPduProcessingType with IMMEDIATE and DEFERRED, PduR_ConfigType.' },
      { heading: 'PduR Error Handling', content: 'PDUR_E_PARAM_POINTER, PDUR_E_PARAM_CONFIG, PDUR_E_INVALID_REQUEST, PDUR_E_PDU_ID_INVALID, PDUR_E_ROUTING_PATH_GROUP_INVALID, PDUR_E_UNINIT, PDUR_E_INVALID_BUFFER_LENGTH, PDUR_E_BUFFER_REQUEST_SDU_FAILED.' },
      { heading: 'PduR Configuration', content: 'PDUR_DEV_ERROR_DETECT, PDUR_VERSION_INFO_API, PDUR_NUMBER_OF_ROUTING_PATHS, PDUR_NUMBER_OF_ROUTING_PATH_GROUPS, PDUR_MAX_DESTINATIONS_PER_PATH, PDUR_FIFO_DEPTH, PDUR_MAIN_FUNCTION_PERIOD_MS.' },
      { heading: 'PduR Scenario: COM to CAN Transmit Routing', content: 'Engine status signal from COM is routed to CanIf for transmission on the CAN bus. COM calls PduR_ComTransmit, PduR looks up routing path, finds CANIF destination, calls CanIf_Transmit.' },
      { heading: 'PduR Scenario: CAN Receive to COM Routing', content: 'Engine command received from CAN bus is routed to COM. CanIf receives frame and calls PduR_CanIfRxIndication, PduR finds destination COM, calls Com_RxIndication.' },
      { heading: 'PduR Scenario: Transmit Confirmation Routing', content: 'CAN transmit confirmation is routed back to COM. CanIf calls PduR_CanIfTxConfirmation, PduR finds destination COM, calls Com_TxConfirmation.' },
      { heading: 'PduR Dependencies', content: 'Upper layers: Com, Dcm. Lower layers: CanIf, LinIf, EthIf. Common: Det, MemMap.' },
    ],
  },
  {
    title: 'Com Specification',
    path: '/module/services/com',
    sections: [
      { heading: 'Com Module Overview', content: 'Com is a core Service Layer module providing signal-based communication services between application software components via RTE and the underlying PDU-based transport layer via PduR.' },
      { heading: 'Com Key Responsibilities', content: 'Pack and unpack signals into from I-PDUs. Support signal and signal group transmission and reception. Support transmission modes DIRECT PERIODIC MIXED NONE. Support filter algorithms endianness conversion update bits I-PDU group control deadline monitoring notification callbacks signal gateway routing.' },
      { heading: 'Com Signal APIs', content: 'Com_SendSignal sends a signal by packing it into the associated I-PDU. Com_ReceiveSignal receives a signal by unpacking it from the I-PDU. Com_InvalidateSignal invalidates a signal.' },
      { heading: 'Com Signal Group APIs', content: 'Com_SendSignalGroup sends a signal group from shadow buffer to I-PDU. Com_ReceiveSignalGroup receives a signal group. Com_UpdateShadowSignal updates a shadow signal. Com_ReceiveShadowSignal receives a shadow signal.' },
      { heading: 'Com I-PDU Control APIs', content: 'Com_TriggerIPDUSend triggers immediate transmission. Com_IpduGroupControl controls starts stops I-PDU groups. Com_SwitchIpduTxMode switches transmission mode.' },
      { heading: 'Com Data Types', content: 'Com_SignalIdType uint16, Com_SignalGroupIdType uint16, Com_IpduGroupIdType uint16, Com_ConfigType, Com_SignalConfigType with SignalId BitPosition BitSize Endianness TransferProperty FilterAlgorithm, Com_IPduConfigType with PduId DataLength RepeatingEnabled TimePeriod.' },
      { heading: 'Com Error Handling', content: 'COM_E_PARAM, COM_E_UNINIT, COM_E_PARAM_POINTER, COM_E_INVALID_SIGNAL_ID, COM_E_INVALID_SIGNAL_GROUP_ID, COM_E_INVALID_IPDU_ID.' },
      { heading: 'Com Configuration', content: 'COM_DEV_ERROR_DETECT, COM_VERSION_INFO_API, COM_NUM_SIGNALS, COM_NUM_GROUP_SIGNALS, COM_NUM_IPDUS, COM_NUM_IPDU_GROUPS, COM_MAX_IPDU_BUFFER_SIZE, COM_MAIN_FUNCTION_PERIOD_MS, COM_GATEWAY_SUPPORT.' },
      { heading: 'Com Scenario: Signal Send Direct Transmission', content: 'Application sends engine RPM signal packed into I-PDU and transmitted via PduR. Com validates state and signal ID, applies filter, packs signal, sets update bit, triggers transmission.' },
      { heading: 'Com Scenario: Signal Receive RxIndication', content: 'CAN frame received and vehicle speed signal extracted. PduR routes to Com_RxIndication, Com copies data to I-PDU buffer, application calls Com_ReceiveSignal to unpack.' },
      { heading: 'Com Scenario: Signal Group Transmission', content: 'Application updates multiple related signals in a signal group and sends them atomically using shadow buffer.' },
      { heading: 'Com Scenario: Periodic Transmission Mixed Mode', content: 'I-PDU configured with MIXED mode sent periodically with event-triggered retransmissions. Com_MainFunctionTx handles periodic timing.' },
      { heading: 'Com Dependencies', content: 'Upper: RTE. Lower: PduR. Common: Det, MemMap, Os.' },
    ],
  },
  {
    title: 'NvM Specification',
    path: '/module/services/nvm',
    sections: [
      { heading: 'NvM Module Overview', content: 'NvM is a core Service Layer module managing non-volatile memory data, providing asynchronous read write services, block management, data consistency protection, and error recovery.' },
      { heading: 'NvM Block Types', content: 'Native Block: single NV block mapped to one RAM mirror. Redundant Block: two NV blocks primary and secondary for automatic failover. Dataset Block: multiple NV datasets associated with a single RAM block selected via NvM_SetDataIndex.' },
      { heading: 'NvM Lifecycle APIs', content: 'NvM_Init, NvM_DeInit, NvM_GetVersionInfo, NvM_MainFunction for asynchronous job processing.' },
      { heading: 'NvM Single Block Operation APIs', content: 'NvM_ReadBlock reads block from NV to RAM. NvM_WriteBlock writes block from RAM to NV. NvM_RestoreBlockDefaults restores ROM defaults. NvM_EraseNvBlock erases block. NvM_InvalidateNvBlock invalidates block.' },
      { heading: 'NvM System-Level APIs', content: 'NvM_ReadAll reads all configured blocks during ECU startup. NvM_WriteAll writes all dirty blocks during shutdown. NvM_ReadPRAMBlock reads permanent RAM block. NvM_WritePRAMBlock writes permanent RAM block.' },
      { heading: 'NvM Protection Control APIs', content: 'NvM_SetBlockLockStatus locks or unlocks block. NvM_SetBlockProtection enables write protection. NvM_SetWriteOnceStatus sets Write-Once protection.' },
      { heading: 'NvM Data Types', content: 'NvM_RequestResultType with NVM_REQ_OK NVM_REQ_NOT_OK NVM_REQ_PENDING NVM_REQ_INTEGRITY_FAILED NVM_REQ_RESTORED_FROM_ROM. NvM_BlockManagementType with NATIVE REDUNDANT DATASET. NvM_BlockCrcType with CRC_NONE CRC_8 CRC_16 CRC_32. NvM_BlockDescriptorType, NvM_ConfigType.' },
      { heading: 'NvM Error Handling', content: 'NVM_E_NOT_INITIALIZED, NVM_E_BLOCK_PENDING, NVM_E_BLOCK_CONFIG, NVM_E_PARAM_BLOCK_ID, NVM_E_PARAM_BLOCK_DATA_IDX, NVM_E_PARAM_POINTER, NVM_E_BLOCK_LOCKED, NVM_E_WRITE_PROTECTED.' },
      { heading: 'NvM Configuration', content: 'NVM_DEV_ERROR_DETECT, NVM_NUM_OF_NVRAM_BLOCKS, NVM_NUM_OF_DATASETS, NVM_NUM_OF_ROM_BLOCKS, NVM_MAX_NUMBER_OF_WRITE_RETRIES, NVM_MAX_NUMBER_OF_READ_RETRIES, NVM_CALC_RAM_BLOCK_CRC, NVM_USE_CRC_COMP_MECHANISM, NVM_MAIN_FUNCTION_PERIOD_MS, NVM_SIZE_STANDARD_JOB_QUEUE, NVM_SIZE_IMMEDIATE_JOB_QUEUE.' },
      { heading: 'NvM Scenario: ECU Startup ReadAll', content: 'During startup NvM_ReadAll restores all configured NV blocks from non-volatile memory into RAM. Failed blocks recovered from ROM defaults.' },
      { heading: 'NvM Scenario: Application Write Native Block', content: 'Application writes updated configuration data to Native NV block asynchronously via standard job queue and MemIf_Write.' },
      { heading: 'NvM Scenario: Redundant Block Recovery', content: 'Primary copy corrupted, NvM detects integrity failure and automatically recovers from secondary redundant copy transparently.' },
      { heading: 'NvM Scenario: Dataset Block A/B Switching', content: 'Switch between calibration parameter sets using NvM_SetDataIndex to select different datasets transparently.' },
      { heading: 'NvM Scenario: WriteOnce Block Protection', content: 'Software version block with WriteOnce protection prevents rollback attacks by allowing only a single write.' },
      { heading: 'NvM Scenario: CRC Failure ROM Default Recovery', content: 'Stored CRC mismatch detected during read, NvM restores safe ROM default data.' },
      { heading: 'NvM Dependencies', content: 'Upper: RTE, Dcm, Dem. Lower: MemIf, Fee, Ea. Common: Det, MemMap.' },
    ],
  },
  {
    title: 'DCM Specification',
    path: '/module/services/dcm',
    sections: [
      { heading: 'DCM Module Overview', content: 'Diagnostic Communication Manager implements UDS Unified Diagnostic Services ISO 14229-1 and OBD-II protocols. Interface between diagnostic tester and ECU internal software components.' },
      { heading: 'DCM Key Responsibilities', content: 'UDS protocol processing, diagnostic session management, security access control with seed key mechanism, diagnostic service dispatching, DID RID management, DTC information access, data transfer for software download upload.' },
      { heading: 'DCM UDS Service List', content: '0x10 Diagnostic Session Control, 0x11 ECU Reset, 0x19 Read DTC Information, 0x22 Read Data By Identifier, 0x2E Write Data By Identifier, 0x31 Routine Control, 0x34 Request Download, 0x36 Transfer Data, 0x37 Request Transfer Exit, 0x3E Tester Present.' },
      { heading: 'DCM Session and Security APIs', content: 'Dcm_GetSecurityLevel, Dcm_GetSesCtrlType, Dcm_ResetToDefaultSession, Dcm_GetActiveDiagnostic, Dcm_SetActiveDiagnostic.' },
      { heading: 'DCM Data Types', content: 'Dcm_SesCtrlType with DEFAULT PROGRAMMING EXTENDED SAFETY sessions. Dcm_ProtocolType with OBD_ON_CAN UDS_ON_CAN UDS_ON_IP. Dcm_NegativeResponseCodeType with service not supported security access denied invalid key. Dcm_DIDConfigType, Dcm_RIDConfigType, Dcm_ConfigType.' },
      { heading: 'DCM Error Handling', content: 'DCM_E_UNINIT, DCM_E_PARAM, DCM_E_PARAM_POINTER, DCM_E_INIT_FAILED, DCM_E_INTERFACE_TIMEOUT, DCM_E_INTERFACE_RETURN_VALUE, DCM_E_INTERFACE_BUFFER_OVERFLOW.' },
      { heading: 'DCM Scenario: Default Session Diagnostics', content: 'Tester reads ECU part number DID 0xF18C in default session. DCM validates request and calls registered read function.' },
      { heading: 'DCM Scenario: Extended Session Security Access', content: 'Tester switches to extended session and performs security access to unlock level 1 via RequestSeed and SendKey.' },
      { heading: 'DCM Scenario: Read DTC Information', content: 'Tester queries confirmed DTCs. DCM iterates through configured DTCs via DEM interface and builds response.' },
      { heading: 'DCM Scenario: Software Flash Programming', content: 'Tester downloads new software using RequestDownload, TransferData, RequestTransferExit, then ECUReset HardReset.' },
      { heading: 'DCM Dependencies', content: 'Upper: Application RTE. Lower: PduR, CanIf, EthIf, LinIf. Peer: Dem, NvM. Common: Det, MemMap.' },
    ],
  },
  {
    title: 'DEM Specification',
    path: '/module/services/dem',
    sections: [
      { heading: 'DEM Module Overview', content: 'Diagnostic Event Manager manages diagnostic events, Diagnostic Trouble Codes DTCs, freeze frames, extended data records, and fault memory operations.' },
      { heading: 'DEM Key Responsibilities', content: 'DTC management, diagnostic event processing, freeze frame storage, extended data records, debounce algorithms counter-based time-based monitor-internal, aging and displacement, operation cycle management.' },
      { heading: 'DEM Event Management APIs', content: 'Dem_SetEventStatus reports event status Passed Failed PrePassed PreFailed. Dem_GetEventStatus gets last reported status. Dem_ResetEventStatus resets event and debounce counter. Dem_GetFaultDetectionCounter gets FDC.' },
      { heading: 'DEM DTC Management APIs', content: 'Dem_GetStatusOfDTC gets UDS status byte. Dem_ClearDTC clears DTC from fault memory. Dem_GetNumberOfFilteredDTC gets matching count. Dem_GetNextFilteredDTC gets next filtered DTC.' },
      { heading: 'DEM Freeze Frame APIs', content: 'Dem_PrestoreFreezeFrame pre-stores snapshot data. Dem_ClearPrestoredFreezeFrame clears it. Dem_GetFreezeFrameDataByDTC retrieves freeze frame by DTC.' },
      { heading: 'DEM Operation Cycle APIs', content: 'Dem_SetOperationCycleState sets cycle start end. Dem_GetOperationCycleState gets state. Dem_RestartOperationCycle restarts cycle.' },
      { heading: 'DEM Data Types', content: 'Dem_EventIdType uint16, Dem_DTCType uint32, Dem_EventStatusType with PASSED FAILED PREPASSED PREFAILED, Dem_UdsStatusByteType uint8, Dem_DTCOriginType with PRIMARY MIRROR PERMANENT OBD memories, Dem_EventStateType, Dem_EventParameterType, Dem_ConfigType.' },
      { heading: 'DEM Debounce Algorithms', content: 'Counter-based debounce uses signed counter incrementing on PreFailed and decrementing on PrePassed. Time-based debounce tracks time since last status change. Monitor-internal delegates debouncing to reporting module.' },
      { heading: 'DEM DTC Status Byte', content: 'Bit 0 TestFailed, Bit 1 TestFailedThisOperationCycle, Bit 2 PendingDTC, Bit 3 ConfirmedDTC, Bit 4 TestNotCompletedSinceLastClear, Bit 5 TestFailedSinceLastClear, Bit 6 TestNotCompletedThisOperationCycle, Bit 7 WarningIndicatorRequested.' },
      { heading: 'DEM Error Handling', content: 'DEM_E_UNINIT, DEM_E_PARAM_POINTER, DEM_E_PARAM_DATA, DEM_E_PARAM_CONFIG, DEM_E_NODATAAVAILABLE, DEM_E_WRONG_CONDITION, DEM_E_WRONG_CONFIGURATION.' },
      { heading: 'DEM Scenario: Event Reporting and Debounce', content: 'SW-C reports sensor fault with PreFailed multiple times until debounce counter reaches failed threshold 127. DTC transitions to confirmed status.' },
      { heading: 'DEM Scenario: DTC Status Update', content: 'Confirmed DTC cleared by diagnostic tester via ClearDiagnosticInformation. Status reset occurrence counter reset aging counter reset freeze frame invalidated.' },
      { heading: 'DEM Scenario: Freeze Frame Storage', content: 'When DTC becomes confirmed for first time DEM stores freeze frame containing snapshot data with configured DID values.' },
      { heading: 'DEM Scenario: Fault Aging', content: 'Confirmed DTC not failed for long time is aged out of fault memory after DEM_AGING_THRESHOLD 40 cycles.' },
      { heading: 'DEM Dependencies', content: 'Upper: DCM, Application SW-Cs. Lower: NvM. Peer: Dcm. Common: Det, MemMap.' },
    ],
  },
  {
    title: 'OS Integration Specification',
    path: '/module/os/ecum',
    sections: [
      { heading: 'OS Integration Overview', content: 'Integration between OS layer and BSW stack covering EcuM phased startup and orderly shutdown, BswM mode arbitration and action execution, OS Alarm cyclic scheduling of BSW MainFunctions.' },
      { heading: 'EcuM Core Lifecycle APIs', content: 'EcuM_Init Phase I initializes MCAL drivers then starts OS never returns. EcuM_StartupTwo Phase II initializes ECUAL layer after OS start. EcuM_StartupThree Phase III initializes Service layer. EcuM_Shutdown orderly shutdown Service ECUAL MCAL.' },
      { heading: 'EcuM State and Wakeup APIs', content: 'EcuM_GetState, EcuM_SetWakeupEvent, EcuM_ClearWakeupEvent, EcuM_ValidateWakeupEvent, EcuM_RequestShutdown, EcuM_RequestSleep, EcuM_MainFunction.' },
      { heading: 'EcuM Startup Sequence', content: 'Reset Vector EcuM_Init Phase I MCAL Init Mcu Port Gpt Can Spi Adc Pwm Wdg StartOS StartupHook EcuM_StartupTwo ECUAL Init CanIf CanTp MemIf Fee Ea EthIf LinIf IoHwAb EcuM_StartupThree Service Init PduR Com NvM Dcm Dem BswM BswM_RequestMode RUN.' },
      { heading: 'EcuM Shutdown Sequence', content: 'EcuM_Shutdown Phase I Service DeInit reverse order BswM PduR Com Dcm EcuM_OnGoOffTwo Phase II ECUAL MCAL DeInit ShutdownOS.' },
      { heading: 'BswM Core APIs', content: 'BswM_Init, BswM_Deinit, BswM_RequestMode, BswM_GetCurrentMode, BswM_GetState, BswM_MainFunction.' },
      { heading: 'BswM Mode Definitions', content: 'BSWM_MODE_STARTUP 0x00, BSWM_MODE_RUN 0x01, BSWM_MODE_POST_RUN 0x02, BSWM_MODE_SHUTDOWN 0x03, BSWM_MODE_SLEEP 0x04.' },
      { heading: 'BswM Mode Arbitration Logic', content: 'Process mode requests, run arbitration evaluating all user modes selecting highest priority, execute mode actions start COM I-PDU groups enable PDU routing for RUN stop groups disable routing for SHUTDOWN, evaluate rules.' },
      { heading: 'OS Alarm Configuration', content: 'OsAlarm_BswM_MainFunction 10ms, OsAlarm_Com_MainFunction 10ms Com_MainFunctionRx Com_MainFunctionTx, OsAlarm_Dcm_MainFunction 10ms, OsAlarm_NvM_MainFunction 100ms, OsAlarm_Dem_MainFunction 100ms.' },
      { heading: 'OS Integration Error Handling', content: 'ECUM_E_UNINIT module not initialized, ECUM_E_STATE_TRANSITION invalid state transition, ECUM_E_PARAM_POINTER null pointer. BSWM_E_UNINIT, BSWM_E_PARAM_CONFIG invalid config pointer, BSWM_E_REQ_USER_OUT_OF_RANGE, BSWM_E_REQ_MODE_OUT_OF_RANGE, BSWM_E_PARAM_POINTER.' },
      { heading: 'OS Integration Scenario: ECU Cold Startup', content: 'Power-on reset triggers complete phased startup. All MCAL drivers initialized in order, OS started, ECUAL and Service layers initialized, BswM requests RUN mode.' },
      { heading: 'OS Integration Scenario: Mode Switch to RUN', content: 'BswM arbitrates mode request from EcuM to switch to RUN. COM I-PDU groups started, PDU routing enabled, rules evaluated.' },
      { heading: 'OS Integration Scenario: OS Alarm Cyclic Scheduling', content: 'OS Alarms expire periodically and dispatch to BSW MainFunctions at configured rates 10ms or 100ms.' },
      { heading: 'OS Integration Scenario: ECU Orderly Shutdown', content: 'Application requests shutdown. Service layer deinitialized in reverse order, then ECUAL and MCAL, ShutdownOS called.' },
    ],
  },
];

for (const doc of specDocs) {
  addEntry({
    title: doc.title,
    content: doc.sections.map((s) => `${s.heading}: ${s.content}`).join(' '),
    path: doc.path,
    category: '文档',
  });
  for (const section of doc.sections) {
    addEntry({
      title: section.heading,
      content: section.content,
      path: doc.path,
      category: '文档',
    });
  }
}
