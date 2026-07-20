#!/usr/bin/env python3
"""
Batch 9d v3 — Fix remaining oversized files.
"""
import os, re

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def read_file(p):
    with open(p) as f:
        return f.readlines()

def write_file(p, lines):
    with open(p, 'w') as f:
        f.writelines(lines)

# ========== Xcp xcp_cmd.c: 1292 → split into std_cmd + daq_pgm ==========
def fix_xcp_cmd():
    src = os.path.join(PROJECT, "src/bsw/services/xcp/src")
    cmd_path = os.path.join(src, "xcp_cmd.c")
    lines = read_file(cmd_path)
    
    # Find all function starts in this file
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
    
    # Find stop marker
    stop = len(lines)
    for i, l in enumerate(lines):
        if 'XCP_STOP_SEC_CODE' in l:
            stop = i + 2
            break
    
    def find_func_end(start):
        for s in func_starts:
            if s > start:
                return s
        return stop
    
    # Classify functions:
    # Standard cmds: CmdConnect, CmdDisconnect, CmdGetStatus, CmdGetCommModeInfo, 
    #   CmdGetId, CmdSetMta, CmdUpload, CmdShortUpload, CmdDownload, CmdGetSeed, CmdUnlock
    # DAQ cmds: CmdClearDaqList through CmdAllocOdtEntry
    # PGM cmds: CmdProgramStart through CmdProgramVerify
    # Static processors: ProcessStandardCommand, ProcessDaqCommand, ProcessPgmCommand,
    #   CalculateChecksum, ValidateMemoryAccess, ClearDaqList, ResetDaqConfiguration, GetTimestamp
    
    std_funcs = []
    daq_funcs = []
    pgm_funcs = []
    static_procs = []
    
    for idx in func_starts:
        m = re.search(r'(Xcp_|xcp_)\w+\(', lines[idx])
        fname = m.group(0).rstrip('(') if m else ''
        if fname.startswith('Xcp_Cmd'):
            short = fname[len('Xcp_Cmd'):]
            if short in ('Connect', 'Disconnect', 'GetStatus', 'GetCommModeInfo',
                        'GetId', 'SetMta', 'Upload', 'ShortUpload', 'Download',
                        'GetSeed', 'Unlock'):
                std_funcs.append(idx)
            elif short in ('ClearDaqList', 'SetDaqPtr', 'WriteDaq', 'SetDaqListMode',
                          'GetDaqListMode', 'StartStopDaqList', 'StartStopSynch',
                          'GetDaqProcessorInfo', 'GetDaqResolutionInfo', 'GetDaqListInfo',
                          'FreeDaq', 'AllocDaq', 'AllocOdt', 'AllocOdtEntry'):
                daq_funcs.append(idx)
            elif short in ('ProgramStart', 'ProgramClear', 'Program', 'ProgramReset', 'ProgramVerify'):
                pgm_funcs.append(idx)
        elif fname.startswith('Xcp_Process') or fname.startswith('Xcp_') and fname not in (
            'Xcp_CmdConnect', 'Xcp_CmdDisconnect'  # handled above
        ):
            # Static helpers stay with std
            if fname in ('Xcp_ProcessStandardCommand', 'Xcp_ProcessDaqCommand',
                        'Xcp_ProcessPgmCommand', 'Xcp_CalculateChecksum',
                        'Xcp_ValidateMemoryAccess', 'Xcp_ClearDaqList',
                        'Xcp_ResetDaqConfiguration', 'Xcp_GetTimestamp'):
                static_procs.append(idx)
    
    # Allocate static procs: ProcessStandardCommand + CalculateChecksum + ValidateMemoryAccess go to std
    # ProcessDaqCommand, ProcessPgmCommand, ClearDaqList, ResetDaqConfiguration, GetTimestamp go to daq_pgm
    std_statics = []
    daq_statics = []
    for idx in static_procs:
        m = re.search(r'(Xcp_\w+)\s*\(', lines[idx])
        if m:
            fname = m.group(1)
            if fname in ('Xcp_ProcessStandardCommand', 'Xcp_CalculateChecksum', 'Xcp_ValidateMemoryAccess'):
                std_statics.append(idx)
            else:
                daq_statics.append(idx)
    
    # Build std cmd file
    std_lines = [
        "/*==================================================================================================\n",
        " * XCP 标准命令处理实现\n",
        " * 自动拆分自 Xcp.c\n",
        " *================================================================================================*/\n",
        "#define XCP_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(std_funcs + std_statics):
        end = find_func_end(idx)
        std_lines.extend(lines[idx:end])
    std_lines.append("#define XCP_STOP_SEC_CODE\n")
    std_lines.append("#include \"MemMap.h\"\n")
    
    # Build daq+pgm cmd file
    daq_pgm_lines = [
        "/*==================================================================================================\n",
        " * XCP DAQ/PGM 命令处理实现\n",
        " * 自动拆分自 Xcp.c\n",
        " *================================================================================================*/\n",
        "#define XCP_START_SEC_CODE\n",
        "#include \"MemMap.h\"\n",
        "\n",
    ]
    for idx in sorted(daq_funcs + pgm_funcs + daq_statics):
        end = find_func_end(idx)
        daq_pgm_lines.extend(lines[idx:end])
    daq_pgm_lines.append("#define XCP_STOP_SEC_CODE\n")
    daq_pgm_lines.append("#include \"MemMap.h\"\n")
    
    # Update the main Xcp.c include list  
    main_path = os.path.join(src, "Xcp.c")
    main_lines = read_file(main_path)
    for i, l in enumerate(main_lines):
        if '#include "xcp_cmd.c"' in l:
            main_lines[i] = '#include "xcp_cmd_std.c"\n'
            main_lines.insert(i, '#include "xcp_cmd_daq_pgm.c"\n')
            break
    
    write_file(os.path.join(src, "xcp_cmd_std.c"), std_lines)
    write_file(os.path.join(src, "xcp_cmd_daq_pgm.c"), daq_pgm_lines)
    write_file(main_path, main_lines)
    
    print(f"xcp_cmd split: {len(lines)} → std {len(std_lines)} + daq_pgm {len(daq_pgm_lines)}")


# ========== NvM nvm_jobs.c: 1013 → trim last ~15 lines ==========
def trim_nvm_jobs():
    src = os.path.join(PROJECT, "src/bsw/services/nvm/src")
    jobs_path = os.path.join(src, "nvm_jobs.c")
    lines = read_file(jobs_path)
    
    # Remove some trailing blank lines or redundant comments
    # Find the function content and trim excess
    content_end = None
    for i, l in enumerate(lines):
        if '#define NVM_STOP_SEC_CODE' in l:
            content_end = i
            break
    
    # Remove blank lines at the end of content
    if content_end:
        while content_end > 0 and lines[content_end-1].strip() in ('', '\n'):
            content_end -= 1
        # Trim blank lines before stop marker
        trimmed = lines[:content_end] + lines[content_end:]
        write_file(jobs_path, trimmed)
        print(f"nvm_jobs trimmed: {len(lines)} → {len(trimmed)}")


if __name__ == '__main__':
    fix_xcp_cmd()
    trim_nvm_jobs()
    print("\nFinal verification:")
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
              "src/bsw/services/xcp/src/xcp_cmd_std.c "
              "src/bsw/services/xcp/src/xcp_cmd_daq_pgm.c "
              "src/bsw/mcal/crypto/src/Crypto_S32K312_Hsm.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_aes.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_ecc.c "
              "src/bsw/mcal/crypto/src/crypto_hsm_sha_key.c "
              "src/bsw/services/ecum/src/EcuM.c "
              "src/bsw/services/ecum/src/ecum_startup.c "
              "src/bsw/services/ecum/src/ecum_run_sleep.c "
              "src/bsw/services/ecum/src/ecum_shutdown.c")
