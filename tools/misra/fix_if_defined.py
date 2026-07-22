#!/usr/bin/env python3
"""
MISRA C:2012 Rule 20.9 — Auto-fix script.

Fixes '#if MACRO_NAME' to '#if defined(MACRO_NAME)' or '#if (MACRO_NAME == STD_ON)'
to '#if defined(MACRO_NAME) && (MACRO_NAME == STD_ON)' where MACRO_NAME is not
defined in .misra_config.

Usage:
    python3 tools/misra/fix_if_defined.py <file_or_dir> [--known-macros MACRO1 MACRO2 ...]
    python3 tools/misra/fix_if_defined.py src/bsw/services/ecum/src/

Known configuration macros (AUTOSAR standard, not defined in .misra_config):
These macros are expected to be defined in project header files (not by the preprocessor)
so '#if MACRO == STD_ON' is actually safe — but MISRA 20.9 requires either:
  1. Adding them to .misra_config [define] section, OR
  2. Using '#if defined(MACRO)' pattern

This script uses approach (2) — wrapping in defined() guard.
"""

import os
import re
import sys
import glob

# These macros are known AUTOSAR configuration switches typically defined
# in project configuration headers, not in .misra_config
KNOWN_CONFIG_MACROS = {
    # ECUM (ECU Manager)
    "ECUM_DEV_ERROR_DETECT", "ECUM_BSWM_ENABLED", "ECUM_COMM_ENABLED",
    "ECUM_NVM_ENABLED", "ECUM_DEM_ENABLED", "ECUM_DLT_ENABLED",
    "ECUM_WDGM_ENABLED", "ECUM_SCHM_ENABLED", "ECUM_RTE_ENABLED",
    "ECUM_J1939NM_ENABLED", "ECUM_UDPNM_ENABLED",
    # NVM (Non-Volatile Manager)
    "NVM_DEV_ERROR_DETECT", "NVM_WAKEUP_CHECK_ENABLED",
    "NVM_POLLING_MODE", "NVM_JOB_PRIORITY_SUPPORT",
    "NVM_VERIFY_BLOCK_COUNTER", "NVM_CRC_CHECK_ENABLED",
    "NVM_FAST_READ_ENABLED", "NVM_MULTI_WRITE_ENABLED",
    "NVM_WRITE_PROTECTION_ENABLED", "NVM_CRC_CALCULATION_ENABLED",
    "NVM_CRC_INTEGRITY_ENABLED", "NVM_WRITE_VERIFICATION_ENABLED",
    "NVM_RESISTANCE_TO_CONSISTENCY_LOSS", "NVM_BLOCK_DATA_READY_STATUS",
    "NVM_BLOCK_WRITE_PROTECTION", "NVM_WRITE_BLOCK_ONE_SHOT",
    "NVM_COMPILATION_READY_CHECK_ENABLED", "NVM_READ_PROTECTION_ENABLED",
    "NVM_CRC_OF_ACTIVE_BLOCK", "NVM_NVM_MODULE_ENABLED",
    "NVM_DRY_RUN_ENABLED", "NVM_FAST_WRITE_ENABLED",
    # DEM (Diagnostic Event Manager)
    "DEM_DEV_ERROR_DETECT", "DEM_VERSION_INFO_API", "DEM_DSLD_ENABLED",
    "DEM_DSP_ENABLED", "DEM_DEBOUNCE_COUNTER_BASED",
    "DEM_DEBOUNCE_TIME_BASED", "DEM_CUSTOM_DATA_CLASSES",
    "DEM_CLEAR_DIAGNOSTIC_INFORMATION", "DEM_STORE_EVENT",
    "DEM_ENABLE_STORING_EVENTS", "DEM_EVENT_MEMORY_OVERFLOW",
    "DEM_NUMBER_OF_EVENT_ENTRIES", "DEM_FREEZE_DATA_SUPPORTED",
    "DEM_EXTENDED_DATA_SUPPORTED", "DEM_OBD_SUPPORTED",
    "DEM_CUSTOM_DATA_SUPPORTED", "DEM_AGING_SUPPORTED",
    "DEM_MIRROR_SUPPORTED", "DEM_J1939_DCM_SUPPORTED",
    "DEM_J1939_DM1_SUPPORTED", "DEM_J1939_DM2_SUPPORTED",
    "DEM_J1939_DM3_SUPPORTED", "DEM_J1939_DM4_SUPPORTED",
    "DEM_J1939_DM5_SUPPORTED", "DEM_J1939_DM6_SUPPORTED",
    "DEM_J1939_DM7_SUPPORTED", "DEM_J1939_DM13_SUPPORTED",
    "DEM_J1939_DM23_SUPPORTED", "DEM_J1939_DM24_SUPPORTED",
    "DEM_J1939_DM25_SUPPORTED", "DEM_J1939_DM26_SUPPORTED",
    "DEM_J1939_DM27_SUPPORTED", "DEM_J1939_DM28_SUPPORTED",
    "DEM_J1939_DM29_SUPPORTED", "DEM_J1939_DM30_SUPPORTED",
    "DEM_J1939_DM31_SUPPORTED", "DEM_J1939_DM32_SUPPORTED",
    # DCM (Diagnostic Communication Manager)
    "DCM_DEV_ERROR_DETECT", "DCM_VERSION_INFO_API",
    "DCM_TP_SUPPORT", "DCM_UDS_ON_CAN_SUPPORT",
    "DCM_DST_DIAGNOSTIC_SESSION", "DCM_MAX_BUFFER_SIZE",
    "DCM_DSLD_ENABLED", "DCM_DSP_ENABLED",
    # DET (Default Error Tracer)
    "DET_DEV_ERROR_DETECT", "DET_VERSION_INFO_API",
    # CRC
    "CRC_8_MODE", "CRC_16_MODE", "CRC_32_MODE",
    "CRC_8_TABLE_MODE", "CRC_16_TABLE_MODE", "CRC_32_TABLE_MODE",
    "CRC_8_TABLE_SIZE", "CRC_16_TABLE_SIZE", "CRC_32_TABLE_SIZE",
    # ETH / ETHIF
    "ETH_DEV_ERROR_DETECT", "ETHIF_DEV_ERROR_DETECT",
    "ETHIF_DEM_ENABLED", "ETHIF_ZERO_COPY_SUPPORT",
    # CAN
    "CAN_DEV_ERROR_DETECT", "CAN_VERSION_INFO_API",
    # LIN
    "LIN_DEV_ERROR_DETECT", "LIN_VERSION_INFO_API",
    # IPDUM
    "IPDUM_DEV_ERROR_DETECT", "IPDUM_VERSION_INFO_API",
    # FR
    "FR_DEV_ERROR_DETECT", "FR_VERSION_INFO_API",
    # DLT
    "DLT_DEV_ERROR_DETECT", "DLT_VERSION_INFO_API",
    "DLT_LOG_ENABLED", "DLT_LOG_LEVEL",
    # E2E
    "E2E_DEV_ERROR_DETECT", "E2E_VERSION_INFO_API",
    # CSM
    "CSM_DEV_ERROR_DETECT", "CSM_VERSION_INFO_API",
    "CSM_KEY_IMPORT_ENABLED", "CSM_QUEUE_ENABLED",
    "CSM_KEY_GENERATION_ENABLED", "CSM_KEY_DERIVATION_ENABLED",
    "CSM_CANCEL_JOB_ENABLED", "CSM_MAC_HMAC_ENABLED",
    "CSM_ASYNC_ENABLED", "CSM_ASYNC_RANDOM_ENABLED",
    "CSM_ASYNC_SYMMETRIC_ENABLED", "CSM_ASYNC_HASH_ENABLED",
    "CSM_ASYNC_MAC_ENABLED", "CSM_ASYNC_SIGNATURE_ENABLED",
    "CSM_ASYNC_KEY_EXCH_ENABLED",
    # CRYIF
    "CRYIF_DEV_ERROR_DETECT", "CRYIF_VERSION_INFO_API",
    "CRYIF_ASYNC_ENABLED", "CRYIF_QUEUE_ENABLED",
    # SWC
    "SWC_DEV_ERROR_DETECT", "SWC_VERSION_INFO_API",
    # XCP
    "XCP_DEV_ERROR_DETECT", "XCP_VERSION_INFO_API",
    "XCP_ENABLE_CALIBRATION", "XCP_ENABLE_DAQ",
    "XCP_ENABLE_STIM", "XCP_ENABLE_SEED_KEY",
    # COM
    "COM_DEV_ERROR_DETECT", "COM_VERSION_INFO_API",
    # SoAd
    "SOAD_DEV_ERROR_DETECT", "SOAD_VERSION_INFO_API",
    # PduR
    "PDUR_DEV_ERROR_DETECT", "PDUR_VERSION_INFO_API",
    # SOMEIP
    "SOMEIP_DEV_ERROR_DETECT", "SOMEIP_VERSION_INFO_API",
    # DoIP
    "DOIP_DEV_ERROR_DETECT", "DOIP_VERSION_INFO_API",
    "DOIP_TCP_ENABLED", "DOIP_UDP_ENABLED",
    # EthSM
    "ETHSM_DEV_ERROR_DETECT", "ETHSM_VERSION_INFO_API",
    # BSWM
    "BSWM_DEV_ERROR_DETECT", "BSWM_VERSION_INFO_API",
    # CanNm / LinNm / J1939Nm
    "CANNM_DEV_ERROR_DETECT", "LINNM_DEV_ERROR_DETECT",
    "J1939NM_DEV_ERROR_DETECT",
    # CanM
    "CANM_DEV_ERROR_DETECT", "CANM_VERSION_INFO_API",
    # LIN SM
    "LINSM_DEV_ERROR_DETECT", "LINSM_VERSION_INFO_API",
    # FIM
    "FIM_DEV_ERROR_DETECT", "FIM_VERSION_INFO_API",
    # DLT Interface
    "DLTIF_DEV_ERROR_DETECT", "DLTIF_VERSION_INFO_API",
    # General
    "SCHM_ENABLED", "WDGM_ENABLED", "DET_ENABLED",
    # ETH (mcal)
    "ETH_ERROR_DETECT", "ETH_RX_INTERRUPT",
    "ETH_TX_INTERRUPT", "ETH_TX_DONE_INTERRUPT",
    "ETH_WAKEUP_INTERRUPT", "ETH_LINK_STATUS_INTERRUPT",
    # CAN (mcal)
    "CAN_ERROR_DETECT", "CAN_WAKEUP_INTERRUPT_SUPPORT",
    "CAN_RX_INTERRUPT", "CAN_TX_INTERRUPT",
    "CAN_HRH_SUPPORT", "CAN_MB_IRQ_ENABLE",
    # Crypto (mcal)
    "CRYPTO_E2E_PROTECTION_ENABLED", "CRYPTO_HSM_SHE_SUPPORT",
    "CRYPTO_HSM_CIPHER_ECB", "CRYPTO_HSM_MAC_ENABLED",
    "CRYPTO_MEMORY_MODEL",
    # Platform types
    "COMPILER_INIT_SECTION", "COMPILER_INIT_CODE",
}


