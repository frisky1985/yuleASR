#!/usr/bin/env python3
"""
Batch 9d v2 — Split 5 large C files, each sub-file ≤ 1000 lines.
Uses precise line-range extraction.
"""
import os, re

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def read_file(p):
    with open(p) as f:
        return f.readlines()

def write_file(p, lines):
    with open(p, 'w') as f:
        f.writelines(lines)

def find_stop_sec(lines, marker):
    for i, l in enumerate(lines):
        if marker in l:
            return i
    return len(lines) - 1

# ========== 1. Csm.c: keys 1238 → split key into 2 ==========
def fix_csm():
    src = os.path.join(PROJECT, "src/bsw/services/csm/src")
    # The csm_keys.c is 1238 lines. Split it: 
    # csm_keys.c: KeyElementSet through KeyGenerate (basic key ops)
    # csm_key_exch.c: KeyDerive, KeyExchangeCalcPubVal, KeyExchangeCalcSecret
    keys_path = os.path.join(src, "csm_keys.c")
    klines = read_file(keys_path)
    # Find function starts in this file
    func_starts = []
    for i, l in enumerate(klines):
        if re.match(r'^(Std_ReturnType|void)\s+Csm_\w+\(', l):
            func_starts.append(i)
        elif re.match(r'^Csm_\w+\(', l) and i > 0:
            prev = klines[i-1].strip()
            if prev in ('void', 'Std_ReturnType'):
                if i-1 not in func_starts:
                    func_starts.append(i-1)
    func_starts.sort()
    
    # Functions in csm_keys.c: KeyElementSet, KeySetValid, KeyElementGet, KeyElementCopy,
    #   KeyCopy, KeyElementIdsGet, KeyGenerate, KeyDerive, KeyExchangeCalcPubVal, KeyExchangeCalcSecret
    # Split: basic (1-7) go to csm_keys.c, key_exch (8-10) go to csm_key_exch.c
    
    if len(func_starts) >= 7:
        split_idx = func_starts[7]  # KeyDerive starts here
    else:
        print("WARN: csm_keys.c has fewer functions than expected")
        return
    
    # Create key_exch file
    exch_lines = [
        "/*==================================================================================================\n",
        " * 密钥交换/派生 API 实现\n",
        " * 自动拆分自 Csm.c\n",
        " *================================================================================================*/\n",
        "#define CSM_START_SEC_CODE\n",
        "#include \"Csm_MemMap.h\"\n",
        "\n",
    ]
    # Find stop marker
    stop = find_stop_sec(klines, "CSM_STOP_SEC_CODE")
    if stop < 0:
        stop = len(klines)
    else:
        stop = stop + 2  # include #include line
    
    exch_lines.extend(klines[split_idx:stop])
    
    # Truncate keys file at split point
    # Keep the header + start marker block + first 7 functions
    new_keys = klines[:split_idx]
    # Add stop marker
    new_keys.append("#define CSM_STOP_SEC_CODE\n")
    new_keys.append("#include \"Csm_MemMap.h\"\n")
    new_keys.append("\n")
    
    # Update main Csm.c to include the new file
    main_path = os.path.join(src, "Csm.c")
    main_lines = read_file(main_path)
    # Replace #include \"csm_keys.c\" with both includes
    for i, l in enumerate(main_lines):
        if '#include "csm_keys.c"' in l:
            main_lines[i] = '#include "csm_keys.c"\n'
            main_lines.insert(i, '#include "csm_key_exch.c"\n')
            break
    
    write_file(os.path.join(src, "csm_key_exch.c"), exch_lines)
    write_file(keys_path, new_keys)
    write_file(main_path, main_lines)
    print(f"Csm fix: keys {len(klines)} → {len(new_keys)} + exch {len(exch_lines)}")

