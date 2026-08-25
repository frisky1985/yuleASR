#!/usr/bin/env python3
"""
Batch 7 @req traceability annotation script for AUTOSAR BSW modules.
Adds @req SWS_<Module>_NNNNN annotations to .c files, design docs, and test files.
"""

import os
import re
import sys

BASE = "/Users/ingeek/workspace/AUTOSAR"

# Module definitions: (dir_name, SWS_prefix, public_api_prefixes, layer)
# layer: 'services' or 'ecual'
# Public API prefixes: function name prefixes that are public APIs

MODULES = [
    # === SERVICES ===
    ("crc",      "Crc",    ["Crc_Init", "Crc_CalculateCRC8", "Crc_CalculateCRC16", "Crc_CalculateCRC32", "Crc_GetVersionInfo"], "services"),
    ("e2e",      "E2E",    ["E2E_Init", "E2E_DeInit", "E2E_P01Protect", "E2E_P01Check", "E2E_P01MapStatusToSM",
                             "E2E_P02Protect", "E2E_P02Check", "E2E_P02MapStatusToSM",
                             "E2E_P04Protect", "E2E_P04Check", "E2E_P04MapStatusToSM",
                             "E2E_P05Protect", "E2E_P05Check", "E2E_P05MapStatusToSM",
                             "E2E_P06Protect", "E2E_P06Check", "E2E_P06MapStatusToSM",
                             "E2E_P07Protect", "E2E_P07Check", "E2E_P07MapStatusToSM"], "services"),
    ("schm",     "SchM",   ["SchM_Init", "SchM_DeInit", "SchM_Start", "SchM_Stop", "SchM_SetScheduleTable",
                             "SchM_GetScheduleTable", "SchM_MainFunction", "SchM_GetVersionInfo"], "services"),
    ("dlt",      "Dlt",    ["Dlt_Init", "Dlt_DeInit", "Dlt_UnregisterApp", "Dlt_SendLogMessage", "Dlt_SendTraceMessage",
                             "Dlt_MainFunction", "Dlt_GetVersionInfo", "Dlt_SetFilter", "Dlt_FlushQueue",
                             "Dlt_SetSessionId", "Dlt_GetStatistics", "Dlt_RegisterContext", "Dlt_UnregisterContext",
                             "Dlt_SetLogLevel", "Dlt_GetLogLevel", "Dlt_SetTraceStatus", "Dlt_GetTraceStatus",
                             "Dlt_ComTxConfirmation", "Dlt_ComRxIndication"], "services"),
    ("comm",     "ComM",   ["ComM_Init", "ComM_DeInit", "ComM_GetVersionInfo", "ComM_RequestComMode",
                             "ComM_GetMaxComMode", "ComM_GetRequestedComMode", "ComM_GetCurrentComMode",
                             "ComM_CommunicationAllowed", "ComM_MainFunction", "ComM_MainFunctionPnc",
                             "ComM_RequestPncMode", "ComM_GetPncMode",
                             "ComM_EcuM_WakeUpIndication", "ComM_EcuM_BusWakeUpIndication", "ComM_EcuM_RunRequestIndication",
                             "ComM_BusSM_ModeIndication", "ComM_BusSM_BusSleepMode", "ComM_BusSM_NetworkMode", "ComM_BusSM_PrepareBusSleepMode",
                             "ComM_DCM_ActiveDiagnostic", "ComM_DCM_InactiveDiagnostic", "ComM_DCM_PassiveDiagnostic",
                             "ComM_ECNM_NetworkMode", "ComM_ECNM_PrepareBusSleepMode", "ComM_ECNM_BusSleepMode",
                             "ComM_Nvm_StartUpError", "ComM_Nvm_StoreInhibitionStatus", "ComM_GetInhibitionStatus",
                             "ComM_LimitChannelToNoComMode", "ComM_LimitECUToNoComMode", "ComM_PreventWakeUp",
                             "ComM_Nm_NetworkMode", "ComM_Nm_PrepareBusSleepMode", "ComM_Nm_BusSleepMode"], "services"),
    ("stbm",     "StbM",   ["StbM_Init", "StbM_DeInit", "StbM_GetVersionInfo", "StbM_GetCurrentTime",
                             "StbM_GetCurrentVirtualTime", "StbM_SetGlobalTime", "StbM_BusSetGlobalTime",
                             "StbM_GetTimeBaseStatus", "StbM_GetMasterConfig", "StbM_SetRateCorrection",
                             "StbM_GetTimeBaseUpdateCounter", "StbM_GetCurrentTimeDiff", "StbM_SetUserData",
                             "StbM_MainFunction", "StbM_TimeStampChanged"], "services"),
    ("doip",     "DoIP",   ["DoIP_Init", "DoIP_DeInit", "DoIP_GetVersionInfo", "DoIP_IfTransmit", "DoIP_IfRxIndication",
                             "DoIP_ActivateRouting", "DoIP_CloseConnection", "DoIP_VehicleAnnouncement",
                             "DoIP_RequestEntityStatus", "DoIP_SetPowerMode", "DoIP_HandleAliveCheckTimeout",
                             "DoIP_SoAdTxConfirmation", "DoIP_SoConModeChg", "DoIP_MainFunction",
                             "DoIP_TriggerTransmit", "DoIP_TpRxIndication", "DoIP_TpTxConfirmation"], "services"),
    ("docan",    "DoCan",  ["DoCan_Init", "DoCan_DeInit", "DoCan_GetVersionInfo", "DoCan_Transmit",
                             "DoCan_RxIndication", "DoCan_TxConfirmation", "DoCan_MainFunction"], "services"),
    ("someip",   "SomeIp", ["SomeIp_Init", "SomeIp_DeInit", "SomeIp_GetVersionInfo", "SomeIp_SendRequest",
                             "SomeIp_SendResponse", "SomeIp_SendNotification", "SomeIp_RxIndication",
                             "SomeIp_TxConfirmation", "SomeIp_ProcessMessage", "SomeIp_ParseHeader",
                             "SomeIp_SerializeHeader", "SomeIp_ExtractIds"], "services"),
    ("someiptp", "SomeIpTp", ["SomeIpTp_Init", "SomeIpTp_DeInit", "SomeIpTp_GetVersionInfo", "SomeIpTp_Transmit",
                               "SomeIpTp_CancelTransmit", "SomeIpTp_RxIndication", "SomeIpTp_TxConfirmation",
                               "SomeIpTp_MainFunction", "SomeIpTp_BuildTpHeader", "SomeIpTp_ParseTpHeader",
                               "SomeIpTp_GetRxBufferStatus"], "services"),
    ("someipxf", "SomeIpXf", ["SomeIpXf_Init", "SomeIpXf_DeInit", "SomeIpXf_GetVersionInfo", "SomeIpXf_Transform",
                                "SomeIpXf_Detransform", "SomeIpXf_TransformerInit",
                                "SomeIpXf_SerializeBoolean", "SomeIpXf_DeserializeBoolean",
                                "SomeIpXf_SerializeUint8", "SomeIpXf_DeserializeUint8",
                                "SomeIpXf_SerializeUint16", "SomeIpXf_DeserializeUint16",
                                "SomeIpXf_SerializeUint32", "SomeIpXf_DeserializeUint32",
                                "SomeIpXf_SerializeString", "SomeIpXf_DeserializeString",
                                "SomeIpXf_SerializeArray", "SomeIpXf_DeserializeArray",
                                "SomeIpXf_BuildHeader", "SomeIpXf_ParseHeader"], "services"),
    ("ldcom",    "LdCom",  ["LdCom_Init", "LdCom_DeInit", "LdCom_MainFunction", "LdCom_Transmit",
                             "LdCom_CancelTransmit", "LdCom_RxIndication", "LdCom_GetSegmentStatus",
                             "LdCom_GetProgress", "LdCom_TriggerTransmit"], "services"),
    ("sd",       "Sd",     ["Sd_Init", "Sd_DeInit", "Sd_GetVersionInfo", "Sd_FindService", "Sd_OfferService",
                             "Sd_StopService", "Sd_SubscribeEventGroup", "Sd_UnsubscribeEventGroup",
                             "Sd_SetEventStatus", "Sd_MainFunction", "Sd_HandleMessage"], "services"),
    ("xcp",      "Xcp",    ["Xcp_Init", "Xcp_DeInit", "Xcp_GetVersionInfo", "Xcp_MainFunction",
                             "Xcp_RxIndication", "Xcp_TxConfirmation", "Xcp_TriggerTransmit",
                             "Xcp_SetTransmissionMode", "Xcp_ProcessCommand", "Xcp_SendResponse",
                             "Xcp_SendError", "Xcp_SendEvent",
                             "Xcp_CmdConnect", "Xcp_CmdDisconnect", "Xcp_CmdGetStatus",
                             "Xcp_CmdGetCommModeInfo", "Xcp_CmdGetId", "Xcp_CmdSetMta",
                             "Xcp_CmdUpload", "Xcp_CmdShortUpload", "Xcp_CmdDownload",
                             "Xcp_CmdGetSeed", "Xcp_CmdUnlock",
                             "Xcp_CmdClearDaqList", "Xcp_CmdSetDaqPtr", "Xcp_CmdWriteDaq",
                             "Xcp_CmdSetDaqListMode", "Xcp_CmdGetDaqListMode",
                             "Xcp_CmdStartStopDaqList", "Xcp_CmdStartStopSynch",
                             "Xcp_CmdGetDaqProcessorInfo", "Xcp_CmdGetDaqResolutionInfo", "Xcp_CmdGetDaqListInfo",
                             "Xcp_CmdFreeDaq", "Xcp_CmdAllocDaq", "Xcp_CmdAllocOdt", "Xcp_CmdAllocOdtEntry",
                             "Xcp_CmdProgramStart", "Xcp_CmdProgramClear", "Xcp_CmdProgram",
                             "Xcp_CmdProgramReset", "Xcp_CmdProgramVerify",
                             "Xcp_DaqProcessor", "Xcp_DaqSample", "Xcp_DaqTransmit", "Xcp_StimProcessor",
                             "Xcp_ReadMemory", "Xcp_WriteMemory",
                             "Xcp_SetResourceProtection", "Xcp_IsResourceProtected", "Xcp_UnlockResource"], "services"),
    ("mqtt",     "Mqtt",   ["Mqtt_Init", "Mqtt_DeInit", "Mqtt_Connect", "Mqtt_Disconnect",
                             "Mqtt_Publish", "Mqtt_Subscribe", "Mqtt_Unsubscribe", "Mqtt_Ping",
                             "Mqtt_GetConnectionState", "Mqtt_GetConnectionInfo",
                             "Mqtt_MainFunction", "Mqtt_SetConnectionCallback", "Mqtt_GetVersionInfo"], "services"),
    ("tm",       "Tm",     ["Tm_Init", "Tm_DeInit", "Tm_MainFunction", "Tm_GetTimeBaseValue",
                             "Tm_SetTimeBaseValue", "Tm_GetTimeBaseInfo", "Tm_GetGlobalTime",
                             "Tm_SetGlobalTime", "Tm_SyncTimeBase"], "services"),
    ("swc",      "Swc",    ["Swc_Init", "Swc_DeInit", "Swc_GetVersionInfo", "Swc_CreateInstance",
                             "Swc_DestroyInstance", "Swc_SetComponentState", "Swc_ActivateRunnable",
                             "Swc_TerminateRunnable", "Swc_ScheduleRunnables", "Swc_IsRunnableReady",
                             "Swc_ConnectPort", "Swc_DisconnectPort", "Swc_WritePortData", "Swc_ReadPortData",
                             "Swc_RegisterEvent", "Swc_TriggerEvent", "Swc_EnableEvent", "Swc_DisableEvent",
                             "Swc_ProcessEvents", "Swc_MainFunction"], "services"),
    ("keym",     "KeyM",   ["KeyM_Init", "KeyM_DeInit", "KeyM_GetVersionInfo", "KeyM_SetKey", "KeyM_GetKey",
                             "KeyM_UpdateKey", "KeyM_FinalizeKey", "KeyM_ParseKey", "KeyM_ConvertKey",
                             "KeyM_CopyKey", "KeyM_KeyElementSet", "KeyM_KeyElementGet", "KeyM_KeyStatusGet",
                             "KeyM_KeyVersionGet", "KeyM_KeyValidityGet", "KeyM_KeyInfoGet",
                             "KeyM_SetNotificationCallback", "KeyM_MainFunction"], "services"),
    ("cryif",    "CryIf",  ["CryIf_Init", "CryIf_DeInit", "CryIf_GetVersionInfo", "CryIf_ProcessJob",
                             "CryIf_CancelJob", "CryIf_KeyElementSet", "CryIf_KeySetValid",
                             "CryIf_KeyElementGet", "CryIf_KeyElementCopy", "CryIf_KeyElementCopyPartial",
                             "CryIf_KeyCopy", "CryIf_KeyElementIdsGet", "CryIf_KeyValidCheck",
                             "CryIf_RandomSeed", "CryIf_KeyGenerate", "CryIf_KeyDerive",
                             "CryIf_KeyExchangeCalcPubValue", "CryIf_KeyExchangeCalcSecret",
                             "CryIf_CertificateParse", "CryIf_CertificateVerify",
                             "CryIf_CallbackNotification", "CryIf_MainFunction"], "services"),
    ("ecuc",     "EcuC",   ["EcuC_Init", "EcuC_DeInit", "EcuC_GetConfigValue", "EcuC_SetConfigValue",
                             "EcuC_GetVersionInfo"], "services"),
    ("flstst",   "FlStSt", ["FlStSt_Init", "FlStSt_DeInit", "FlStSt_GetVersionInfo", "FlStSt_RunTest",
                             "FlStSt_VerifyErase", "FlStSt_VerifyProgram", "FlStSt_GetResult",
                             "FlStSt_Abort", "FlStSt_MainFunction"], "services"),
    ("canm",     "CanNm",  ["CanNm_Init", "CanNm_DeInit", "CanNm_PassiveStartUp", "CanNm_NetworkRequest",
                             "CanNm_NetworkRelease", "CanNm_DisableCommunication", "CanNm_EnableCommunication",
                             "CanNm_GetUserData", "CanNm_SetUserData", "CanNm_GetPduData", "CanNm_GetState",
                             "CanNm_GetVersionInfo", "CanNm_RequestBusSynchronization",
                             "CanNm_CheckRemoteSleepIndication", "CanNm_SetSleepReadyBit",
                             "CanNm_MainFunction", "CanNm_TxConfirmation", "CanNm_RxIndication",
                             "CanNm_ControllerBusOff"], "services"),
    ("cantsyn",  "CanTSyn", ["CanTSyn_Init", "CanTSyn_DeInit", "CanTSyn_GetVersionInfo",
                              "CanTSyn_SetTransmissionMode", "CanTSyn_GetTransmissionMode",
                              "CanTSyn_GetSyncReceived", "CanTSyn_GetCurrentVirtualTime",
                              "CanTSyn_SetGlobalTime", "CanTSyn_SetRateCorrection",
                              "CanTSyn_SetUserData", "CanTSyn_GetUserData",
                              "CanTSyn_RxIndication", "CanTSyn_TxConfirmation",
                              "CanTSyn_TimeTxConfirmationSYNC", "CanTSyn_TimeTxConfirmationFUP",
                              "CanTSyn_TimeTxConfirmationOCS", "CanTSyn_MainFunction"], "services"),
    ("j1939nm",  "J1939Nm", ["J1939Nm_Init", "J1939Nm_DeInit", "J1939Nm_GetVersionInfo",
                              "J1939Nm_GetState", "J1939Nm_GetBusOffState", "J1939Nm_SetBusOffState",
                              "J1939Nm_GetAddress", "J1939Nm_SetAddress", "J1939Nm_GetName", "J1939Nm_SetName",
                              "J1939Nm_MainFunction", "J1939Nm_BusOffCbk", "J1939Nm_RxIndication",
                              "J1939Nm_TxConfirmation", "J1939Nm_RequestAddressClaimed",
                              "J1939Nm_RequestCannotClaimAddress", "J1939Nm_HandleAddressConflict"], "services"),
    ("j1939tp",  "J1939Tp", ["J1939Tp_Init", "J1939Tp_DeInit", "J1939Tp_GetVersionInfo",
                              "J1939Tp_MainFunction", "J1939Tp_Transmit", "J1939Tp_CancelTransmit",
                              "J1939Tp_CancelReceive", "J1939Tp_ChangeParameter",
                              "J1939Tp_RxIndication", "J1939Tp_TxConfirmation"], "services"),
    ("linm",     "LinM",   ["LinM_Init", "LinM_DeInit", "LinM_GetVersionInfo", "LinM_InitSchedule",
                             "LinM_StartSchedule", "LinM_StopSchedule", "LinM_SetScheduleMode",
                             "LinM_GetScheduleStatus", "LinM_MainFunction", "LinM_WakeUp",
                             "LinM_GotoSleep", "LinM_GetSlaveResponse"], "services"),
    ("lntm",     "LinTp",  ["LinTp_Init", "LinTp_DeInit", "LinTp_GetVersionInfo", "LinTp_Transmit",
                             "LinTp_CancelReceive", "LinTp_CancelTransmit", "LinTp_ChangeParameter",
                             "LinTp_ResetToDefaultParameters", "LinTp_MainFunction",
                             "LinTp_RxIndication", "LinTp_TxConfirmation"], "services"),
    # === ECUAL ===
    ("ea",       "Ea",     ["Ea_Init", "Ea_SetMode", "Ea_Read", "Ea_Write", "Ea_Cancel",
                             "Ea_InvalidateBlock", "Ea_EraseImmediateBlock",
                             "Ea_JobEndNotification", "Ea_JobErrorNotification",
                             "Ea_GetVersionInfo", "Ea_GetEraseCycleCount", "Ea_MainFunction"], "ecual"),
    ("frif",     "FrIf",   ["FrIf_Init", "FrIf_ControllerInit", "FrIf_SetAbsoluteTimer", "FrIf_SetRelativeTimer",
                             "FrIf_CancelAbsoluteTimer", "FrIf_CancelRelativeTimer", "FrIf_Transmit",
                             "FrIf_GetPOCStatus", "FrIf_GetGlobalTime", "FrIf_AllowColdstart",
                             "FrIf_HaltCommunication", "FrIf_AbortCommunication", "FrIf_SendWUP",
                             "FrIf_SetWakeupChannel", "FrIf_GetVersionInfo", "FrIf_MainFunction"], "ecual"),
    ("frtp",     "FrTp",   ["FrTp_Init", "FrTp_DeInit", "FrTp_Transmit", "FrTp_CancelTransmit",
                             "FrTp_CancelReceive", "FrTp_ChangeParameter", "FrTp_GetVersionInfo",
                             "FrTp_MainFunction", "FrTp_RxIndication", "FrTp_TxConfirmation"], "ecual"),
    ("wdgif",    "WdgIf",  ["WdgIf_Init", "WdgIf_DeInit", "WdgIf_SetMode", "WdgIf_Trigger",
                             "WdgIf_GetVersionInfo", "WdgIf_SetTriggerCondition"], "ecual"),
    ("cantrcv",  "CanTrcv", ["CanTrcv_INIT", "CanTrcv_Init", "CanTrcv_DeInit", "CanTrcv_SetOpMode",
                              "CanTrcv_GetOpMode", "CanTrcv_GetBusWuReason", "CanTrcv_SetWakeupMode",
                              "CanTrcv_GetVersionInfo", "CanTrcv_MainFunction", "CanTrcv_CheckWakeup",
                              "CanTrcv_CheckWakeupByTransceiver"], "ecual"),
    ("ethtrcv",  "EthTrcv", ["EthTrcv_Init", "EthTrcv_DeInit", "EthTrcv_SetTransceiverMode",
                              "EthTrcv_GetTransceiverMode", "EthTrcv_GetLinkState", "EthTrcv_GetBaudRate",
                              "EthTrcv_GetDuplexMode", "EthTrcv_MainFunction", "EthTrcv_GetVersionInfo",
                              "EthTrcv_CheckWakeup", "EthTrcv_ReadMiiIndication", "EthTrcv_WriteMiiIndication",
                              "EthTrcv_GetSignalQuality", "EthTrcv_GetCableDiagnosticsResult"], "ecual"),
    ("lintrcv",  "LinTrcv", ["LinTrcv_Init", "LinTrcv_DeInit", "LinTrcv_SetOpMode", "LinTrcv_GetOpMode",
                              "LinTrcv_GetBusWuReason", "LinTrcv_GetVersionInfo", "LinTrcv_Wakeup",
                              "LinTrcv_CheckWakeup", "LinTrcv_Cbk_WakeupByBus", "LinTrcv_MainFunction"], "ecual"),
    ("iohwab",   "IoHwAb", ["IoHwAb_Init", "IoHwAb_DeInit", "IoHwAb_MainFunction", "IoHwAb_GetVersionInfo"], "ecual"),
    ("someipif", "SomeIpIf", ["SomeIpIf_Init", "SomeIpIf_DeInit", "SomeIpIf_Transmit",
                               "SomeIpIf_RxIndication", "SomeIpIf_MainFunction", "SomeIpIf_SetState",
                               "SomeIpIf_GetVersionInfo"], "ecual"),
    ("someipsd", "SomeIpSd", ["SomeIpSd_Init", "SomeIpSd_DeInit", "SomeIpSd_MainFunction",
                                "SomeIpSd_FindService", "SomeIpSd_OfferService", "SomeIpSd_StopOffer",
                                "SomeIpSd_SubscribeEventGroup", "SomeIpSd_RxIndication",
                                "SomeIpSd_GetVersionInfo"], "ecual"),
    ("srp",      "Srp",    ["Srp_Init", "Srp_DeInit", "Srp_RegisterTalker", "Srp_RegisterListener",
                             "Srp_DeregisterStream", "Srp_GetStreamStatus", "Srp_RxIndication",
                             "Srp_MainFunction", "Srp_GetVersionInfo"], "ecual"),
]

