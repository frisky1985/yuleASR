# -*- coding: utf-8 -*-
"""
yuleASR RTE Code Generator Package
===================================
Generates AUTOSAR Classic Platform R20-11 RTE C code from ARXML system
descriptions.

Supported SWC Communication Patterns:
  - Sender-Receiver (basic)          — Rte_Read / Rte_Write
  - Sender-Receiver (data semantics) — Rte_InitData, Rte_Send/Rte_Receive (QUEUED)
  - Client-Server                    — Rte_Call / Rte_Server
  - Mode Switch                      — Rte_Switch / Rte_Mode
  - NvBlock                          — Rte_ReadBlock / Rte_WriteBlock
  - Trigger                          — Rte_Trigger / Rte_Event
"""
__version__ = '1.1.0'

from .rte_generator import (
    # Main API
    generate_rte,
    build_rte_ir_from_arxml,

    # Utilities
    map_to_c_type,
    extract_short_name_from_ref,
    resolve_data_semantics,

    # Data models
    DataSemantics,
    ComPattern,
    NvBlockType,
    RteSwcInfo,
    RtePortInfo,
    RteDataElementInfo,
    RteRunnableInfo,
    RteModeSwitchInfo,
    RteNvBlockInfo,
    RteTriggerInfo,

    # Internal generators (useful for testing)
    _generate_rte_h,
    _generate_rte_c,
    _generate_rte_type_h,
    _generate_swc_header,
    _generate_swc_source,
    _generate_mode_switch_api_declarations,
    _generate_mode_switch_api_impl,
    _generate_nvblock_api_declarations,
    _generate_nvblock_api_impl,
    _generate_trigger_api_declarations,
    _generate_trigger_api_impl,
    _generate_data_semantics_api_impl,
)
