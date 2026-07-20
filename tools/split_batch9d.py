#!/usr/bin/env python3
"""
Batch 9d — Split 5 large C files (>=2000 lines) into sub-files of ≤1000 lines.
Each main file retains headers/defines/globals and #includes child .c files.
"""

import os
import re

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def read_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    return lines

def write_file(path, lines):
    with open(path, 'w', encoding='utf-8') as f:
        f.writelines(lines)

def extract_section(lines, start_comment, end_comment):
    """Extract lines between two marker strings (inclusive)."""
    start = None
    end = None
    for i, l in enumerate(lines):
        if start_comment in l and start is None:
            start = i
        if end_comment in l and start is not None:
            end = i
            break
    if start is not None and end is not None:
        return lines[start:end+1], start, end
    return None, None, None

def find_sec_markers(lines):
    """Find all SEC_CODE/SEC_CONFIG start/stop markers."""
    markers = []
    for i, l in enumerate(lines):
        if '#define' in l and ('_START_SEC_CODE' in l or '_START_SEC_CONFIG' in l or '_START_SEC_VAR' in l or '_START_SEC_CONST' in l):
            markers.append((i, 'start'))
        if '#define' in l and ('_STOP_SEC_CODE' in l or '_STOP_SEC_CONFIG' in l or '_STOP_SEC_VAR' in l or '_STOP_SEC_CONST' in l):
            markers.append((i, 'stop'))
    return markers