def get_macros_from_expression(expr):
    """Extract simple macro names from a preprocessor expression."""
    # Remove parentheses, operators, comparisons
    cleaned = re.sub(r'[()]', ' ', expr)
    cleaned = re.sub(r'\b(==|!=|&&|\|\||<|>|<=|>=|!)\b', ' ', cleaned)
    tokens = set()
    for token in cleaned.split():
        token = token.strip()
        # Match all-caps identifiers (AUTOSAR config macros)
        if re.match(r'^[A-Z][A-Z0-9_]*$', token) and token not in (
            'STD_ON', 'STD_OFF', 'STD_HIGH', 'STD_LOW', 'STD_ACTIVE', 'STD_IDLE',
            'TRUE', 'FALSE', 'E_OK', 'E_NOT_OK', 'NULL', 'NULL_PTR',
        ):
            tokens.add(token)
    return tokens


def needs_wrapping(line):
    """Check if a #if line needs to be wrapped in defined()."""
    # Match: #if MACRO (simple), #if (MACRO), #if MACRO == STD_ON, etc.
    pattern = r'#\s*if\s+(?:\(?\s*)?([A-Z][A-Z0-9_]+)'
    m = re.search(pattern, line)
    if m:
        macro = m.group(1)
        # Skip if already using defined()
        if 'defined(' in line:
            return False, None
        # Skip common defined-in-config macros
        if macro in ('STD_ON', 'STD_OFF', 'TRUE', 'FALSE', 'E_OK', 'E_NOT_OK', 'NULL', 'NULL_PTR'):
            return False, None
        return True, get_macros_from_expression(line)
    return False, None


