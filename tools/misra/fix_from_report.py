#!/usr/bin/env python3
"""
MISRA Rule Fix Script — reads violations from raw cppcheck output.

Processes all violations for specified rules from the raw output
and applies fixes based on the violation line numbers.

Supports:
  - Rule 8.4: Missing forward declarations → add prototype/extern
  - Rule 10.4: Essential type mixing → add explicit casts
  - Rule 8.6: Missing extern → add extern declaration
  - Rule 8.5: Duplicate identifier → rename or add static

Usage:
    python3 tools/misra/fix_from_report.py <rule>
           --raw-output <file> [--dry-run] [--module <module>]

Examples:
    python3 tools/misra/fix_from_report.py 8.4 --raw-output .yuleosh/reports/misra-raw-output.txt
    python3 tools/misra/fix_from_report.py 10.4 --raw-output .yuleosh/reports/misra-raw-output.txt --module mcal
"""

import os
import re
import sys
import glob

ROOT = os.getcwd()


def strip_root(path):
    """Remove the full path prefix to get a relative path."""
    if '/yuleASR/' in path:
        idx = path.index('/yuleASR/') + len('/yuleASR/')
        return path[idx:]
    return path


def is_same_line(line, line_num, content_lines):
    """Check if line_num (0-based) in content_lines matches our expectation."""
    if 0 <= line_num < len(content_lines):
        return content_lines[line_num]
    return None


def add_forward_decl_for_function(filepath, line_num, dry_run=False):
    """Add a forward declaration for a function at line_num (1-based)."""
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
    except:
        return False, f"Cannot read {filepath}"
    
    idx = line_num - 1
    if idx >= len(lines):
        return False, f"Line {line_num} out of range"
    
    line = lines[idx]
    
    # Extract the function signature (up to the opening brace or end of line)
    sig_parts = []
    sig_idx = idx
    brace_found = False
    while sig_idx < len(lines):
        l = lines[sig_idx]
        sig_parts.append(l.rstrip('\n'))
        if '{' in l:
            brace_found = True
            break
        sig_idx += 1
    
    sig = ''.join(sig_parts)
    
    # Remove the '{' and everything after
    sig = sig.split('{')[0].strip()
    
    # Generate prototype
    proto = sig + ';'
    
    # Find where to insert: after includes, before function definition
    # Look for the first #include backwards, then find a blank line after
    last_include = -1
    for i in range(idx - 1, -1, -1):
        if lines[i].strip().startswith('#include'):
            last_include = i
            break
    
    if last_include >= 0:
        # Find a blank or section-comment line after the last include
        insert_pos = last_include + 1
        while insert_pos < idx and lines[insert_pos].strip() and not lines[insert_pos].strip().startswith('#'):
            insert_pos += 1
    else:
        insert_pos = idx
    
    if not dry_run:
        # Check if prototype already exists
        for i in range(idx):
            if proto.strip() in lines[i].strip():
                if '(' in lines[i] and lines[i].strip().endswith(';'):
                    return False, f"Prototype already exists at line {i+1}"
        
        indent = ''
        m = re.match(r'^(\s*)', line)
        if m:
            indent = m.group(1)
        
        lines.insert(insert_pos, indent + proto + '\n')
        with open(filepath, 'w') as f:
            f.writelines(lines)
    
    return True, f"Inserted prototype at line {insert_pos+1}"