# ============================================================
# 1. Csm.c → csm_main + csm_keys + csm_jobs
# ============================================================
def split_csm():
    src = os.path.join(PROJECT, "src/bsw/services/csm/src/Csm.c")
    lines = read_file(src)
    
    # Header: lines 0-183 (incl defines, types, globals, prototypes)
    # STATIC functions: lines 184-619 (Csm_ReportError through Csm_LoadKeyElement)
    # KEY functions: lines 737-1965 (KeyElementSet through KeyExchangeCalcSecret)
    # JOB functions: lines 1966-2660 (Hash through GetJobState)
    # GetVersionInfo: lines 2660-2678
    # STOP_SEC_CODE: line 2678
    
    # Identify exact boundaries
    # Look for the CSM_STOP_SEC_CODE / #include "Csm_MemMap.h" end
    stop_sec_idx = None
    for i, l in enumerate(lines):
        if 'CSM_STOP_SEC_CODE' in l and i > 2600:
            stop_sec_idx = i
            break
    
    # Header (includes, defines, types, globals, prototypes, STATIC functions + Csm_Init + Csm_DeInit)
    # Write the full original first, then modify
    
    backup = src + ".9d.bak"
    if not os.path.exists(backup):
        write_file(backup, lines)
    
    # Find key function sections
    # Init/DeInit: line ~621 Init, ~712 DeInit  
    # Key funcs: KeyElementSet(line 737) through KeyExchangeCalcSecret(line 1965)
    # Job/crypto funcs: Hash(line 1966) through GetJobState(line ~2660)
    
    init_end = 0
    key_start = 0
    key_end = 0
    job_end = 0
    
    for i, l in enumerate(lines):
        if 'Std_ReturnType Csm_Init' in l and init_end == 0:
            pass  # keep track
        if 'Std_ReturnType Csm_DeInit' in l and 'void' in lines[i]:
            # Find the end of DeInit (next top-level function)
            pass
    
    # Better approach: use regex on function name patterns
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(Std_ReturnType|void|STATIC (Std_ReturnType|void))\s+Csm_\w+\(', l)
        if m:
            func_starts.append(i)
    
    # Also handle multi-line function declarations (function name on continuation)
    for i, l in enumerate(lines):
        if re.match(r'^Csm_\w+\(', l) and i > 0:
            if re.match(r'^(Std_ReturnType|void|STATIC (Std_ReturnType|void))\s*$', lines[i-1]):
                func_starts.append(i-1)
    
    func_starts.sort()
    
    # Group functions
    statics = []    # Csm_ReportError .. Csm_LoadKeyElement
    inits = []      # Csm_Init, Csm_DeInit
    keys = []       # KeyElementSet .. KeyExchangeCalcSecret
    jobs = []       # Hash .. GetJobState
    version = []    # GetVersionInfo
    
    for idx in func_starts:
        name_match = re.search(r'Csm_(\w+)', lines[idx])
        if name_match:
            fname = name_match.group(1)
            if fname in ('ReportError', 'NotifyEvent', 'ValidateConfig', 'FindKeyIndex', 
                        'FindJobIndex', 'FindKeyElementIndex', 'QueueJob', 'DequeueJob',
                        'ProcessQueue', 'ExecuteJob', 'ResetJob', 'ValidateKeyUsage',
                        'UpdateKeyStatus', 'PersistKeyElement', 'LoadKeyElement'):
                statics.append(idx)
            elif fname in ('Init', 'DeInit'):
                inits.append(idx)
            elif fname in ('KeyElementSet', 'KeySetValid', 'KeyElementGet', 'KeyElementCopy',
                         'KeyCopy', 'KeyElementIdsGet', 'KeyGenerate', 'KeyDerive',
                         'KeyExchangeCalcPubVal', 'KeyExchangeCalcSecret'):
                keys.append(idx)
            elif fname in ('Hash', 'MacGenerate', 'MacVerify', 'Encrypt', 'Decrypt',
                         'SignatureGenerate', 'SignatureVerify', 'RandomGenerate',
                         'JobKeySetUp', 'JobKeySetUpAsync', 'CancelJob', 'MainFunction',
                         'RegisterCallback', 'GetKeyStatus', 'GetJobState'):
                jobs.append(idx)
            elif fname == 'GetVersionInfo':
                version.append(idx)
    
    # Find end of each function (next function or STOP_SEC_CODE)
    def find_func_end(start_idx, all_starts, end_marker):
        for s in all_starts:
            if s > start_idx:
                return s
        return end_marker
    
    # The stop marker for code section
    code_stop = None
    for i, l in enumerate(lines):
        if 'CSM_STOP_SEC_CODE' in l:
            code_stop = i+1  # include the #include line
            break
    
    all_starts = statics + inits + keys + jobs + version
    all_starts.sort()
    
    # Build key file
    key_lines = []
    key_lines.append("/*==================================================================================================\n")
    key_lines.append(" * 密钥管理 API 实现\n")
    key_lines.append(" * 自动拆分自 Csm.c\n")
    key_lines.append(" *================================================================================================*/\n")
    key_lines.append("#define CSM_START_SEC_CODE\n")
    key_lines.append("#include \"Csm_MemMap.h\"\n")
    key_lines.append("\n")
    
    key_funcs_list = keys
    for i, idx in enumerate(key_funcs_list):
        end = find_func_end(idx, all_starts, code_stop)
        if i < len(key_funcs_list) - 1:
            end = find_func_end(idx, all_starts, key_funcs_list[i+1])
        else:
            # Last key function ends at first job function
            if jobs:
                end = jobs[0]
            else:
                end = code_stop
        key_lines.extend(lines[idx:end])
    
    key_lines.append("#define CSM_STOP_SEC_CODE\n")
    key_lines.append("#include \"Csm_MemMap.h\"\n")
    
    # Build job file
    job_lines = []
    job_lines.append("/*==================================================================================================\n")
    job_lines.append(" * 作业/密码服务 API 实现\n")
    job_lines.append(" * 自动拆分自 Csm.c\n")
    job_lines.append(" *================================================================================================*/\n")
    job_lines.append("#define CSM_START_SEC_CODE\n")
    job_lines.append("#include \"Csm_MemMap.h\"\n")
    job_lines.append("\n")
    
    for i, idx in enumerate(jobs):
        if i < len(jobs) - 1:
            end = jobs[i+1]
        else:
            end = code_stop
        job_lines.extend(lines[idx:end])
    
    job_lines.append("#define CSM_STOP_SEC_CODE\n")
    job_lines.append("#include \"Csm_MemMap.h\"\n")
    
    # The main file: keep everything except key/job functions
    # Remove key functions
    remaining = list(lines)  # copy
    # Remove from end to start (to preserve indices)
    ranges_to_remove = []
    for idx in reversed(keys + jobs):
        end = find_func_end(idx, all_starts, code_stop)
        ranges_to_remove.append((idx, end))
    
    for start, end in ranges_to_remove:
        del remaining[start:end]
    
    # Insert #include for sub-files before the STOP_SEC_CODE marker
    insert_pos = None
    for i, l in enumerate(remaining):
        if 'CSM_STOP_SEC_CODE' in l:
            insert_pos = i
            break
    
    if insert_pos:
        include_block = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"csm_keys.c\"\n",
            "#include \"csm_jobs.c\"\n",
        ]
        for il in reversed(include_block):
            remaining.insert(insert_pos, il)
    
    # Write all files
    write_file(os.path.join(PROJECT, "src/bsw/services/csm/src/csm_keys.c"), key_lines)
    write_file(os.path.join(PROJECT, "src/bsw/services/csm/src/csm_jobs.c"), job_lines)
    write_file(src, remaining)
    
    print(f"Csm.c: original {len(lines)} lines → main {len(remaining)} lines, keys {len(key_lines)} lines, jobs {len(job_lines)} lines")
    print(f"        keys funcs: {len(keys)}, job funcs: {len(jobs)}")

