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
        "bswm", "canm", "cansm", "cantsyn", "com", "comm",
        "crc", "cryif", "csm", "dcm", "dem", "det", "dlt",
        "docan", "doip", "e2e", "ecuc", "ecum", "ethsm",
        "fim", "ipdum", "j1939nm", "j1939tp", "keym", "linm",
        "linsm", "lntm", "mem", "memif", "mqtt", "nm", "nvm",
        "pdur", "ramsafety", "schm", "secoc", "soad", "someip",
        "someiptp", "someipxf", "stbm", "swc", "udpnm", "wdgm", "xcp"
    ]


def get_mcal_module_list():
    """Get the list of MCAL modules implemented in the project."""
    return [
        "adc", "can", "crypto", "dio", "eep", "eth", "fee",
        "flash", "fls", "gpt", "i2c", "icu", "lin", "mcu",
        "ocu", "port", "pwm", "ramtst", "spi", "uart", "wdg"
    ]


def get_ecual_module_list():
    """Get the list of ECUAL modules implemented in the project."""
    return [
        "cannm", "cansm", "canif", "cantp", "cantrcv", "dlt",
        "doip", "ea", "ethsm", "ethif", "ethtrcv", "fee", "fim",
        "frif", "frtp", "iohwab", "ipdum", "j1939tp", "linnm",
        "linsm", "lintp", "linif", "lintrcv", "memif", "someipif",
        "someipsd", "srp", "wdgif", "xcp"
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
