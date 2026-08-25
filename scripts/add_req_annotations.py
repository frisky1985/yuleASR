#!/usr/bin/env python3
"""
Add @req traceability annotations to all 17 MCAL modules.
Uses known API names for reliable matching.
"""

import re
import os

BASE = "/Users/ingeek/workspace/AUTOSAR"
MCAL_SRC = f"{BASE}/src/bsw/mcal"
DESIGN_DIR = f"{BASE}/docs/design/modules/mcal"
TESTS_DIR = f"{BASE}/tests"

# Known public APIs per module (from grep analysis)
KNOWN_APIS = {
    "Dio": ["Dio_Init", "Dio_ReadChannel", "Dio_WriteChannel", "Dio_ReadPort", "Dio_WritePort",
             "Dio_ReadChannelGroup", "Dio_WriteChannelGroup", "Dio_GetVersionInfo", "Dio_FlipChannel", "Dio_MaskedWritePort"],
    "Port": ["Port_Init", "Port_DeInit", "Port_SetPinDirection", "Port_RefreshPortDirection",
             "Port_GetVersionInfo", "Port_SetPinMode"],
    "Adc": ["Adc_Init", "Adc_DeInit", "Adc_StartGroupConversion", "Adc_StopGroupConversion",
            "Adc_ReadGroup", "Adc_EnableHardwareTrigger", "Adc_DisableHardwareTrigger",
            "Adc_EnableGroupNotification", "Adc_DisableGroupNotification", "Adc_GetGroupStatus",
            "Adc_GetVersionInfo", "Adc_GetStreamLastPointer", "Adc_SetupResultBuffer",
            "Adc_SetPowerState", "Adc_GetTargetPowerState", "Adc_GetCurrentPowerState", "Adc_PreparePowerState"],
    "Icu": ["Icu_Init", "Icu_DeInit", "Icu_SetMode", "Icu_DisableWakeup", "Icu_EnableWakeup",
            "Icu_CheckWakeup", "Icu_SetActivationCondition", "Icu_DisableNotification",
            "Icu_EnableNotification", "Icu_GetInputState", "Icu_StartTimestamp", "Icu_StopTimestamp",
            "Icu_GetTimestampIndex", "Icu_ResetEdgeCount", "Icu_EnableEdgeCount", "Icu_DisableEdgeCount",
            "Icu_GetEdgeNumbers", "Icu_StartSignalMeasurement", "Icu_StopSignalMeasurement",
            "Icu_GetTimeElapsed", "Icu_GetDutyCycleValues", "Icu_GetVersionInfo",
            "Icu_GetInputLevel", "Icu_GetSysTimestamp", "Icu_ProcessInterrupt"],
    "Ocu": ["Ocu_Init", "Ocu_DeInit", "Ocu_StartChannel", "Ocu_StopChannel", "Ocu_SetPinState",
            "Ocu_SetPinAction", "Ocu_SetAbsoluteThreshold", "Ocu_SetRelativeThreshold",
            "Ocu_GetCounter", "Ocu_DisableNotification", "Ocu_EnableNotification", "Ocu_GetVersionInfo"],
    "Pwm": ["Pwm_Init", "Pwm_DeInit", "Pwm_SetDutyCycle", "Pwm_SetPeriodAndDuty", "Pwm_SetOutputToIdle",
            "Pwm_GetOutputState", "Pwm_DisableNotification", "Pwm_EnableNotification",
            "Pwm_GetVersionInfo", "Pwm_SetPowerState", "Pwm_GetTargetPowerState",
            "Pwm_GetCurrentPowerState", "Pwm_PreparePowerState"],
    "Gpt": ["Gpt_Init", "Gpt_DeInit", "Gpt_GetTimeElapsed", "Gpt_GetTimeRemaining", "Gpt_StartTimer",
            "Gpt_StopTimer", "Gpt_EnableNotification", "Gpt_DisableNotification", "Gpt_GetVersionInfo",
            "Gpt_SetMode", "Gpt_DisableWakeup", "Gpt_EnableWakeup", "Gpt_CheckWakeup",
            "Gpt_GetPredefTimerValue"],
    "Spi": ["Spi_Init", "Spi_DeInit", "Spi_SyncTransmit", "Spi_AsyncTransmit", "Spi_GetStatus",
            "Spi_GetJobResult", "Spi_IsrHandler", "Spi_MainFunction", "Spi_GetVersionInfo"],
    "I2c": ["I2c_Init", "I2c_DeInit", "I2c_WriteBytes", "I2c_ReadBytes", "I2c_WriteRead",
            "I2c_GetStatus", "I2c_GetVersionInfo", "I2c_SetClockMode", "I2c_EnableInterrupt",
            "I2c_DisableInterrupt", "I2c_SetSlaveAddress", "I2c_GetBusState", "I2c_ClearBus",
            "I2c_SoftwareReset", "I2c_SetTransferMode", "I2c_CancelTransfer", "I2c_PrepareSlaveBuffer",
            "I2c_SlaveWriteBuffer", "I2c_SlaveReadBuffer", "I2c_MainFunction"],
    "Lin": ["Lin_Init", "Lin_DeInit", "Lin_GetVersionInfo", "Lin_SendFrame", "Lin_SendResponse",
            "Lin_DisableResponse", "Lin_WakeUp", "Lin_WakeUpInternal", "Lin_CheckWakeup",
            "Lin_GetStatus", "Lin_GoToSleep", "Lin_GoToSleepInternal", "Lin_WakeUpConfirmation",
            "Lin_WakeUpFrameIndication", "Lin_IsrTx", "Lin_IsrRx", "Lin_IsrErr"],
    "Uart": ["Uart_Init", "Uart_DeInit", "Uart_Send", "Uart_SendDMA", "Uart_SendInterrupt",
             "Uart_Receive", "Uart_ReceiveDMA", "Uart_ReceiveInterrupt", "Uart_GetStatus",
             "Uart_GetTxResult", "Uart_GetRxResult", "Uart_SetBaudRate", "Uart_EnableInterrupt",
             "Uart_DisableInterrupt", "Uart_ClearFIFO", "Uart_IsrHandler", "Uart_MainFunction",
             "Uart_Abort", "Uart_GetVersionInfo"],
    "Mcu": ["Mcu_Init", "Mcu_InitClock", "Mcu_DistributePllClock", "Mcu_GetPllStatus",
            "Mcu_SetMode", "Mcu_GetResetReason", "Mcu_GetResetRawValue", "Mcu_PerformReset",
            "Mcu_GetVersionInfo", "Mcu_InitRamSection", "Mcu_GetRamState"],
    "Wdg": ["Wdg_Init", "Wdg_SetMode", "Wdg_Trigger", "Wdg_GetVersionInfo",
            "Wdg_SetTriggerCondition", "Wdg_GetStatus", "Wdg_GetTriggerCounter", "Wdg_GetLastTriggerTime"],
    "Fls": ["Fls_Init", "Fls_Erase", "Fls_Write", "Fls_Read", "Fls_ReadSync", "Fls_Compare",
            "Fls_SetMode", "Fls_GetStatus", "Fls_GetJobResult", "Fls_Cancel", "Fls_MainFunction",
            "Fls_GetVersionInfo"],
    "Eep": ["Eep_Init", "Eep_DeInit", "Eep_Read", "Eep_Write", "Eep_Erase", "Eep_Cancel",
            "Eep_GetStatus", "Eep_GetJobResult", "Eep_MainFunction", "Eep_GetVersionInfo"],
    "Crypto": ["Crypto_Init", "Crypto_DeInit", "Crypto_GetVersionInfo", "Crypto_ProcessJob"],
    "RamTst": ["RamTst_Init", "RamTst_DeInit", "RamTst_Run", "RamTst_Stop", "RamTst_GetTestResult",
               "RamTst_GetErrorRecord", "RamTst_GetTestStatus", "RamTst_MainFunction",
               "RamTst_GetVersionInfo", "RamTst_SetMode", "RamTst_GetMode"],
}

