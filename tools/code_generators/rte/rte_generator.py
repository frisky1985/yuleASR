#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML → RTE C Code Generator
======================================
AUTOSAR Classic Platform R20-11 compliant Runtime Environment code generator.

Reads .arxml system description files and generates:
  - Rte.h            — Global RTE header
  - Rte_Type.h       — Shared type definitions
  - Rte_Swc.h        — Per-SWC RTE interface header
  - Rte_Swc.c        — Per-SWC RTE implementation

Supported SWC Communication Patterns:
  - Sender-Receiver (basic)        — Rte_Read / Rte_Write
  - Sender-Receiver (data semantics) — INIT, MIMC (last-is-best), QUEUED
  - Client-Server                  — Rte_Call / Rte_Server
  - Mode Switch                    — Rte_Switch / Rte_Mode
  - NvBlock                        — Rte_ReadBlock / Rte_WriteBlock / Rte_SetResource
  - Trigger                        — Rte_Trigger / Rte_Event

Usage:
  python rte_generator.py -i <input.arxml> -o <output_dir>
  python rte_generator.py -i input.arxml -o generated/ --swc BCM_Door --swc BCM_Light

Integration with yuleOSH pipeline:
  yuleosh stage rte_generation --input config/input/arxml/ --output generated/
"""

import sys
import os
import argparse
import json
import logging
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Any, Tuple
from enum import Enum, auto

# ---------------------------------------------------------------------------
#  Import existing yuleASR ARXML parser
# ---------------------------------------------------------------------------
_ARXML_PARSER_PATH = os.path.join(
    os.path.dirname(__file__), '..', '..', 'arxml', 'parser'
)
if os.path.isdir(_ARXML_PARSER_PATH):
    sys.path.insert(0, os.path.normpath(_ARXML_PARSER_PATH))

try:
    from arxml_parser import (
        ARXMLParser, parse_arxml,
        SoftwareComponent, PortPrototype, PortInterface,
        RunnableEntity, RTEEvent, InternalBehavior,
        DataType, DataElement,
        ARXMLParseError,
    )
except ImportError:
    ARXMLParser = None
    parse_arxml = None
    logging.warning(
        "yuleASR ARXML parser not found at %s. "
        "Falling back to embedded XML parsing.", _ARXML_PARSER_PATH
    )

# ---------------------------------------------------------------------------
#  Logging
# ---------------------------------------------------------------------------
logging.basicConfig(level=logging.INFO, format="[RTE-GEN] %(levelname)s %(message)s")
logger = logging.getLogger("rte_generator")


# ============================================================================
#  Enums for communication semantics
# ============================================================================
class DataSemantics(Enum):
    """AUTOSAR data semantics for Sender-Receiver communication."""
    DEFAULT = "implicit"            # Default implicit/explicit via Rte_Read/Rte_Write
    INIT   = "init"                 # Init data semantics (Rte_InitData)
    MIMC   = "mimc"                 # Last-is-best / Multiple-Instant-Multiple-Consumer
    QUEUED = "queued"               # Queued (FIFO) data semantics


class ComPattern(Enum):
    """Supported AUTOSAR communication patterns."""
    SENDER_RECEIVER  = "SenderReceiver"
    CLIENT_SERVER    = "ClientServer"
    MODE_SWITCH      = "ModeSwitch"
    NV_BLOCK         = "NvBlock"
    TRIGGER          = "Trigger"
    PARAMETER        = "Parameter"


class NvBlockType(Enum):
    """NvBlock SWC role classification."""
    NVM_BLOCK        = "NvBlock"          # Standard NvBlock
    NV_MANAGER       = "NvManager"        # NVRAM Manager
    NV_MIRROR        = "CalMirror"         # Calibration Mirror


# ============================================================================
#  Internal Data Models
# ============================================================================
class RteSwcInfo:
    """Normalised SWC information for RTE generation."""
    __slots__ = (
        'name', 'name_upper', 'short_name', 'component_type',
        'runnable_entities', 'ports',
        'mode_switch_ports', 'nv_block_ports', 'trigger_ports',
        'nv_block_type',
    )

    def __init__(self, name: str, component_type: str = "APPLICATION"):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.short_name = name
        self.component_type = component_type
        self.runnable_entities: List['RteRunnableInfo'] = []
        self.ports: List['RtePortInfo'] = []
        self.mode_switch_ports: List['RteModeSwitchInfo'] = []
        self.nv_block_ports: List['RteNvBlockInfo'] = []
        self.trigger_ports: List['RteTriggerInfo'] = []
        self.nv_block_type: NvBlockType = NvBlockType.NVM_BLOCK


class RtePortInfo:
    """Normalised port information (Sender-Receiver and Client-Server)."""
    __slots__ = (
        'name', 'name_upper', 'port_type', 'interface_name',
        'interface_type', 'data_elements', 'operations', 'direction',
        'data_semantics', 'queue_length',
    )

    def __init__(self, name: str, port_type: str = "P_PORT",
                 interface_name: str = "", interface_type: str = "SenderReceiver"):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.port_type = port_type          # P_PORT or R_PORT
        self.interface_name = interface_name
        self.interface_type = interface_type
        self.data_elements: List['RteDataElementInfo'] = []
        self.operations: List[Dict[str, Any]] = []
        self.direction = "Provided" if port_type == "P_PORT" else "Required"
        self.data_semantics: DataSemantics = DataSemantics.DEFAULT
        self.queue_length: int = 0


class RteDataElementInfo:
    """Normalised data element."""
    __slots__ = ('name', 'name_upper', 'c_type', 'type_ref', 'com_signal_id',
                 'data_semantics', 'init_value', 'queue_length', 'is_queued')

    def __init__(self, name: str, c_type: str = "uint8"):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.c_type = c_type
        self.type_ref = ""
        self.com_signal_id = 0
        self.data_semantics: DataSemantics = DataSemantics.DEFAULT
        self.init_value: Optional[str] = None
        self.queue_length: int = 0
        self.is_queued: bool = False


class RteRunnableInfo:
    """Normalised runnable entity."""
    __slots__ = ('name', 'name_upper', 'symbol', 'period_ms',
                 'can_be_concurrent', 'events',
                 'trigger_refs', 'mode_switch_refs', 'nv_block_refs')

    def __init__(self, name: str):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.symbol = name
        self.period_ms = 0.0
        self.can_be_concurrent = False
        self.events: List[Dict[str, Any]] = []
        self.trigger_refs: List[str] = []
        self.mode_switch_refs: List[str] = []
        self.nv_block_refs: List[str] = []


class RteModeSwitchInfo:
    """Mode Switch port information (ModeDeclarationGroupPrototype)."""
    __slots__ = ('name', 'name_upper', 'port_type', 'mode_group_name',
                 'mode_declarations', 'is_master', 'interface_name',
                 'direction')

    def __init__(self, name: str, mode_group_name: str = "",
                 port_type: str = "P_PORT"):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.port_type = port_type
        self.mode_group_name = mode_group_name
        self.mode_declarations: List[str] = []
        self.is_master: bool = (port_type == "P_PORT")
        self.interface_name = ""
        self.direction = "Provided" if port_type == "P_PORT" else "Required"


class RteNvBlockInfo:
    """NvBlock-related port / service information."""
    __slots__ = ('name', 'name_upper', 'port_type', 'block_id',
                 'block_size', 'data_type', 'status',
                 'interface_name')

    def __init__(self, name: str, block_id: int = 1):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.port_type = "R_PORT"  # NvBlock consumers are R_PORT
        self.block_id = block_id
        self.block_size = 0
        self.data_type = "uint8"
        self.status = "OK"
        self.interface_name = ""


class RteTriggerInfo:
    """Trigger port information (TriggerInterface)."""
    __slots__ = ('name', 'name_upper', 'port_type', 'trigger_name',
                 'trigger_description', 'interface_name',
                 'direction')

    def __init__(self, name: str, trigger_name: str = "",
                 port_type: str = "P_PORT"):
        self.name = name
        self.name_upper = name.upper() if name else ""
        self.port_type = port_type
        self.trigger_name = trigger_name
        self.trigger_description = ""
        self.interface_name = ""
        self.direction = "Provided" if port_type == "P_PORT" else "Required"


# ============================================================================
#  AUTOSAR → C Type Mapping
# ============================================================================
AUTOSAR_TO_C_TYPE = {
    "uint8": "uint8",
    "uint16": "uint16",
    "uint32": "uint32",
    "sint8": "sint8",
    "sint16": "sint16",
    "sint32": "sint32",
    "boolean": "boolean",
    "float32": "float32",
    "float64": "float64",
    "real": "float32",
    "integer": "sint32",
    "UINT8": "uint8",
    "UINT16": "uint16",
    "UINT32": "uint32",
    "SINT8": "sint8",
    "SINT16": "sint16",
    "SINT32": "sint32",
    "BOOLEAN": "boolean",
    "FLOAT32": "float32",
    "FLOAT64": "float64",
}

SW_BASE_TYPE_SIZE_MAP = {
    8: "uint8",
    16: "uint16",
    32: "uint32",
    64: "uint64",
}


def map_to_c_type(autosar_type_name: str) -> str:
    """Map an AUTOSAR type name to a C type."""
    if not autosar_type_name:
        return "uint8"
    base = autosar_type_name.split('/')[-1]
    return AUTOSAR_TO_C_TYPE.get(base, AUTOSAR_TO_C_TYPE.get(base.lower(), "uint8"))


def extract_short_name_from_ref(ref: str) -> str:
    """Extract the last segment from a reference path, e.g. /A/B/C → C."""
    if not ref:
        return ""
    return ref.rstrip('/').split('/')[-1]


def resolve_data_semantics(semantics_str: str) -> DataSemantics:
    """Resolve AUTOSAR data semantics string to enum."""
    mapping = {
        "implicit": DataSemantics.DEFAULT,
        "explicit": DataSemantics.DEFAULT,
        "init": DataSemantics.INIT,
        "mimc": DataSemantics.MIMC,
        "last-is-best": DataSemantics.MIMC,
        "queued": DataSemantics.QUEUED,
    }
    key = semantics_str.lower().strip() if semantics_str else "implicit"
    return mapping.get(key, DataSemantics.DEFAULT)


# ============================================================================
#  ARXML → Internal IR Builder
# ============================================================================
def build_rte_ir_from_arxml(arxml_path: str,
                             swc_filter: Optional[List[str]] = None
                             ) -> Tuple[List[RteSwcInfo], Dict[str, Any]]:
    """
    Parse an ARXML file and build an intermediate representation (IR)
    suitable for RTE code generation.

    Returns: (swc_list, metadata)
    """
    if ARXMLParser is None:
        raise RuntimeError("yuleASR ARXML parser not available; cannot parse ARXML")

    logger.info("Parsing ARXML: %s", arxml_path)
    parser = parse_arxml(arxml_path)

    components = parser.parse_software_components()
    interfaces = parser.parse_port_interfaces()
    data_types = parser.parse_data_types()

    interface_map: Dict[str, PortInterface] = {}
    for iface in interfaces:
        interface_map[iface.name] = iface
        if iface.short_name:
            interface_map[iface.short_name] = iface

    type_map: Dict[str, DataType] = {}
    for dt in data_types:
        type_map[dt.name] = dt
        if dt.short_name:
            type_map[dt.short_name] = dt

    logger.info("  Found: %d SWCs, %d interfaces, %d data types",
                len(components), len(interfaces), len(data_types))

    swc_list: List[RteSwcInfo] = []

    for comp in components:
        if swc_filter and comp.name not in swc_filter:
            logger.debug("  Skipping SWC: %s (not in filter)", comp.name)
            continue

        swc_info = RteSwcInfo(comp.name, comp.component_type)
        logger.info("  Processing SWC: %s (%s)", comp.name, comp.component_type)

        # Build standard Sender-Receiver / Client-Server ports
        for port in comp.ports:
            port_info = RtePortInfo(
                name=port.name,
                port_type=port.port_type,
                interface_name=extract_short_name_from_ref(port.interface_ref or ""),
            )

            iface_name = port_info.interface_name
            if iface_name and iface_name in interface_map:
                resolved = interface_map[iface_name]
                port_info.interface_type = resolved.interface_type

                # Map data elements (Sender-Receiver)
                for de in resolved.data_elements:
                    de_type = "uint8"
                    if de.type_ref:
                        type_name = extract_short_name_from_ref(de.type_ref)
                        if type_name in type_map:
                            mapped = type_map[type_name]
                            if mapped.base_type:
                                de_type = map_to_c_type(mapped.base_type)
                            elif mapped.category:
                                de_type = map_to_c_type(mapped.category)
                        else:
                            de_type = map_to_c_type(type_name)

                    de_info = RteDataElementInfo(de.name, de_type)
                    de_info.type_ref = de.type_ref or ""
                    # Attach data semantics if available
                    if hasattr(de, 'data_semantics') and de.data_semantics:
                        de_info.data_semantics = resolve_data_semantics(de.data_semantics)
                    if hasattr(de, 'init_value') and de.init_value is not None:
                        de_info.init_value = str(de.init_value)
                    if hasattr(de, 'queue_length') and de.queue_length > 0:
                        de_info.queue_length = de.queue_length
                        de_info.is_queued = True
                        de_info.data_semantics = DataSemantics.QUEUED
                    port_info.data_elements.append(de_info)

                # Map operations (Client-Server)
                for op in resolved.operations:
                    port_info.operations.append(op)

            swc_info.ports.append(port_info)

        # Build Mode Switch ports from component port interface analysis
        # (detected via ModeDeclarationGroupPrototype in the interface)
        for port in comp.ports:
            _try_add_mode_switch_port(swc_info, port, interface_map)

        # Build Trigger ports (TriggerInterface)
        for port in comp.ports:
            _try_add_trigger_port(swc_info, port, interface_map)

        # Build NvBlock ports (NvBlockSwComponentType or NvDataInterface)
        if hasattr(comp, 'nv_block_data') and comp.nv_block_data:
            swc_info.nv_block_type = NvBlockType.NVM_BLOCK
            for nv_data in comp.nv_block_data:
                nv_info = RteNvBlockInfo(
                    name=nv_data.get('name', 'NvBlock'),
                    block_id=nv_data.get('block_id', 1),
                )
                nv_info.block_size = nv_data.get('block_size', 0)
                nv_info.data_type = map_to_c_type(nv_data.get('type_ref', 'uint8'))
                nv_info.interface_name = nv_data.get('interface_name', '')
                swc_info.nv_block_ports.append(nv_info)

        # Build runnable entities
        for ib in comp.internal_behaviors:
            for runnable in ib.runnables:
                run_info = RteRunnableInfo(runnable.name)
                run_info.symbol = runnable.symbol or runnable.name
                run_info.period_ms = runnable.minimum_start_interval
                run_info.can_be_concurrent = runnable.can_be_invoked_concurrently

                # Attach events
                for evt in ib.events:
                    if evt.start_on_event_ref and runnable.name in evt.start_on_event_ref:
                        run_info.events.append({
                            'name': evt.name,
                            'type': evt.event_type,
                            'period_ms': evt.period_ms or 0.0,
                        })
                        # Detect trigger/mode-switch events
                        if evt.event_type and 'trigger' in evt.event_type.lower():
                            run_info.trigger_refs.append(evt.name)
                        if evt.event_type and 'mode' in evt.event_type.lower():
                            run_info.mode_switch_refs.append(evt.name)

                swc_info.runnable_entities.append(run_info)

        swc_list.append(swc_info)

    metadata = {
        'generated': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'source_file': os.path.basename(arxml_path),
        'swc_count': len(swc_list),
        'errors': parser.validate(),
    }

    return swc_list, metadata


def _try_add_mode_switch_port(swc_info: RteSwcInfo, port: Any,
                               interface_map: Dict[str, PortInterface]) -> None:
    """Detect and add Mode Switch port if interface is ModeSwitchInterface."""
    if not hasattr(port, 'interface_ref') or not port.interface_ref:
        return
    iface_name = extract_short_name_from_ref(port.interface_ref)
    if iface_name not in interface_map:
        return
    resolved = interface_map[iface_name]
    if not hasattr(resolved, 'interface_type') or resolved.interface_type != "ModeSwitch":
        return
    if not hasattr(resolved, 'mode_declarations'):
        return

    mode_info = RteModeSwitchInfo(
        name=port.name,
        mode_group_name=resolved.mode_declarations.get('mode_group_name', ''),
        port_type=port.port_type,
    )
    mode_info.mode_declarations = resolved.mode_declarations.get('declarations', [])
    mode_info.interface_name = iface_name
    swc_info.mode_switch_ports.append(mode_info)


def _try_add_trigger_port(swc_info: RteSwcInfo, port: Any,
                           interface_map: Dict[str, PortInterface]) -> None:
    """Detect and add Trigger port if interface is TriggerInterface."""
    if not hasattr(port, 'interface_ref') or not port.interface_ref:
        return
    iface_name = extract_short_name_from_ref(port.interface_ref)
    if iface_name not in interface_map:
        return
    resolved = interface_map[iface_name]
    if not hasattr(resolved, 'interface_type') or resolved.interface_type != "Trigger":
        return

    trigger_info = RteTriggerInfo(
        name=port.name,
        trigger_name=resolved.trigger_name if hasattr(resolved, 'trigger_name') else '',
        port_type=port.port_type,
    )
    trigger_info.interface_name = iface_name
    if hasattr(resolved, 'trigger_description'):
        trigger_info.trigger_description = resolved.trigger_description
    swc_info.trigger_ports.append(trigger_info)


# ============================================================================
#  ARXML ← Direct XML fallback parser (extended)
# ============================================================================
def _parse_arxml_direct(arxml_path: str) -> Tuple[List[RteSwcInfo], Dict[str, Any]]:
    """
    Fallback: parse ARXML using Python's xml.etree directly.
    Supports AUTOSAR R4.0/R20-11 schema with Sender-Receiver, Client-Server,
    ModeSwitch, Trigger, and NvBlock SWCs.
    """
    import xml.etree.ElementTree as ET

    tree = ET.parse(arxml_path)
    root = tree.getroot()

    ns_uri = ''
    if root.tag.startswith('{'):
        ns_uri = root.tag[1:root.tag.index('}')]

    ns = {'ar': ns_uri} if ns_uri else {}

    def tag(name: str) -> str:
        return f'{{{ns_uri}}}{name}' if ns_uri else name

    def text(elem, path: str, default: str = "") -> str:
        el = elem.find(path, ns)
        return el.text.strip() if el is not None and el.text else default

    def texts(elem, path: str) -> List[str]:
        return [e.text.strip() for e in elem.findall(path, ns) if e.text]

    swc_list: List[RteSwcInfo] = []
    iface_map: Dict[str, Dict] = {}

    # --- Pass 1: collect all interface types ---
    # SenderReceiver
    for iface in root.findall(f'.//{tag("SENDER-RECEIVER-INTERFACE")}', ns):
        iname = text(iface, tag("SHORT-NAME"))
        if not iname:
            continue
        iface_info = {
            'name': iname,
            'type': 'SenderReceiver',
            'data_elements': []
        }
        for de in iface.findall(f'.//{tag("DATA-ELEMENT-PROTOTYPE")}', ns):
            de_name = text(de, tag("SHORT-NAME"))
            de_type_ref = text(de, tag("TYPE-TREF"))
            de_semantics = text(de, tag("SW-DATA-DEF-PROPS") + '/' + tag("SW-DATA-DEF-PROPS-VARIANTS") + '/' +
                                   tag("SW-DATA-DEF-PROPS-CONDITIONAL") + '/' + tag("SW-CALIBRATION-ACCESS"),
                                   "")
            de_info = {
                'name': de_name,
                'type_ref': de_type_ref,
                'data_semantics': de_semantics,
            }
            # Check for init value
            init_val = text(de, tag("INIT-VALUE"), "")
            if init_val:
                de_info['init_value'] = init_val
            # Check for queue length
            queue_len_str = text(de, tag("QUEUE-LENGTH"), "0")
            if queue_len_str and queue_len_str.isdigit() and int(queue_len_str) > 0:
                de_info['queue_length'] = int(queue_len_str)
            iface_info['data_elements'].append(de_info)
        iface_map[iname] = iface_info

    # ClientServer
    for iface in root.findall(f'.//{tag("CLIENT-SERVER-INTERFACE")}', ns):
        iname = text(iface, tag("SHORT-NAME"))
        if not iname:
            continue
        iface_info = {
            'name': iname,
            'type': 'ClientServer',
            'operations': []
        }
        for op in iface.findall(f'.//{tag("CLIENT-SERVER-OPERATION")}', ns):
            op_name = text(op, tag("SHORT-NAME"))
            op_args = []
            for arg in op.findall(f'.//{tag("ARGUMENT-ARGUMENT-PROTOTYPE")}', ns):
                a_name = text(arg, tag("SHORT-NAME"))
                a_type = text(arg, tag("TYPE-TREF"))
                a_dir = text(arg, tag("DIRECTION"), "IN")
                op_args.append({'name': a_name, 'type_ref': a_type, 'direction': a_dir})
            iface_info['operations'].append({'name': op_name, 'arguments': op_args})
        iface_map[iname] = iface_info

    # ModeSwitch
    for iface in root.findall(f'.//{tag("MODE-SWITCH-INTERFACE")}', ns):
        iname = text(iface, tag("SHORT-NAME"))
        if not iname:
            continue
        mg_name = ""
        decls = []
        for mg in iface.findall(f'.//{tag("MODE-DECLARATION-GROUP-PROTOTYPE")}', ns):
            mg_name = text(mg, tag("SHORT-NAME"))
            # Find mode declarations from referenced group
            for mode_ref in mg.findall(f'.//{tag("TYPE-TREF")}', ns):
                ref_path = text(mode_ref, ".", "")
                if ref_path:
                    # Try to resolve
                    pass
        iface_info = {
            'name': iname,
            'type': 'ModeSwitch',
            'mode_group_name': mg_name,
            'declarations': decls,
        }
        iface_map[iname] = iface_info

    # Trigger
    for iface in root.findall(f'.//{tag("TRIGGER-INTERFACE")}', ns):
        iname = text(iface, tag("SHORT-NAME"))
        if not iname:
            continue
        trig_name = ""
        trig_desc = ""
        for trig in iface.findall(f'.//{tag("TRIGGER")}', ns):
            trig_name = text(trig, tag("SHORT-NAME"), trig_name)
            trig_desc = text(trig, tag("DESC") + '/' + tag("L-2"), "")
        iface_info = {
            'name': iname,
            'type': 'Trigger',
            'trigger_name': trig_name,
            'trigger_description': trig_desc,
        }
        iface_map[iname] = iface_info

    # NvDataInterface (for NvBlock consumers)
    for iface in root.findall(f'.//{tag("NV-DATA-INTERFACE")}', ns):
        iname = text(iface, tag("SHORT-NAME"))
        if not iname:
            continue
        iface_info = {
            'name': iname,
            'type': 'NvData',
            'data_elements': [],
        }
        for de in iface.findall(f'.//{tag("DATA-ELEMENT-PROTOTYPE")}', ns):
            de_name = text(de, tag("SHORT-NAME"))
            de_type_ref = text(de, tag("TYPE-TREF"))
            iface_info['data_elements'].append({
                'name': de_name,
                'type_ref': de_type_ref,
            })
        iface_map[iname] = iface_info

    # --- Pass 2: collect SWCs ---
    for swc_tag in ['APPLICATION-SW-COMPONENT-TYPE',
                    'COMPOSITION-SW-COMPONENT-TYPE',
                    'SERVICE-SW-COMPONENT-TYPE',
                    'ECU-ABSTRACTION-SW-COMPONENT-TYPE',
                    'COMPLEX-DEVICE-DRIVER-SW-COMPONENT-TYPE',
                    'NV-BLOCK-SW-COMPONENT-TYPE']:
        for comp in root.findall(f'.//{tag(swc_tag)}', ns):
            cname = text(comp, tag("SHORT-NAME"))
            if not cname:
                continue
            swc_info = RteSwcInfo(cname, swc_tag)

            # Ports
            for pport in comp.findall(f'.//{tag("P-PORT-PROTOTYPE")}', ns):
                pname = text(pport, tag("SHORT-NAME"))
                iref = text(pport, tag("PROVIDED-INTERFACE-TREF"))
                iface_name = extract_short_name_from_ref(iref)
                _add_parsed_port(swc_info, pname, "P_PORT", iface_name, iface_map)

            for rport in comp.findall(f'.//{tag("R-PORT-PROTOTYPE")}', ns):
                pname = text(rport, tag("SHORT-NAME"))
                iref = text(rport, tag("REQUIRED-INTERFACE-TREF"))
                iface_name = extract_short_name_from_ref(iref)
                _add_parsed_port(swc_info, pname, "R_PORT", iface_name, iface_map)

            # Runnables
            for ib_tag_name in [tag("SWC-INTERNAL-BEHAVIOR"), tag("INTERNAL-BEHAVIOR")]:
                for ib in comp.findall(f'.//{ib_tag_name}', ns):
                    for runnable in ib.findall(f'.//{tag("RUNNABLE-ENTITY")}', ns):
                        rname = text(runnable, tag("SHORT-NAME"))
                        if not rname:
                            continue
                        run_info = RteRunnableInfo(rname)
                        run_info.symbol = text(runnable, tag("SYMBOL"), rname)

                        # Check for trigger events
                        for evt in ib.findall(f'.//{tag("TRIGGER-EVENT")}', ns):
                            on_ref = text(evt, tag("START-ON-EVENT-REF"), "")
                            if rname in on_ref:
                                run_info.trigger_refs.append(
                                    text(evt, tag("SHORT-NAME"), "TriggerEvent")
                                )
                        for evt in ib.findall(f'.//{tag("MODE-SWITCH-EVENT")}', ns):
                            on_ref = text(evt, tag("START-ON-EVENT-REF"), "")
                            if rname in on_ref:
                                run_info.mode_switch_refs.append(
                                    text(evt, tag("SHORT-NAME"), "ModeSwitchEvent")
                                )

                        swc_info.runnable_entities.append(run_info)

            swc_list.append(swc_info)

    errors = []
    metadata = {
        'generated': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'source_file': os.path.basename(arxml_path),
        'swc_count': len(swc_list),
        'errors': errors,
        'parser': 'direct_xml',
    }

    return swc_list, metadata


def _add_parsed_port(swc_info: RteSwcInfo, pname: str, ptype: str,
                     iface_name: str, iface_map: Dict[str, Dict]) -> None:
    """Add a port to swc_info based on parsed interface type."""
    if not pname:
        pname = "Port"
    iface_data = iface_map.get(iface_name, {})
    iface_type = iface_data.get('type', 'SenderReceiver')

    if iface_type == 'ModeSwitch':
        mode_info = RteModeSwitchInfo(pname, iface_data.get('mode_group_name', ''), ptype)
        mode_info.mode_declarations = iface_data.get('declarations', [])
        mode_info.interface_name = iface_name
        swc_info.mode_switch_ports.append(mode_info)

    elif iface_type == 'Trigger':
        trigger_info = RteTriggerInfo(pname, iface_data.get('trigger_name', ''), ptype)
        trigger_info.trigger_description = iface_data.get('trigger_description', '')
        trigger_info.interface_name = iface_name
        swc_info.trigger_ports.append(trigger_info)

    elif iface_type in ('NvData', 'NvBlock'):
        for de in iface_data.get('data_elements', []):
            nv_info = RteNvBlockInfo(
                name=de.get('name', pname + '_Data'),
                block_id=1,
            )
            nv_info.data_type = map_to_c_type(de.get('type_ref', 'uint8'))
            nv_info.interface_name = iface_name
            swc_info.nv_block_ports.append(nv_info)

    else:
        # Standard Sender-Receiver or Client-Server
        port_info = RtePortInfo(pname, ptype, iface_name, iface_type)
        for de in iface_data.get('data_elements', []):
            de_type = map_to_c_type(de.get('type_ref', ''))
            de_info = RteDataElementInfo(de['name'], de_type)
            if de.get('data_semantics'):
                de_info.data_semantics = resolve_data_semantics(de['data_semantics'])
            if de.get('init_value'):
                de_info.init_value = de['init_value']
            if de.get('queue_length', 0) > 0:
                de_info.queue_length = de['queue_length']
                de_info.is_queued = True
                de_info.data_semantics = DataSemantics.QUEUED
            port_info.data_elements.append(de_info)

        for op in iface_data.get('operations', []):
            port_info.operations.append(op)

        swc_info.ports.append(port_info)


# ============================================================================
#  C Code Generation — Mode Switch helpers
# ============================================================================
def _generate_mode_switch_api_declarations(swc: RteSwcInfo) -> str:
    """Generate Mode Switch API declarations for a SWC."""
    lines = []
    for ms in swc.mode_switch_ports:
        lines.append("")
        lines.append(f"/* Mode Switch: {ms.name} ({ms.port_type}) */")
        if ms.mode_declarations:
            lines.append(f"/*   Modes: {', '.join(ms.mode_declarations)} */")

        if ms.port_type == "P_PORT":
            # Mode provider: Rte_Switch API
            lines.append(
                f"extern Std_ReturnType Rte_Switch_{swc.name}_{ms.name}"
                f"(uint8 mode);"
            )
        else:
            # Mode consumer: Rte_Mode API
            lines.append(
                f"extern Std_ReturnType Rte_Mode_{swc.name}_{ms.name}"
                f"(uint8* mode);"
            )

    return '\n'.join(lines)


def _generate_mode_switch_api_impl(swc: RteSwcInfo) -> str:
    """Generate Mode Switch API implementations for a SWC."""
    lines = []
    for ms in swc.mode_switch_ports:
        buf = f"Rte_ModeBuf_{swc.name}_{ms.name}"
        upd = f"{buf}_Activated"

        if ms.port_type == "P_PORT":
            lines.extend([
                "/**",
                f" * @brief   Switch mode via port {ms.name}",
                f" * @param   mode  New mode value",
                f" * @return  E_OK if successful",
                " */",
                f"Std_ReturnType Rte_Switch_{swc.name}_{ms.name}(uint8 mode)",
                "{",
                f"    {buf} = mode;",
                f"    {upd} = TRUE;",
                "    return RTE_E_OK;",
                "}",
                "",
            ])
        else:
            lines.extend([
                "/**",
                f" * @brief   Read current mode from port {ms.name}",
                f" * @param   mode  Output pointer for current mode",
                f" * @return  E_OK if mode available",
                " */",
                f"Std_ReturnType Rte_Mode_{swc.name}_{ms.name}(uint8* mode)",
                "{",
                f"    if (mode == NULL_PTR)",
                "    {",
                "        return RTE_E_INVALID;",
                "    }",
                f"    if ({upd})",
                "    {",
                f"        *mode = {buf};",
                f"        {upd} = FALSE;",
                "        return RTE_E_OK;",
                "    }",
                "    return RTE_E_NO_DATA;",
                "}",
                "",
            ])

    return '\n'.join(lines)


def _generate_mode_switch_local_buffers(swc: RteSwcInfo) -> str:
    """Generate local buffer declarations for mode switch ports."""
    lines = []
    for ms in swc.mode_switch_ports:
        lines.append(f"STATIC uint8 Rte_ModeBuf_{swc.name}_{ms.name} = 0U;")
        lines.append(f"STATIC boolean Rte_ModeBuf_{swc.name}_{ms.name}_Activated = FALSE;")
    return '\n'.join(lines)


# ============================================================================
#  C Code Generation — NvBlock helpers
# ============================================================================
def _generate_nvblock_api_declarations(swc: RteSwcInfo) -> str:
    """Generate NvBlock API declarations for a SWC."""
    lines = []
    for nv in swc.nv_block_ports:
        lines.append("")
        lines.append(f"/* NvBlock: {nv.name} (Block ID: {nv.block_id}) */")

        # Service-like NvBlock API: ReadBlock / WriteBlock
        lines.append(
            f"extern Std_ReturnType Rte_ReadBlock_{swc.name}_{nv.name}"
            f"(void* data, uint16 length);"
        )
        lines.append(
            f"extern Std_ReturnType Rte_WriteBlock_{swc.name}_{nv.name}"
            f"(const void* data, uint16 length);"
        )
        lines.append(
            f"extern Std_ReturnType Rte_InvalidateBlock_{swc.name}_{nv.name}(void);"
        )

    return '\n'.join(lines)


def _generate_nvblock_api_impl(swc: RteSwcInfo) -> str:
    """Generate NvBlock API implementations for a SWC."""
    lines = []
    for nv in swc.nv_block_ports:
        buf = f"Rte_NvBuf_{swc.name}_{nv.name}"
        valid = f"{buf}_Valid"

        lines.extend([
            "/**",
            f" * @brief   Read NvBlock data for {nv.name}",
            f" * @param   data    Output buffer",
            f" * @param   length  Number of bytes to read",
            f" * @return  E_OK if data available, E_NOT_OK otherwise",
            " */",
            f"Std_ReturnType Rte_ReadBlock_{swc.name}_{nv.name}"
            f"(void* data, uint16 length)",
            "{",
            f"    if ((data == NULL_PTR) || (length > sizeof({buf})))",
            "    {",
            "        return RTE_E_INVALID;",
            "    }",
            f"    if (!{valid})",
            "    {",
            "        return RTE_E_NO_DATA;",
            "    }",
            f"    (void)memcpy(data, &{buf}, length);",
            "    return RTE_E_OK;",
            "}",
            "",
            "/**",
            f" * @brief   Write NvBlock data for {nv.name}",
            f" * @param   data    Input data to write",
            f" * @param   length  Number of bytes to write",
            f" * @return  E_OK if successful",
            " */",
            f"Std_ReturnType Rte_WriteBlock_{swc.name}_{nv.name}"
            f"(const void* data, uint16 length)",
            "{",
            f"    if ((data == NULL_PTR) || (length > sizeof({buf})))",
            "    {",
            "        return RTE_E_INVALID;",
            "    }",
            f"    (void)memcpy(&{buf}, data, length);",
            f"    {valid} = TRUE;",
            "    return RTE_E_OK;",
            "}",
            "",
            "/**",
            f" * @brief   Invalidate NvBlock data for {nv.name}",
            f" * @return  E_OK",
            " */",
            f"Std_ReturnType Rte_InvalidateBlock_{swc.name}_{nv.name}(void)",
            "{",
            f"    {valid} = FALSE;",
            "    return RTE_E_OK;",
            "}",
            "",
        ])

    return '\n'.join(lines)


def _generate_nvblock_local_buffers(swc: RteSwcInfo) -> str:
    """Generate local buffer declarations for NvBlock data."""
    lines = []
    for nv in swc.nv_block_ports:
        buf_size = max(nv.block_size, 4) if nv.block_size else 64
        lines.append(
            f"STATIC uint8 Rte_NvBuf_{swc.name}_{nv.name}[{buf_size}];"
        )
        lines.append(f"STATIC boolean Rte_NvBuf_{swc.name}_{nv.name}_Valid = FALSE;")
    return '\n'.join(lines)


# ============================================================================
#  C Code Generation — Trigger helpers
# ============================================================================
def _generate_trigger_api_declarations(swc: RteSwcInfo) -> str:
    """Generate Trigger API declarations for a SWC."""
    lines = []
    for tr in swc.trigger_ports:
        lines.append("")
        lines.append(f"/* Trigger: {tr.name} ({tr.trigger_name}, {tr.port_type}) */")

        if tr.port_type == "P_PORT":
            # Trigger provider: Rte_Trigger_ API
            lines.append(
                f"extern Std_ReturnType Rte_Trigger_{swc.name}_{tr.name}(void);"
            )
        else:
            lines.append(
                f"extern Std_ReturnType Rte_Event_{swc.name}_{tr.name}(void);"
            )

    return '\n'.join(lines)


def _generate_trigger_api_impl(swc: RteSwcInfo) -> str:
    """Generate Trigger API implementations for a SWC."""
    lines = []
    for tr in swc.trigger_ports:
        flag = f"Rte_TriggerFlag_{swc.name}_{tr.name}"

        if tr.port_type == "P_PORT":
            lines.extend([
                "/**",
                f" * @brief   Raise trigger on port {tr.name} ({tr.trigger_name})",
                f" * @return  E_OK if trigger raised",
                " */",
                f"Std_ReturnType Rte_Trigger_{swc.name}_{tr.name}(void)",
                "{",
                f"    {flag} = TRUE;",
                "    return RTE_E_OK;",
                "}",
                "",
            ])
        else:
            lines.extend([
                "/**",
                f" * @brief   Consume trigger event on port {tr.name}",
                f" * @return  E_OK if trigger event occurred",
                " */",
                f"Std_ReturnType Rte_Event_{swc.name}_{tr.name}(void)",
                "{",
                f"    if ({flag})",
                "    {",
                f"        {flag} = FALSE;",
                "        return RTE_E_OK;",
                "    }",
                "    return RTE_E_NO_DATA;",
                "}",
                "",
            ])

    return '\n'.join(lines)


def _generate_trigger_local_buffers(swc: RteSwcInfo) -> str:
    """Generate local flags for trigger ports."""
    lines = []
    for tr in swc.trigger_ports:
        lines.append(f"STATIC boolean Rte_TriggerFlag_{swc.name}_{tr.name} = FALSE;")
    return '\n'.join(lines)


# ============================================================================
#  C Code Generation — Data Semantics variants
# ============================================================================
def _generate_data_semantics_api_impl(swc: RteSwcInfo, port: RtePortInfo,
                                       de: RteDataElementInfo) -> str:
    """Generate Rte API implementation with correct data semantics.

    Returns the implementation string for a single data element, handling
    INIT, MIMC, and QUEUED semantics differently from the default.
    """
    buf_name = f"Rte_Buf_{swc.name}_{port.name}_{de.name}"
    upd_name = f"{buf_name}_Updated"
    lines = []

    if de.data_semantics == DataSemantics.INIT:
        # INIT semantics: Rte_InitData + Rte_Write acts as initialisation
        if port.port_type == "P_PORT":
            lines.extend([
                f"/**",
                f" * @brief   Init {de.name} via port {port.name} (INIT semantics)",
                f" * @param   data  Pointer to init data",
                f" * @return  E_OK",
                f" */",
                f"Std_ReturnType Rte_InitData_{swc.name}_{port.name}_{de.name}("
                f"const {de.c_type}* data)",
                "{",
                f"    if (data == NULL_PTR) return RTE_E_INVALID;",
                f"    {buf_name} = *data;",
                f"    {upd_name} = TRUE;",
                "    return RTE_E_OK;",
                "}",
                "",
            ])

    elif de.data_semantics == DataSemantics.QUEUED:
        # QUEUED semantics: FIFO queue access
        qlen = de.queue_length if de.queue_length > 0 else 4
        idx_name = f"{buf_name}_HeadIdx"
        cnt_name = f"{buf_name}_Count"
        if port.port_type == "P_PORT":
            lines.extend([
                f"/**",
                f" * @brief   Send {de.name} to queue ({qlen} slots, QUEUED)",
                f" * @param   data  Pointer to data to enqueue",
                f" * @return  E_OK if queued, RTE_E_LIMIT if full",
                f" */",
                f"Std_ReturnType Rte_Send_{swc.name}_{port.name}_{de.name}("
                f"const {de.c_type}* data)",
                "{",
                f"    if (data == NULL_PTR) return RTE_E_INVALID;",
                f"    if ({cnt_name} >= {qlen}) return RTE_E_LIMIT;",
                f"    {buf_name}[({idx_name} + {cnt_name}) % {qlen}] = *data;",
                f"    {cnt_name}++;",
                "    return RTE_E_OK;",
                "}",
                "",
            ])
        else:
            lines.extend([
                f"/**",
                f" * @brief   Receive {de.name} from queue ({qlen} slots, QUEUED)",
                f" * @param   data  Pointer to receive buffer",
                f" * @return  E_OK if data dequeued, RTE_E_NO_DATA if empty",
                f" */",
                f"Std_ReturnType Rte_Receive_{swc.name}_{port.name}_{de.name}("
                f"{de.c_type}* data)",
                "{",
                f"    if (data == NULL_PTR) return RTE_E_INVALID;",
                f"    if ({cnt_name} == 0U) return RTE_E_NO_DATA;",
                f"    *data = {buf_name}[{idx_name}];",
                f"    {idx_name} = ({idx_name} + 1U) % {qlen};",
                f"    {cnt_name}--;",
                "    return RTE_E_OK;",
                "}",
                "",
            ])

    return '\n'.join(lines)


def _generate_data_semantics_api_declarations(swc: RteSwcInfo, port: RtePortInfo,
                                                de: RteDataElementInfo) -> str:
    """Generate API declarations for non-default data semantics."""
    lines = []
    if de.data_semantics == DataSemantics.INIT and port.port_type == "P_PORT":
        lines.append(
            f"extern Std_ReturnType Rte_InitData_{swc.name}_{port.name}_{de.name}"
            f"(const {de.c_type}* data);"
        )
    elif de.data_semantics == DataSemantics.QUEUED:
        suffix = "Send" if port.port_type == "P_PORT" else "Receive"
        lines.append(
            f"extern Std_ReturnType Rte_{suffix}_{swc.name}_{port.name}_{de.name}"
            f"({('const ' if port.port_type == 'P_PORT' else '')}{de.c_type}* data);"
        )
    return '\n'.join(lines)


# ============================================================================
#  C Code Generation — Data semantics buffer declarations
# ============================================================================
def _generate_data_semantics_buffer_decls(swc: RteSwcInfo, port: RtePortInfo,
                                           de: RteDataElementInfo) -> str:
    """Generate buffer declarations for non-default data semantics."""
    buf_name = f"Rte_Buf_{swc.name}_{port.name}_{de.name}"
    lines = []
    if de.data_semantics == DataSemantics.QUEUED:
        qlen = de.queue_length if de.queue_length > 0 else 4
        lines.append(f"STATIC {de.c_type} {buf_name}[{qlen}];")
        lines.append(f"STATIC uint16 {buf_name}_HeadIdx = 0U;")
        lines.append(f"STATIC uint16 {buf_name}_Count = 0U;")
    elif de.data_semantics == DataSemantics.INIT:
        lines.append(f"STATIC {de.c_type} {buf_name};")
        lines.append(f"STATIC boolean {buf_name}_InitDone = FALSE;")
    return '\n'.join(lines)


# ============================================================================
#  C Code Generation — Main generators
# ============================================================================
def _generate_rte_h(swc_list: List[RteSwcInfo], metadata: Dict[str, Any]) -> str:
    """Generate Rte.h — global RTE header."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/*==================================================================================================",
        f"*  Rte.h — Runtime Environment Global Header",
        f"*  Generated : {now}",
        f"*  Source    : {metadata.get('source_file', 'N/A')}",
        f"*  SWCs      : {metadata.get('swc_count', 0)}",
        "*",
        "*  (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.",
        "*  All Rights Reserved.",
        "*==================================================================================================*/",
        "",
        "#ifndef RTE_H",
        "#define RTE_H",
        "",
        "/*==================================================================================================",
        "*                                          INCLUDES",
        "*==================================================================================================*/",
        '#include "Std_Types.h"',
        '#include "Rte_Type.h"',
        "",
        "/*==================================================================================================",
        "*                                     VERSION INFORMATION",
        "*==================================================================================================*/",
        "#define RTE_VENDOR_ID                   (0x0001U)",
        "#define RTE_MODULE_ID                   (0x0070U)",
        "#define RTE_AR_RELEASE_MAJOR_VERSION    (0x04U)",
        "#define RTE_AR_RELEASE_MINOR_VERSION    (0x04U)",
        "#define RTE_SW_MAJOR_VERSION            (0x01U)",
        "#define RTE_SW_MINOR_VERSION            (0x00U)",
        "#define RTE_SW_PATCH_VERSION            (0x00U)",
        "",
        "/*==================================================================================================",
        "*                                     RETURN TYPES",
        "*==================================================================================================*/",
        "#ifndef RTE_E_OK",
        "#define RTE_E_OK                ((Std_ReturnType)0x00U)",
        "#define RTE_E_NOT_OK            ((Std_ReturnType)0x01U)",
        "#endif",
        "",
        "/*==================================================================================================",
        "*                                     STATUS TYPES",
        "*==================================================================================================*/",
        "typedef uint8 Rte_StatusType;",
        "#define RTE_STATUS_OK           ((Rte_StatusType)0x00U)",
        "#define RTE_STATUS_ERROR        ((Rte_StatusType)0x01U)",
        "",
        "/*==================================================================================================",
        "*                                     EXTENDED RETURN CODES",
        "*==================================================================================================*/",
        "#define RTE_E_INVALID           ((Std_ReturnType)0x02U)",
        "#define RTE_E_TRANSIENT_FAULT   ((Std_ReturnType)0x03U)",
        "#define RTE_E_LIMIT             ((Std_ReturnType)0x04U)",
        "#define RTE_E_NO_DATA           ((Std_ReturnType)0x05U)",
        "#define RTE_E_TIMEOUT           ((Std_ReturnType)0x06U)",
        "#define RTE_E_MODE_INVALID      ((Std_ReturnType)0x07U)",
        "#define RTE_E_TRIGGER_PENDING   ((Std_ReturnType)0x08U)",
        "#define RTE_E_OUT_OF_RANGE      ((Std_ReturnType)0x09U)",
        "",
        "/*==================================================================================================",
        "*                                     RTE INIT / MAIN",
        "*==================================================================================================*/",
        "extern void Rte_Init(void);",
        "extern void Rte_Start(void);",
        "extern void Rte_MainFunction(void);",
        "",
    ]

    # Include per-SWC headers
    for swc in swc_list:
        lines.append(f'#include "Rte_{swc.name}.h"')

    lines.extend([
        "",
        "/*==================================================================================================",
        "*                                     RTE BUFFER STRUCT",
        "*==================================================================================================*/",
    ])

    # Generate buffer struct (standard S-R data)
    lines.append("/* Global RTE buffer definition */")
    lines.append("typedef struct")
    lines.append("{")
    for swc in swc_list:
        for port in swc.ports:
            for de in port.data_elements:
                if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                    lines.append(f"    {de.c_type} Rte_Buf_{swc.name}_{port.name}_{de.name};")
                    lines.append(f"    boolean Rte_Buf_{swc.name}_{port.name}_{de.name}_Updated;")
    lines.append("} Rte_BufferType;")
    lines.append("")
    lines.append("extern Rte_BufferType Rte_Buffer;")
    lines.append("")

    # RTE API declarations
    lines.append("/*==================================================================================================")
    lines.append("*                               RTE API FUNCTION PROTOTYPES")
    lines.append("*==================================================================================================*/")
    lines.append("#define RTE_START_SEC_CODE")
    lines.append('#include "MemMap.h"')
    lines.append("")

    # Per-SWC Read/Write declarations (standard)
    for swc in swc_list:
        for port in swc.ports:
            for de in port.data_elements:
                if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                    if port.port_type == "P_PORT":
                        lines.append(
                            f"extern Std_ReturnType Rte_Write_{swc.name}_{port.name}_{de.name}"
                            f"(const {de.c_type}* data);"
                        )
                    else:
                        lines.append(
                            f"extern Std_ReturnType Rte_Read_{swc.name}_{port.name}_{de.name}"
                            f"({de.c_type}* data);"
                        )
                else:
                    # Non-default semantics declarations
                    lines.append(_generate_data_semantics_api_declarations(swc, port, de))

    lines.append("")
    lines.append("#define RTE_STOP_SEC_CODE")
    lines.append('#include "MemMap.h"')
    lines.append("")

    # Convenience macros
    lines.append("/*==================================================================================================")
    lines.append("*                               CONVENIENCE MACROS")
    lines.append("*==================================================================================================*/")
    for swc in swc_list:
        for port in swc.ports:
            for de in port.data_elements:
                suffix = f"{swc.name.upper()}_{port.name.upper()}_{de.name.upper()}"
                if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                    if port.port_type == "P_PORT":
                        lines.append(
                            f"#define Rte_Write_{suffix}(data)  "
                            f"Rte_Write_{swc.name}_{port.name}_{de.name}(data)"
                        )
                    else:
                        lines.append(
                            f"#define Rte_Read_{suffix}(data)   "
                            f"Rte_Read_{swc.name}_{port.name}_{de.name}(data)"
                        )
                elif de.data_semantics == DataSemantics.INIT and port.port_type == "P_PORT":
                    lines.append(
                        f"#define Rte_InitData_{suffix}(data)  "
                        f"Rte_InitData_{swc.name}_{port.name}_{de.name}(data)"
                    )
                elif de.data_semantics == DataSemantics.QUEUED:
                    op = "Send" if port.port_type == "P_PORT" else "Receive"
                    lines.append(
                        f"#define Rte_{op}_{suffix}(data)       "
                        f"Rte_{op}_{swc.name}_{port.name}_{de.name}(data)"
                    )

    lines.extend([
        "",
        "#endif /* RTE_H */",
        "/*==================================================================================================",
        "*                                       END OF FILE",
        "*==================================================================================================*/",
    ])

    return '\n'.join(lines)


