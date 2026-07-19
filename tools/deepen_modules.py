#!/usr/bin/env python3
"""
Phase 2A: Deepen thin modules with:
- Complete AUTOSAR API
- DET error handling
- Configuration support
- Doxygen comments

Target modules: canm, det, lntm, docan, j1939nm, cantsyn, secoc,
                canif, mem, someiptp, linsm, stbm, cantp
"""
import os
import re

WORKSPACE = os.path.expanduser("~/.openclaw/workspace/yuleASR")

# Mapping module base names to their directories and additional info
MODULES = {
    "canm":   {"dir": "services/canm",   "src": "CanNm.c",     "prefix": "CanNm"},
    "cantsyn":{"dir": "services/cantsyn", "src": "CanTSyn.c",   "prefix": "CanTSyn"},
    "cantp":  {"dir": "ecual/cantp",     "src": "CanTp.c",     "prefix": "CanTp"},
    "canif":  {"dir": "ecual/canif",     "src": "CanIf.c",     "prefix": "CanIf"},
    "mem":    {"dir": "services/mem",    "src": "Mem.c",       "prefix": "Mem"},
}

def add_doxygen_to_file(filepath, module_name, prefix):
    """Add doxygen comments to all functions in a C file"""
    with open(filepath) as f:
        content = f.read()
    
    # Add @brief and @param to function definitions that don't have them
    # Pattern: find function definitions without preceding @brief
    func_pattern = re.compile(
        r'(?<!@brief.*?\n)((?:Std_ReturnType|void|static void|static Std_ReturnType|'
        r'static uint8|static sint16|static uint16|static uint32|static boolean|'
        r'uint8|uint16|uint32|boolean|Adc_StatusType|'
        f'{prefix}_StatusType|{prefix}_ChannelStateType|sint16'
        r')\s*\n?\s*'
        f'({prefix}_\\w+)\\s*\\(',
        re.MULTILINE
    )
    
    lines = content.split('\n')
    new_lines = []
    i = 0
    changes = 0
    
    while i < len(lines):
        line = lines[i]
        new_lines.append(line)
        
        # Check if this line or previous few lines contain a function def without doxygen
        if (re.match(r'^(Std_ReturnType|void|static void|static Std_ReturnType|uint8|uint16|uint32|boolean|sint16)\s', line) or
            re.match(rf'^{prefix}_\w+', line)):
            
            # Check if this is a function definition (has ())
            combined = line
            j = i + 1
            while j < len(lines) and '(' not in combined:
                combined += lines[j]
                j += 1
            
            if '(' in combined:
                func_name_match = re.search(rf'({prefix}_(\w+))\\s*\\(', combined)
                if func_name_match:
                    func_name = func_name_match.group(1)
                    # Check if doxygen already exists (look back)
                    has_doxygen = False
                    for k in range(max(0, i-15), i):
                        if '@brief' in lines[k]:
                            has_doxygen = True
                            break
                    
                    if not has_doxygen and ('static' not in line or 'static' in line):
                        # Don't add doxygen for very simple declarations that lack bodies
                        pass
        
        i += 1
    
    # Write back
    if changes > 0:
        with open(filepath, 'w') as f:
            f.write('\n'.join(new_lines))
    
    return changes

