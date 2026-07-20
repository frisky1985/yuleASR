#!/usr/bin/env python3
"""
Fix compilation errors caused by the split:
- Restore static prototypes that were removed from main files
- Fix sub-file static declarations
- Add missing includes for called functions across sub-files
"""
import os, re

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def read_file(p):
    with open(p) as f:
        return f.readlines()

def write_file(p, lines):
    with open(p, 'w') as f:
        f.writelines(lines)

# ========== Fix Xcp.c: restore static prototypes that were removed ==========
def fix_xcp_prototypes():
    src = os.path.join(PROJECT, "src/bsw/services/xcp/src")
    main_path = os.path.join(src, "Xcp.c")
    lines = read_file(main_path)
    
    # Restore static prototypes from backup
    bak_path = main_path + ".9d.bak"
    bak_lines = read_file(bak_path)
    
    # Extract static prototypes from backup
    static_protos = []
    capture = False
    for l in bak_lines:
        if '#define XCP_START_SEC_CODE' in l:
            capture = True
            continue
        if capture and re.match(r'^static (void|Std_ReturnType|boolean|uint16|uint32)\s+Xcp_\w+\(', l):
            static_protos.append(l)
        if capture and not l.startswith('static') and not l.startswith(' '):
            if static_protos:
                break
    
    if not static_protos:
        # Try earlier section
        capture = False
        for l in bak_lines:
            if 'Xcp_GetTimestamp' in l and 'static' in l:
                static_protos.append(l)
                break
    
    # Actually, the prototypes are at lines 115-122 of the backup
    # Let me check the exact section
    for i, l in enumerate(bak_lines):
        if 'static void Xcp_ProcessStandardCommand' in l:
            j = i
            while j < len(bak_lines) and bak_lines[j].startswith('static '):
                static_protos.append(bak_lines[j])
                j += 1
            break
    
    print(f"Found {len(static_protos)} static prototypes in backup")
    
    # Find where to insert in main file - before the code section
    insert_pos = None
    for i, l in enumerate(lines):
        if 'XCP_START_SEC_CODE' in l:
            insert_pos = i
            break
    
    if insert_pos and static_protos:
        # Insert static prototypes
        insertion = ['\n/* Restored static prototypes */\n']
        for p in static_protos:
            insertion.append(p)
        insertion.append('\n')
        
        for il in reversed(insertion):
            lines.insert(insert_pos, il)
        
        write_file(main_path, lines)
        print(f"Inserted {len(static_protos)} static prototypes at line {insert_pos}")
    else:
        print(f"Cannot find insertion point (insert_pos={insert_pos}, protos={len(static_protos)})")

# ========== Fix static on sub-file implementations ==========
def fix_subfile_static():
    files = [
        ("xcp/src/xcp_cmd_daq_pgm.c", "Xcp_ProcessDaqCommand", "Xcp_ProcessPgmCommand", "Xcp_ClearDaqList", "Xcp_ResetDaqConfiguration", "Xcp_GetTimestamp"),
        ("xcp/src/xcp_cmd_std.c", "Xcp_ProcessStandardCommand", "Xcp_CalculateChecksum", "Xcp_ValidateMemoryAccess"),
    ]
    
    for rel_path, *func_names in files:
        full = os.path.join(PROJECT, f"src/bsw/services/{rel_path}")
        lines = read_file(full)
        
        new_lines = []
        for l in lines:
            # Add static back to function implementations
            for fname in func_names:
                pattern = f'void {fname}('
                pattern2 = f'uint16 {fname}('
                pattern3 = f'boolean {fname}('
                pattern4 = f'uint32 {fname}('
                if l.strip().startswith(pattern) or l.strip().startswith(pattern2) or \
                   l.strip().startswith(pattern3) or l.strip().startswith(pattern4):
                    l = f'static {l}'
                    break
            new_lines.append(l)
        
        write_file(full, new_lines)
        print(f"Fixed {rel_path}: added static to function implementations")

# ========== Fix NvM.c: restore static prototypes ==========
def fix_nvm_prototypes():
    src = os.path.join(PROJECT, "src/bsw/services/nvm/src")
    main_path = os.path.join(src, "NvM.c")
    lines = read_file(main_path)
    
    # Check which static prototypes are missing
    bak_path = main_path + ".9d.bak"
    bak_lines = read_file(bak_path)
    
    # Extract static prototypes from backup
    static_protos = []
    for i, l in enumerate(bak_lines):
        if l.startswith('STATIC ') and '(' in l and ')' in l and ';' in l:
            static_protos.append(l)
            # Check if next line is continuation
            j = i + 1
            while j < len(bak_lines) and bak_lines[j].strip() and not bak_lines[j].startswith('STATIC ') and not bak_lines[j].startswith('#define'):
                if bak_lines[j].strip().startswith('(') or bak_lines[j].strip() == '':
                    break
                # Multi-line protos might not exist in this simple format
                break
    
    # Find which ones are missing from main
    existing = set()
    for l in lines:
        if 'STATIC' in l and ';' in l and ('(' in l):
            existing.add(l.strip())
    
    missing = [p for p in static_protos if p.strip() not in existing]
    print(f"NvM: {len(missing)} missing static prototypes out of {len(static_protos)}")
    
    if missing:
        # Insert before NVM_START_SEC_CODE
        insert_pos = None
        for i, l in enumerate(lines):
            if 'NVM_START_SEC_CODE' in l:
                insert_pos = i
                break
        
        if insert_pos:
            insertion = ['\n']
            for p in missing:
                insertion.append(p)
            for il in reversed(insertion):
                lines.insert(insert_pos, il)
            write_file(main_path, lines)
            print(f"Inserted {len(missing)} static prototypes")

# ========== Fix NvM duplicated prototypes in nvm_jobs.c ==========
def fix_nvm_jobs_protos():
    src = os.path.join(PROJECT, "src/bsw/services/nvm/src")
    path = os.path.join(src, "nvm_jobs.c")
    lines = read_file(path)
    
    # Remove STATIC from function implementations (protos are in main now)
    new_lines = []
    for l in lines:
        if l.startswith('STATIC ') and ('(' in l):
            l = l.replace('STATIC ', '', 1)
        new_lines.append(l)
    
    write_file(path, new_lines)
    print(f"nvm_jobs.c: removed STATIC from all functions")

if __name__ == '__main__':
    fix_xcp_prototypes()
    fix_subfile_static()
    fix_nvm_prototypes()
    fix_nvm_jobs_protos()
    
    print("\nRebuilding...")
    for target in ['service_xcp', 'service_nvm', 'service_ecum']:
        os.system(f"cd {PROJECT} && cmake --build build-coverage --target {target} 2>&1 | grep -E 'error:|Built' | head -5")