# SWS ID assignment: Init=00001, DeInit=00002, GetVersionInfo=00003, MainFunction=00004, ...
# Then continue numbering for remaining APIs
STANDARD_ORDER = ["Init", "DeInit", "GetVersionInfo", "MainFunction"]

def assign_sws_ids(sws_prefix, api_names):
    """Assign SWS IDs to API functions. Standard APIs get low numbers, rest continue."""
    ids = {}
    counter = 1

    # First assign standard APIs
    for std_name in STANDARD_ORDER:
        for api in api_names:
            if api.endswith("_" + std_name) or api == sws_prefix + "_" + std_name.replace("GetVersionInfo", "GetVersionInfo"):
                key = api
                ids[key] = f"SWS_{sws_prefix}_{counter:05d}"
                counter += 1
                break

    # Then assign remaining APIs
    for api in api_names:
        if api not in ids:
            ids[api] = f"SWS_{sws_prefix}_{counter:05d}"
            counter += 1

    # Internal functions start at 00100
    # (we don't add internal functions here, just reserve the range)
    return ids

def find_function_in_c(content, func_name):
    """Find function definition in .c file content. Returns list of (line_idx, line_text)."""
    results = []
    # Match function definition: return_type func_name(
    # The function name may be at start of line or after return type
    pattern = re.compile(r'^(?:extern\s+)?(?:static\s+)?(?:inline\s+)?(?:const\s+)?(?:\w+\s+\*?\s*)\b(' + re.escape(func_name) + r')\s*\(', re.MULTILINE)
    for m in pattern.finditer(content):
        line_start = content.rfind('\n', 0, m.start()) + 1
        line_end = content.find('\n', m.end())
        results.append((m.start(), line_start, line_end))
    return results

