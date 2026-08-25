#!/usr/bin/env python3
"""Fix design docs that already have SWS ID column but empty values."""
import re, os

BASE = "/Users/ingeek/workspace/AUTOSAR/docs/design/modules/mcal"

KNOWN_APIS = {
    "Port": ["Port_Init", "Port_DeInit", "Port_SetPinDirection", "Port_RefreshPortDirection",
             "Port_GetVersionInfo", "Port_SetPinMode"],
    "Pwm": ["Pwm_Init", "Pwm_DeInit", "Pwm_SetDutyCycle", "Pwm_SetPeriodAndDuty", "Pwm_SetOutputToIdle",
            "Pwm_GetOutputState", "Pwm_DisableNotification", "Pwm_EnableNotification",
            "Pwm_GetVersionInfo", "Pwm_SetPowerState", "Pwm_GetTargetPowerState",
            "Pwm_GetCurrentPowerState", "Pwm_PreparePowerState"],
    "Spi": ["Spi_Init", "Spi_DeInit", "Spi_SyncTransmit", "Spi_AsyncTransmit", "Spi_GetStatus",
            "Spi_GetJobResult", "Spi_IsrHandler", "Spi_MainFunction", "Spi_GetVersionInfo"],
    "Uart": ["Uart_Init", "Uart_DeInit", "Uart_Send", "Uart_SendDMA", "Uart_SendInterrupt",
             "Uart_Receive", "Uart_ReceiveDMA", "Uart_ReceiveInterrupt", "Uart_GetStatus",
             "Uart_GetTxResult", "Uart_GetRxResult", "Uart_SetBaudRate", "Uart_EnableInterrupt",
             "Uart_DisableInterrupt", "Uart_ClearFIFO", "Uart_IsrHandler", "Uart_MainFunction",
             "Uart_Abort", "Uart_GetVersionInfo"],
    "Wdg": ["Wdg_Init", "Wdg_SetMode", "Wdg_Trigger", "Wdg_GetVersionInfo",
            "Wdg_SetTriggerCondition", "Wdg_GetStatus", "Wdg_GetTriggerCounter", "Wdg_GetLastTriggerTime"],
    "RamTst": ["RamTst_Init", "RamTst_DeInit", "RamTst_Run", "RamTst_Stop", "RamTst_GetTestResult",
               "RamTst_GetErrorRecord", "RamTst_GetTestStatus", "RamTst_MainFunction",
               "RamTst_GetVersionInfo", "RamTst_SetMode", "RamTst_GetMode"],
}

def sws_id(prefix, num):
    return f"SWS_{prefix}_{num:05d}"

def fix_doc(prefix, apis):
    doc_file = f"{BASE}/{prefix.lower()}-design.md"
    if not os.path.exists(doc_file):
        # Try alternate name
        for name in [f"{prefix.lower()}-design.md", f"{prefix}-design.md"]:
            if os.path.exists(f"{BASE}/{name}"):
                doc_file = f"{BASE}/{name}"
                break

    if not os.path.exists(doc_file):
        print(f"  Not found for {prefix}")
        return

    with open(doc_file, 'r') as f:
        lines = f.read().split('\n')

    new_lines = []
    in_api_table = False
    api_counter = 0
    count = 0

    for i, line in enumerate(lines):
        # Detect API table header
        if 'SWS' in line and '需求' in line and '|' in line and not in_api_table:
            in_api_table = True
            new_lines.append(line)
            continue

        if in_api_table and '|' in line and '---' in line:
            new_lines.append(line)
            continue

        if in_api_table and '|' in line:
            # Find which API this row refers to
            matched = None
            for api_name in apis:
                # Check with backticks or without
                if api_name in line or f'`{api_name}`' in line:
                    matched = api_name
                    break

            if matched:
                api_counter_idx = apis.index(matched) + 1
                sid = sws_id(prefix, api_counter_idx)
                # Replace the last empty cell before closing |
                # The line ends with "| |" or "| SWS_... |"
                # Replace trailing "| |" with "| SWS_xxx |"
                if line.rstrip().endswith('| |'):
                    line = line.rstrip()[:-2] + f' {sid} |'
                    count += 1
                elif re.search(r'\|\s*$', line.rstrip()):
                    # Check if the last cell is empty
                    cells = line.split('|')
                    if len(cells) >= 2 and cells[-2].strip() == '':
                        cells[-2] = f' {sid} '
                        line = '|'.join(cells)
                        count += 1
            new_lines.append(line)
        else:
            if in_api_table and line.strip() == '':
                in_api_table = False
            elif in_api_table and '|' not in line:
                in_api_table = False
            new_lines.append(line)

    with open(doc_file, 'w') as f:
        f.write('\n'.join(new_lines))

    print(f"  {prefix}: {count} SWS IDs filled in {doc_file}")

for prefix, apis in KNOWN_APIS.items():
    fix_doc(prefix, apis)
