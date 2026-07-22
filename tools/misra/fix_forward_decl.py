#!/usr/bin/env python3
"""
MISRA C:2012 Rule 8.4 — Batch auto-fix script using cppcheck raw output.

Reads violation locations from cppcheck raw output and adds forward
declarations (prototypes for functions, extern for variables) before
each definition that lacks a visible declaration.

All fixes for the same file are aggregated before writing.

Usage:
    python3 tools/misra/fix_forward_decl.py <raw_output_file>
    python3 tools/misra/fix_forward_decl.py .yuleosh/reports/misra-raw-output.txt
    python3 tools/misra/fix_forward_decl.py .yuleosh/reports/misra-raw-output.txt --dry-run
    python3 tools/misra/fix_forward_decl.py .yuleosh/reports/misra-raw-output.txt --module mcal
"""

import os
import re
import sys

ROOT = os.getcwd()


def parse_violations(raw_file, rule='8.4'):
    """Parse raw cppcheck output for rule violations grouped by file."""
    file_lines = {}
    with open(raw_file, 'r') as f:
        for line in f:
            if f'misra-c2012-{rule}' in line and '/yuleASR/src/' in line:
                m = re.search(r'/yuleASR/(src/[^:]+):(\d+):', line)
                if m:
                    filepath = m.group(1)
                    line_num = int(m.group(2))
                    if filepath not in file_lines:
                        file_lines[filepath] = set()
                    file_lines[filepath].add(line_num)
    
    # Sort line numbers for each file
    result = {}
    for fp, lines in file_lines.items():
        result[fp] = sorted(lines)
    
    return result


def get_last_include_pos(lines):
    """Find the last #include line index."""
    last = -1
    for i, l in enumerate(lines):
        if l.strip().startswith('#include'):
            last = i
    return last


def get_insertion_anchor(lines, def_idx):
    """Find the best anchor line for inserting BEFORE a definition.
    Returns the index where the declaration should be inserted.
    Tries to place it just after the last include, before section comments.
    """
    last_include = get_last_include_pos(lines)
    
    if last_include >= 0:
        # Insert after includes. Look for a blank line or section comment
        insert = last_include + 1
        # Skip blank lines and section comments
        while insert < min(def_idx, last_include + 10):
            stripped = lines[insert].strip()
            # Skip blank lines, comment blocks, section comment lines
            is_comment = (stripped.startswith('/*') or stripped.startswith('*') or
                          stripped.startswith('**') or '*/' in stripped or
                          stripped.startswith('#ifndef') or stripped.startswith('#define') or
                          stripped.startswith('#endif') or stripped.startswith('#if') or
                          stripped.startswith('#else') or stripped.startswith('#elif') or
                          stripped.startswith('#undef') or stripped.startswith('#pragma') or
                          stripped.startswith('#error') or stripped.startswith('#warning') or
                          stripped.startswith('/**'))
            if stripped == '' or is_comment:
                insert += 1
            else:
                break
        return insert
    
    return def_idx


def is_function_def(lines, idx):
    """Check if line[idx] starts a function definition."""
    line = lines[idx].strip()
    if not line:
        return False
    
    # Must have parens
    if '(' not in line:
        return False
    
    # Must NOT end with ; (would be declaration)
    if line.endswith(';'):
        return False
    
    # Check for opening brace within next 20 lines
    for i in range(idx, min(idx + 20, len(lines))):
        l = lines[i].strip()
        l_clean = re.sub(r'/\*.*?\*/', '', l)
        l_clean = re.sub(r'//.*$', '', l_clean)
        if '{' in l_clean:
            return True
    
    return False


def get_function_prototype(lines, idx):
    """Extract function signature from definition at idx and create prototype."""
    # Gather signature lines until '{'
    sig_lines = []
    for i in range(idx, min(idx + 20, len(lines))):
        l = lines[i].rstrip('\n')
        sig_lines.append(l)
        if '{' in l:
            break
    
    sig = ''.join(sig_lines)
    sig = sig.split('{')[0].strip()
    
    # Remove trailing whitespace and add ;
    if not sig.endswith(';'):
        sig += ';'
    
    return sig