# ============================================================
# 2. NvM.c → nvm_main + nvm_read + nvm_write
# ============================================================
def split_nvm():
    src = os.path.join(PROJECT, "src/bsw/services/nvm/src/NvM.c")
    lines = read_file(src)
    
    backup = src + ".9d.bak"
    if not os.path.exists(backup):
        write_file(backup, lines)
    
    # Function starts
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(STATIC )?(Std_ReturnType|void|boolean|uint8|uint16|uint32|const)\s+NvM_\w+\(', l)
        if m:
            func_starts.append(i)
    
    # Also find function names on continuation line
    for i, l in enumerate(lines):
        if re.match(r'^NvM_\w+\(', l) and i > 0:
            prev = lines[i-1].strip()
            if prev in ('void', 'Std_ReturnType', 'STATIC void', 'STATIC Std_ReturnType',
                       'STATIC boolean', 'STATIC uint8', 'STATIC uint16', 'STATIC uint32',
                       'STATIC const', 'uint8', 'uint16', 'uint32', 'boolean'):
                func_starts.append(i-1)
    
    func_starts.sort()
    
    # Find code section boundaries
    code_start = None
    code_stop = None
    for i, l in enumerate(lines):
        if 'NVM_START_SEC_CODE' in l:
            code_start = i
        if 'NVM_STOP_SEC_CODE' in l:
            code_stop = i
            break
    
    # Classify functions
    statics_names = {'QueuePush', 'QueuePop', 'QueueIsEmpty', 'QueueIsFull', 
                    'GetBlockDescriptor', 'ValidateBlockId',
                    'CalculateCrc8', 'CalculateCrc16', 'CalculateCrc32', 'CalculateCrc',
                    'GetCrcSize', 'CopyRomDataToRam', 'InvokeJobEndCallback',
                    'ReadRedundantBlock', 'ProcessReadJob', 'WriteRedundantBlock',
                    'ProcessWriteJob', 'ProcessRestoreJob', 'ProcessEraseJob',
                    'ProcessInvalidateJob', 'QueueCancelJobs', 'UpdateBatchOperationStatus',
                    'GetErrorStatus', 'SetRamBlockStatus'}
    
    read_funcs_names = {'ReadBlock', 'ReadAll', 'ReadPRAMBlock'}
    write_funcs_names = {'WriteBlock', 'WriteAll', 'WritePRAMBlock', 'WriteBlockOnce',
                        'KillWriteAll', 'KillReadAll'}
    
    publics_names = {'Init', 'RestoreBlockDefaults', 'SetDataIndex', 'SetBlockLockStatus',
                    'SetBlockProtection', 'SetWriteOnceStatus', 'CancelJobs',
                    'GetVersionInfo', 'EraseNvBlock', 'InvalidateNvBlock', 'MainFunction'}
    
    # Find end of code section
    for i, l in enumerate(lines):
        if 'NVM_STOP_SEC_CODE' in l:
            code_end = i
            break
    
    # Build classification
    all_starts = list(func_starts)
    if code_stop:
        all_starts.append(code_stop)
    all_starts.sort()
    
    def find_func_end(start_idx):
        pos = all_starts.index(start_idx) if start_idx in all_starts else -1
        if pos >= 0 and pos < len(all_starts) - 1:
            return all_starts[pos + 1]
        return len(lines)
    
    statics_funcs = []  # queue, CRC, helper static functions
    read_funcs = []     # ReadBlock, ReadAll, ReadPRAMBlock
    write_funcs = []    # WriteBlock, WriteAll, WritePRAMBlock, WriteBlockOnce, KillWriteAll, KillReadAll
    
    # Public APIs + MainFunction stay in main
    public_funcs = [idx for idx in func_starts if not any(
        re.search(r'NvM_' + n + r'\s*\(', lines[idx]) for n in statics_names | read_funcs_names | write_funcs_names
    )]
    
    for idx in func_starts:
        m = re.search(r'NvM_(\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname in read_funcs_names:
                read_funcs.append(idx)
            elif fname in write_funcs_names:
                write_funcs.append(idx)
            elif fname in statics_names:
                statics_funcs.append(idx)
    
    combined_remove = read_funcs + write_funcs
    
    # Build read file
    read_lines = []
    read_lines.append("/*==================================================================================================\n")
    read_lines.append(" * NvM 读操作实现\n")
    read_lines.append(" * 自动拆分自 NvM.c\n")
    read_lines.append(" *================================================================================================*/\n")
    read_lines.append("#define NVM_START_SEC_CODE\n")
    read_lines.append("#include \"MemMap.h\"\n")
    read_lines.append("\n")
    
    for idx in read_funcs:
        end = find_func_end(idx)
        read_lines.extend(lines[idx:end])
    
    read_lines.append("#define NVM_STOP_SEC_CODE\n")
    read_lines.append("#include \"MemMap.h\"\n")
    
    # Build write file
    write_lines = []
    write_lines.append("/*==================================================================================================\n")
    write_lines.append(" * NvM 写操作实现\n")
    write_lines.append(" * 自动拆分自 NvM.c\n")
    write_lines.append(" *================================================================================================*/\n")
    write_lines.append("#define NVM_START_SEC_CODE\n")
    write_lines.append("#include \"MemMap.h\"\n")
    write_lines.append("\n")
    
    for idx in write_funcs:
        end = find_func_end(idx)
        write_lines.extend(lines[idx:end])
    
    write_lines.append("#define NVM_STOP_SEC_CODE\n")
    write_lines.append("#include \"MemMap.h\"\n")
    
    # Modify main: remove read/write functions
    remaining = list(lines)
    ranges_to_remove = sorted(combined_remove, reverse=True)
    # For each function to remove, find its end
    rm_ranges = []
    for idx in combined_remove:
        end = find_func_end(idx)
        rm_ranges.append((idx, end))
    
    for start, end in sorted(rm_ranges, reverse=True):
        del remaining[start:end]
    
    # Insert includes before STOP_SEC_CODE
    insert_pos = None
    for i, l in enumerate(remaining):
        if 'NVM_STOP_SEC_CODE' in l:
            insert_pos = i
            break
    
    if insert_pos:
        include_block = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"nvm_read.c\"\n",
            "#include \"nvm_write.c\"\n",
        ]
        for il in reversed(include_block):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(PROJECT, "src/bsw/services/nvm/src/nvm_read.c"), read_lines)
    write_file(os.path.join(PROJECT, "src/bsw/services/nvm/src/nvm_write.c"), write_lines)
    write_file(src, remaining)
    
    print(f"NvM.c: original {len(lines)} lines → main {len(remaining)} lines, read {len(read_lines)} lines, write {len(write_lines)} lines")