def _generate_rte_type_h(swc_list: List[RteSwcInfo], metadata: Dict[str, Any]) -> str:
    """Generate Rte_Type.h — shared type definitions."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/*==================================================================================================",
        f"*  Rte_Type.h — RTE Type Definitions",
        f"*  Generated : {now}",
        "*",
        "*  (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.",
        "*  All Rights Reserved.",
        "*==================================================================================================*/",
        "",
        "#ifndef RTE_TYPE_H",
        "#define RTE_TYPE_H",
        "",
        "/*==================================================================================================",
        "*                                          INCLUDES",
        "*==================================================================================================*/",
        '#include "Std_Types.h"',
        '#include "Compiler.h"',
        '#include <string.h>',
        "",
        "/*==================================================================================================",
        "*                                     TYPE DEFINITIONS",
        "*==================================================================================================*/",
    ]

    # Collect unique type mappings
    seen_types: Dict[str, str] = {}
    for swc in swc_list:
        for port in swc.ports:
            for de in port.data_elements:
                key = de.c_type
                if key not in seen_types and key not in ('uint8', 'uint16', 'uint32',
                                                         'sint8', 'sint16', 'sint32',
                                                         'boolean', 'float32', 'float64',
                                                         'uint64', 'sint64'):
                    seen_types[key] = key
        # Also scan NvBlock data types
        for nv in swc.nv_block_ports:
            key = nv.data_type
            if key not in seen_types and key not in ('uint8', 'uint16', 'uint32',
                                                     'sint8', 'sint16', 'sint32',
                                                     'boolean', 'float32', 'float64',
                                                     'uint64', 'sint64'):
                seen_types[key] = key

    if seen_types:
        for type_name in sorted(seen_types):
            lines.append(f"typedef uint8 {type_name};")
    else:
        lines.append("/* All types map to standard AUTOSAR types — no custom typedefs needed */")

    lines.extend([
        "",
        "#ifndef NULL_PTR",
        "#define NULL_PTR    ((void*)0)",
        "#endif",
        "",
        "#ifndef STATIC",
        "#define STATIC      static",
        "#endif",
        "",
        "#endif /* RTE_TYPE_H */",
        "/*==================================================================================================",
        "*                                       END OF FILE",
        "*==================================================================================================*/",
    ])

    return '\n'.join(lines)


def _generate_swc_header(swc: RteSwcInfo) -> str:
    """Generate Rte_SwcName.h — per-SWC RTE interface header."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/*==================================================================================================",
        f"*  Rte_{swc.name}.h — RTE Interface for {swc.name}",
        f"*  Generated : {now}",
        f"*  Type      : {swc.component_type}",
        "*",
        "*  (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.",
        "*  All Rights Reserved.",
        "*==================================================================================================*/",
        "",
        f"#ifndef RTE_{swc.name_upper}_H",
        f"#define RTE_{swc.name_upper}_H",
        "",
        "/*==================================================================================================",
        "*                                          INCLUDES",
        "*==================================================================================================*/",
        '#include "Rte.h"',
        '#include "Rte_Type.h"',
        "",
        "/*==================================================================================================",
        "*                                     COMPONENT ID",
        "*==================================================================================================*/",
        f"#define RTE_INSTANCE_{swc.name_upper}    (0x00U)",
        f"#define RTE_{swc.name_upper}_SWC_ID      (0x01U)",
        "",
        "/*==================================================================================================",
        "*                                     PORT API: Read/Write",
        "*==================================================================================================*/",
    ]

    # Add API prototypes per port (standard S-R)
    for port in swc.ports:
        lines.append("")
        lines.append(f"/* Port: {port.name} ({port.port_type}, {port.interface_type}) */")

        for de in port.data_elements:
            if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                if port.port_type == "P_PORT":
                    lines.append(
                        f"extern Std_ReturnType Rte_Write_{swc.name}_{port.name}_{de.name}"
                        f"(const {de.c_type}* data);"
                    )
                else:
                    lines.append(
                        f"extern Std_ReturnType Rte_Read_{swc.name}_{port.name}_{de.name}"
                        f"({de.c_type}* data);"
                    )
            else:
                lines.append(_generate_data_semantics_api_declarations(swc, port, de))

    # Client-Server operation declarations
    for port in swc.ports:
        if port.interface_type == "ClientServer":
            for op in port.operations:
                op_name = op.get('name', 'Operation')
                args = op.get('arguments', [])
                params = []
                for a in args:
                    p_type = map_to_c_type(a.get('type_ref', 'uint8'))
                    direction = a.get('direction', 'IN')
                    if direction.upper() in ('IN', 'IN'):
                        params.append(f"{p_type} {a['name']}")
                    else:
                        params.append(f"{p_type}* {a['name']}")
                params_str = ", ".join(params) if params else "void"

                if port.port_type == "R_PORT":
                    lines.append(
                        f"extern Std_ReturnType Rte_Call_{swc.name}_{port.name}_{op_name}"
                        f"({params_str});"
                    )
                else:
                    lines.append(
                        f"extern Std_ReturnType Rte_Server_{swc.name}_{port.name}_{op_name}"
                        f"({params_str});"
                    )

    # Mode Switch declarations
    ms_decls = _generate_mode_switch_api_declarations(swc)
    if ms_decls:
        lines.append("")
        lines.append("/*==================================================================================================")
        lines.append("*                                     MODE SWITCH API")
        lines.append("*==================================================================================================*/")
        lines.append(ms_decls)

    # NvBlock declarations
    nv_decls = _generate_nvblock_api_declarations(swc)
    if nv_decls:
        lines.append("")
        lines.append("/*==================================================================================================")
        lines.append("*                                     NVBLOCK API")
        lines.append("*==================================================================================================*/")
        lines.append(nv_decls)

    # Trigger declarations
    trig_decls = _generate_trigger_api_declarations(swc)
    if trig_decls:
        lines.append("")
        lines.append("/*==================================================================================================")
        lines.append("*                                     TRIGGER API")
        lines.append("*==================================================================================================*/")
        lines.append(trig_decls)

    # Runnable declarations
    if swc.runnable_entities:
        lines.append("")
        lines.append("/*==================================================================================================")
        lines.append("*                                     RUNNABLE ENTRIES")
        lines.append("*==================================================================================================*/")
        for runnable in swc.runnable_entities:
            lines.append(f"extern void {runnable.symbol}(void);")

    lines.extend([
        "",
        "#endif /* RTE_{name_upper}_H */".format(name_upper=swc.name_upper),
        "/*==================================================================================================",
        "*                                       END OF FILE",
        "*==================================================================================================*/",
    ])

    return '\n'.join(lines)