# ========== 2. NvM.c: 1932 → split more aggressively ==========
def fix_nvm():
    src = os.path.join(PROJECT, "src/bsw/services/nvm/src")
    main_path = os.path.join(src, "NvM.c")
    # Restore from backup if available, otherwise re-read
    backup = main_path + ".bak"
    # Check if .9d.bak exists
    bak9d = main_path + ".9d.bak"
    if os.path.exists(bak9d):
        lines = read_file(bak9d)
    else:
        lines = read_file(main_path)
    
    # The original NvM.c structure:
    # L1-22:  includes
    # L24-140: defines, types, globals
    # L142-169: prototypes
    # L174-915: static functions (queue, CRC, helpers, job processing)
    # L918-end: public APIs
    
    # We need to split into:
    # nvm_main.c: header + defines + types + globals + prototypes + queue/CRC helpers + Init + GetVersionInfo + GetErrorStatus + SetRamBlockStatus
    # nvm_jobs.c: static job processing functions + NvM_MainFunction
    # nvm_read.c: ReadBlock, ReadAll, ReadPRAMBlock, RestoreBlockDefaults, EraseNvBlock, InvalidateNvBlock
    # nvm_write.c: WriteBlock, WriteAll, WritePRAMBlock, WriteBlockOnce, SetDataIndex, SetBlockLockStatus, SetBlockProtection, SetWriteOnceStatus, KillWriteAll, KillReadAll, CancelJobs
    
    # Find all function start positions
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(STATIC )?(Std_ReturnType|void|boolean|uint8|uint16|uint32)\s+NvM_\w+\(', l)
        if m:
            func_starts.append(i)
    for i, l in enumerate(lines):
        if re.match(r'^NvM_\w+\(', l) and i > 0:
            prev = lines[i-1].strip()
            if prev in ('void', 'Std_ReturnType', 'STATIC void', 'STATIC Std_ReturnType',
                       'STATIC boolean', 'STATIC uint8', 'STATIC uint16', 'STATIC uint32',
                       'boolean', 'uint8', 'uint16', 'uint32'):
                if i-1 not in func_starts:
                    func_starts.append(i-1)
    func_starts.sort()
    
    code_end_idx = find_stop_sec(lines, "NVM_STOP_SEC_CODE")
    if code_end_idx >= len(lines) - 1:
        # find the actual end
        for i, l in enumerate(lines):
            if 'NVM_STOP_SEC_CODE' in l:
                code_end_idx = i + 2
                break
    
    all_starts = list(func_starts)
    # Add code_end to get last function's end
    last_end = code_end_idx if code_end_idx > 0 else len(lines)
    # Replace last entry's end with the proper one
    # Actually find a function that ends very late
    api_funcs = [idx for idx in func_starts if idx > 900]
    if api_funcs:
        api_funcs.sort()
    
    def find_func_end(start):
        for s in func_starts:
            if s > start:
                return s
        return last_end
    
    # Classify functions
    queue_helpers = {'QueuePush', 'QueuePop', 'QueueIsEmpty', 'QueueIsFull',
                     'QueueCancelJobs'}
    crc_helpers = {'CalculateCrc8', 'CalculateCrc16', 'CalculateCrc32', 'CalculateCrc', 'GetCrcSize'}
    block_helpers = {'GetBlockDescriptor', 'ValidateBlockId', 'CopyRomDataToRam', 'InvokeJobEndCallback'}
    redundant_helpers = {'ReadRedundantBlock', 'WriteRedundantBlock'}
    job_process = {'ProcessReadJob', 'ProcessWriteJob', 'ProcessRestoreJob', 
                   'ProcessEraseJob', 'ProcessInvalidateJob', 'UpdateBatchOperationStatus'}
    read_apis = {'ReadBlock', 'ReadAll', 'ReadPRAMBlock', 'RestoreBlockDefaults'}
    write_apis = {'WriteBlock', 'WriteAll', 'WritePRAMBlock', 'WriteBlockOnce',
                  'SetDataIndex', 'SetBlockLockStatus', 'SetBlockProtection', 
                  'SetWriteOnceStatus', 'KillWriteAll', 'KillReadAll'}
    nvm_cancel = {'CancelJobs'}
    erase_inval = {'EraseNvBlock', 'InvalidateNvBlock'}
    
    # Categorize
    job_funcs = []
    read_funcs = []
    write_funcs = []
    
    for idx in func_starts:
        m = re.search(r'NvM_(\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname in job_process or fname in queue_helpers or fname in redundant_helpers:
                job_funcs.append(idx)
            elif fname in read_apis or fname in erase_inval or fname == 'CancelJobs':
                read_funcs.append(idx)
            elif fname in write_apis:
                write_funcs.append(idx)
    
    # STATIC helpers that stay in main
    static_stay = [idx for idx in func_starts if idx in func_starts and 
                   re.search(r'NvM_(\w+)\s*\(', lines[idx]) and 
                   re.search(r'NvM_(\w+)\s*\(', lines[idx]).group(1) in 
                   (x for x in ['CalculateCrc8','CalculateCrc16','CalculateCrc32','CalculateCrc',
                                'GetCrcSize','GetBlockDescriptor','ValidateBlockId',
                                'CopyRomDataToRam','InvokeJobEndCallback'])]
    
    # NvM_MainFunction stays in main
    main_function_idx = None
    for idx in func_starts:
        if 'NvM_MainFunction' in lines[idx]:
            main_function_idx = idx
            break
    
    # Build job file (contains static job processing + MainFunction)
    job_lines = [
        "/*==================================================================================================\n",
        " * NvM 作业处理实现\n",
        " * 自动拆分自 NvM.c\n",
        " *================================================================================================*/\n",
        "#define NVM_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    
    # Extract job processing functions
    for idx in sorted(job_funcs):
        end = find_func_end(idx)
        job_lines.extend(lines[idx:end])
    
    # Add MainFunction at the end
    if main_function_idx:
        mf_end = find_func_end(main_function_idx)
        job_lines.extend(lines[main_function_idx:mf_end])
    
    job_lines.append("#define NVM_STOP_SEC_CODE\n")
    job_lines.append("#include \"MemMap.h\"\n")
    
    # Build read file
    read_lines = [
        "/*==================================================================================================\n",
        " * NvM 读操作实现\n",
        " * 自动拆分自 NvM.c\n",
        " *================================================================================================*/\n",
        "#define NVM_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(read_funcs):
        end = find_func_end(idx)
        read_lines.extend(lines[idx:end])
    read_lines.append("#define NVM_STOP_SEC_CODE\n")
    read_lines.append("#include \"MemMap.h\"\n")
    
    # Build write file
    write_file_lines = [
        "/*==================================================================================================\n",
        " * NvM 写操作实现\n",
        " * 自动拆分自 NvM.c\n",
        " *================================================================================================*/\n",
        "#define NVM_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(write_funcs):
        end = find_func_end(idx)
        write_file_lines.extend(lines[idx:end])
    write_file_lines.append("#define NVM_STOP_SEC_CODE\n")
    write_file_lines.append("#include \"MemMap.h\"\n")
    
    # Build main: keep everything except those extracted
    removed_funcs = set(job_funcs) | set(read_funcs) | set(write_funcs)
    if main_function_idx:
        removed_funcs.add(main_function_idx)
    
    remaining = list(lines)
    rm_ranges = [(idx, find_func_end(idx)) for idx in sorted(removed_funcs, reverse=True)]
    for start, end in rm_ranges:
        del remaining[start:end]
    
    # Insert includes before stop marker
    insert_pos = find_stop_sec(remaining, "NVM_STOP_SEC_CODE")
    if insert_pos >= 0:
        inc = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"nvm_jobs.c\"\n",
            "#include \"nvm_read.c\"\n",
            "#include \"nvm_write.c\"\n",
        ]
        for il in reversed(inc):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(src, "nvm_jobs.c"), job_lines)
    write_file(os.path.join(src, "nvm_read.c"), read_lines)
    write_file(os.path.join(src, "nvm_write.c"), write_file_lines)
    write_file(main_path, remaining)
    
    # Remove old nvm_read.c / nvm_write.c if they exist (our new ones will replace them)
    print(f"NvM fix: original ~2367 → main {len(remaining)} + jobs {len(job_lines)} + read {len(read_lines)} + write {len(write_file_lines)}")

# ========== 3. Xcp.c: 1031 → pull some transport back ==========
def fix_xcp():
    src = os.path.join(PROJECT, "src/bsw/services/xcp/src")
    main_path = os.path.join(src, "Xcp.c")
    
    # Restore original
    bak9d = main_path + ".9d.bak"
    if os.path.exists(bak9d):
        lines = read_file(bak9d)
    else:
        lines = read_file(main_path)
    
    # Find code section
    code_start = None
    code_stop = None
    for i, l in enumerate(lines):
        if 'XCP_START_SEC_CODE' in l:
            code_start = i
        if 'XCP_STOP_SEC_CODE' in l:
            code_stop = i
            break
    
    # Find all func starts
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(static )?(void|Std_ReturnType|boolean|uint8|uint16|uint32)\s+[Xx]cp_\w+\(', l)
        if m:
            func_starts.append(i)
    for i, l in enumerate(lines):
        if re.match(r'^[Xx]cp_\w+\(', l) and i > 0:
            prev = lines[i-1].strip()
            if prev in ('void', 'Std_ReturnType', 'boolean', 'uint8', 'uint16', 'uint32',
                       'static void', 'static Std_ReturnType'):
                if i-1 not in func_starts:
                    func_starts.append(i-1)
    func_starts.sort()
    
    code_end = code_stop + 2 if code_stop else len(lines)
    
    def find_func_end(start):
        for s in func_starts:
            if s > start:
                return s
        return code_end
    
    # Classify: DAQ processing + memory access + resource protection go in transport
    transport_funcs_names = {'Xcp_DaqProcessor', 'Xcp_DaqSample', 'Xcp_DaqTransmit',
                            'Xcp_StimProcessor', 'Xcp_ReadMemory', 'Xcp_WriteMemory',
                            'Xcp_SetResourceProtection', 'Xcp_IsResourceProtected',
                            'Xcp_UnlockResource'}
    # Static functions
    static_names = {'Xcp_ProcessStandardCommand', 'Xcp_ProcessDaqCommand', 'Xcp_ProcessPgmCommand',
                   'Xcp_CalculateChecksum', 'Xcp_ValidateMemoryAccess', 'Xcp_ClearDaqList',
                   'Xcp_ResetDaqConfiguration', 'Xcp_GetTimestamp'}
    
    transport_funcs = []
    cmd_funcs = []
    static_cmd_funcs = []
    
    for idx in func_starts:
        m = re.search(r'(Xcp_\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname in transport_funcs_names:
                transport_funcs.append(idx)
            elif fname.startswith('Xcp_Cmd'):
                cmd_funcs.append(idx)
            elif fname in static_names:
                static_cmd_funcs.append(idx)
    
    # Build transport file (init, deinit, version, callback, DAQ, memory, resource)
    transport_lines = [
        "/*==================================================================================================\n",
        " * XCP 传输层/初始化实现\n",
        " * 自动拆分自 Xcp.c\n",
        " *================================================================================================*/\n",
        "#define XCP_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(transport_funcs):
        end = find_func_end(idx)
        transport_lines.extend(lines[idx:end])
    transport_lines.append("#define XCP_STOP_SEC_CODE\n")
    transport_lines.append("#include \"MemMap.h\"\n")
    
    # Build cmd file (command handlers + static command processors)
    cmd_lines = [
        "/*==================================================================================================\n",
        " * XCP 命令处理实现\n",
        " * 自动拆分自 Xcp.c\n",
        " *================================================================================================*/\n",
        "#define XCP_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(cmd_funcs + static_cmd_funcs):
        end = find_func_end(idx)
        cmd_lines.extend(lines[idx:end])
    cmd_lines.append("#define XCP_STOP_SEC_CODE\n")
    cmd_lines.append("#include \"MemMap.h\"\n")
    
    # Remove cmd + static cmd from main
    removed = set(cmd_funcs) | set(static_cmd_funcs) | set(transport_funcs)
    remaining = list(lines)
    rm_ranges = [(idx, find_func_end(idx)) for idx in sorted(removed, reverse=True)]
    for start, end in rm_ranges:
        del remaining[start:end]
    
    # Insert includes
    insert_pos = find_stop_sec(remaining, "XCP_STOP_SEC_CODE")
    if insert_pos >= 0:
        inc = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"xcp_transport.c\"\n",
            "#include \"xcp_cmd.c\"\n",
        ]
        for il in reversed(inc):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(src, "xcp_transport.c"), transport_lines)
    write_file(os.path.join(src, "xcp_cmd.c"), cmd_lines)
    write_file(main_path, remaining)
    print(f"Xcp fix: original ~2035 → main {len(remaining)} + transport {len(transport_lines)} + cmd {len(cmd_lines)}")

if __name__ == '__main__':
    print("Fixing 3 oversized files...")
    fix_csm()
    fix_nvm()
    fix_xcp()
    print("\nVerifying all file sizes:")
    os.system("cd ~/.openclaw/workspace/yuleASR && wc -l "
              "src/bsw/services/csm/src/Csm.c "
              "src/bsw/services/csm/src/csm_keys.c "
              "src/bsw/services/csm/src/csm_key_exch.c "
              "src/bsw/services/csm/src/csm_jobs.c "
              "src/bsw/services/nvm/src/NvM.c "
              "src/bsw/services/nvm/src/nvm_jobs.c "
              "src/bsw/services/nvm/src/nvm_read.c "
              "src/bsw/services/nvm/src/nvm_write.c "
              "src/bsw/services/xcp/src/Xcp.c "
              "src/bsw/services/xcp/src/xcp_transport.c "
              "src/bsw/services/xcp/src/xcp_cmd.c "
              "src/bsw/mcal/crypto/src/Crypto_S32K312_Hsm.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_aes.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_ecc.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_sha_key.c "
              "src/bsw/services/ecum/src/EcuM.c "
              "src/bsw/services/ecum/src/ecum_startup.c "
              "src/bsw/services/ecum/src/ecum_run_sleep.c "
              "src/bsw/services/ecum/src/ecum_shutdown.c")