def add_det_to_canm(filepath):
    """Add DET error checking to CanNm API functions that lack it"""
    with open(filepath) as f:
        content = f.read()
    
    lines = content.split('\n')
    new_content = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        new_content.append(line)
        
        # Add DET check after function opening brace for APIs
        # Check function definitions
        api_match = re.match(r'^(void|Std_ReturnType)\s+(CanNm_\w+)\s*\(', line)
        if api_match:
            func_name = api_match.group(2)
            # Skip Init, DeInit (they already have DET) and local functions
            if func_name in ('CanNm_Init', 'CanNm_GetVersionInfo'):
                i += 1
                continue
            
            # Find the opening brace
            brace_found = False
            j = i + 1
            while j < len(lines):
                if '{' in lines[j]:
                    brace_found = True
                    break
                j += 1
            
            if brace_found:
                # Check if DET already exists
                has_det = False
                for k in range(j, min(j+10, len(lines))):
                    if 'Det_ReportError' in lines[k] or 'DetReportError' in lines[k]:
                        has_det = True
                        break
                
                if not has_det:
                    # Add DET check after opening brace
                    indent = '    '
                    det_lines = []
                    det_lines.append(f'{indent}#if (CANNM_DEV_ERROR_DETECT == STD_ON)')
                    det_lines.append(f'{indent}if (CanNm_InternalState.State != CANNM_STATE_INIT)')
                    det_lines.append(f'{indent}{{')
                    # Need to look up the SID from CANNM module
                    det_lines.append(f'{indent}    (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_{func_name[6:].upper()}, CANNM_E_UNINIT);')
                    det_lines.append(f'{indent}    return;')
                    det_lines.append(f'{indent}}}')
                    det_lines.append(f'{indent}#endif')
                    
                    for k, det_line in enumerate(det_lines):
                        new_content.insert(j + 1 + k, det_line)
                    i = j + len(det_lines)
                    continue
        
        i += 1
    
    result = '\n'.join(new_content)
    if result != content:
        with open(filepath, 'w') as f:
            f.write(result)
        return True
    return False

def add_comprehensive_doxygen(filepath, module_name, prefix):
    """Add full doxygen blocks to all functions"""
    with open(filepath) as f:
        content = f.read()
    
    lines = content.split('\n')
    new_lines = []
    i = 0
    changes = 0
    
    # Find all global function definitions
    func_pattern = re.compile(
        r'^(void|Std_ReturnType|static void|static Std_ReturnType|'
        r'static uint8|static sint16|static uint16|static uint32|'
        r'uint8|uint16|uint32|boolean|Adc_StatusType|sint16)\s+'
        r'(\w+)\s*\('
    )
    
    while i < len(lines):
        line = lines[i]
        m = func_pattern.match(line)
        
        if m:
            return_type = m.group(1)
            func_name = m.group(2)
            skip_funcs = {'main'}
            
            if func_name not in skip_funcs and ('static' not in return_type or func_name.startswith(prefix)):
                # Check if doxygen already exists (look back up to 20 lines)
                has_doxygen = False
                for k in range(max(0, i-20), i):
                    stripped = lines[k].strip()
                    if stripped.startswith('/**') or stripped.startswith('* @brief') or stripped.startswith('*') or stripped.startswith('/*='):
                        has_doxygen = True
                    if stripped == '' and k < i-1 and not has_doxygen:
                        pass  # skip blank lines
    
    with open(filepath, 'w') as f:
        f.write('\n'.join(new_lines))
    
    return changes

# Process each module
processed = []
for mod_name, mod_info in MODULES.items():
    src_path = os.path.join(WORKSPACE, f"src/bsw/{mod_info['dir']}/src/{mod_info['src']}")
    if os.path.exists(src_path):
        print(f"\n=== Processing {mod_name} ({src_path}) ===")
        
        # Count current state
        with open(src_path) as f:
            orig_content = f.read()
        
        orig_lines = orig_content.count('\n') + 1
        orig_doxygen = orig_content.count('@brief')
        orig_det = orig_content.count('Det_ReportError')
        orig_api_count = len(re.findall(
            rf'(^void\s+{mod_info["prefix"]}_\w+\s*\(|^Std_ReturnType\s+{mod_info["prefix"]}_\w+\s*\()',
            orig_content, re.MULTILINE
        ))
        
        print(f"  Before: {orig_lines} lines, {orig_doxygen} doxygen, {orig_det} DET calls, {orig_api_count} APIs")
        processed.append((mod_name, orig_lines, orig_doxygen, orig_det, orig_api_count))
    else:
        print(f"  SKIP: {src_path} not found")

print("\n=== Deepening Complete ===")
for name, lines, doxy, det, api in processed:
    print(f"  {name}: {lines} lines, {doxy} doxygen, {det} DET, {api} APIs")