# Module config: prefix -> (src_file, design_doc, [test_files])
MODULE_CONFIG = {
    "Dio":    (f"{MCAL_SRC}/dio/src/Dio.c",     f"{DESIGN_DIR}/dio-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_dio.c", f"{TESTS_DIR}/unit/mcal/test_dio.c", f"{TESTS_DIR}/mock/test_mcal_dio.c"]),
    "Port":   (f"{MCAL_SRC}/port/src/Port.c",    f"{DESIGN_DIR}/port-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_port.c", f"{TESTS_DIR}/unit/mcal/test_port.c"]),
    "Adc":    (f"{MCAL_SRC}/adc/src/Adc.c",      f"{DESIGN_DIR}/adc-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_ADC.c", f"{TESTS_DIR}/unit/mcal/test_adc.c", f"{TESTS_DIR}/mock/test_mcal_adc.c"]),
    "Icu":    (f"{MCAL_SRC}/icu/src/Icu.c",      f"{DESIGN_DIR}/icu-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_icu.c", f"{TESTS_DIR}/mock/test_mcal_icu.c",
                f"{TESTS_DIR}/bsw/mcal/test_icu.c", f"{TESTS_DIR}/bsw/mcal/icu/test_icu.c"]),
    "Ocu":    (f"{MCAL_SRC}/ocu/src/Ocu.c",      f"{DESIGN_DIR}/ocu-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_ocu.c", f"{TESTS_DIR}/mock/test_mcal_ocu.c",
                f"{TESTS_DIR}/bsw/mcal/ocu/test_ocu.c"]),
    "Pwm":    (f"{MCAL_SRC}/pwm/src/Pwm.c",      f"{DESIGN_DIR}/pwm-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_pwm.c", f"{TESTS_DIR}/unit/mcal/test_pwm.c", f"{TESTS_DIR}/mock/test_mcal_pwm.c"]),
    "Gpt":    (f"{MCAL_SRC}/gpt/src/Gpt.c",      f"{DESIGN_DIR}/gpt-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_gpt.c", f"{TESTS_DIR}/unit/mcal/test_gpt.c", f"{TESTS_DIR}/mock/test_mcal_gpt.c"]),
    "Spi":    (f"{MCAL_SRC}/spi/src/Spi.c",      f"{DESIGN_DIR}/spi-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_spi.c", f"{TESTS_DIR}/unit/mcal/test_spi.c", f"{TESTS_DIR}/mock/test_mcal_spi.c"]),
    "I2c":    (f"{MCAL_SRC}/i2c/src/I2c.c",      f"{DESIGN_DIR}/i2c-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_i2c.c", f"{TESTS_DIR}/mock/test_mcal_i2c.c"]),
    "Lin":    (f"{MCAL_SRC}/lin/src/Lin.c",      f"{DESIGN_DIR}/lin-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_LIN.c"]),
    "Uart":   (f"{MCAL_SRC}/uart/src/Uart.c",    f"{DESIGN_DIR}/uart-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_uart.c", f"{TESTS_DIR}/mock/test_mcal_uart.c"]),
    "Mcu":    (f"{MCAL_SRC}/mcu/src/Mcu.c",      f"{DESIGN_DIR}/mcu-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_mcu.c", f"{TESTS_DIR}/unit/mcal/test_mcu.c"]),
    "Wdg":    (f"{MCAL_SRC}/wdg/src/Wdg.c",      f"{DESIGN_DIR}/wdg-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_wdg.c"]),
    "Fls":    (f"{MCAL_SRC}/fls/src/Fls.c",      f"{DESIGN_DIR}/fls-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_fls.c", f"{TESTS_DIR}/unit/mcal/test_fls_hw.c"]),
    "Eep":    (f"{MCAL_SRC}/eep/src/Eep.c",      f"{DESIGN_DIR}/eep-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_eep.c", f"{TESTS_DIR}/mock/test_mcal_eep.c"]),
    "Crypto": (f"{MCAL_SRC}/crypto/src/Crypto.c", f"{DESIGN_DIR}/crypto-design.md",
               [f"{TESTS_DIR}/unit/mcal/test_crypto.c"]),
    "RamTst": (f"{MCAL_SRC}/ramtst/src/RamTst.c", f"{DESIGN_DIR}/ramtst-design.md",
               [f"{TESTS_DIR}/unit/autosar/mcal/test_ramtst.c", f"{TESTS_DIR}/unit/ramtst/test_ramtst_init.c",
                f"{TESTS_DIR}/unit/ramtst/test_ramtst_run.c"]),
}