# ============================================================
# 3. Xcp.c → xcp_transport + xcp_cmd
# ============================================================
def split_xcp():
    src = os.path.join(PROJECT, "src/bsw/services/xcp/src/Xcp.c")
    lines = read_file(src)
    
    backup = src + ".9d.bak"
    if not os.path.exists(backup):
        write_file(backup, lines)
    
    # Find code section boundaries
    code_start = None
    code_stop = None
    for i, l in enumerate(lines):
        if 'XCP_START_SEC_CODE' in l:
            code_start = i
        if 'XCP_STOP_SEC_CODE' in l:
            code_stop = i
            break
    
    # All function starts in code section
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(static )?(void|Std_ReturnType|boolean|uint8|uint16|uint32)\s+[Xx]cp_\w+\(', l)
        if m:
            func_starts.append(i)
    
    # Multi-line declarations
    for i, l in enumerate(lines):
        if re.match(r'^[Xx]cp_\w+\(', l) and i > 0:
            prev = lines[i-1].strip()
            if prev in ('void', 'Std_ReturnType', 'boolean', 'uint8', 'uint16', 'uint32', 'static void', 'static Std_ReturnType'):
                if i-1 not in func_starts:
                    func_starts.append(i-1)
    
    func_starts.sort()
    
    # Split into transport/init vs command handler functions
    transport_names = {'Xcp_Init', 'Xcp_DeInit', 'Xcp_GetVersionInfo', 'Xcp_MainFunction',
                      'Xcp_RxIndication', 'Xcp_TxConfirmation', 'Xcp_TriggerTransmit',
                      'Xcp_SetTransmissionMode', 'Xcp_GetSessionStatus', 'Xcp_ProcessCommand',
                      'Xcp_SendResponse', 'Xcp_SendError', 'Xcp_SendEvent',
                      'Xcp_ReadMemory', 'Xcp_WriteMemory', 'Xcp_SetResourceProtection',
                      'Xcp_IsResourceProtected', 'Xcp_UnlockResource',
                      'Xcp_DaqProcessor', 'Xcp_DaqSample', 'Xcp_DaqTransmit', 'Xcp_StimProcessor'}
    
    # Cmd handler starts with Xcp_Cmd
    cmd_starts = []
    transport_starts = []
    
    for idx in func_starts:
        m = re.search(r'(Xcp_\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname.startswith('Xcp_Cmd'):
                cmd_starts.append(idx)
            elif fname in transport_names or fname.startswith('Xcp_Process'):
                transport_starts.append(idx)
            else:
                transport_starts.append(idx)
    
    # Also include the static local functions (Xcp_CalculateChecksum, Xcp_ValidateMemoryAccess, etc.)
    
    all_funcs = list(func_starts)
    if code_stop:
        all_funcs.append(code_stop)
    all_funcs.sort()
    
    def find_func_end(start_idx):
        all_f = list(func_starts)
        if code_stop:
            all_f.append(code_stop)
        all_f.sort()
        pos = all_f.index(start_idx) if start_idx in all_f else -1
        if pos >= 0 and pos < len(all_f) - 1:
            return all_f[pos + 1]
        return code_stop if code_stop else len(lines)
    
    # Build transport file
    transport_lines = []
    transport_lines.append("/*==================================================================================================\n")
    transport_lines.append(" * XCP 传输层/初始化实现\n")
    transport_lines.append(" * 自动拆分自 Xcp.c\n")
    transport_lines.append(" *================================================================================================*/\n")
    transport_lines.append("#define XCP_START_SEC_CODE\n")
    transport_lines.append("#include \"MemMap.h\"\n")
    transport_lines.append("\n")
    
    for idx in transport_starts:
        end = find_func_end(idx)
        transport_lines.extend(lines[idx:end])
    
    transport_lines.append("#define XCP_STOP_SEC_CODE\n")
    transport_lines.append("#include \"MemMap.h\"\n")
    
    # Build cmd file  
    cmd_lines = []
    cmd_lines.append("/*==================================================================================================\n")
    cmd_lines.append(" * XCP 命令处理实现\n")
    cmd_lines.append(" * 自动拆分自 Xcp.c\n")
    cmd_lines.append(" *================================================================================================*/\n")
    cmd_lines.append("#define XCP_START_SEC_CODE\n")
    cmd_lines.append("#include \"MemMap.h\"\n")
    cmd_lines.append("\n")
    
    for idx in cmd_starts:
        end = find_func_end(idx)
        cmd_lines.extend(lines[idx:end])
    
    cmd_lines.append("#define XCP_STOP_SEC_CODE\n")
    cmd_lines.append("#include \"MemMap.h\"\n")
    
    # Remove cmd functions from main, keep transport
    remaining = list(lines)
    remove_set = set(cmd_starts)
    # Find all code-section functions that are cmd handlers
    rm_ranges = []
    for idx in sorted(cmd_starts, reverse=True):
        end = find_func_end(idx)
        rm_ranges.append((idx, end))
    
    for start, end in rm_ranges:
        del remaining[start:end]
    
    # Insert includes
    insert_pos = None
    for i, l in enumerate(remaining):
        if 'XCP_STOP_SEC_CODE' in l:
            insert_pos = i
            break
    
    if insert_pos:
        include_block = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"xcp_transport.c\"\n",
            "#include \"xcp_cmd.c\"\n",
        ]
        for il in reversed(include_block):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(PROJECT, "src/bsw/services/xcp/src/xcp_transport.c"), transport_lines)
    write_file(os.path.join(PROJECT, "src/bsw/services/xcp/src/xcp_cmd.c"), cmd_lines)
    write_file(src, remaining)
    
    print(f"Xcp.c: original {len(lines)} lines → main {len(remaining)} lines, transport {len(transport_lines)} lines, cmd {len(cmd_lines)} lines")

