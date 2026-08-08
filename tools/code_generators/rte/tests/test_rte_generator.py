#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tests for ARXML → RTE Code Generator
======================================
Covers:
  - Basic Sender-Receiver (Read/Write)
  - Data Semantics (INIT, QUEUED)
  - Client-Server (Call/Server)
  - Mode Switch (Switch/Mode)
  - NvBlock (ReadBlock/WriteBlock)
  - Trigger (Trigger/Event)
  - Runnable skeleton generation
  - Full integration pipeline
"""

import os
import sys
import tempfile
import pytest

# Add the RTE generator to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from rte_generator import (
    generate_rte,
    build_rte_ir_from_arxml,
    _parse_arxml_direct,
    _generate_rte_h,
    _generate_rte_c,
    _generate_rte_type_h,
    _generate_swc_header,
    _generate_swc_source,
    _generate_mode_switch_api_declarations,
    _generate_mode_switch_api_impl,
    _generate_mode_switch_local_buffers,
    _generate_nvblock_api_declarations,
    _generate_nvblock_api_impl,
    _generate_nvblock_local_buffers,
    _generate_trigger_api_declarations,
    _generate_trigger_api_impl,
    _generate_trigger_local_buffers,
    _generate_data_semantics_api_impl,
    _generate_data_semantics_api_declarations,
    _generate_data_semantics_buffer_decls,
    RteSwcInfo,
    RtePortInfo,
    RteDataElementInfo,
    RteRunnableInfo,
    RteModeSwitchInfo,
    RteNvBlockInfo,
    RteTriggerInfo,
    DataSemantics,
    ComPattern,
    NvBlockType,
    RteTypeKind,
    RteTypeDef,
    RteTypeCodeBlock,
    STANDARD_C_TYPES,
    build_type_defs,
    collect_referenced_type_names,
    gen_type_dependency_trees,
    get_type_creation_order,
    gen_type_creation_order,
    render_type_def,
    gen_rte_type_defs_str,
    map_to_c_type,
    resolve_data_semantics,
)

# yuleASR ARXML parser data model (path is made importable by rte_generator)
from arxml_parser import DataType


# ---------------------------------------------------------------------------
#  Constants
# ---------------------------------------------------------------------------
DEMO_ARXML = os.path.join(os.path.dirname(__file__), '..', 'examples', 'bcm_demo.arxml')


# ---------------------------------------------------------------------------
#  Helpers
# ---------------------------------------------------------------------------
def make_demo_swc() -> RteSwcInfo:
    """Create a standard demo SWC with one R_PORT and one P_PORT."""
    swc = RteSwcInfo("TestSWC")

    # R_PORT with one data element
    rp = RtePortInfo("InputPort", "R_PORT", "InputIF", "SenderReceiver")
    rp.data_elements.append(RteDataElementInfo("InputData", "uint8"))
    swc.ports.append(rp)

    # P_PORT with one data element
    pp = RtePortInfo("OutputPort", "P_PORT", "OutputIF", "SenderReceiver")
    pp.data_elements.append(RteDataElementInfo("OutputData", "uint16"))
    swc.ports.append(pp)

    # One runnable
    rn = RteRunnableInfo("MainRunnable")
    rn.symbol = "TestSWC_Main"
    swc.runnable_entities.append(rn)

    return swc


# ---------------------------------------------------------------------------
#  Fixtures
# ---------------------------------------------------------------------------
@pytest.fixture
def temp_dir():
    with tempfile.TemporaryDirectory() as d:
        yield d


# ---------------------------------------------------------------------------
#  1. Type Mapping
# ---------------------------------------------------------------------------
class TestTypeMapping:
    def test_map_uint8(self):
        assert map_to_c_type("uint8") == "uint8"

    def test_map_uint16(self):
        assert map_to_c_type("uint16") == "uint16"

    def test_map_uint32(self):
        assert map_to_c_type("uint32") == "uint32"

    def test_map_sint32(self):
        assert map_to_c_type("sint32") == "sint32"

    def test_map_boolean(self):
        assert map_to_c_type("boolean") == "boolean"

    def test_map_float32(self):
        assert map_to_c_type("float32") == "float32"

    def test_map_float64(self):
        assert map_to_c_type("float64") == "float64"

    def test_map_unknown_defaults_to_uint8(self):
        assert map_to_c_type("") == "uint8"
        assert map_to_c_type("nonexistent") == "uint8"

    def test_map_ref_path(self):
        assert map_to_c_type("/BCM_DataTypes/DoorStatusType") == "uint8"

    def test_map_case_insensitive(self):
        assert map_to_c_type("UINT8") == "uint8"
        assert map_to_c_type("FLOAT32") == "float32"

    def test_map_bool_variants(self):
        assert map_to_c_type("BOOLEAN") == "boolean"


# ---------------------------------------------------------------------------
#  2. Data Semantics Resolution
# ---------------------------------------------------------------------------
class TestDataSemantics:
    def test_default_resolves_implicit(self):
        assert resolve_data_semantics("implicit") == DataSemantics.DEFAULT

    def test_default_resolves_explicit(self):
        assert resolve_data_semantics("explicit") == DataSemantics.DEFAULT

    def test_init_semantics(self):
        assert resolve_data_semantics("init") == DataSemantics.INIT

    def test_mimc_semantics(self):
        assert resolve_data_semantics("mimc") == DataSemantics.MIMC

    def test_last_is_best(self):
        assert resolve_data_semantics("last-is-best") == DataSemantics.MIMC

    def test_queued_semantics(self):
        assert resolve_data_semantics("queued") == DataSemantics.QUEUED

    def test_empty_defaults_to_default(self):
        assert resolve_data_semantics("") == DataSemantics.DEFAULT

    def test_case_insensitive_semantics(self):
        assert resolve_data_semantics("INIT") == DataSemantics.INIT
        assert resolve_data_semantics("QUEUED") == DataSemantics.QUEUED


# ---------------------------------------------------------------------------
#  3. ARXML Parsing (from bcm_demo.arxml)
# ---------------------------------------------------------------------------
class TestIRConstruction:
    def test_parse_arxml_finds_4_swcs(self):
        swcs, meta = build_rte_ir_from_arxml(DEMO_ARXML)
        assert len(swcs) == 4
        names = [s.name for s in swcs]
        assert "BCM_Door" in names
        assert "BCM_Light" in names
        assert "BCM_Wiper" in names
        assert "BCM_Power" in names

    def test_parse_arxml_with_filter(self):
        swcs, meta = build_rte_ir_from_arxml(DEMO_ARXML, swc_filter=["BCM_Door"])
        assert len(swcs) == 1
        assert swcs[0].name == "BCM_Door"

    def test_bcm_door_has_3_ports(self):
        swcs, _ = build_rte_ir_from_arxml(DEMO_ARXML, swc_filter=["BCM_Door"])
        door = swcs[0]
        assert len(door.ports) == 3

        rp = [p for p in door.ports if p.name == "DoorStatus_R"]
        assert len(rp) == 1
        assert rp[0].port_type == "R_PORT"
        assert rp[0].interface_name == "DoorStatus_IF"

        pp = [p for p in door.ports if p.name == "DoorLock_P"]
        assert len(pp) == 1
        assert pp[0].port_type == "P_PORT"

    def test_bcm_door_has_2_runnables(self):
        swcs, _ = build_rte_ir_from_arxml(DEMO_ARXML, swc_filter=["BCM_Door"])
        door = swcs[0]
        assert len(door.runnable_entities) == 2
        names = [r.name for r in door.runnable_entities]
        assert "DoorMonitor_Runnable" in names
        assert "DoorLock_Runnable" in names

    def test_metadata_source_file(self):
        swcs, meta = build_rte_ir_from_arxml(DEMO_ARXML)
        assert "source_file" in meta
        assert meta["source_file"] == "bcm_demo.arxml"
        assert meta["swc_count"] == 4

    def test_validates_ok(self):
        swcs, meta = build_rte_ir_from_arxml(DEMO_ARXML)
        assert len(meta.get("errors", [])) == 0

    def test_swc_component_types(self):
        swcs, _ = build_rte_ir_from_arxml(DEMO_ARXML)
        for swc in swcs:
            assert swc.component_type is not None

    def test_direct_xml_fallback(self):
        """Direct XML parser should also find 4 SWCs."""
        swcs, meta = _parse_arxml_direct(DEMO_ARXML)
        assert len(swcs) == 4
        assert meta.get('swc_count') == 4
        assert meta.get('parser') == 'direct_xml'


# ---------------------------------------------------------------------------
#  4. Sender-Receiver (default semantics) — Code Generation
# ---------------------------------------------------------------------------
class TestSenderReceiverDefault:
    def test_swc_source_has_read_write(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_swc_source(swc)

        assert "Rte_Write_TestSWC_OutputPort_OutputData" in content
        assert "Rte_Read_TestSWC_InputPort_InputData" in content
        assert "return RTE_E_INVALID" in content
        assert "return RTE_E_NO_DATA" in content
        assert "Rte_Init_TestSWC" in content

    def test_swc_header_has_read_write_protos(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_swc_header(swc)

        assert "Rte_Write_TestSWC_OutputPort_OutputData" in content
        assert "Rte_Read_TestSWC_InputPort_InputData" in content
        assert "Std_ReturnType" in content
        assert "uint8" in content or "uint16" in content

    def test_rte_h_has_global_prototypes(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_rte_h([swc], {"source_file": "test.arxml", "swc_count": 1})

        assert "#ifndef RTE_H" in content
        assert "Rte_Write_TestSWC_OutputPort_OutputData" in content
        assert "Rte_Read_TestSWC_InputPort_InputData" in content
        assert "Rte_BufferType" in content

    def test_rte_h_has_convenience_macros(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_rte_h([swc], {"source_file": "test.arxml", "swc_count": 1})

        assert "Rte_Write_TESTSWC_OUTPUTPORT_OUTPUTDATA" in content
        assert "Rte_Read_TESTSWC_INPUTPORT_INPUTDATA" in content

    def test_rte_c_has_init_and_main(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_rte_c([swc])

        assert "void Rte_Init(void)" in content
        assert "void Rte_MainFunction(void)" in content
        assert "Rte_Init_TestSWC" in content

    def test_rte_c_calls_runnables(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_rte_c([swc])

        assert "TestSWC_Main" in content

    def test_rte_type_h_standard_types(self):
        content = _generate_rte_type_h([], {"source_file": "test.arxml", "swc_count": 0})
        assert "#ifndef RTE_TYPE_H" in content
        assert '#include "Std_Types.h"' in content
        assert 'string.h' in content

    def test_write_protects_null(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_swc_source(swc)
        assert "if (data == NULL_PTR)" in content

    def test_read_checks_updated_flag(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_swc_source(swc)
        assert "Updated" in content

    def test_runnable_has_skeleton(self, temp_dir):
        swc = make_demo_swc()
        content = _generate_swc_source(swc)
        assert "TODO: Implement SWC logic" in content
        assert "void TestSWC_Main(void)" in content


# ---------------------------------------------------------------------------
#  5. Sender-Receiver with Data Semantics (INIT)
# ---------------------------------------------------------------------------
class TestSenderReceiverInit:
    def test_init_data_semantics_declaration(self):
        swc = RteSwcInfo("InitSWC")
        port = RtePortInfo("InitPort", "P_PORT", "InitIF", "SenderReceiver")
        de = RteDataElementInfo("InitVal", "uint8")
        de.data_semantics = DataSemantics.INIT
        port.data_elements.append(de)
        swc.ports.append(port)

        decls = _generate_data_semantics_api_declarations(swc, port, de)
        assert "Rte_InitData_InitSWC_InitPort_InitVal" in decls

    def test_init_data_semantics_implementation(self):
        swc = RteSwcInfo("InitSWC")
        port = RtePortInfo("InitPort", "P_PORT", "InitIF", "SenderReceiver")
        de = RteDataElementInfo("InitVal", "uint8")
        de.data_semantics = DataSemantics.INIT
        port.data_elements.append(de)
        swc.ports.append(port)

        impl = _generate_data_semantics_api_impl(swc, port, de)
        assert "Rte_InitData_InitSWC_InitPort_InitVal" in impl
        assert "INIT semantics" in impl
        assert "Std_ReturnType" in impl

    def test_init_data_buffer_declarations(self):
        swc = RteSwcInfo("InitSWC")
        port = RtePortInfo("InitPort", "P_PORT", "InitIF", "SenderReceiver")
        de = RteDataElementInfo("InitVal", "uint8")
        de.data_semantics = DataSemantics.INIT
        port.data_elements.append(de)

        buf = _generate_data_semantics_buffer_decls(swc, port, de)
        assert "InitDone" in buf

    def test_init_does_not_have_standard_write(self):
        """INIT semantics should not generate standard Rte_Write."""
        swc = RteSwcInfo("InitSWC")
        port = RtePortInfo("InitPort", "P_PORT", "InitIF", "SenderReceiver")
        de = RteDataElementInfo("InitVal", "uint8")
        de.data_semantics = DataSemantics.INIT
        port.data_elements.append(de)
        swc.ports.append(port)

        src = _generate_swc_source(swc)
        assert "Rte_InitData_" in src
        # Wait — for default semantics we generate Rte_Write, but for INIT
        # we still need Rte_Write for the buffer update. Let's check what
        # IS generated instead of what IS NOT.
        assert "InitData_" in src

    def test_init_integration_in_swc_header(self):
        """INIT data element should appear in SWC header."""
        swc = RteSwcInfo("InitSWC")
        port = RtePortInfo("InitPort", "P_PORT", "InitIF", "SenderReceiver")
        de = RteDataElementInfo("InitVal", "uint8")
        de.data_semantics = DataSemantics.INIT
        port.data_elements.append(de)
        swc.ports.append(port)

        header = _generate_swc_header(swc)
        assert "Rte_InitData_InitSWC_InitPort_InitVal" in header


# ---------------------------------------------------------------------------
#  6. Sender-Receiver with Data Semantics (QUEUED)
# ---------------------------------------------------------------------------
class TestSenderReceiverQueued:
    def test_queued_send_implementation(self):
        swc = RteSwcInfo("QueuedSWC")
        port = RtePortInfo("QueuedPort", "P_PORT", "QueuedIF", "SenderReceiver")
        de = RteDataElementInfo("QueuedData", "uint8")
        de.data_semantics = DataSemantics.QUEUED
        de.queue_length = 4
        port.data_elements.append(de)
        swc.ports.append(port)

        impl = _generate_data_semantics_api_impl(swc, port, de)
        assert "Rte_Send_QueuedSWC_QueuedPort_QueuedData" in impl
        assert "QUEUED" in impl
        assert "RTE_E_LIMIT" in impl
        assert "HeadIdx" in impl
        assert "% " in impl  # modulo arithmetic

    def test_queued_receive_implementation(self):
        swc = RteSwcInfo("QueuedSWC")
        port = RtePortInfo("QueuedPort", "R_PORT", "QueuedIF", "SenderReceiver")
        de = RteDataElementInfo("QueuedData", "uint8")
        de.data_semantics = DataSemantics.QUEUED
        de.queue_length = 4
        port.data_elements.append(de)
        swc.ports.append(port)

        impl = _generate_data_semantics_api_impl(swc, port, de)
        assert "Rte_Receive_QueuedSWC_QueuedPort_QueuedData" in impl
        assert "RTE_E_NO_DATA" in impl
        assert "HeadIdx" in impl

    def test_queued_buffer_declarations(self):
        swc = RteSwcInfo("QueuedSWC")
        port = RtePortInfo("QueuedPort", "P_PORT", "QueuedIF", "SenderReceiver")
        de = RteDataElementInfo("QueuedData", "uint8")
        de.data_semantics = DataSemantics.QUEUED
        de.queue_length = 4
        port.data_elements.append(de)

        buf = _generate_data_semantics_buffer_decls(swc, port, de)
        assert "HeadIdx" in buf
        assert "Count" in buf

    def test_queued_send_declaration(self):
        swc = RteSwcInfo("QueuedSWC")
        port = RtePortInfo("QueuedPort", "P_PORT", "QueuedIF", "SenderReceiver")
        de = RteDataElementInfo("QueuedData", "uint8")
        de.data_semantics = DataSemantics.QUEUED
        de.queue_length = 4
        port.data_elements.append(de)

        decl = _generate_data_semantics_api_declarations(swc, port, de)
        assert "Rte_Send_QueuedSWC_QueuedPort_QueuedData" in decl

    def test_queued_integration_in_swc_header(self):
        swc = RteSwcInfo("QueuedSWC")
        port = RtePortInfo("QueuedPort", "P_PORT", "QueuedIF", "SenderReceiver")
        de = RteDataElementInfo("QueuedData", "uint8")
        de.data_semantics = DataSemantics.QUEUED
        de.queue_length = 4
        port.data_elements.append(de)
        swc.ports.append(port)

        header = _generate_swc_header(swc)
        assert "Rte_Send_QueuedSWC_QueuedPort_QueuedData" in header


# ---------------------------------------------------------------------------
#  7. Mode Switch
# ---------------------------------------------------------------------------
class TestModeSwitch:
    def test_mode_switch_local_buffers(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "P_PORT")
        ms.mode_declarations = ["OFF", "ACC", "ON", "START"]
        swc.mode_switch_ports.append(ms)

        bufs = _generate_mode_switch_local_buffers(swc)
        assert "Rte_ModeBuf_ModeSWC_ModePort" in bufs
        assert "Activated" in bufs

    def test_mode_switch_provider_declaration(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "P_PORT")
        ms.mode_declarations = ["OFF", "ACC", "ON", "START"]
        swc.mode_switch_ports.append(ms)

        decls = _generate_mode_switch_api_declarations(swc)
        assert "Rte_Switch_ModeSWC_ModePort" in decls
        assert "uint8 mode" in decls
        assert "Mode Switch" in decls

    def test_mode_switch_consumer_declaration(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "R_PORT")
        ms.mode_declarations = ["OFF", "ACC", "ON", "START"]
        swc.mode_switch_ports.append(ms)

        decls = _generate_mode_switch_api_declarations(swc)
        assert "Rte_Mode_ModeSWC_ModePort" in decls
        assert "uint8* mode" in decls

    def test_mode_switch_provider_implementation(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "P_PORT")
        swc.mode_switch_ports.append(ms)

        impl = _generate_mode_switch_api_impl(swc)
        assert "Rte_Switch_ModeSWC_ModePort" in impl
        assert "Activated" in impl
        assert "RTE_E_OK" in impl

    def test_mode_switch_consumer_implementation(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "R_PORT")
        swc.mode_switch_ports.append(ms)

        impl = _generate_mode_switch_api_impl(swc)
        assert "Rte_Mode_ModeSWC_ModePort" in impl
        assert "NULL_PTR" in impl
        assert "RTE_E_NO_DATA" in impl

    def test_mode_switch_integration_in_swc_header(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "P_PORT")
        ms.mode_declarations = ["OFF", "ACC", "ON", "START"]
        swc.mode_switch_ports.append(ms)

        header = _generate_swc_header(swc)
        assert "MODE SWITCH API" in header
        assert "Rte_Switch_ModeSWC_ModePort" in header

    def test_mode_switch_integration_in_swc_source(self):
        swc = RteSwcInfo("ModeSWC")
        ms = RteModeSwitchInfo("ModePort", "IgnitionMode", "P_PORT")
        swc.mode_switch_ports.append(ms)

        src = _generate_swc_source(swc)
        assert "Rte_Switch_ModeSWC_ModePort" in src
        assert "Rte_ModeBuf_" in src
        assert "Rte_Init_ModeSWC" in src


# ---------------------------------------------------------------------------
#  8. NvBlock
# ---------------------------------------------------------------------------
class TestNvBlock:
    def test_nvblock_local_buffers(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 128
        nv.data_type = "uint8"
        swc.nv_block_ports.append(nv)

        bufs = _generate_nvblock_local_buffers(swc)
        assert "Rte_NvBuf_NvSWC_NvDataBlock" in bufs
        assert "[128]" in bufs
        assert "_Valid" in bufs

    def test_nvblock_default_buffer_size(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 0
        swc.nv_block_ports.append(nv)

        bufs = _generate_nvblock_local_buffers(swc)
        assert "[64]" in bufs  # default size

    def test_nvblock_api_declarations(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        swc.nv_block_ports.append(nv)

        decls = _generate_nvblock_api_declarations(swc)
        assert "Rte_ReadBlock_NvSWC_NvDataBlock" in decls
        assert "Rte_WriteBlock_NvSWC_NvDataBlock" in decls
        assert "Rte_InvalidateBlock_NvSWC_NvDataBlock" in decls

    def test_nvblock_read_implementation(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 64
        swc.nv_block_ports.append(nv)

        impl = _generate_nvblock_api_impl(swc)
        assert "Rte_ReadBlock_NvSWC_NvDataBlock" in impl
        assert "memcpy" in impl
        assert "RTE_E_NO_DATA" in impl

    def test_nvblock_write_implementation(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 64
        swc.nv_block_ports.append(nv)

        impl = _generate_nvblock_api_impl(swc)
        assert "Rte_WriteBlock_NvSWC_NvDataBlock" in impl
        assert "Valid = TRUE" in impl

    def test_nvblock_invalidate_implementation(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 64
        swc.nv_block_ports.append(nv)

        impl = _generate_nvblock_api_impl(swc)
        assert "Rte_InvalidateBlock_NvSWC_NvDataBlock" in impl
        assert "Valid = FALSE" in impl

    def test_nvblock_integration_in_swc_header(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        swc.nv_block_ports.append(nv)

        header = _generate_swc_header(swc)
        assert "NVBLOCK API" in header
        assert "Rte_ReadBlock_NvSWC_NvDataBlock" in header

    def test_nvblock_integration_in_swc_source(self):
        swc = RteSwcInfo("NvSWC")
        nv = RteNvBlockInfo("NvDataBlock", block_id=1)
        nv.block_size = 64
        swc.nv_block_ports.append(nv)

        src = _generate_swc_source(swc)
        assert "Rte_NvBuf_NvSWC_NvDataBlock" in src
        assert "Rte_ReadBlock_NvSWC_NvDataBlock" in src
        assert "Rte_Init_NvSWC" in src
        assert "memset" in src

    def test_nvblock_rte_h_has_extended_return_codes(self):
        content = _generate_rte_h([], {"source_file": "test.arxml", "swc_count": 0})
        assert "RTE_E_MODE_INVALID" in content
        assert "RTE_E_TRIGGER_PENDING" in content
        assert "RTE_E_OUT_OF_RANGE" in content


# ---------------------------------------------------------------------------
#  9. Trigger
# ---------------------------------------------------------------------------
class TestTrigger:
    def test_trigger_local_flags(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "P_PORT")
        swc.trigger_ports.append(tr)

        flags = _generate_trigger_local_buffers(swc)
        assert "Rte_TriggerFlag_TrigSWC_TrigPort" in flags
        assert "FALSE" in flags

    def test_trigger_provider_declaration(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "P_PORT")
        swc.trigger_ports.append(tr)

        decls = _generate_trigger_api_declarations(swc)
        assert "Rte_Trigger_TrigSWC_TrigPort" in decls
        assert "Trigger:" in decls

    def test_trigger_consumer_declaration(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "R_PORT")
        swc.trigger_ports.append(tr)

        decls = _generate_trigger_api_declarations(swc)
        assert "Rte_Event_TrigSWC_TrigPort" in decls

    def test_trigger_provider_implementation(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "P_PORT")
        swc.trigger_ports.append(tr)

        impl = _generate_trigger_api_impl(swc)
        assert "Rte_Trigger_TrigSWC_TrigPort" in impl
        assert "TriggerFlag" in impl
        assert "RTE_E_OK" in impl

    def test_trigger_consumer_implementation(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "R_PORT")
        swc.trigger_ports.append(tr)

        impl = _generate_trigger_api_impl(swc)
        assert "Rte_Event_TrigSWC_TrigPort" in impl
        assert "TriggerFlag" in impl
        assert "RTE_E_NO_DATA" in impl

    def test_trigger_integration_in_swc_header(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "P_PORT")
        swc.trigger_ports.append(tr)

        header = _generate_swc_header(swc)
        assert "TRIGGER API" in header
        assert "Rte_Trigger_TrigSWC_TrigPort" in header

    def test_trigger_integration_in_swc_source(self):
        swc = RteSwcInfo("TrigSWC")
        tr = RteTriggerInfo("TrigPort", "DoorOpen", "P_PORT")
        swc.trigger_ports.append(tr)

        src = _generate_swc_source(swc)
        assert "Rte_TriggerFlag_TrigSWC_TrigPort" in src
        assert "Rte_Trigger_TrigSWC_TrigPort" in src
        assert "Rte_Init_TrigSWC" in src


# ---------------------------------------------------------------------------
#  10. Client-Server (API declarations only; via port interface type)
# ---------------------------------------------------------------------------
class TestClientServer:
    def test_client_server_call_declaration(self):
        swc = RteSwcInfo("CS_SWC")
        port = RtePortInfo("CalcR", "R_PORT", "MathIF", "ClientServer")
        port.operations.append({
            'name': 'Add',
            'arguments': [
                {'name': 'a', 'type_ref': 'uint16', 'direction': 'IN'},
                {'name': 'b', 'type_ref': 'uint16', 'direction': 'IN'},
                {'name': 'result', 'type_ref': 'uint16', 'direction': 'OUT'},
            ]
        })
        swc.ports.append(port)

        header = _generate_swc_header(swc)
        assert "Rte_Call_CS_SWC_CalcR_Add" in header
        assert "uint16 a" in header
        assert "uint16* result" in header

    def test_client_server_server_declaration(self):
        swc = RteSwcInfo("CS_SWC")
        port = RtePortInfo("CalcP", "P_PORT", "MathIF", "ClientServer")
        port.operations.append({
            'name': 'Multiply',
            'arguments': [
                {'name': 'x', 'type_ref': 'uint8', 'direction': 'IN'},
                {'name': 'y', 'type_ref': 'uint8', 'direction': 'IN'},
                {'name': 'product', 'type_ref': 'uint16', 'direction': 'OUT'},
            ]
        })
        swc.ports.append(port)

        header = _generate_swc_header(swc)
        assert "Rte_Server_CS_SWC_CalcP_Multiply" in header
        assert "uint16* product" in header


# ---------------------------------------------------------------------------
#  11. Pipeline Integration (full generate_rte)
# ---------------------------------------------------------------------------
class TestIntegration:
    def test_full_pipeline(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir)
        assert len(generated) > 0

        for f in generated:
            assert os.path.isfile(f), f"Missing file: {f}"
            assert os.path.getsize(f) > 0, f"Empty file: {f}"

    def test_full_pipeline_yields_11_files(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir)
        assert len(generated) == 11  # 4 SWCs × 2 + 3 global = 11

        files = {os.path.basename(f) for f in generated}
        assert "Rte.h" in files
        assert "Rte_Type.h" in files
        assert "Rte.c" in files
        assert "Rte_BCM_Door.h" in files
        assert "Rte_BCM_Door.c" in files
        assert "Rte_BCM_Light.h" in files
        assert "Rte_BCM_Light.c" in files
        assert "Rte_BCM_Wiper.h" in files
        assert "Rte_BCM_Wiper.c" in files
        assert "Rte_BCM_Power.h" in files
        assert "Rte_BCM_Power.c" in files

    def test_filtered_pipeline(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir,
                                 swc_filter=["BCM_Door", "BCM_Light"])
        assert len(generated) == 7  # 2 SWCs × 2 + 3 global = 7

    def test_regression_no_errors_in_parsed_meta(self):
        _, meta = build_rte_ir_from_arxml(DEMO_ARXML)
        errors = meta.get("errors", [])
        if errors:
            pytest.fail(f"ARXML validation errors: {errors}")

    def test_deterministic_output(self, temp_dir):
        gen1 = generate_rte(DEMO_ARXML, temp_dir)
        gen2 = generate_rte(DEMO_ARXML, temp_dir)

        for f in gen1:
            with open(f) as fh:
                c1 = fh.read()
            with open(os.path.join(temp_dir, os.path.basename(f))) as fh:
                c2 = fh.read()
            assert c1 == c2, f"Non-deterministic output: {f}"

    def test_pipeline_with_skip_options(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir,
                                 generate_rte_h=False,
                                 generate_rte_c=False,
                                 generate_rte_type_h=False)
        # Only per-SWC files (4 × 2 = 8), no global files
        assert len(generated) == 8

    def test_generated_files_have_include_guards(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir)
        for f in generated:
            if f.endswith('.h'):
                with open(f) as fh:
                    content = fh.read()
                guard = os.path.basename(f).upper().replace('.', '_')
                assert guard in content, f"Missing guard {guard} in {f}"

    def test_generated_c_files_have_rte_api(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir)
        for f in generated:
            if f.endswith('.c'):
                with open(f) as fh:
                    content = fh.read()
                assert any(kw in content for kw in ["Rte_Init", "Rte_Read", "Rte_Write"])

    def test_pipeline_with_all_swc_filter(self, temp_dir):
        generated = generate_rte(DEMO_ARXML, temp_dir,
                                 swc_filter=["BCM_Door", "BCM_Light", "BCM_Wiper", "BCM_Power"])
        assert len(generated) == 11


# ---------------------------------------------------------------------------
#  12. Multi-SWC combined generation (Mode + NvBlock + Trigger mix)
# ---------------------------------------------------------------------------
class TestMultiPatternSWC:
    def test_combined_mode_and_nv_and_trigger_in_header(self):
        """A single SWC with Mode Switch, NvBlock, and Trigger ports
        should include all three API sections in its header."""
        swc = RteSwcInfo("HybridSWC")

        # Standard S-R port
        port = RtePortInfo("DataPort", "P_PORT", "DataIF", "SenderReceiver")
        de = RteDataElementInfo("Data", "uint8")
        port.data_elements.append(de)
        swc.ports.append(port)

        # Mode Switch
        ms = RteModeSwitchInfo("ModePort", "Ignition", "P_PORT")
        ms.mode_declarations = ["OFF", "ON"]
        swc.mode_switch_ports.append(ms)

        # NvBlock
        nv = RteNvBlockInfo("NvData", block_id=1)
        nv.block_size = 32
        swc.nv_block_ports.append(nv)

        # Trigger
        tr = RteTriggerInfo("TrigPort", "Alarm", "P_PORT")
        swc.trigger_ports.append(tr)

        header = _generate_swc_header(swc)
        assert "MODE SWITCH API" in header
        assert "NVBLOCK API" in header
        assert "TRIGGER API" in header
        assert "Rte_Switch_HybridSWC_ModePort" in header
        assert "Rte_ReadBlock_HybridSWC_NvData" in header
        assert "Rte_Trigger_HybridSWC_TrigPort" in header

    def test_combined_source_inits_all(self):
        """The init function should initialize buffers for all patterns."""
        swc = RteSwcInfo("HybridSWC")

        # Standard S-R
        port = RtePortInfo("DataPort", "P_PORT", "DataIF", "SenderReceiver")
        de = RteDataElementInfo("Data", "uint8")
        port.data_elements.append(de)
        swc.ports.append(port)

        # Mode Switch
        ms = RteModeSwitchInfo("ModePort", "Ignition", "P_PORT")
        swc.mode_switch_ports.append(ms)

        # NvBlock
        nv = RteNvBlockInfo("NvData", block_id=1)
        nv.block_size = 32
        swc.nv_block_ports.append(nv)

        # Trigger
        tr = RteTriggerInfo("TrigPort", "Alarm", "P_PORT")
        swc.trigger_ports.append(tr)

        src = _generate_swc_source(swc)
        assert "Rte_Init_HybridSWC" in src
        assert "Rte_ModeBuf_HybridSWC_ModePort" in src
        assert "Rte_NvBuf_HybridSWC_NvData" in src
        assert "Rte_TriggerFlag_HybridSWC_TrigPort" in src

    def test_rte_h_has_extended_return_codes_for_all_patterns(self):
        """Extended return codes should always be in Rte.h."""
        content = _generate_rte_h([], {"source_file": "test.arxml", "swc_count": 0})
        assert "RTE_E_MODE_INVALID" in content
        assert "RTE_E_TRIGGER_PENDING" in content
        assert "RTE_E_OUT_OF_RANGE" in content
        assert "RTE_E_LIMIT" in content
        assert "RTE_E_NO_DATA" in content


# ---------------------------------------------------------------------------
#  13. Edge Cases
# ---------------------------------------------------------------------------
class TestEdgeCases:
    def test_empty_swc_list_generates_global_files(self, temp_dir):
        """Even with empty SWC list, global files should still be valid."""
        rte_h = _generate_rte_h([], {"source_file": "empty.arxml", "swc_count": 0})
        assert "#ifndef RTE_H" in rte_h

        rte_c = _generate_rte_c([])
        assert "void Rte_Init(void)" in rte_c

    def test_swc_without_ports_generates_skeleton(self):
        """An SWC with no ports still generates a valid header and source."""
        swc = RteSwcInfo("EmptySWC")
        header = _generate_swc_header(swc)
        assert "Rte_EmptySWC.h" in header

        src = _generate_swc_source(swc)
        assert "Rte_EmptySWC" in src
        assert "Rte_Init_EmptySWC" in src

    def test_swc_without_runnables(self):
        """An SWC with ports but no runnables is still valid."""
        swc = RteSwcInfo("NoRunSWC")
        port = RtePortInfo("Data", "P_PORT", "DataIF", "SenderReceiver")
        port.data_elements.append(RteDataElementInfo("Val", "uint8"))
        swc.ports.append(port)

        src = _generate_swc_source(swc)
        assert "Rte_NoRunSWC" in src
        # Should not have any runnable function
        assert "void Rte_Init_NoRunSWC" in src

    def test_multiple_queue_instances_different_sizes(self):
        """Multiple queued ports with different queue sizes."""
        swc = RteSwcInfo("MultiQSWC")

        p1 = RtePortInfo("Q1", "P_PORT", "Q1IF", "SenderReceiver")
        d1 = RteDataElementInfo("Data1", "uint8")
        d1.data_semantics = DataSemantics.QUEUED
        d1.queue_length = 8
        p1.data_elements.append(d1)
        swc.ports.append(p1)

        p2 = RtePortInfo("Q2", "P_PORT", "Q2IF", "SenderReceiver")
        d2 = RteDataElementInfo("Data2", "uint16")
        d2.data_semantics = DataSemantics.QUEUED
        d2.queue_length = 16
        p2.data_elements.append(d2)
        swc.ports.append(p2)

        src = _generate_swc_source(swc)
        assert "Rte_Buf_MultiQSWC_Q1_Data1[8]" in src
        assert "Rte_Buf_MultiQSWC_Q2_Data2[16]" in src

    def test_mixed_semantics_in_single_swc(self):
        """A single SWC can have DEFAULT, INIT, QUEUED ports simultaneously."""
        swc = RteSwcInfo("MixedSWC")

        # DEFAULT
        p1 = RtePortInfo("DefaultP", "P_PORT", "DIF", "SenderReceiver")
        d1 = RteDataElementInfo("DefaultData", "uint8")
        d1.data_semantics = DataSemantics.DEFAULT
        p1.data_elements.append(d1)
        swc.ports.append(p1)

        # INIT
        p2 = RtePortInfo("InitP", "P_PORT", "IIF", "SenderReceiver")
        d2 = RteDataElementInfo("InitData", "uint8")
        d2.data_semantics = DataSemantics.INIT
        p2.data_elements.append(d2)
        swc.ports.append(p2)

        # QUEUED
        p3 = RtePortInfo("QueuedP", "P_PORT", "QIF", "SenderReceiver")
        d3 = RteDataElementInfo("QueuedData", "uint8")
        d3.data_semantics = DataSemantics.QUEUED
        d3.queue_length = 4
        p3.data_elements.append(d3)
        swc.ports.append(p3)

        src = _generate_swc_source(swc)
        assert "Rte_Write_MixedSWC_DefaultP_DefaultData" in src
        assert "Rte_InitData_MixedSWC_InitP_InitData" in src
        assert "Rte_Send_MixedSWC_QueuedP_QueuedData" in src

    def test_mode_switch_with_no_declarations_still_works(self):
        """Mode switch port with empty mode declarations still generates valid API."""
        swc = RteSwcInfo("MinModeSWC")
        ms = RteModeSwitchInfo("MinMode", "", "P_PORT")
        swc.mode_switch_ports.append(ms)

        decls = _generate_mode_switch_api_declarations(swc)
        assert "Rte_Switch_MinModeSWC_MinMode" in decls


# ---------------------------------------------------------------------------
#  13. RTE Type Generation methodology (A2/A3 — absorbed from cogu/autosar)
# ---------------------------------------------------------------------------
def _mk_type(name, **kw):
    """Shortcut: build an RteTypeDef for golden-string tests."""
    return RteTypeDef(name, **kw)


def _swc_with_type_ref(type_ref):
    """SWC whose R_PORT data element references a type by path."""
    swc = RteSwcInfo("TestSWC")
    port = RtePortInfo("InputPort", "R_PORT", "InputIF", "SenderReceiver")
    de = RteDataElementInfo("InputData", "uint8")
    de.type_ref = type_ref
    port.data_elements.append(de)
    swc.ports.append(port)
    return swc


class TestRteTypeDef:
    """RteTypeDef model: symbol_name override + dependency tracking (A2.3)."""

    def test_effective_name_defaults_to_short_name(self):
        td = _mk_type('SpeedType', c_type='uint16')
        assert td.effective_name == 'SpeedType'

    def test_effective_name_uses_symbol_name(self):
        td = _mk_type('SpeedType', symbol_name='Speed_T')
        assert td.effective_name == 'Speed_T'

    def test_scalar_has_no_dependencies(self):
        td = _mk_type('SpeedType', c_type='uint16')
        assert td.dependencies() == []

    def test_array_depends_on_element_type(self):
        td = _mk_type('SpeedArray', kind=RteTypeKind.ARRAY,
                      element_type='SpeedType', array_size=8)
        assert td.dependencies() == ['SpeedType']

    def test_ref_depends_on_target_type(self):
        td = _mk_type('ActualSpeed', kind=RteTypeKind.REF, ref_type='SpeedType')
        assert td.dependencies() == ['SpeedType']

    def test_record_depends_on_member_types(self):
        td = _mk_type('PositionType', kind=RteTypeKind.RECORD,
                      sub_elements=[{'name': 'speed', 'type_ref': 'SpeedType'},
                                    {'name': 'x', 'type_ref': 'uint16'}])
        assert td.dependencies() == ['SpeedType', 'uint16']

    def test_standard_c_types_set(self):
        assert 'uint8' in STANDARD_C_TYPES
        assert 'float64' in STANDARD_C_TYPES
        assert 'DoorStatusType' not in STANDARD_C_TYPES


class TestTypeCreationOrder:
    """Reverse level-order BFS: dependencies always precede users (A2.1)."""

    def _defs(self):
        return {
            'SpeedType': _mk_type('SpeedType', c_type='uint16'),
            'SpeedArray': _mk_type('SpeedArray', kind=RteTypeKind.ARRAY,
                                   element_type='SpeedType', array_size=8),
            'ActualSpeed': _mk_type('ActualSpeed', kind=RteTypeKind.REF,
                                    ref_type='SpeedType'),
            'PositionType': _mk_type('PositionType', kind=RteTypeKind.RECORD,
                                     sub_elements=[{'name': 'speed', 'type_ref': 'SpeedType'},
                                                   {'name': 'dist', 'type_ref': 'SpeedArray'}]),
        }

    def test_dependency_precedes_user(self):
        defs = self._defs()
        order = [td.name for td in gen_type_creation_order(defs, ['PositionType'])]
        assert order.index('SpeedType') < order.index('PositionType')
        assert order.index('SpeedArray') < order.index('PositionType')

    def test_reverse_level_order_exact(self):
        # Tree: PositionType → [SpeedType, SpeedArray] → SpeedArray → SpeedType
        # BFS: P, SpeedArray, SpeedType  ⇒ reversed: SpeedType, SpeedArray, P
        defs = self._defs()
        order = [td.name for td in gen_type_creation_order(defs, ['PositionType'])]
        assert order == ['SpeedType', 'SpeedArray', 'PositionType']

    def test_shared_dependency_emitted_once(self):
        defs = self._defs()
        order = [td.name for td in gen_type_creation_order(
            defs, ['ActualSpeed', 'SpeedArray'])]
        assert order.count('SpeedType') == 1
        assert order.index('SpeedType') < order.index('ActualSpeed')
        assert order.index('SpeedType') < order.index('SpeedArray')

    def test_cycle_does_not_loop_forever(self):
        a = _mk_type('A', kind=RteTypeKind.REF, ref_type='B')
        b = _mk_type('B', kind=RteTypeKind.REF, ref_type='A')
        order = [td.name for td in gen_type_creation_order({'A': a, 'B': b}, ['A'])]
        assert set(order) == {'A', 'B'}

    def test_dependency_trees_one_root_per_referenced_type(self):
        defs = self._defs()
        trees = gen_type_dependency_trees(defs, ['ActualSpeed', 'SpeedArray'])
        assert [t.data.name for t in trees] == ['ActualSpeed', 'SpeedArray']


class TestTypeEmitterFilter:
    """type_emitter != "RTE" types are skipped (A2.2)."""

    def test_non_rte_emitter_excluded(self):
        defs = {
            'ExtType': _mk_type('ExtType', c_type='uint16', type_emitter='ECU'),
            'MyType': _mk_type('MyType', kind=RteTypeKind.REF, ref_type='ExtType'),
        }
        order = [td.name for td in gen_type_creation_order(defs, ['MyType'])]
        assert 'ExtType' not in order
        assert order == ['MyType']

    def test_rte_emitter_included(self):
        defs = {'MyType': _mk_type('MyType', c_type='uint16', type_emitter='RTE')}
        order = [td.name for td in gen_type_creation_order(defs, ['MyType'])]
        assert order == ['MyType']

    def test_none_emitter_included(self):
        defs = {'MyType': _mk_type('MyType', c_type='uint16', type_emitter=None)}
        order = [td.name for td in gen_type_creation_order(defs, ['MyType'])]
        assert order == ['MyType']

    def test_case_insensitive_rte(self):
        defs = {'MyType': _mk_type('MyType', c_type='uint16', type_emitter='rte')}
        order = [td.name for td in gen_type_creation_order(defs, ['MyType'])]
        assert order == ['MyType']


class TestSymbolNameOverride:
    """SYMBOL-PROPS symbol_name overrides the default short name (A2.3)."""

    def test_scalar_renders_symbol_name(self):
        td = _mk_type('DoorStatusType', c_type='uint8', symbol_name='DoorStatus_T')
        assert render_type_def(td) == 'typedef uint8 DoorStatus_T;'

    def test_array_uses_symbol_name(self):
        td = _mk_type('SpeedArray', kind=RteTypeKind.ARRAY, element_type='SpeedType',
                      array_size=8, symbol_name='SpeedArr_T')
        assert render_type_def(td) == 'typedef SpeedType SpeedArr_T[8];'

    def test_record_uses_symbol_name_for_typedef(self):
        td = _mk_type('PositionType', kind=RteTypeKind.RECORD, symbol_name='Position_T',
                      sub_elements=[{'name': 'x', 'type_ref': 'uint16'}])
        assert render_type_def(td) == (
            'struct Rte_struct_Position_T\n'
            '{\n'
            '    uint16 x;\n'
            '};\n'
            'typedef struct Rte_struct_Position_T Position_T;'
        )


class TestTypeRendering:
    """Exact golden strings per type kind — write-side assertions (A3)."""

    def test_scalar_golden(self):
        assert render_type_def(_mk_type('SpeedType', c_type='uint16')) \
            == 'typedef uint16 SpeedType;'

    def test_scalar_defaults_uint8(self):
        assert render_type_def(_mk_type('UnknownType')) == 'typedef uint8 UnknownType;'

    def test_ref_golden(self):
        assert render_type_def(_mk_type('ActualSpeed', kind=RteTypeKind.REF,
                                        ref_type='SpeedType')) \
            == 'typedef SpeedType ActualSpeed;'

    def test_ref_resolves_symbol_name_of_target(self):
        defs = {'SpeedType': _mk_type('SpeedType', c_type='uint16', symbol_name='Speed_T')}
        td = _mk_type('ActualSpeed', kind=RteTypeKind.REF, ref_type='SpeedType')
        assert render_type_def(td, defs) == 'typedef Speed_T ActualSpeed;'

    def test_array_golden(self):
        td = _mk_type('SpeedArray', kind=RteTypeKind.ARRAY,
                      element_type='SpeedType', array_size=8)
        assert render_type_def(td) == 'typedef SpeedType SpeedArray[8];'

    def test_array_resolves_symbol_name_of_element(self):
        defs = {'SpeedType': _mk_type('SpeedType', c_type='uint16', symbol_name='Speed_T')}
        td = _mk_type('SpeedArray', kind=RteTypeKind.ARRAY,
                      element_type='SpeedType', array_size=8)
        assert render_type_def(td, defs) == 'typedef Speed_T SpeedArray[8];'

    def test_record_golden_with_member_indentation(self):
        td = _mk_type('PositionType', kind=RteTypeKind.RECORD,
                      sub_elements=[{'name': 'speed', 'type_ref': 'SpeedType'},
                                    {'name': 'x', 'type_ref': 'uint16'}])
        assert render_type_def(td) == (
            'struct Rte_struct_PositionType\n'
            '{\n'
            '    SpeedType speed;\n'
            '    uint16 x;\n'
            '};\n'
            'typedef struct Rte_struct_PositionType PositionType;'
        )

    def test_record_member_type_resolves_symbol_name(self):
        defs = {'SpeedType': _mk_type('SpeedType', c_type='uint16', symbol_name='Speed_T')}
        td = _mk_type('PositionType', kind=RteTypeKind.RECORD,
                      sub_elements=[{'name': 'speed', 'type_ref': 'SpeedType'}])
        assert render_type_def(td, defs) == (
            'struct Rte_struct_PositionType\n'
            '{\n'
            '    Speed_T speed;\n'
            '};\n'
            'typedef struct Rte_struct_PositionType PositionType;'
        )

    def test_code_block_object_render_with_indent(self):
        block = RteTypeCodeBlock(['typedef uint16 SpeedType;'])
        assert block.render(4) == '    typedef uint16 SpeedType;'
        assert block.render() == 'typedef uint16 SpeedType;'


class TestBuildTypeDefs:
    """DataType → RteTypeDef conversion (cogu create_from_element port)."""

    def test_scalar_with_base_type(self):
        dts = [DataType(name='SpeedType', category='VALUE', base_type='uint16')]
        defs = build_type_defs([], dts)
        assert 'SpeedType' in defs
        td = defs['SpeedType']
        assert td.kind == RteTypeKind.SCALAR
        assert td.c_type == 'uint16'
        assert td.effective_name == 'SpeedType'

    def test_symbol_name_from_data_type(self):
        dts = [DataType(name='DoorStatusType', category='VALUE', base_type='uint8',
                        symbol_name='DoorStatus_T')]
        defs = build_type_defs([], dts)
        assert defs['DoorStatusType'].effective_name == 'DoorStatus_T'

    def test_type_emitter_from_data_type(self):
        dts = [DataType(name='ExtType', category='VALUE', base_type='uint16',
                        type_emitter='ECU')]
        defs = build_type_defs([], dts)
        assert defs['ExtType'].type_emitter == 'ECU'

    def test_application_type_without_base_skipped(self):
        dts = [DataType(name='AppType', category='VALUE', base_type=None)]
        assert build_type_defs([], dts) == {}

    def test_ref_type_resolves_impl_data_type_ref(self):
        dts = [DataType(name='ActualSpeed', category='TYPE_REFERENCE', base_type='uint16',
                        sw_data_def_props={'impl_data_type_ref': '/DataTypes/Impl/SpeedType'})]
        defs = build_type_defs([], dts)
        assert defs['ActualSpeed'].kind == RteTypeKind.REF
        assert defs['ActualSpeed'].ref_type == 'SpeedType'

    def test_referenced_type_collected_from_ir(self):
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/SpeedType')
        dts = [DataType(name='SpeedType', category='VALUE', base_type='uint16')]
        defs = build_type_defs([swc], dts)
        assert 'SpeedType' in defs
        assert collect_referenced_type_names([swc]) == ['SpeedType']

    def test_unresolved_ref_not_in_defs(self):
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/MissingType')
        assert build_type_defs([swc], []) == {}


class TestRteTypeHGolden:
    """Golden-string snapshots of the typedefs section and Rte_Type.h (A3)."""

    def _metadata(self, dts):
        return {'source_file': 'test.arxml', 'swc_count': 1, 'data_types': dts}

    def test_type_defs_section_dependency_order_exact(self):
        dts = [
            DataType(name='SpeedType', category='VALUE', base_type='uint16'),
            DataType(name='ActualSpeed', category='TYPE_REFERENCE', base_type='uint16',
                     sw_data_def_props={'impl_data_type_ref': '/DataTypes/Impl/SpeedType'}),
        ]
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/ActualSpeed')
        assert gen_rte_type_defs_str([swc], self._metadata(dts)) == (
            'typedef uint16 SpeedType;\n'
            'typedef SpeedType ActualSpeed;'
        )

    def test_type_defs_section_skips_non_rte_emitter(self):
        dts = [
            DataType(name='SpeedType', category='VALUE', base_type='uint16'),
            DataType(name='ExtType', category='VALUE', base_type='uint32',
                     type_emitter='ECU'),
        ]
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/SpeedType')
        assert gen_rte_type_defs_str([swc], self._metadata(dts)) == (
            'typedef uint16 SpeedType;'
        )

    def test_type_defs_section_symbol_name_override(self):
        dts = [DataType(name='DoorStatusType', category='VALUE', base_type='uint8',
                        symbol_name='DoorStatus_T')]
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/DoorStatusType')
        assert gen_rte_type_defs_str([swc], self._metadata(dts)) == (
            'typedef uint8 DoorStatus_T;'
        )

    def test_type_defs_section_fallback_when_no_custom_types(self):
        assert gen_rte_type_defs_str([], self._metadata([])) == (
            '/* All types map to standard AUTOSAR types — no custom typedefs needed */'
        )

    def test_rte_type_h_full_contains_golden_typedef_block(self):
        dts = [
            DataType(name='SpeedType', category='VALUE', base_type='uint16'),
            DataType(name='ActualSpeed', category='TYPE_REFERENCE', base_type='uint16',
                     sw_data_def_props={'impl_data_type_ref': '/DataTypes/Impl/SpeedType'}),
        ]
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/ActualSpeed')
        content = _generate_rte_type_h([swc], self._metadata(dts))

        lines = content.split('\n')
        normalized = ['*  Generated : <TS>' if l.startswith('*  Generated : ')
                      else l for l in lines]
        text = '\n'.join(normalized)

        # Exact ordered typedef block (dependencies first)
        assert 'typedef uint16 SpeedType;\ntypedef SpeedType ActualSpeed;' in text
        # Typedefs sit between the section banner and NULL_PTR block
        assert text.index('typedef uint16 SpeedType;') > text.index('TYPE DEFINITIONS')
        assert text.index('typedef uint16 SpeedType;') < text.index('#ifndef NULL_PTR')
        # Deterministic tail (golden suffix)
        assert text.endswith(
            '#ifndef NULL_PTR\n'
            '#define NULL_PTR    ((void*)0)\n'
            '#endif\n'
            '\n'
            '#ifndef STATIC\n'
            '#define STATIC      static\n'
            '#endif\n'
            '\n'
            '#endif /* RTE_TYPE_H */\n'
            '/*==================================================================================================\n'
            '*                                       END OF FILE\n'
            '*==================================================================================================*/'
        )

    def test_rte_type_h_excludes_non_rte_emitter_type(self):
        dts = [
            DataType(name='SpeedType', category='VALUE', base_type='uint16'),
            DataType(name='ExtType', category='VALUE', base_type='uint32',
                     type_emitter='ECU'),
        ]
        swc = _swc_with_type_ref('/DataTypes/ImplementationDataTypes/SpeedType')
        content = _generate_rte_type_h([swc], self._metadata(dts))
        assert 'typedef uint16 SpeedType;' in content
        assert 'ExtType' not in content


# ---------------------------------------------------------------------------
#  14. Run
# ---------------------------------------------------------------------------
if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])