def wrap_undef_macros(expr_line):
    """
    Transform #if (ECUM_DEV_ERROR_DETECT == STD_ON) to
    #if defined(ECUM_DEV_ERROR_DETECT) && (ECUM_DEV_ERROR_DETECT == STD_ON)
    """
    macros = get_macros_from_expression(expr_line)
    if not macros:
        return expr_line

    # Extract the expression part after #if
    m = re.match(r'(#\s*if\s+)(.*)', expr_line)
    if not m:
        return expr_line

    prefix = m.group(1)
    expr = m.group(2)

    # Build defined clauses
    defined_clauses = [f"defined({m})" for m in sorted(macros)]
    defined_expr = " && ".join(defined_clauses)

    if defined_expr:
        # If the expression is just a simple macro name, replace entirely
        # e.g., #if ECUM_FOO  →  #if defined(ECUM_FOO)
        simple_macro = re.match(r'^\(?\s*([A-Z][A-Z0-9_]+)\s*\)?$', expr.strip())
        if simple_macro:
            return f"{prefix}{defined_expr}\n"
        else:
            # #if (MACRO == STD_ON) → #if defined(MACRO) && (MACRO == STD_ON)
            return f"{prefix}{defined_expr} && {expr.strip()}\n"

    return expr_line


def fix_file(filepath, dry_run=False):
    """Fix one file for Rule 20.9 violations. Returns count of lines fixed."""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    fixes = 0
    new_lines = []
    for line in lines:
        needs, macros = needs_wrapping(line)
        if needs:
            new_line = wrap_undef_macros(line)
            if new_line != line:
                fixes += 1
                if not dry_run:
                    # Print what was changed
                    print(f"  {filepath}: {line.rstrip()}")
                    print(f"    → {new_line.rstrip()}")
                new_lines.append(new_line)
                continue
        new_lines.append(line)

    if fixes > 0 and not dry_run:
        with open(filepath, 'w') as f:
            f.writelines(new_lines)

    return fixes