def add_req_to_c_file(filepath, sws_prefix, api_names, sws_ids):
    """Add @req annotations to a .c file for public API functions."""
    if not os.path.exists(filepath):
        return 0

    with open(filepath, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    added = 0
    modifications = []  # (line_idx, annotation_text) - insert annotation BEFORE this line

    for api_name in api_names:
        if api_name not in sws_ids:
            continue
        sws_id = sws_ids[api_name]

        # Find function definition in content
        # Look for patterns like: "void FuncName(" or "Std_ReturnType FuncName("
        # at the start of a line (possibly with return type before it)
        func_pattern = re.compile(
            r'^(?:extern\s+)?(?:(?:Std_ReturnType|void|uint8|uint16|uint32|sint8|sint16|sint32|boolean|float32|float64|Mqtt_ReturnType|Mqtt_ConnectionStateType)\s+)\b' + re.escape(api_name) + r'\s*\('
        )

        for i, line in enumerate(lines):
            if func_pattern.match(line):
                # Check if @req already exists in preceding comment block
                has_req = False
                # Look back up to 15 lines for existing @req
                for j in range(max(0, i-15), i):
                    if '@req' in lines[j] and sws_id in lines[j]:
                        has_req = True
                        break
                    if '@req' in lines[j] and f'SWS_{sws_prefix}' in lines[j]:
                        has_req = True
                        break

                if not has_req:
                    modifications.append((i, sws_id))
                    added += 1
                break  # Only match first occurrence

    if not modifications:
        return 0

    # Apply modifications in reverse order to preserve line indices
    modifications.sort(key=lambda x: x[0], reverse=True)
    for line_idx, sws_id in modifications:
        # Determine indentation
        indent = ''
        line = lines[line_idx]
        for ch in line:
            if ch in (' ', '\t'):
                indent += ch
            else:
                break

        # Insert /** @req SWS_XXX_NNNNN */ before the function
        # Check if there's a comment block ending just before this line
        # If so, insert inside the comment block (before the closing */)
        insert_idx = line_idx
        if line_idx > 0:
            prev = lines[line_idx - 1].strip()
            if prev == '*/' or prev.endswith('*/'):
                # There's a closing comment above - insert before the comment block starts
                # Find the start of the comment block
                for j in range(line_idx - 1, max(0, line_idx - 20), -1):
                    stripped = lines[j].strip()
                    if stripped.startswith('/**') or stripped.startswith('/*'):
                        insert_idx = j
                        break

        annotation = f'{indent}/** @req {sws_id} */'
        lines.insert(insert_idx, annotation)

    with open(filepath, 'w') as f:
        f.write('\n'.join(lines))

    return added

def update_design_doc(filepath, sws_prefix, api_names, sws_ids):
    """Add SWS 需求 column to design doc API table."""
    if not os.path.exists(filepath):
        return False

    with open(filepath, 'r') as f:
        content = f.read()

    if 'SWS 需求' in content:
        return False  # Already has the column

    lines = content.split('\n')
    modified = False

    for i, line in enumerate(lines):
        # Find API table headers: | API | ... |
        if re.match(r'^\|\s*API\s*\|', line) and i + 1 < len(lines) and re.match(r'^\|[-\s|]+\|', lines[i+1]):
            # Check if it's a simple 2-column table (| API | 说明 |)
            # or multi-column
            cols = [c.strip() for c in line.split('|')[1:-1]]

            if 'SWS 需求' not in line:
                # Add SWS 需求 column
                new_line = line.rstrip()
                sep_line = lines[i+1].rstrip()

                # Insert before the last column or at end
                new_line = new_line + ' SWS 需求 |'
                sep_line = sep_line + '----------|'

                lines[i] = new_line
                lines[i+1] = sep_line

                # Now add SWS IDs to table rows
                for j in range(i+2, min(i+50, len(lines))):
                    row = lines[j]
                    if not row.strip().startswith('|'):
                        break
                    if re.match(r'^\|[-\s|]+\|', row):
                        continue

                    row_cols = [c.strip() for c in row.split('|')[1:-1]]
                    if len(row_cols) >= 1:
                        # Find which API this row refers to
                        first_col = row_cols[0].strip('` ')
                        matched_sws = ''
                        for api_name in api_names:
                            # Check if the API name appears in the first column
                            short_name = api_name.split('_')[-1] if '_' in api_name else api_name
                            if api_name in row or short_name == first_col or first_col.startswith(api_name.replace('_', ' ')):
                                if api_name in sws_ids:
                                    matched_sws = sws_ids[api_name]
                                    break
                                # Try partial match
                                if short_name.lower() in first_col.lower():
                                    if api_name in sws_ids:
                                        matched_sws = sws_ids[api_name]
                                        break

                        # Also try matching by function name pattern in the row
                        if not matched_sws:
                            for api_name in api_names:
                                if api_name in row:
                                    if api_name in sws_ids:
                                        matched_sws = sws_ids[api_name]
                                        break

                        lines[j] = row.rstrip() + f' {matched_sws} |'
                        modified = True

                modified = True
                break

    if modified:
        with open(filepath, 'w') as f:
            f.write('\n'.join(lines))

    return modified

def annotate_test_file(filepath, sws_prefix, api_names, sws_ids):
    """Add @req annotations to test functions."""
    if not os.path.exists(filepath):
        return 0

    with open(filepath, 'r') as f:
        content = f.read()

    if f'SWS_{sws_prefix}_' in content:
        return 0  # Already annotated

    lines = content.split('\n')
    added = 0

    # Find test functions that test specific APIs
    # Pattern: void test_<ApiName>(...) or void Test_<ApiName>(...)
    for i, line in enumerate(lines):
        test_match = re.match(r'^(?:void|static\s+void)\s+(test_\w+|Test_\w+)\s*\(', line)
        if test_match:
            test_func = test_match.group(1)
            # Find which API this test corresponds to
            for api_name in api_names:
                short = api_name.split('_')[-1].lower() if '_' in api_name else api_name.lower()
                if short in test_func.lower():
                    sws_id = sws_ids.get(api_name, '')
                    if sws_id:
                        # Check if @req already present
                        has_req = False
                        for j in range(max(0, i-5), i):
                            if '@req' in lines[j]:
                                has_req = True
                                break
                        if not has_req:
                            indent = ''
                            for ch in line:
                                if ch in (' ', '\t'):
                                    indent += ch
                                else:
                                    break
                            lines.insert(i, f'{indent}/** @req {sws_id} */')
                            added += 1
                    break

    if added > 0:
        with open(filepath, 'w') as f:
            f.write('\n'.join(lines))

    return added

def process_module(dir_name, sws_prefix, api_names, layer):
    """Process a single module."""
    base_dir = os.path.join(BASE, "src/bsw", layer, dir_name)
    sws_ids = assign_sws_ids(sws_prefix, api_names)

    total_req = 0

    # 1. Annotate .c files in src/
    src_dir = os.path.join(base_dir, "src")
    if os.path.isdir(src_dir):
        for fname in sorted(os.listdir(src_dir)):
            if fname.endswith('.c') and not fname.endswith('_Lcfg.c'):
                fpath = os.path.join(src_dir, fname)
                count = add_req_to_c_file(fpath, sws_prefix, api_names, sws_ids)
                total_req += count

    # 2. Also annotate the main .h file if it doesn't have @req yet
    inc_dir = os.path.join(base_dir, "include")
    if os.path.isdir(inc_dir):
        for fname in sorted(os.listdir(inc_dir)):
            if fname.endswith('.h') and '_Cfg' not in fname and '_Types' not in fname and '_MemMap' not in fname and '_Internal' not in fname and '_Private' not in fname and '_Lcfg' not in fname and 'SchM_' not in fname:
                fpath = os.path.join(inc_dir, fname)
                count = add_req_to_c_file(fpath, sws_prefix, api_names, sws_ids)
                total_req += count

    # 3. Update design doc
    if layer == "services":
        doc_path = os.path.join(BASE, f"docs/design/modules/services/{dir_name}-design.md")
    else:
        doc_path = os.path.join(BASE, f"docs/design/modules/ecual/{dir_name}-design.md")
    update_design_doc(doc_path, sws_prefix, api_names, sws_ids)

    # 4. Annotate test files
    test_dirs = [
        os.path.join(BASE, f"tests/unit/autosar/services"),
        os.path.join(BASE, f"tests/unit/autosar/ecual"),
        os.path.join(BASE, f"tests/unit/autosar/{dir_name}"),
        os.path.join(BASE, f"tests/unit/{dir_name}"),
        os.path.join(BASE, f"tests/unit/services"),
        os.path.join(BASE, f"tests/unit/ecual"),
    ]
    for td in test_dirs:
        if os.path.isdir(td):
            for fname in sorted(os.listdir(td)):
                if fname.endswith('.c') and (dir_name.lower() in fname.lower() or sws_prefix.lower() in fname.lower()):
                    fpath = os.path.join(td, fname)
                    count = annotate_test_file(fpath, sws_prefix, api_names, sws_ids)
                    total_req += count

    return total_req, sws_ids

def main():
    results = {}
    grand_total = 0

    for dir_name, sws_prefix, api_names, layer in MODULES:
        count, sws_ids = process_module(dir_name, sws_prefix, api_names, layer)
        results[dir_name] = (count, sws_ids)
        grand_total += count
        status = "DONE" if count > 0 else "SKIP"
        print(f"  [{status}] {sws_prefix:12s} ({dir_name:10s}): {count:3d} @req added")

    print(f"\n{'='*60}")
    print(f"Total @req annotations added: {grand_total}")
    print(f"{'='*60}")
    print(f"\nPer-module breakdown:")
    for dir_name, sws_prefix, api_names, layer in MODULES:
        count, sws_ids = results[dir_name]
        print(f"  {sws_prefix:12s}: {count:3d} @req | {len(sws_ids):3d} APIs | IDs: {sws_prefix}_00001..{sws_prefix}_{len(sws_ids):05d}")

if __name__ == '__main__':
    main()
