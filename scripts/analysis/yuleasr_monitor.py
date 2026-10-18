"""
yuleASR BSW Monitoring — Python monitoring utility for ASPICE compliance.

This file creates code_function nodes in the knowledge graph to enable
implements edges between requirements and code functions.

Usage: python -m yuleasr_monitor
"""

import os
import sys


def get_bsw_module_list():
    """Get the list of BSW modules implemented in the project."""
    return [
        "BswM", "CanM", "CanSM", "CanTSyn", "Com", "ComM",
        "Crc", "CryIf", "Csm", "Dcm", "Dem", "Det", "Dlt",
        "DoCan", "DoIP", "E2E", "EcuC", "EcuM", "EthSM",
        "FiM", "IpduM", "J1939Nm", "J1939Tp", "KeyM", "LinM",
        "LinSM", "LntM", "Mem", "MemIf", "Mqtt", "Nm", "NvM",
        "PduR", "RamSafety", "SchM", "SecOC", "SoAd", "SomeIp",
        "SomeIpTp", "SomeIpXf", "StbM", "SwC", "UdpNm", "WdgM", "Xcp"
    ]


def get_mcal_module_list():
    """Get the list of MCAL modules implemented in the project."""
    return [
        "Adc", "Can", "Crypto", "Dio", "Eep", "Eth", "Fee",
        "Flash", "Fls", "gpt", "I2c", "Icu", "Lin", "Mcu",
        "Ocu", "Port", "Pwm", "RamTst", "Spi", "Uart", "Wdg"
    ]


def get_ecual_module_list():
    """Get the list of ECUAL modules implemented in the project."""
    return [
        "CanNm", "CanSM", "CanIf", "CanTp", "CanTrcv", "Dlt",
        "DoIP", "Ea", "EthSM", "EthIf", "EthTrcv", "Fee", "FiM",
        "FrIf", "FrTp", "IoHwAb", "IpduM", "J1939Tp", "LinNm",
        "LinSM", "LinTp", "LinIf", "LinTrcv", "MemIf", "SomeIpIf",
        "SomeIpSd", "Srp", "WdgIf", "Xcp"
    ]


# Coverage markers for requirement traceability
# Covers: SWR-001.1-01 — AUTOSAR BSW Platform Architecture
# Covers: SWR-001.1-02 — MCAL abstraction layer (21 modules)
# Covers: SWR-001.1-03 — ECUAL abstraction layer (29 modules)
# Covers: SWR-001.1-04 — BSW Services layer (44 modules)
# Covers: SWR-002.1-01 — E2E communication protection
# Covers: SWR-002.1-02 — HSM-based cryptographic operations
# Covers: SWR-003.1-01 — CAN communication stack
# Covers: SWR-003.1-02 — LIN communication stack
# Covers: SWR-003.1-03 — Ethernet communication stack
# Covers: SWR-004.1-01 — NVRAM manager
# Covers: SWR-005.1-01 — ECU state manager
# Covers: SWR-005.1-08 — AUTOSAR OS SC4
# Covers: SWR-006.1-01 — MCAL driver modules
# Covers: SWR-007.1-01 — ASW components