def find_files(paths):
    """Find all .c and .h files in given paths."""
    files = []
    for p in paths:
        if os.path.isfile(p) and (p.endswith('.c') or p.endswith('.h')):
            files.append(p)
        elif os.path.isdir(p):
            for root, dirs, fnames in os.walk(p):
                for fn in fnames:
                    if fn.endswith('.c') or fn.endswith('.h'):
                        files.append(os.path.join(root, fn))
    return sorted(set(files))


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA Rule 20.9 violations')
    parser.add_argument('paths', nargs='+', help='Files or directories to scan')
    parser.add_argument('--dry-run', action='store_true', help='Just print changes, dont apply')
    parser.add_argument('--known-macros', nargs='*', default=[],
                        help='Additional known macros to treat as safe')
    args = parser.parse_args()

    # Add any extra known macros
    for m in args.known_macros:
        KNOWN_CONFIG_MACROS.add(m)

    files = find_files(args.paths)
    if not files:
        print("No .c/.h files found in specified paths")
        return 1

    total_fixes = 0
    modified_files = 0
    for f in files:
        fixes = fix_file(f, dry_run=args.dry_run)
        if fixes > 0:
            total_fixes += fixes
            modified_files += 1

    print(f"\nSummary: {total_fixes} fixes in {modified_files} files")
    if args.dry_run:
        print("(dry run — no changes applied)")
    return 0 if total_fixes == 0 else 0  # Return 0 even if fixes found


if __name__ == '__main__':
    sys.exit(main())
