#!/usr/bin/env python3
"""Fix compilation errors across all 30 deepened modules."""
import os, re, glob

BASE = os.path.expanduser("~/.openclaw/workspace/yuleASR")

MODULES = [
    ("IpduM", "IPDUM"), ("EthSM", "ETHSM"), ("BswM", "BSWM"), ("SchM", "SCHM"),
    ("IoHwAb", "IOHWAB"), ("LinIf", "LINIF"), ("Srp", "SRP"), ("SomeIpIf", "SOMEIPIF"),
    ("EthIf", "ETHIF"), ("WdgIf", "WDGIF"), ("SomeIpSd", "SOMEIPSD"), ("EcuC", "ECUC"),
    ("Dio", "DIO"), ("Nm", "NM"), ("Spi", "SPI"), ("MemIf", "MEMIF"), ("Port", "PORT"),
    ("Pwm", "PWM"), ("LinSM", "LINSM"), ("FiM", "FIM"), ("Can", "CAN"), ("Gpt", "GPT"),
    ("Eep", "EEP"), ("Ea", "EA"), ("Mcu", "MCU"), ("Crc", "CRC"), ("LinM", "LINM"),
    ("CanSm", "CANSM"), ("J1939Tp", "J1939TP"), ("CanIf", "CANIF"),
]

def find_header(name):
    for pattern in [f"src/bsw/*/*/include/{name}.h", f"src/bsw/*/include/{name}.h"]:
        res = glob.glob(os.path.join(BASE, pattern), recursive=True)
        for r in res:
            if "_Cfg" not in r and "_Types" not in r:
                return r
    return None

def fix_header(name, p):
    """Add version macros and INSTANCE_ID to header."""
    hdr = find_header(name)
    if not hdr:
        return "HEADER NOT FOUND"
    
    with open(hdr) as f: content = f.read()
    edits = []
    
    # Add INSTANCE_ID if missing
    if f"{p}_INSTANCE_ID" not in content:
        content = re.sub(
            rf'#define\s+{p}_VENDOR_ID\s+\S+',
            f'\\g<0>\n#define {p}_INSTANCE_ID           0U',
            content
        )
        edits.append("INSTANCE_ID")
    
    # Add AR release version macros if missing
    if f"{p}_AR_RELEASE_MAJOR_VERSION" not in content:
        content = re.sub(
            rf'(#define\s+{p}_MODULE_ID\s+\S+)',
            f'\\g<1>\n#define {p}_AR_RELEASE_MAJOR_VERSION   4U\n#define {p}_AR_RELEASE_MINOR_VERSION   4U\n#define {p}_AR_RELEASE_REVISION_VERSION 0U\n#define {p}_SW_MAJOR_VERSION           1U\n#define {p}_SW_MINOR_VERSION           0U\n#define {p}_SW_PATCH_VERSION           0U',
            content
        )
        edits.append("version_macros")
    
    if edits:
        with open(hdr, 'w') as f: f.write(content)
    return ", ".join(edits) if edits else "ok"

def fix_source(name, p):
    """Fix common issues in .c file."""
    patterns = [
        f"src/bsw/*/*/src/{name}.c",
        f"src/bsw/*/src/{name}.c",
    ]
    src = None
    for pat in patterns:
        res = glob.glob(os.path.join(BASE, pat), recursive=True)
        for r in res:
            if "_Lcfg" not in r and "test" not in r:
                src = r
                break
        if src: break
    
    if not src: return "SRC NOT FOUND"
    
    with open(src) as f: content = f.read()
    edits = []
    
    # Fix CheckInit: replace {p}_STATE_UNINIT with the correct name from the file
    # Find what state names exist in the file
    state_enums = re.findall(rf'{p}_\w+UNINIT\w*', content)
    actual_uninit = None
    for s in state_enums:
        if s != f'{p}_STATE_UNINIT':  # Not the template name
            actual_uninit = s
            break
    
    if actual_uninit and f'{p}_STATE_UNINIT' in content:
        content = content.replace(f'{p}_STATE_UNINIT', actual_uninit)
        edits.append(f"state_name:{actual_uninit}")
    
    # Fix struct field name: CheckInit might reference .state but field might be .internalState
    # Find state struct field names
    field_match = re.search(rf'{name}_State\.(\w+)', content)
    if field_match:
        actual_field = field_match.group(1)
        # In CheckInit, check for wrong field references
        for pat in ['.state ==', '.state)']:
            if pat in content:
                pass  # The check above handles the case
    
    # Remove unused CheckPtr if it exists but is never called
    if f'{name}_Local_CheckPtr(' not in content:
        # Remove the function definition
        content = re.sub(
            r'\n/**\n \* @brief Validate a non-NULL.*?static boolean ' + name + r'_Local_CheckPtr\(.*?return TRUE;\n\}\n',
            '',
            content,
            flags=re.DOTALL
        )
        edits.append("removed_unused_CheckPtr")
    
    # Fix GetVersionInfo: replace hardcoded version with macros if needed
    # Also fix version check to use #if defined guard
    if f'{p}_SW_MAJOR_VERSION' not in content:
        # Check if version check exists but macros aren't defined
        pass
    
    if edits:
        with open(src, 'w') as f: f.write(content)
    return ", ".join(edits) if edits else "ok"

def main():
    total_ok = 0
    total_fixed = 0
    
    print("Fixing headers and sources for all 30 modules...\n")
    for name, p in MODULES:
        hdr_result = fix_header(name, p)
        src_result = fix_source(name, p)
        print(f"{name:20s} header: {hdr_result}")
        if src_result != "ok":
            print(f"{'':20s} source: {src_result}")
    
    print(f"\nDone.")

if __name__ == "__main__":
    main()