def _generate_swc_source(swc: RteSwcInfo) -> str:
    """Generate Rte_SwcName.c — per-SWC RTE implementation."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/*==================================================================================================",
        f"*  Rte_{swc.name}.c — RTE Implementation for {swc.name}",
        f"*  Generated : {now}",
        f"*  Type      : {swc.component_type}",
        "*",
        "*  (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.",
        "*  All Rights Reserved.",
        "*==================================================================================================*/",
        "",
        "/*==================================================================================================",
        "*                                          INCLUDES",
        "*==================================================================================================*/",
        f'#include "Rte_{swc.name}.h"',
        '',
    ]

    # Local buffers
    has_buffers = False
    for port in swc.ports:
        for de in port.data_elements:
            if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                has_buffers = True
                lines.append(
                    f"STATIC {de.c_type} "
                    f"Rte_Buf_{swc.name}_{port.name}_{de.name};"
                )
                lines.append(
                    f"STATIC boolean "
                    f"Rte_Buf_{swc.name}_{port.name}_{de.name}_Updated = FALSE;"
                )
            else:
                # Non-default semantics buffers
                extra = _generate_data_semantics_buffer_decls(swc, port, de)
                if extra:
                    has_buffers = True
                    lines.append(extra)

    # Mode switch buffers
    ms_bufs = _generate_mode_switch_local_buffers(swc)
    if ms_bufs:
        has_buffers = True
        lines.append(ms_bufs)

    # NvBlock buffers
    nv_bufs = _generate_nvblock_local_buffers(swc)
    if nv_bufs:
        has_buffers = True
        lines.append(nv_bufs)

    # Trigger flags
    trig_flags = _generate_trigger_local_buffers(swc)
    if trig_flags:
        has_buffers = True
        lines.append(trig_flags)

    if not has_buffers:
        lines.append("/* No local buffers required */")
    lines.append("")

    # Code section start
    lines.append("#define RTE_START_SEC_CODE")
    lines.append('#include "MemMap.h"')
    lines.append("")

    # Port API implementations (standard S-R)
    for port in swc.ports:
        for de in port.data_elements:
            if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                buf_name = f"Rte_Buf_{swc.name}_{port.name}_{de.name}"
                upd_name = f"{buf_name}_Updated"

                if port.port_type == "P_PORT":
                    lines.extend([
                        f"/**",
                        f" * @brief   Write {de.name} via port {port.name}",
                        f" * @param   data  Pointer to data to send",
                        f" * @return  E_OK if successful, E_NOT_OK otherwise",
                        f" */",
                        f"Std_ReturnType Rte_Write_{swc.name}_{port.name}_{de.name}("
                        f"const {de.c_type}* data)",
                        "{",
                        f"    if (data == NULL_PTR)",
                        "    {",
                        "        return RTE_E_INVALID;",
                        "    }",
                        "",
                        f"    {buf_name} = *data;",
                        f"    {upd_name} = TRUE;",
                        "",
                        "    return RTE_E_OK;",
                        "}",
                        "",
                    ])
                else:
                    lines.extend([
                        f"/**",
                        f" * @brief   Read {de.name} via port {port.name}",
                        f" * @param   data  Pointer to receive buffer",
                        f" * @return  E_OK if data available, E_NOT_OK otherwise",
                        f" */",
                        f"Std_ReturnType Rte_Read_{swc.name}_{port.name}_{de.name}("
                        f"{de.c_type}* data)",
                        "{",
                        f"    if (data == NULL_PTR)",
                        "    {",
                        "        return RTE_E_INVALID;",
                        "    }",
                        "",
                        f"    if ({upd_name})",
                        "    {",
                        f"        *data = {buf_name};",
                        f"        {upd_name} = FALSE;",
                        "        return RTE_E_OK;",
                        "    }",
                        "",
                        "    return RTE_E_NO_DATA;",
                        "}",
                        "",
                    ])
            else:
                # Non-default semantics implementations
                impl = _generate_data_semantics_api_impl(swc, port, de)
                if impl:
                    lines.append(impl)

    # Mode Switch implementations
    ms_impl = _generate_mode_switch_api_impl(swc)
    if ms_impl:
        lines.append(ms_impl)

    # NvBlock implementations
    nv_impl = _generate_nvblock_api_impl(swc)
    if nv_impl:
        lines.append(nv_impl)

    # Trigger implementations
    trig_impl = _generate_trigger_api_impl(swc)
    if trig_impl:
        lines.append(trig_impl)

    # Runnable skeleton implementations
    for runnable in swc.runnable_entities:
        lines.extend([
            f"/**",
            f" * @brief   Runnable: {runnable.name}",
            f" * @note    Generated skeleton — implement SWC logic here.",
            f" */",
            f"void {runnable.symbol}(void)",
            "{",
            "    /* TODO: Implement SWC logic */",
            "}",
            "",
        ])

    # Code section end
    lines.append("#define RTE_STOP_SEC_CODE")
    lines.append('#include "MemMap.h"')
    lines.append("")

    # RTE Init for this SWC
    lines.extend([
        f"/*==================================================================================================",
        f"*                              INITIALIZATION",
        f"*==================================================================================================*/",
        f"void Rte_Init_{swc.name}(void)",
        "{",
    ])
    for port in swc.ports:
        for de in port.data_elements:
            buf = f"Rte_Buf_{swc.name}_{port.name}_{de.name}"
            upd = f"{buf}_Updated"
            if de.data_semantics in (DataSemantics.DEFAULT, DataSemantics.MIMC):
                lines.append(f"    {buf} = ({de.c_type})0U;")
                lines.append(f"    {upd} = FALSE;")

    # Init mode switch buffers
    for ms in swc.mode_switch_ports:
        buf = f"Rte_ModeBuf_{swc.name}_{ms.name}"
        lines.append(f"    {buf} = 0U;")
        lines.append(f"    Rte_ModeBuf_{swc.name}_{ms.name}_Activated = FALSE;")

    # Init NvBlock buffers
    for nv in swc.nv_block_ports:
        buf = f"Rte_NvBuf_{swc.name}_{nv.name}"
        lines.append(f"    (void)memset({buf}, 0, sizeof({buf}));")
        lines.append(f"    {buf}_Valid = FALSE;")

    # Init trigger flags
    for tr in swc.trigger_ports:
        lines.append(f"    Rte_TriggerFlag_{swc.name}_{tr.name} = FALSE;")

    lines.append("}")
    lines.append("")

    lines.extend([
        "/*==================================================================================================",
        "*                                       END OF FILE",
        "*==================================================================================================*/",
    ])

    return '\n'.join(lines)


def _generate_rte_c(swc_list: List[RteSwcInfo]) -> str:
    """Generate Rte.c — global RTE implementation."""
    swc_names = [swc.name for swc in swc_list]
    runnable_symbols = []
    for swc in swc_list:
        for runnable in swc.runnable_entities:
            runnable_symbols.append((swc.name, runnable.symbol))

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "/*==================================================================================================",
        f"*  Rte.c — RTE Global Implementation",
        f"*  Generated : {now}",
        "*",
        "*  (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.",
        "*  All Rights Reserved.",
        "*==================================================================================================*/",
        "",
        "/*==================================================================================================",
        "*                                          INCLUDES",
        "*==================================================================================================*/",
        '#include "Rte.h"',
    ]
    for name in swc_names:
        lines.append(f'#include "Rte_{name}.h"')
    lines.append("")

    lines.extend([
        "/*==================================================================================================",
        "*                                     GLOBAL BUFFER",
        "*==================================================================================================*/",
        "Rte_BufferType Rte_Buffer;",
        "",
        "/*==================================================================================================",
        "*                                     LOCAL PROTOTYPES",
        "*==================================================================================================*/",
    ])
    for name in swc_names:
        lines.append(f"STATIC void Rte_Init_{name}(void);")

    if runnable_symbols:
        lines.append("")
        for _, sym in runnable_symbols:
            lines.append(f"extern void {sym}(void);")

    lines.extend([
        "",
        "#define RTE_START_SEC_CODE",
        '#include "MemMap.h"',
        "",
        "/*==================================================================================================",
        "*                                     RTE INIT",
        "*==================================================================================================*/",
        "void Rte_Init(void)",
        "{",
    ])
    for name in swc_names:
        lines.append(f"    Rte_Init_{name}();")
    lines.extend([
        "}",
        "",
        "/*==================================================================================================",
        "*                                     RTE START",
        "*==================================================================================================*/",
        "void Rte_Start(void)",
        "{",
        "    /* Start RTE scheduling */",
        "}",
        "",
        "/*==================================================================================================",
        "*                                     RTE MAIN FUNCTION",
        "*==================================================================================================*/",
        "void Rte_MainFunction(void)",
        "{",
    ])
    for swc_name, sym in runnable_symbols:
        lines.append(f"    {sym}();")
    lines.extend([
        "}",
        "",
        "#define RTE_STOP_SEC_CODE",
        '#include "MemMap.h"',
        "",
        "/*==================================================================================================",
        "*                                       END OF FILE",
        "*==================================================================================================*/",
    ])

    return '\n'.join(lines)


# ============================================================================
#  Main Generator Entry Point
# ============================================================================
def generate_rte(arxml_path: str,
                 output_dir: str,
                 swc_filter: Optional[List[str]] = None,
                 generate_rte_h: bool = True,
                 generate_rte_c: bool = True,
                 generate_rte_type_h: bool = True) -> List[str]:
    """
    Parse an ARXML file and generate RTE code.

    Args:
        arxml_path: Path to .arxml file
        output_dir: Output directory for generated files
        swc_filter: Optional list of SWC names to generate (None = all)
        generate_rte_h: Generate Rte.h
        generate_rte_c: Generate Rte.c
        generate_rte_type_h: Generate Rte_Type.h

    Returns:
        List of generated file paths
    """
    os.makedirs(output_dir, exist_ok=True)

    # Parse ARXML → IR
    try:
        if ARXMLParser is not None:
            swc_list, metadata = build_rte_ir_from_arxml(arxml_path, swc_filter)
        else:
            swc_list, metadata = _parse_arxml_direct(arxml_path)
    except Exception as e:
        logger.error("Failed to parse ARXML: %s", e)
        raise

    if not swc_list:
        logger.warning("No SWCs found in ARXML (filter: %s)", swc_filter)
        return []

    logger.info("Generating RTE code for %d SWCs into %s", len(swc_list), output_dir)
    generated = []

    # Global headers
    if generate_rte_type_h:
        path = os.path.join(output_dir, "Rte_Type.h")
        with open(path, 'w') as f:
            f.write(_generate_rte_type_h(swc_list, metadata))
        generated.append(path)
        logger.info("  Rte_Type.h")

    if generate_rte_h:
        path = os.path.join(output_dir, "Rte.h")
        with open(path, 'w') as f:
            f.write(_generate_rte_h(swc_list, metadata))
        generated.append(path)
        logger.info("  Rte.h")

    if generate_rte_c:
        path = os.path.join(output_dir, "Rte.c")
        with open(path, 'w') as f:
            f.write(_generate_rte_c(swc_list))
        generated.append(path)
        logger.info("  Rte.c")

    # Per-SWC headers and sources
    for swc in swc_list:
        h_path = os.path.join(output_dir, f"Rte_{swc.name}.h")
        with open(h_path, 'w') as f:
            f.write(_generate_swc_header(swc))
        generated.append(h_path)
        logger.info("  Rte_%s.h", swc.name)

        c_path = os.path.join(output_dir, f"Rte_{swc.name}.c")
        with open(c_path, 'w') as f:
            f.write(_generate_swc_source(swc))
        generated.append(c_path)
        logger.info("  Rte_%s.c", swc.name)

    logger.info("RTE generation complete: %d files", len(generated))
    return generated


# ============================================================================
#  CLI
# ============================================================================
def main():
    parser = argparse.ArgumentParser(
        description="yuleASR ARXML → RTE C Code Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -i config/input/arxml/example.arxml -o generated/rte/
  %(prog)s -i config/input/arxml/example.arxml -o generated/rte/ --swc BCM_Door
  %(prog)s -i input.arxml -o out/ --no-rte-c
        """
    )
    parser.add_argument('-i', '--input', required=True,
                        help='Input ARXML file path')
    parser.add_argument('-o', '--output', required=True,
                        help='Output directory for generated RTE code')
    parser.add_argument('--swc', action='append', dest='swc_filter',
                        help='Filter: only generate for specified SWC (repeatable)')
    parser.add_argument('--no-rte-h', action='store_true',
                        help='Skip generating Rte.h')
    parser.add_argument('--no-rte-c', action='store_true',
                        help='Skip generating Rte.c')
    parser.add_argument('--no-rte-type-h', action='store_true',
                        help='Skip generating Rte_Type.h')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()

    if args.verbose:
        logger.setLevel(logging.DEBUG)

    if not os.path.isfile(args.input):
        logger.error("Input file not found: %s", args.input)
        sys.exit(1)

    try:
        generated = generate_rte(
            arxml_path=args.input,
            output_dir=args.output,
            swc_filter=args.swc_filter,
            generate_rte_h=not args.no_rte_h,
            generate_rte_c=not args.no_rte_c,
            generate_rte_type_h=not args.no_rte_type_h,
        )
        print(f"\n✓ RTE generation complete: {len(generated)} file(s)")
        for g in generated:
            print(f"  {g}")
    except Exception as e:
        logger.error("RTE generation failed: %s", e)
        sys.exit(1)


if __name__ == '__main__':
    main()