# ============================================================
# 4. Crypto_S32K312_Hsm.c → crypto_hsm_main + crypto_hsm_aes + crypto_hsm_ecc + crypto_hsm_sha_key
# ============================================================
def split_crypto():
    src = os.path.join(PROJECT, "src/bsw/mcal/crypto/src/Crypto_S32K312_Hsm.c")
    lines = read_file(src)
    
    backup = src + ".9d.bak"
    if not os.path.exists(backup):
        write_file(backup, lines)
    
    # Find code section bounds
    code_start = None
    code_stop = None
    for i, l in enumerate(lines):
        if 'CRYPTO_START_SEC_CODE' in l:
            code_start = i
        if 'CRYPTO_STOP_SEC_CODE' in l:
            code_stop = i
            break
    
    # Find all functions
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(STATIC )?(Std_ReturnType|void|uint8|uint16|uint32|boolean)\s+S32K312_Hsm_\w+\(', l)
        if m:
            func_starts.append(i)
    
    # Multiline
    for i, l in enumerate(lines):
        if re.match(r'^S32K312_Hsm_\w+\(', l) and i > 0:
            prev = lines[i-1].strip()
            if prev in ('void', 'Std_ReturnType', 'STATIC void', 'STATIC Std_ReturnType',
                       'STATIC uint32', 'STATIC uint8', 'boolean', 'uint32', 'uint8'):
                if i-1 not in func_starts:
                    func_starts.append(i-1)
    
    func_starts.sort()
    
    all_starts = list(func_starts)
    if code_stop:
        all_starts.append(code_stop)
    all_starts.sort()
    
    def find_func_end(start_idx):
        pos = all_starts.index(start_idx)
        if pos < len(all_starts) - 1:
            return all_starts[pos + 1]
        return code_stop if code_stop else len(lines)
    
    # Classify
    aes_funcs = []
    ecc_funcs = []
    sha_funcs = []
    key_funcs = []
    init_funcs = []
    local_funcs = []
    util_funcs = []
    
    for idx in func_starts:
        m = re.search(r'S32K312_Hsm_(\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname.startswith('Aes'):
                aes_funcs.append(idx)
            elif fname.startswith('Ecc') or fname.startswith('Ecc'):
                ecc_funcs.append(idx)
            elif fname.startswith('Sha'):
                sha_funcs.append(idx)
            elif fname.startswith('Key') or fname.startswith('Key'):
                key_funcs.append(idx)
            elif fname in ('Init', 'DeInit', 'SelfTest', 'GetStatus'):
                init_funcs.append(idx)
            elif fname in ('WaitReady', 'ClearError', 'GetFirmwareVersion'):
                util_funcs.append(idx)
            else:
                local_funcs.append(idx)
    
    # Build AES file
    aes_lines = []
    aes_lines.append("/**********************************************************************************************************************\n")
    aes_lines.append(" * AES 操作实现\n")
    aes_lines.append(" * 自动拆分自 Crypto_S32K312_Hsm.c\n")
    aes_lines.append(" *********************************************************************************************************************/\n")
    aes_lines.append("#define CRYPTO_START_SEC_CODE\n")
    aes_lines.append("#include \"MemMap.h\"\n")
    aes_lines.append("\n")
    for idx in aes_funcs:
        end = find_func_end(idx)
        aes_lines.extend(lines[idx:end])
    aes_lines.append("#define CRYPTO_STOP_SEC_CODE\n")
    aes_lines.append("#include \"MemMap.h\"\n")
    
    # Build ECC file
    ecc_lines = []
    ecc_lines.append("/**********************************************************************************************************************\n")
    ecc_lines.append(" * ECC 操作实现\n")
    ecc_lines.append(" * 自动拆分自 Crypto_S32K312_Hsm.c\n")
    ecc_lines.append(" *********************************************************************************************************************/\n")
    ecc_lines.append("#define CRYPTO_START_SEC_CODE\n")
    ecc_lines.append("#include \"MemMap.h\"\n")
    ecc_lines.append("\n")
    for idx in ecc_funcs:
        end = find_func_end(idx)
        ecc_lines.extend(lines[idx:end])
    ecc_lines.append("#define CRYPTO_STOP_SEC_CODE\n")
    ecc_lines.append("#include \"MemMap.h\"\n")
    
    # Build SHA+Key file
    sha_key_lines = []
    sha_key_lines.append("/**********************************************************************************************************************\n")
    sha_key_lines.append(" * SHA-256 与密钥存储操作实现\n")
    sha_key_lines.append(" * 自动拆分自 Crypto_S32K312_Hsm.c\n")
    sha_key_lines.append(" *********************************************************************************************************************/\n")
    sha_key_lines.append("#define CRYPTO_START_SEC_CODE\n")
    sha_key_lines.append("#include \"MemMap.h\"\n")
    sha_key_lines.append("\n")
    for idx in sha_funcs + key_funcs:
        end = find_func_end(idx)
        sha_key_lines.extend(lines[idx:end])
    sha_key_lines.append("#define CRYPTO_STOP_SEC_CODE\n")
    sha_key_lines.append("#include \"MemMap.h\"\n")
    
    # Remove AES, ECC, SHA, Key from main (keep init, util, local)
    remove_all = aes_funcs + ecc_funcs + sha_funcs + key_funcs
    remaining = list(lines)
    rm_ranges = [(idx, find_func_end(idx)) for idx in remove_all]
    for start, end in sorted(rm_ranges, reverse=True):
        del remaining[start:end]
    
    insert_pos = None
    for i, l in enumerate(remaining):
        if 'CRYPTO_STOP_SEC_CODE' in l:
            insert_pos = i
            break
    
    if insert_pos:
        include_block = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"crypto_hsm_aes.c\"\n",
            "#include \"crypto_hsm_ecc.c\"\n",
            "#include \"crypto_hsm_sha_key.c\"\n",
        ]
        for il in reversed(include_block):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(PROJECT, "src/bsw/mcal/crypto/src/crypto_hsm_aes.c"), aes_lines)
    write_file(os.path.join(PROJECT, "src/bsw/mcal/crypto/src/crypto_hsm_ecc.c"), ecc_lines)
    write_file(os.path.join(PROJECT, "src/bsw/mcal/crypto/src/crypto_hsm_sha_key.c"), sha_key_lines)
    write_file(src, remaining)
    
    print(f"Crypto_S32K312_Hsm.c: original {len(lines)} lines → main {len(remaining)} lines, aes {len(aes_lines)} lines, ecc {len(ecc_lines)} lines, sha_key {len(sha_key_lines)} lines")