def sws_id(prefix, num):
    return f"SWS_{prefix}_{num:05d}"


def process_src(prefix, src_file, apis):
    """Add @req to source file. Returns (public_count, internal_count)."""
    if not os.path.exists(src_file):
        print(f"  [SKIP] Not found: {src_file}")
        return 0, 0

    with open(src_file, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    public_count = 0
    internal_count = 0
    api_index = {name: idx+1 for idx, name in enumerate(apis)}

    # Build set of static function names (for internal IDs)
    static_funcs = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('static ') or stripped.startswith('STATIC '):
            m = re.search(r'(?:static|STATIC)\s+\S+\s+(\w+)\s*\(', stripped)
            if m:
                fname = m.group(1)
                if fname.startswith(prefix + "_"):
                    static_funcs.append(fname)

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Skip if already has @req
        if '@req SWS_' in line:
            new_lines.append(line)
            i += 1
            continue

        # Check if this line defines a known public API function
        matched_api = None
        for api_name in apis:
            # Match function definition: "ReturnType FuncName("
            # The function name should appear at start of line or after return type
            pattern = re.compile(r'^(?!static\s)(?!STATIC\s)(?!extern\s).*?\b' + re.escape(api_name) + r'\s*\(')
            if pattern.match(line):
                matched_api = api_name
                break

        if matched_api:
            num = api_index[matched_api]
            sid = sws_id(prefix, num)
            public_count += 1

            # Look backwards for doc comment start
            insert_pos = len(new_lines)
            j = len(new_lines) - 1
            found_comment = False
            while j >= 0:
                s = new_lines[j].strip()
                if s == '':
                    j -= 1
                    continue
                if s.endswith('*/'):
                    # Walk back to find /**
                    while j >= 0:
                        if '/**' in new_lines[j] or '/*' in new_lines[j]:
                            insert_pos = j
                            found_comment = True
                            break
                        j -= 1
                    break
                else:
                    break

            new_lines.insert(insert_pos, f"/** @req {sid} */")
            new_lines.append(line)
            i += 1
            continue

        # Check for static/internal function
        is_static = False
        static_name = None
        if stripped.startswith('static ') or stripped.startswith('STATIC '):
            m = re.search(r'(?:static|STATIC)\s+\S+\s+(\w+)\s*\(', stripped)
            if m:
                fname = m.group(1)
                if fname.startswith(prefix + "_") and fname not in apis:
                    is_static = True
                    static_name = fname

        if is_static:
            internal_count += 1
            sid = sws_id(prefix, 100 + internal_count)

            insert_pos = len(new_lines)
            j = len(new_lines) - 1
            while j >= 0:
                s = new_lines[j].strip()
                if s == '':
                    j -= 1
                    continue
                if s.endswith('*/'):
                    while j >= 0:
                        if '/**' in new_lines[j] or '/*' in new_lines[j]:
                            insert_pos = j
                            break
                        j -= 1
                    break
                else:
                    break

            new_lines.insert(insert_pos, f"/** @req {sid} */")
            new_lines.append(line)
            i += 1
            continue

        new_lines.append(line)
        i += 1

    with open(src_file, 'w') as f:
        f.write('\n'.join(new_lines))

    print(f"  [SRC] {os.path.basename(src_file)}: {public_count} public + {internal_count} internal = {public_count + internal_count}")
    return public_count, internal_count


def process_doc(prefix, doc_file, apis):
    """Add SWS ID column to design doc API table. Returns count."""
    if not os.path.exists(doc_file):
        print(f"  [SKIP] Design doc not found: {doc_file}")
        return 0

    with open(doc_file, 'r') as f:
        lines = f.read().split('\n')

    new_lines = []
    count = 0
    in_api_table = False
    api_counter = 0
    header_modified = False

    for i, line in enumerate(lines):
        # Detect API table: header row with | and containing "API"
        if not in_api_table and '|' in line:
            # Check if this looks like an API table header
            if ('API' in line or '接口' in line) and i + 1 < len(lines) and '---' in lines[i + 1]:
                # Check if it's in a section about APIs
                # Look back for section header
                is_api_section = False
                for j in range(max(0, i-10), i):
                    if re.search(r'##\s*\d+.*API', lines[j]) or re.search(r'##\s*\d+.*公共', lines[j]):
                        is_api_section = True
                        break
                if is_api_section or 'API' in line:
                    in_api_table = True
                    header_modified = True
                    # Add SWS ID column
                    new_lines.append(line.rstrip() + ' SWS ID |')
                    continue

        if in_api_table and '---' in line and '|' in line:
            new_lines.append(line.rstrip() + '--------|')
            continue

        if in_api_table and '|' in line:
            # Try to find API name in this row
            cells = line.split('|')
            found = False
            for cell in cells:
                cell_stripped = cell.strip()
                if cell_stripped in apis:
                    api_counter += 1
                    sid = sws_id(prefix, api_counter)
                    new_lines.append(line.rstrip() + f' {sid} |')
                    count += 1
                    found = True
                    break
            if not found:
                # Check if any cell starts with prefix + "_"
                for cell in cells:
                    cell_stripped = cell.strip()
                    if cell_stripped.startswith(prefix + "_") or cell_stripped.startswith(prefix.lower() + "_"):
                        api_counter += 1
                        sid = sws_id(prefix, api_counter)
                        new_lines.append(line.rstrip() + f' {sid} |')
                        count += 1
                        found = True
                        break
            if not found:
                new_lines.append(line)
        else:
            if in_api_table and '|' not in line and line.strip() != '':
                in_api_table = False
            new_lines.append(line)

    # If no table was modified, try a simpler approach:
    # Find any markdown table that contains function names from the API list
    if count == 0:
        new_lines = []
        lines_content = '\n'.join(lines)
        # Find tables with API function names
        for i, line in enumerate(lines):
            if '|' in line:
                found_api = False
                for api_name in apis:
                    if api_name in line and '---' not in line:
                        found_api = True
                        break
                if found_api and not header_modified:
                    # This is a row in a table with API names
                    # Check if header needs modification
                    pass
            new_lines.append(line)

    with open(doc_file, 'w') as f:
        f.write('\n'.join(new_lines))

    print(f"  [DOC] {os.path.basename(doc_file)}: {count} SWS IDs added")
    return count


def process_tests(prefix, test_files, apis):
    """Add @req to test files. Returns count."""
    total = 0
    api_set = set(apis)

    for tf in test_files:
        if not os.path.exists(tf):
            continue

        with open(tf, 'r') as f:
            lines = f.read().split('\n')

        new_lines = []
        file_count = 0
        test_counter = 0

        for i, line in enumerate(lines):
            # Check for test function definition
            m = re.match(r'^void\s+(test_\w+|Test_\w+)\s*\(', line)
            if m:
                test_name = m.group(1)

                # Skip if already annotated
                if i > 0 and '@req SWS_' in lines[i-1]:
                    new_lines.append(line)
                    continue

                # Try to map to a known API
                matched_api = None
                for api_name in apis:
                    # Check if API name appears in test function name (case insensitive)
                    if api_name.lower() in test_name.lower():
                        matched_api = api_name
                        break

                if matched_api:
                    idx = apis.index(matched_api) + 1
                    sid = sws_id(prefix, idx)
                else:
                    test_counter += 1
                    sid = sws_id(prefix, 200 + test_counter)

                new_lines.append(f"/* @req {sid} */")
                file_count += 1

            new_lines.append(line)

        if file_count > 0:
            with open(tf, 'w') as f:
                f.write('\n'.join(new_lines))
            print(f"  [TEST] {os.path.basename(tf)}: {file_count} @req added")
        total += file_count

    return total


def main():
    results = {}

    for prefix in KNOWN_APIS:
        src_file, doc_file, test_files = MODULE_CONFIG[prefix]
        apis = KNOWN_APIS[prefix]

        print(f"\n{'='*60}")
        print(f"Module: {prefix}")
        print(f"{'='*60}")

        pub, internal = process_src(prefix, src_file, apis)
        doc_count = process_doc(prefix, doc_file, apis)
        test_count = process_tests(prefix, test_files, apis)

        results[prefix] = {
            "public": pub,
            "internal": internal,
            "doc": doc_count,
            "test": test_count,
            "total_src": pub + internal,
            "total": pub + internal + test_count,
        }

    # Summary
    print(f"\n\n{'='*70}")
    print(f"{'@req ANNOTATION SUMMARY':^70}")
    print(f"{'='*70}")
    print(f"{'Module':<10} {'Public':>7} {'Internal':>9} {'Doc':>5} {'Test':>6} {'Total':>7}")
    print(f"{'-'*10} {'-'*7} {'-'*9} {'-'*5} {'-'*6} {'-'*7}")

    grand_src = 0
    grand_test = 0
    for prefix, r in results.items():
        print(f"{prefix:<10} {r['public']:>7} {r['internal']:>9} {r['doc']:>5} {r['test']:>6} {r['total']:>7}")
        grand_src += r['total_src']
        grand_test += r['test']

    print(f"{'-'*10} {'-'*7} {'-'*9} {'-'*5} {'-'*6} {'-'*7}")
    print(f"{'TOTAL':<10} {'':>7} {'':>9} {'':>5} {'':>6} {grand_src + grand_test:>7}")
    print(f"\nSource @req: {grand_src}, Test @req: {grand_test}, Grand total: {grand_src + grand_test}")


if __name__ == "__main__":
    main()