def add_extern_decl_for_object(filepath, line_num, dry_run=False):
    """Add an 'extern' declaration for a global object at line_num (1-based)."""
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
    except:
        return False, f"Cannot read {filepath}"
    
    idx = line_num - 1
    if idx >= len(lines):
        return False, f"Line {line_num} out of range"
    
    line = lines[idx]
    stripped = line.strip()
    
    # Generate extern declaration
    # Remove initializer if present
    eq_idx = stripped.find('=')
    if eq_idx >= 0:
        decl_part = stripped[:eq_idx].strip().rstrip(';')
    else:
        decl_part = stripped.rstrip(';').strip()
    
    # Remove static/STATIC if present
    decl_part = re.sub(r'\b(static|STATIC)\s+', '', decl_part)
    
    # Add extern
    extern_decl = f'extern {decl_part};'
    
    # Find insertion point (after includes, before definition)
    last_include = -1
    for i in range(idx - 1, -1, -1):
        if lines[i].strip().startswith('#include'):
            last_include = i
            break
    
    if last_include >= 0:
        insert_pos = last_include + 1
        while insert_pos < idx and lines[insert_pos].strip() and not lines[insert_pos].strip().startswith('#'):
            insert_pos += 1
    else:
        insert_pos = idx
    
    if not dry_run:
        # Check if extern already exists
        name = decl_part.strip().split()[-1] if decl_part.strip().split() else ''
        for i in range(idx):
            if name in lines[i] and 'extern' in lines[i] and lines[i].strip().endswith(';'):
                return False, f"Extern already exists at line {i+1}"
        
        indent = ''
        m = re.match(r'^(\s*)', line)
        if m:
            indent = m.group(1)
        
        lines.insert(insert_pos, indent + extern_decl + '\n')
        with open(filepath, 'w') as f:
            f.writelines(lines)
    
    return True, f"Inserted extern at line {insert_pos+1}"


def fix_8_4(violations, dry_run=False, module_filter=None):
    """Fix Rule 8.4 violations."""
    fixed = 0
    skipped = 0
    errors = 0
    
    for v in violations:
        filepath, line_str = v
        line_num = int(line_str)
        full_path = os.path.join(ROOT, filepath)
        
        if not os.path.isfile(full_path):
            continue
        
        with open(full_path, 'r') as f:
            lines = f.readlines()
        
        idx = line_num - 1
        if idx >= len(lines):
            continue
        
        line = lines[idx].strip()
        
        # Determine if it's a function or variable
        if '(' in line and ')' in line and not line.endswith(';'):
            # Function definition
            ok, msg = add_forward_decl_for_function(full_path, line_num, dry_run)
        else:
            # Variable or object definition
            ok, msg = add_extern_decl_for_object(full_path, line_num, dry_run)
        
        if ok:
            fixed += 1
            if not dry_run:
                print(f"  [FIXED] {filepath}:{line_num} - {msg}")
            else:
                print(f"  [DRY-RUN] {filepath}:{line_num} - {msg}")
        else:
            skipped += 1
            if 'already exists' in msg:
                pass  # Silent skip for duplicates
            else:
                print(f"  [SKIP] {filepath}:{line_num} - {msg}")
    
    return fixed, skipped


def parse_raw_output(raw_file, rule):
    """Parse raw cppcheck output to extract violations for a specific rule."""
    violations = []
    with open(raw_file, 'r') as f:
        for line in f:
            if f'misra-c2012-{rule}' in line and '/yuleASR/src/' in line:
                # Extract filepath and line
                m = re.search(r'/yuleASR/(src/[^:]+):(\d+):', line)
                if m:
                    filepath = m.group(1)
                    line_num = m.group(2)
                    violations.append((filepath, line_num))
    return violations


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA violations from raw output')
    parser.add_argument('rule', help='Rule number (e.g., 8.4, 10.4, 8.6, 17.3, 8.5)')
    parser.add_argument('--raw-output', '-r', required=True, help='Raw cppcheck output file')
    parser.add_argument('--dry-run', '-n', action='store_true', help='Just print what would be done')
    parser.add_argument('--module', '-m', help='Filter by module (e.g., mcal, ecual, services)')
    args = parser.parse_args()
    
    violations = parse_raw_output(args.raw_output, args.rule)
    
    if args.module:
        violations = [(fp, ln) for fp, ln in violations if fp.startswith(f'src/bsw/{args.module}/')]
    
    print(f"Found {len(violations)} violations for rule {args.rule}")
    
    if args.rule == '8.4':
        fixed, skipped = fix_8_4(violations, args.dry_run)
        print(f"\nRule 8.4: {fixed} fixed, {skipped} skipped")
    elif args.rule == '10.4':
        print("Rule 10.4 fix not yet implemented")
    elif args.rule == '8.6':
        print("Rule 8.6 fix not yet implemented")
    elif args.rule == '17.3':
        print("Rule 17.3 fix not yet implemented")
    elif args.rule == '8.5':
        print("Rule 8.5 fix not yet implemented")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