# ============================================================
# 5. EcuM.c → ecum_main + ecum_startup + ecum_wakeup_sleep + ecum_shutdown
# ============================================================
def split_ecum():
    src = os.path.join(PROJECT, "src/bsw/services/ecum/src/EcuM.c")
    lines = read_file(src)
    
    backup = src + ".9d.bak"
    if not os.path.exists(backup):
        write_file(backup, lines)
    
    # Find all functions
    func_starts = []
    for i, l in enumerate(lines):
        m = re.match(r'^(void|Std_ReturnType|static |STATIC |EcuM_SubStateType|EcuM_WakeupStatusType|EcuM_StateType|boolean|uint8|uint32)\s+EcuM_\w+\(', l)
        if m:
            func_starts.append(i)
    
    func_starts.sort()
    
    # Find end marker
    code_stop = None
    for i, l in enumerate(lines):
        if 'STOP_SEC_CODE' in l and 'ECUM' not in l:
            code_stop = i
            break
    
    all_starts = list(func_starts)
    if code_stop:
        all_starts.append(code_stop)
    all_starts.sort()
    
    def find_func_end(start_idx):
        pos = all_starts.index(start_idx)
        if pos < len(all_starts) - 1:
            return all_starts[pos + 1]
        return code_stop if code_stop else len(lines)
    
    # Classify by state machine phase
    startup_names = {'EcuM_Init', 'EcuM_StartupOne', 'EcuM_StartupTwo',
                    'EcuM_ProcessStartupOne', 'EcuM_ProcessStartupTwo'}
    run_sleep_names = {'EcuM_MainFunction', 'EcuM_ProcessRun', 'EcuM_ProcessPostRun',
                      'EcuM_GoSleep', 'EcuM_GoHalt', 'EcuM_GoPoll',
                      'EcuM_ProcessGoSleep', 'EcuM_ProcessSleep', 'EcuM_ProcessHalt',
                      'EcuM_ProcessPoll', 'EcuM_WakeupRestart', 'EcuM_ProcessWakeupOne',
                      'EcuM_ProcessWakeupTwo', 'EcuM_PerformSleep',
                      'EcuM_RequestRUN', 'EcuM_ReleaseRUN', 'EcuM_KillAllRUNRequests',
                      'EcuM_CheckRunRequests', 'EcuM_CheckSleepTransition',
                      'EcuM_SetWakeupEvent', 'EcuM_ClearWakeupEvent', 'EcuM_CheckWakeup',
                      'EcuM_EnableWakeupSources', 'EcuM_DisableWakeupSources',
                      'EcuM_GetStatusOfWakeupSource', 'EcuM_GetWakeupSources',
                      'EcuM_CheckValidation',
                      'EcuM_ValidateWakeupSources', 'EcuM_ExpireWakeupSources',
                      'EcuM_GetWakeupSourceIndex', 'EcuM_IsValidWakeupSource'}
    shutdown_names = {'EcuM_Shutdown', 'EcuM_ProcessGoOffOne', 'EcuM_ProcessGoOffTwo',
                     'EcuM_PerformShutdown', 'EcuM_PerformReset'}
    api_names = {'EcuM_GetState', 'EcuM_GetSubState',
                'EcuM_SelectShutdownTarget', 'EcuM_GetShutdownTarget',
                'EcuM_GetLastShutdownTarget', 'EcuM_SelectShutdownCause',
                'EcuM_GetShutdownCause',
                'EcuM_SelectBootTarget', 'EcuM_GetBootTarget',
                'EcuM_SelectApplicationMode', 'EcuM_GetApplicationMode',
                'EcuM_ComM_RequestComMode', 'EcuM_ComM_ReleaseComMode',
                'EcuM_StartBswMode', 'EcuM_StopBswMode', 'EcuM_GetVersionInfo',
                'EcuM_UpdateSubState', 'EcuM_DisableInterrupts', 'EcuM_EnableInterrupts'}
    callout_names = {'EcuM_DriverInitOne', 'EcuM_DriverInitTwo', 'EcuM_DriverInitThree',
                    'EcuM_DriverRestart',
                    'EcuM_AL_DriverInitOne', 'EcuM_AL_DriverInitTwo', 'EcuM_AL_DriverInitThree',
                    'EcuM_AL_DriverRestart',
                    'EcuM_AL_SwitchOff', 'EcuM_AL_Reset', 'EcuM_AL_EnterSleep',
                    'EcuM_AL_WakeupCheck', 'EcuM_AL_WakeupValidation', 'EcuM_AL_WakeupReaction'}
    
    # Categorize
    startup_funcs = []
    run_sleep_funcs = []
    shutdown_funcs = []
    
    for idx in func_starts:
        m = re.search(r'EcuM_(\w+)\s*\(', lines[idx])
        if m:
            fname_full = 'EcuM_' + m.group(1)
            if fname_full in startup_names:
                startup_funcs.append(idx)
            elif fname_full in run_sleep_names:
                run_sleep_funcs.append(idx)
            elif fname_full in shutdown_names:
                shutdown_funcs.append(idx)
            else:
                # API and callout stay in main
                pass
    
    # Build startup file
    startup_lines = []
    startup_lines.append("/*******************************************************************************\n")
    startup_lines.append(" * EcuM 启动阶段实现\n")
    startup_lines.append(" * 自动拆分自 EcuM.c\n")
    startup_lines.append(" ******************************************************************************/\n")
    # We need proper SEC markers - look at original to see what's used
    startup_lines.append("#define ECUM_START_SEC_CODE\n")
    startup_lines.append("#include \"MemMap.h\"\n")
    startup_lines.append("\n")
    for idx in startup_funcs:
        end = find_func_end(idx)
        startup_lines.extend(lines[idx:end])
    startup_lines.append("#define ECUM_STOP_SEC_CODE\n")
    startup_lines.append("#include \"MemMap.h\"\n")
    
    # Build run/sleep file
    run_sleep_lines = []
    run_sleep_lines.append("/*******************************************************************************\n")
    run_sleep_lines.append(" * EcuM 运行/睡眠/唤醒实现\n")
    run_sleep_lines.append(" * 自动拆分自 EcuM.c\n")
    run_sleep_lines.append(" ******************************************************************************/\n")
    run_sleep_lines.append("#define ECUM_START_SEC_CODE\n")
    run_sleep_lines.append("#include \"MemMap.h\"\n")
    run_sleep_lines.append("\n")
    for idx in run_sleep_funcs:
        end = find_func_end(idx)
        run_sleep_lines.extend(lines[idx:end])
    run_sleep_lines.append("#define ECUM_STOP_SEC_CODE\n")
    run_sleep_lines.append("#include \"MemMap.h\"\n")
    
    # Build shutdown file
    shutdown_lines = []
    shutdown_lines.append("/*******************************************************************************\n")
    shutdown_lines.append(" * EcuM 关机阶段实现\n")
    shutdown_lines.append(" * 自动拆分自 EcuM.c\n")
    shutdown_lines.append(" ******************************************************************************/\n")
    shutdown_lines.append("#define ECUM_START_SEC_CODE\n")
    shutdown_lines.append("#include \"MemMap.h\"\n")
    shutdown_lines.append("\n")
    for idx in shutdown_funcs:
        end = find_func_end(idx)
        shutdown_lines.extend(lines[idx:end])
    shutdown_lines.append("#define ECUM_STOP_SEC_CODE\n")
    shutdown_lines.append("#include \"MemMap.h\"\n")
    
    # Remove startup, run_sleep, shutdown from main
    remove_all = startup_funcs + run_sleep_funcs + shutdown_funcs
    remaining = list(lines)
    rm_ranges = [(idx, find_func_end(idx)) for idx in remove_all]
    for start, end in sorted(rm_ranges, reverse=True):
        del remaining[start:end]
    
    insert_pos = None
    for i, l in enumerate(remaining):
        if 'STOP_SEC_CODE' in l and 'ECUM' not in l:
            insert_pos = i
            break
    
    if insert_pos:
        include_block = [
            "\n",
            "/*==================================================================================================\n",
            " *  子文件包含 (批量拆分)\n",
            " *================================================================================================*/\n",
            "#include \"ecum_startup.c\"\n",
            "#include \"ecum_run_sleep.c\"\n",
            "#include \"ecum_shutdown.c\"\n",
        ]
        for il in reversed(include_block):
            remaining.insert(insert_pos, il)
    
    write_file(os.path.join(PROJECT, "src/bsw/services/ecum/src/ecum_startup.c"), startup_lines)
    write_file(os.path.join(PROJECT, "src/bsw/services/ecum/src/ecum_run_sleep.c"), run_sleep_lines)
    write_file(os.path.join(PROJECT, "src/bsw/services/ecum/src/ecum_shutdown.c"), shutdown_lines)
    write_file(src, remaining)
    
    print(f"EcuM.c: original {len(lines)} lines → main {len(remaining)} lines, startup {len(startup_lines)} lines, run_sleep {len(run_sleep_lines)} lines, shutdown {len(shutdown_lines)} lines")


# Run all
if __name__ == '__main__':
    # Backup existing split files first
    import glob
    
    print("=" * 60)
    print("Batch 9d — 大文件拆分")
    print("=" * 60)
    
    split_csm()
    print()
    split_nvm()
    print()
    split_xcp()
    print()
    split_crypto()
    print()
    split_ecum()
    print()
    print("=" * 60)
    print("All splits completed. Verify with: wc -l on all files.")
    print("=" * 60)