def get_variable_extern(lines, idx):
    """Extract variable declaration and create extern version."""
    line = lines[idx].strip()
    line = re.sub(r'/\*.*?\*/', '', line)
    line = line.split('//')[0].strip()
    
    # Remove initializer if present
    eq_idx = line.find('=')
    if eq_idx >= 0:
        before_eq = line[:eq_idx].strip().rstrip(';')
    else:
        before_eq = line.rstrip(';').strip()
    
    # Remove static/STATIC if present
    before_eq = re.sub(r'\b(static|STATIC)\s+', '', before_eq)
    
    return f'extern {before_eq};'


def already_has_decl(lines, name, upto_idx):
    """Check if a declaration for 'name' exists in lines up to upto_idx."""
    for i in range(upto_idx):
        l = lines[i].strip()
        if name in l:
            if l.endswith(';') and (l.startswith('extern') or '(' in l):
                return True
    return False


def fix_file(filepath, violations, dry_run=False):
    """Fix all 8.4 violations in one file, batched."""
    full_path = os.path.join(ROOT, filepath)
    if not os.path.isfile(full_path):
        return 0
    
    with open(full_path, 'r') as f:
        lines = f.readlines()
    
    # Collect all fixes to apply
    fixes = []  # list of (insert_after_idx, prototype_line)
    
    for line_num in violations:
        idx = line_num - 1  # 0-based
        if idx >= len(lines):
            continue
        
        line = lines[idx].strip()
        name = None
        
        # Determine violation type
        is_func = is_function_def(lines, idx)
        
        if is_func:
            proto = get_function_prototype(lines, idx)
            # Extract function name
            m = re.search(r'(\w+)\s*\(', proto.replace('extern', ''))
            if m:
                name = m.group(1)
            fix_line = proto
        else:
            # Variable/object definition
            fix_line = get_variable_extern(lines, idx)
            # Extract variable name
            m = re.search(r'extern\s+.*?\s+(\w+)\s*;', fix_line)
            if m:
                name = m.group(1)
        
        if not name:
            continue
        
        # Skip if already has a declaration
        if already_has_decl(lines, name, idx):
            continue
        
        insert_at = get_insertion_anchor(lines, idx)
        
        # Check if this exact same fix_line already exists
        already_there = False
        for i in range(min(insert_at + 5, len(lines))):
            if lines[i].strip() == fix_line or fix_line in lines[i]:
                already_there = True
                break
        
        if already_there:
            continue
        
        indent = ''
        m = re.match(r'^(\s*)', lines[idx] if idx < len(lines) else '')
        if m:
            indent = m.group(1)
        
        fixes.append((insert_at, indent + fix_line + '\n', name))
    
    if not fixes:
        return 0
    
    # Sort fixes by insertion position (descending to not mess up indices)
    fixes.sort(key=lambda x: x[0], reverse=True)
    
    total = 0
    for insert_at, proto_line, name in fixes:
        # Check if it's still needed (another fix may have already added it)
        if already_has_decl(lines, name, len(lines)):
            continue
        
        if not dry_run:
            lines.insert(insert_at, proto_line)
        total += 1
        action = "DRY-RUN:" if dry_run else "FIXED:"
        print(f"  [{action}] {os.path.basename(filepath)} + {name} at line {insert_at+1}")
    
    if not dry_run and total > 0:
        with open(full_path, 'w') as f:
            f.writelines(lines)
    
    return total


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA Rule 8.4 violations')
    parser.add_argument('raw_output', help='Raw cppcheck output file (.yuleosh/reports/misra-raw-output.txt)')
    parser.add_argument('--dry-run', '-n', action='store_true', help='Just print changes')
    parser.add_argument('--module', '-m', help='Filter by module prefix (e.g., mcal, ecual, services)')
    args = parser.parse_args()
    
    violations = parse_violations(args.raw_output, '8.4')
    
    if args.module:
        prefix = f'src/bsw/{args.module}/'
        violations = {fp: ln for fp, ln in violations.items() if fp.startswith(prefix)}
    
    total_files = len(violations)
    total_violations = sum(len(v) for v in violations.values())
    print(f"Found {total_violations} violations in {total_files} files")
    
    total_fixes = 0
    fixed_files = 0
    for filepath, line_nums in sorted(violations.items()):
        fixes = fix_file(filepath, line_nums, args.dry_run)
        if fixes > 0:
            total_fixes += fixes
            fixed_files += 1
    
    print(f"\nRule 8.4: {total_fixes} declarations added in {fixed_files} files")
    return 0


if __name__ == '__main__':
    sys.exit(main())
