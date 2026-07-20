#!/usr/bin/env python3
"""
Batch 9 — Large File Splitter
Splits 5 large C source files (>2000 lines each) into modular sub-files
that are textually included by the main aggregator file.

Usage: python3 tools/split_large_files.py
"""
import os
import re

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def write_aggregator(path, content):
    """Write a file ensuring the directory exists."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)
    print(f"  Written: {path} ({len(content.splitlines())} lines)")

def read_file(path):
    with open(path, 'r') as f:
        return f.read()

def split_csm():
    """Split Csm.c into csm_main.c, _csm_keys_impl.c, _csm_jobs_impl.c"""
    src = read_file(os.path.join(PROJECT, "src/bsw/services/csm/src/Csm.c"))
    lines = src.splitlines(keepends=True)
    
    # ============ CSM LINE MAP ============
    # Lines 0-90: License, header comment
    # Lines 91-186: #includes, macros, types, global vars, static declarations  
    # Lines 187-614: Static helper functions (CSM_START_SEC_CODE block 1)
    # Lines 615-712: Csm_Init, Csm_DeInit  
    # Lines 713-1100: Key management API (Csm_KeyElementSet..Csm_KeyGenerate)
    # Lines 1101-1962: Key derive/exchange (Csm_KeyDerive..Csm_KeyExchangeCalcSecret)
    # Lines 1963-2505: Crypto ops (Csm_Hash..Csm_RandomGenerate)
    # Lines 2506-2682: Job/misc API (Csm_JobKeySetUp..Csm_GetVersionInfo)
    # Lines 2683-end: Config data section
    
    # Block boundaries (0-indexed):
    # 0: header start
    # 91: includes/macros/types start
    # 187: CSM_START_SEC_CODE (static helpers begin)
    # 615: /* API函数实现 */ — API section starts
    # 713: /* Key mgmt API starts  
    # 1101: KeyDerive starts
    # 1963: Hash starts
    # 2506: JobKeySetUp starts
    # 2680: Csm_GetVersionInfo
    # 2684: CSM_STOP_SEC_CODE
    # 2689: CSM_START_SEC_CONFIG_DATA_UNSPECIFIED (0-indexed: 2686? check)
    
    # Let me find exact line numbers for key markers
    text = src
    
    # Find exact marker positions (0-indexed lines)
    markers = {}
    for i, line in enumerate(lines):
        if 'Csm_Init(const Csm_ConfigType* config)' in line:
            markers['init'] = i
        elif 'Csm_KeyElementSet(' in line and 'return' not in line and 'Std_ReturnType' in line:
            markers['key_element_set'] = i
        elif 'Csm_KeyGenerate(uint32 keyId)' in line:
            markers['key_generate'] = i
        elif 'Csm_KeyDerive(uint32 keyId, uint32 targetKeyId)' in line:
            markers['key_derive'] = i
        elif 'Csm_KeyExchangeCalcSecret(' in line:
            markers['key_exch_secret'] = i
        elif 'Csm_Hash(' in line and 'Std_ReturnType' in line:
            markers['hash'] = i
        elif 'Csm_RandomGenerate(' in line and 'Std_ReturnType' in line:
            markers['random_gen'] = i
        elif 'Csm_JobKeySetUp(uint32 jobId, uint32 keyId)' in line:
            markers['jobkeysetup'] = i
        elif 'Csm_GetVersionInfo' in line:
            markers['versioninfo'] = i
        elif '#define CSM_STOP_SEC_CODE' in line:
            markers['stop_code'] = i
        elif '#define CSM_START_SEC_CONFIG_DATA_UNSPECIFIED' in line:
            markers['start_config'] = i
        elif '#define CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED' in line:
            markers['stop_config'] = i
    
    # For safety, use the markers we have
    # Section 1: Headers + types + globals + static decls (0 to just before CSM_START_SEC_CODE)
    # Actually let's find the specific boundaries
    
    # I'll use these approximate boundaries:
    sec_header_end = markers.get('init', 618)  # Just before Csm_Init
    
    # The shared preamble: everything up to and including static functions
    # This goes into csm_main.c
    
    # Key API functions: from Csm_KeyElementSet to just before Csm_KeyDerive
    # These go into _csm_keys_impl.c
    
    # Job/crypto API functions: from Csm_KeyDerive to end of code section
    # These go into _csm_jobs_impl.c
    
    # Let me compute safe boundaries
    init_line = markers.get('init', 621 - 1)
    ks_line = markers.get('key_element_set', 737 - 1)
    kd_line = markers.get('key_derive', 1113 - 1)
    hs_line = markers.get('hash', 1966 - 1)
    job_line = markers.get('jobkeysetup', 2509 - 1)
    stop_code = markers.get('stop_code', 2680 - 1)
    start_cfg = markers.get('start_config', 2686 - 1)
    stop_cfg = markers.get('stop_config', 2802 - 1)
    
    # Preamble: everything up to and including the section comment before Csm_Init
    # "==================================================================" at line ~614
    # Find the API section header
    api_section_header = None
    for i in range(init_line - 20, init_line):
        if 'API函数实现' in lines[i] or 'API' in lines[i]:
            api_section_header = i
            break
    if api_section_header is None:
        api_section_header = init_line - 1
        # Walk back to find the section separator
        while api_section_header > 0 and not lines[api_section_header].startswith('/*='):
            api_section_header -= 1
    
    # Preamble end: the blank line after the section comment, before Csm_Init
    # Let's have the preamble include everything from 0 to just before Csm_Init's doxygen comment
    preamble_end = init_line - 1
    while preamble_end > 0 and (lines[preamble_end].strip() == '' or lines[preamble_end].startswith('/*')):
        preamble_end -= 1
    # Include the section separator
    preamble_end = api_section_header + 1
    
    # ---- CSM_MAIN.C (aggregator + config data) ----
    preamble = ''.join(lines[0:preamble_end])
    
    # Find the config data start: CSM_START_SEC_CONFIG_DATA_UNSPECIFIED to end
    config_data = ''.join(lines[start_cfg:stop_cfg+1])
    
    # The common license header for sub-files
    lic_header = ''.join(lines[0:30])  # First 30 lines = license + brief file comment
    
    csm_main_content = f'''/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/**
 * @file Csm.c
 * @brief CSM (Crypto Services Manager) 聚合文件
 * 
 * 包含子模块:
 * - _csm_keys_impl.c  : 密钥管理实现
 * - _csm_jobs_impl.c  : 作业/密码服务实现
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */
{preamble}

/* 密钥管理实现 */
#include "_csm_keys_impl.c"

/* 作业/密码服务实现 */
#include "_csm_jobs_impl.c"

/*==================================================================================================
*                                       配置定义
==================================================================================================*/
{config_data}
'''
    
    # ---- KEY API: from Csm_KeyElementSet to just before Csm_KeyDerive ----
    key_impl_lines = []
    # Section header
    key_section_header = f'''/*==================================================================================================
 * 密钥管理 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

'''
    key_impl_lines.append(key_section_header)
    
    # Add functions from Csm_KeyElementSet doxygen to just before Csm_KeyDerive doxygen
    for i in range(ks_line, kd_line):
        key_impl_lines.append(lines[i])
    
    key_impl_lines.append(f'''
#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"
''')
    
    csm_keys_content = ''.join(key_impl_lines)
    
    # ---- JOB/CRYPTO API: from Csm_KeyDerive to CSM_STOP_SEC_CODE (exclusive, since it's in sub-file) ----
    job_impl_lines = []
    job_section_header = f'''/*==================================================================================================
 * 作业/密码服务 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

'''
    job_impl_lines.append(job_section_header)
    
    # Add functions from Csm_KeyDerive to stop_code marker
    # Include the "/* API函数实现 */" section that covers job-related APIs
    for i in range(kd_line, stop_code):
        job_impl_lines.append(lines[i])
    
    job_impl_lines.append(f'''
#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"
''')
    
    csm_jobs_content = ''.join(job_impl_lines)
    
    # Write the files
    base_dir = os.path.join(PROJECT, "src/bsw/services/csm/src")
    
    # Backup original
    os.rename(os.path.join(base_dir, "Csm.c"), os.path.join(base_dir, "Csm.c.bak"))
    
    write_aggregator(os.path.join(base_dir, "Csm.c"), csm_main_content)
    write_aggregator(os.path.join(base_dir, "_csm_keys_impl.c"), csm_keys_content)
    write_aggregator(os.path.join(base_dir, "_csm_jobs_impl.c"), csm_jobs_content)
    
    print(f"  Csm.c: 2803 → Csm.c (agg) + _csm_keys_impl.c + _csm_jobs_impl.c")


def split_nvm():
    """Split NvM.c into nvm_main.c, _nvm_read_impl.c, _nvm_write_impl.c"""
    src = read_file(os.path.join(PROJECT, "src/bsw/services/nvm/src/NvM.c"))
    lines = src.splitlines(keepends=True)
    
    # NvM structure:
    # Lines 0-176: License, includes, macros
    # Lines 176-916: Static helpers, internal functions (NVM_START_SEC_CODE)
    # Lines 917-976: NvM_Init
    # Lines 977-1123: NvM_ReadBlock
    # Lines 1124-1473: Other simple APIs (SetDataIndex, WriteBlockOnce, etc.)
    # Lines 1474-1686: NvM_GetVersionInfo, NvM_GetErrorStatus, NvM_SetRamBlockStatus
    # Lines 1687-2126: NvM_MainFunction (massive)
    # Lines 2127-2361: NvM_ReadAll, NvM_WriteAll, kill functions
    # Lines 2362: NVM_STOP_SEC_CODE
    
    # Find markers
    markers = {}
    for i, line in enumerate(lines):
        if 'NvM_Init(const NvM_ConfigType* ConfigPtr)' in line:
            markers['init'] = i
        elif 'NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)' in line:
            markers['read_block'] = i
        elif 'NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)' in line:
            markers['write_block'] = i
        elif 'NvM_GetVersionInfo' in line and 'Std_ReturnType' not in line:
            markers['versioninfo'] = i
        elif 'NvM_MainFunction(void)' in line:
            markers['mainfunc'] = i
        elif 'NvM_ReadAll(void)' in line and 'Std_ReturnType' in line:
            markers['readall'] = i
        elif 'NvM_WriteAll(void)' in line and 'Std_ReturnType' in line:
            markers['writeall'] = i
        elif '#define NVM_STOP_SEC_CODE' in line:
            markers['stop_code'] = i
        elif '#define NVM_START_SEC_VAR_CLEARED_UNSPECIFIED' in line:
            markers['start_var'] = i
        elif '#define NVM_START_SEC_CODE' in line:
            markers['start_code'] = i
    
    init_l = markers.get('init', 918 - 1)
    read_l = markers.get('read_block', 976 - 1)
    write_l = markers.get('write_block', 1040 - 1)
    mainfunc_l = markers.get('mainfunc', 1687 - 1)
    readall_l = markers.get('readall', 2131 - 1)
    writeall_l = markers.get('writeall', 2179 - 1)
    stop_code = markers.get('stop_code', 2362 - 1)
    start_var = markers.get('start_var', 134 - 1)
    start_code = markers.get('start_code', 174 - 1)
    
    # Preamble: license + includes + macros + types + static forward decls
    # up to (but not including) the first CSM_START_SEC_CODE
    preamble = ''.join(lines[0:start_code])
    
    # Find static helpers section: from CSM_START_SEC_CODE to just before NvM_Init
    # These are internal helpers needed by all sub-modules
    static_helpers = ''.join(lines[start_code:init_l])
    
    # Read operations: from NvM_ReadBlock to just before NvM_WriteBlock
    read_funcs = ''.join(lines[read_l:write_l])
    
    # Write operations: from NvM_WriteBlock to just before NvM_MainFunction
    # Exclude NvM_GetVersionInfo, NvM_GetErrorStatus (keep in write portion)
    write_funcs_lines = []
    for i in range(write_l, mainfunc_l):
        write_funcs_lines.append(lines[i])
    write_funcs = ''.join(write_funcs_lines)
    
    # MainFunction and bulk ops: everything from NvM_MainFunction to end of code
    main_bulk_funcs_lines = []
    for i in range(mainfunc_l, stop_code):
        main_bulk_funcs_lines.append(lines[i])
    main_bulk_funcs = ''.join(main_bulk_funcs_lines)
    
    # Build NvM.c aggregator
    nvm_main_content = f'''/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/**
 * @file NvM.c
 * @brief NvM 聚合文件
 * 
 * 包含子模块:
 * - _nvm_read_impl.c  : NVM读取操作
 * - _nvm_write_impl.c : NVM写入/控制操作
 * - _nvm_main_bulk_impl.c : NVM主函数/批量操作
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */
{preamble}
{static_helpers}

/* NVM读取操作 */
#include "_nvm_read_impl.c"

/* NVM写入/控制操作 */
#include "_nvm_write_impl.c"

/* NVM主函数/批量操作 */
#include "_nvm_main_bulk_impl.c"

#define NVM_STOP_SEC_CODE
#include "MemMap.h"
'''
    
    read_impl = f'''/*==================================================================================================
 * NVM 读取操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

{read_funcs}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"
'''
    
    write_impl = f'''/*==================================================================================================
 * NVM 写入/控制操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

{write_funcs}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"
'''
    
    main_bulk_impl = f'''/*==================================================================================================
 * NVM 主函数/批量操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

{main_bulk_funcs}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"
'''
    
    base_dir = os.path.join(PROJECT, "src/bsw/services/nvm/src")
    os.rename(os.path.join(base_dir, "NvM.c"), os.path.join(base_dir, "NvM.c.bak"))
    
    write_aggregator(os.path.join(base_dir, "NvM.c"), nvm_main_content)
    write_aggregator(os.path.join(base_dir, "_nvm_read_impl.c"), read_impl)
    write_aggregator(os.path.join(base_dir, "_nvm_write_impl.c"), write_impl)
    write_aggregator(os.path.join(base_dir, "_nvm_main_bulk_impl.c"), main_bulk_impl)
    
    print(f"  NvM.c: 2367 → NvM.c (agg) + 3 sub-files")


def split_xcp():
    """Split Xcp.c into xcp_transport.c, _xcp_cmd_impl.c"""
    src = read_file(os.path.join(PROJECT, "src/bsw/services/xcp/src/Xcp.c"))
    lines = src.splitlines(keepends=True)
    
    # Xcp structure:
    # Lines 0-130: License, includes, types, variables, const, forward decls
    # Lines 131-263: Xcp_Init, Xcp_DeInit, Xcp_GetVersionInfo
    # Lines 264-530: Transport/main functions (Xcp_MainFunction, RxIndication, TxConfirmation,
    #                TriggerTransmit, SetTransmissionMode, ProcessCommand, SendResponse, etc.)
    # Lines 531-1494: CMD functions (Csm_CmdConnect..Csm_CmdProgramReset, Csm_CmdProgramVerify)
    # Lines 1495-1660: DAQ functions (Csm_DaqProcessor, DaqSample, DaqTransmit, StimProcessor)
    # Lines 1661-1790: Memory functions (ReadMemory, WriteMemory)
    # Lines 1791-2011: Static dispatch + utility functions
    # Lines 2012-end: Config data
    
    markers = {}
    for i, line in enumerate(lines):
        if '#define XCP_START_SEC_CODE' in line:
            markers['start_code'] = i
        elif 'Xcp_Init(const Xcp_ConfigType* ConfigPtr)' in line:
            markers['init'] = i
        elif 'Xcp_CmdConnect' in line and 'void' in line and len(line) < 50:
            markers['cmd_connect'] = i
        elif 'Xcp_CmdDisconnect' in line and 'void' in line and len(line) < 50:
            markers['cmd_disconnect'] = i
        elif 'Xcp_DaqProcessor(void)' in line:
            markers['daq_proc'] = i
        elif 'Xcp_ReadMemory(uint32 Addr' in line:
            markers['read_mem'] = i
        elif 'static void Xcp_ProcessStandardCommand' in line:
            markers['static_dispatch'] = i
        elif '#define XCP_STOP_SEC_CODE' in line:
            markers['stop_code'] = i
        elif '#define XCP_START_SEC_CONFIG_DATA_UNSPECIFIED' in line:
            markers['start_cfg'] = i
    
    start_code = markers.get('start_code', 129 - 1)
    init_l = markers.get('init', 133 - 1)
    cmd_l = markers.get('cmd_connect', 513 - 1)
    daq_l = markers.get('daq_proc', 1523 - 1)
    read_mem = markers.get('read_mem', 1661 - 1)
    static_dispatch = markers.get('static_dispatch', 1759 - 1)
    stop_code = markers.get('stop_code', 2012 - 1)
    start_cfg = markers.get('start_cfg', 2018 - 1)
    
    # Preamble: everything from start to start_code (inclusive)
    preamble = ''.join(lines[0:start_code])
    
    # Transport layer: CSM_START_SEC_CODE to just before cmd functions
    transport_funcs = ''.join(lines[start_code:cmd_l])
    
    # Command layer: from cmd functions to end of code section
    cmd_funcs = ''.join(lines[cmd_l:stop_code])
    
    # Config data
    config_data = ''.join(lines[start_cfg:])
    
    # Build Xcp.c aggregator
    xcp_main_content = f'''/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Platform             : NXP i.MX8M Mini
 *
 * Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 *================================================================================================*/
/**
 * @file Xcp.c
 * @brief XCP 聚合文件
 *
 * 包含子模块:
 * - _xcp_cmd_impl.c : XCP 命令处理层
 *
 * @author yuleASR Team
 * @version 1.0.0
 */
{preamble}

/* 传输层实现 */
#define XCP_START_SEC_CODE
#include "Xcp_MemMap.h"

{transport_funcs}

/* XCP 命令处理层 */
#include "_xcp_cmd_impl.c"

#define XCP_STOP_SEC_CODE
#include "Xcp_MemMap.h"

/* 配置数据 */
{config_data}
'''
    
    cmd_impl = f'''/*==================================================================================================
 * XCP 命令处理实现
 * 自动拆分自 Xcp.c
 *================================================================================================*/
#define XCP_START_SEC_CODE
#include "Xcp_MemMap.h"

{cmd_funcs}

#define XCP_STOP_SEC_CODE
#include "Xcp_MemMap.h"
'''
    
    base_dir = os.path.join(PROJECT, "src/bsw/services/xcp/src")
    os.rename(os.path.join(base_dir, "Xcp.c"), os.path.join(base_dir, "Xcp.c.bak"))
    
    write_aggregator(os.path.join(base_dir, "Xcp.c"), xcp_main_content)
    write_aggregator(os.path.join(base_dir, "_xcp_cmd_impl.c"), cmd_impl)
    
    print(f"  Xcp.c: 2035 → Xcp.c (agg) + _xcp_cmd_impl.c")


def split_crypto_hsm():
    """Split Crypto_S32K312_Hsm.c into sub-files by HSM service"""
    src = read_file(os.path.join(PROJECT, "src/bsw/mcal/crypto/src/Crypto_S32K312_Hsm.c"))
    lines = src.splitlines(keepends=True)
    
    markers = {}
    for i, line in enumerate(lines):
        if '#define CRYPTO_START_SEC_CODE' in line:
            markers['start_code'] = i
        elif 'S32K312_Hsm_Init(const S32K312_HsmConfigType* config)' in line:
            markers['init'] = i
        elif 'S32K312_Hsm_AesInit' in line and 'Std_ReturnType' in line:
            markers['aes_init'] = i
        elif 'S32K312_Hsm_EccInit' in line and 'Std_ReturnType' in line:
            markers['ecc_init'] = i
        elif 'S32K312_Hsm_Sha256Init' in line and 'Std_ReturnType' in line:
            markers['sha_init'] = i
        elif 'S32K312_Hsm_KeyImport' in line and 'Std_ReturnType' in line:
            markers['key_import'] = i
        elif 'S32K312_Hsm_WaitReady' in line and 'Std_ReturnType' in line:
            markers['wait_ready'] = i
        elif '#define CRYPTO_STOP_SEC_CODE' in line:
            markers['stop_code'] = i
    
    start_code = markers.get('start_code', 180 - 1)
    init_l = markers.get('init', 186 - 1)
    aes_l = markers.get('aes_init', 471 - 1)
    ecc_l = markers.get('ecc_init', 984 - 1)
    sha_l = markers.get('sha_init', 1371 - 1)
    key_import = markers.get('key_import', 1528 - 1)
    wait_ready = markers.get('wait_ready', 1668 - 1)
    stop_code = markers.get('stop_code', 1900 - 1)
    
    preamble = ''.join(lines[0:start_code])
    
    # Init & lifecycle
    init_funcs = ''.join(lines[start_code:aes_l])
    
    # AES functions only
    aes_funcs = ''.join(lines[aes_l:ecc_l])
    
    # ECC functions
    ecc_funcs = ''.join(lines[ecc_l:sha_l])
    
    # SHA functions
    sha_funcs = ''.join(lines[sha_l:key_import])
    
    # Key management + remaining (KeyImport through end of code)
    key_rest_funcs = ''.join(lines[key_import:stop_code])
    
    crypto_main = f'''/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Platform             : NXP i.MX8M Mini
 *
 * Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 *================================================================================================*/
/**
 * @file Crypto_S32K312_Hsm.c
 * @brief S32K312 HSM 聚合文件
 *
 * 包含子模块:
 * - _crypto_hsm_aes_impl.c  : AES 加解密实现
 * - _crypto_hsm_ecc_impl.c  : ECC 签名验签实现
 * - _crypto_hsm_sha_impl.c  : SHA 哈希实现
 * - _crypto_hsm_key_impl.c  : 密钥管理实现
 *
 * @author yuleASR Team
 * @version 1.0.0
 */
{preamble}

/* 初始化和生命周期 */
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"
{init_funcs}

/* AES 实现 */
#include "_crypto_hsm_aes_impl.c"

/* ECC 实现 */
#include "_crypto_hsm_ecc_impl.c"

/* SHA 实现 */
#include "_crypto_hsm_sha_impl.c"

/* 密钥管理 + 辅助函数 */
#include "_crypto_hsm_key_impl.c"

#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"
'''
    
    aes_impl = f'''/*==================================================================================================
 * S32K312 HSM AES 加解密实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"
{aes_funcs}
#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"
'''
    
    ecc_impl = f'''/*==================================================================================================
 * S32K312 HSM ECC 签名验签实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"
{ecc_funcs}
#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"
'''
    
    sha_impl = f'''/*==================================================================================================
 * S32K312 HSM SHA 哈希实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"
{sha_funcs}
#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"
'''
    
    key_impl = f'''/*==================================================================================================
 * S32K312 HSM 密钥管理及辅助函数
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"
{key_rest_funcs}
#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"
'''
    
    base_dir = os.path.join(PROJECT, "src/bsw/mcal/crypto/src")
    os.rename(os.path.join(base_dir, "Crypto_S32K312_Hsm.c"), 
              os.path.join(base_dir, "Crypto_S32K312_Hsm.c.bak"))
    
    write_aggregator(os.path.join(base_dir, "Crypto_S32K312_Hsm.c"), crypto_main)
    write_aggregator(os.path.join(base_dir, "_crypto_hsm_aes_impl.c"), aes_impl)
    write_aggregator(os.path.join(base_dir, "_crypto_hsm_ecc_impl.c"), ecc_impl)
    write_aggregator(os.path.join(base_dir, "_crypto_hsm_sha_impl.c"), sha_impl)
    write_aggregator(os.path.join(base_dir, "_crypto_hsm_key_impl.c"), key_impl)
    
    print(f"  Crypto_S32K312_Hsm.c: 1905 → Crypto_S32K312_Hsm.c (agg) + 4 sub-files")


def split_ecum():
    """Split EcuM.c into sub-files by state machine phase"""
    src = read_file(os.path.join(PROJECT, "src/bsw/services/ecum/src/EcuM.c"))
    lines = src.splitlines(keepends=True)
    
    markers = {}
    for i, line in enumerate(lines):
        if 'static void EcuM_ProcessStartupOne(void);' in line:
            markers['fwd_decls'] = i
        elif 'void EcuM_Init(void)' in line:
            markers['init'] = i
        elif 'void EcuM_StartupOne(void)' in line:
            markers['startup_one'] = i
        elif 'void EcuM_MainFunction(void)' in line:
            markers['mainfunc'] = i
        elif 'void EcuM_GoSleep(void)' in line:
            markers['gosleep'] = i
        elif 'static void EcuM_ProcessGoOffOne' in line:
            markers['goffone_start'] = i
        elif 'Std_ReturnType EcuM_RequestRUN' in line:
            markers['request_run'] = i
        elif 'Std_ReturnType EcuM_GetState' in line:
            markers['getstate'] = i
        elif 'Std_ReturnType EcuM_SelectShutdownTarget' in line:
            markers['select_shutdown'] = i
        elif 'void EcuM_SetWakeupEvent' in line:
            markers['set_wakeup'] = i
        elif 'Std_ReturnType EcuM_EnableWakeupSources' in line:
            markers['enable_wakeup'] = i
        elif 'static void EcuM_ValidateWakeupSources(void)' in line:
            markers['validate_wakeup'] = i
        elif 'Std_ReturnType EcuM_SelectBootTarget' in line:
            markers['select_boot'] = i
        elif 'void EcuM_GetVersionInfo' in line:
            markers['versioninfo'] = i
        elif 'static void EcuM_UpdateSubState' in line:
            markers['update_substate'] = i
    
    # Python is using 0-indexed lines
    init_l = markers.get('init', 138 - 1)
    startup_one = markers.get('startup_one', 185 - 1)
    mainfunc_l = markers.get('mainfunc', 358 - 1)
    gosleep_l = markers.get('gosleep', 453 - 1)
    goffone_l = markers.get('goffone_start', 719 - 1)
    request_run = markers.get('request_run', 866 - 1)
    wakeup_l = markers.get('set_wakeup', 1174 - 1)
    validate_wakeup = markers.get('validate_wakeup', 1398 - 1)
    select_boot = markers.get('select_boot', 1506 - 1)
    versioninfo = markers.get('versioninfo', 1678 - 1)
    
    # Preamble: includes, macros, statics, forward declarations
    fwd_decl = markers.get('fwd_decls')
    preamble = ''.join(lines[0:fwd_decl + 1]) if fwd_decl else ''.join(lines[0:init_l])
    
    # Startup phase: EcuM_Init through EcuM_MainFunction (inclusive of startup processing)
    startup_funcs_lines = []
    for i in range(init_l, mainfunc_l):
        startup_funcs_lines.append(lines[i])
    startup_funcs = ''.join(startup_funcs_lines)
    
    # Sleep/MainFunction processing
    main_sleep_lines = []
    for i in range(mainfunc_l, goffone_l):
        main_sleep_lines.append(lines[i])
    main_sleep_funcs = ''.join(main_sleep_lines)
    
    # Shutdown phase: GoOffOne through RequestRUN
    shutdown_lines = []
    for i in range(goffone_l, request_run):
        shutdown_lines.append(lines[i])
    shutdown_funcs = ''.join(shutdown_lines)
    
    # Run mode + Wakeup APIs
    run_wakeup_lines = []
    for i in range(request_run, select_boot):
        run_wakeup_lines.append(lines[i])
    run_wakeup_funcs = ''.join(run_wakeup_lines)
    
    # Boot/Mode/Version info + utility functions (rest of file)
    rest_lines = []
    for i in range(select_boot, len(lines)):
        rest_lines.append(lines[i])
    rest_funcs = ''.join(rest_lines)
    
    ecum_main = f'''/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Platform             : NXP i.MX8M Mini
 *
 * Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 *================================================================================================*/
/**
 * @file EcuM.c
 * @brief ECU State Manager 聚合文件
 *
 * 包含子模块:
 * - _ecum_startup_impl.c    : 启动阶段 (StartupOne/Two, Init)
 * - _ecum_run_sleep_impl.c  : 运行/睡眠阶段
 * - _ecum_shutdown_impl.c   : 关闭阶段 (GoOffOne/Two)
 * - _ecum_run_wakeup_impl.c : 运行请求/唤醒管理
 * - _ecum_rest_impl.c       : Boot/Mode/辅助函数
 *
 * @author yuleASR Team
 * @version 1.0.0
 */
{preamble}

/* 启动阶段 */
#include "_ecum_startup_impl.c"

/* 运行/睡眠阶段 */
#include "_ecum_run_sleep_impl.c"

/* 关闭阶段 */
#include "_ecum_shutdown_impl.c"

/* 运行请求/唤醒管理 */
#include "_ecum_run_wakeup_impl.c"

/* Boot/Mode/辅助函数 */
#include "_ecum_rest_impl.c"
'''
    
    startup_impl = f'''/*==================================================================================================
 * ECU State Manager — 启动阶段实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

{startup_funcs}
'''
    
    run_sleep_impl = f'''/*==================================================================================================
 * ECU State Manager — 运行/睡眠阶段实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

{main_sleep_funcs}
'''
    
    shutdown_impl = f'''/*==================================================================================================
 * ECU State Manager — 关闭阶段实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

{shutdown_funcs}
'''
    
    run_wakeup_impl = f'''/*==================================================================================================
 * ECU State Manager — 运行请求/唤醒管理实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

{run_wakeup_funcs}
'''
    
    rest_impl = f'''/*==================================================================================================
 * ECU State Manager — Boot/Mode/辅助函数
 * 自动拆分自 EcuM.c
 *================================================================================================*/

{rest_funcs}
'''
    
    base_dir = os.path.join(PROJECT, "src/bsw/services/ecum/src")
    os.rename(os.path.join(base_dir, "EcuM.c"), os.path.join(base_dir, "EcuM.c.bak"))
    
    write_aggregator(os.path.join(base_dir, "EcuM.c"), ecum_main)
    write_aggregator(os.path.join(base_dir, "_ecum_startup_impl.c"), startup_impl)
    write_aggregator(os.path.join(base_dir, "_ecum_run_sleep_impl.c"), run_sleep_impl)
    write_aggregator(os.path.join(base_dir, "_ecum_shutdown_impl.c"), shutdown_impl)
    write_aggregator(os.path.join(base_dir, "_ecum_run_wakeup_impl.c"), run_wakeup_impl)
    write_aggregator(os.path.join(base_dir, "_ecum_rest_impl.c"), rest_impl)
    
    print(f"  EcuM.c: 1863 → EcuM.c (agg) + 5 sub-files")


def update_cmake_filters():
    """Update CMakeLists.txt to exclude underscore-prefixed included files from GLOB"""
    # Services CMakeLists.txt
    services_cmake = os.path.join(PROJECT, "src/bsw/services/CMakeLists.txt")
    content = read_file(services_cmake)
    
    # Add underscore prefix filter
    old_filter = 'list(FILTER module_sources EXCLUDE REGEX ".*[_Tt]est\\.c$")'
    new_filter = '''    # Filter out test files (_test.c, _Test.c)
    list(FILTER module_sources EXCLUDE REGEX ".*[_Tt]est\\.c$")
    # Exclude sub-module files that are textually #included by their aggregator
    list(FILTER module_sources EXCLUDE REGEX ".*/_[a-z][a-z_]*\\.c$")'''
    
    if old_filter in content:
        content = content.replace(old_filter, new_filter)
        write_file(services_cmake, content)
        print(f"  Updated: {services_cmake} (added underscore filter)")
    else:
        # Try to find the filter line differently
        for line in content.splitlines():
            if 'FILTER' in line and 'EXCLUDE' in line and 'test' in line.lower():
                print(f"  WARNING: Could not find exact filter to replace in {services_cmake}")
                print(f"    Found: {line.strip()}")
                break
    
    # MCAL CMakeLists.txt  
    mcal_cmake = os.path.join(PROJECT, "src/bsw/mcal/CMakeLists.txt")
    content_mcal = read_file(mcal_cmake)
    
    # MCAL doesn't have a test filter, just add one
    glob_line = '    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)'
    new_glob = '''    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    # Exclude sub-module files that are textually #included by their aggregator
    list(FILTER module_sources EXCLUDE REGEX ".*/_[a-z][a-z_]*\\.c$")'''
    
    if glob_line in content_mcal:
        content_mcal = content_mcal.replace(glob_line, new_glob)
        write_file(mcal_cmake, content_mcal)
        print(f"  Updated: {mcal_cmake} (added underscore filter)")
    
    # Also update the services cmake for ecuM (which is in services)
    # Actually the services cmake already has GLOB_RECURSE pattern, so the above fix should cover it


def write_file(path, content):
    with open(path, 'w') as f:
        f.write(content)


if __name__ == '__main__':
    print("=== File Splitting Start ===\n")
    
    print("1. Splitting Csm.c...")
    split_csm()
    
    print("\n2. Splitting NvM.c...")
    split_nvm()
    
    print("\n3. Splitting Xcp.c...")
    split_xcp()
    
    print("\n4. Splitting Crypto_S32K312_Hsm.c...")
    split_crypto_hsm()
    
    print("\n5. Splitting EcuM.c...")
    split_ecum()
    
    print("\n6. Updating CMake filters...")
    update_cmake_filters()
    
    print("\n=== File Splitting Complete ===")
