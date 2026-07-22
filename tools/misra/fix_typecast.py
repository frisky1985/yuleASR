#!/usr/bin/env python3
"""
MISRA C:2012 Rule 10.4 — Auto-fix script.

Fixes "essential type mixing" violations by adding explicit casts where
expressions mix different essential type categories (boolean, character,
enum, signed, unsigned, float, complex).

This script reads violation locations from cppcheck raw output and adds
explicit casts at flagged positions.

For each violation:
1. Read the line and the expression at the column position
2. Determine what types are being mixed
3. Add an explicit cast to the dominant type

Usage:
    python3 tools/misra/fix_typecast.py <raw_output_file>
    python3 tools/misra/fix_typecast.py .yuleosh/reports/misra-raw-output.txt --dry-run
    python3 tools/misra/fix_typecast.py .yuleosh/reports/misra-raw-output.txt --module mcal/lin
"""

import os
import re
import sys

# Essential type categories (MISRA C:2012 §8.10)
ESSENTIAL_TYPES = {
    'boolean': {'boolean', 'bool'},
    'signed': {'int8', 'int16', 'int32', 'int64', 'signed char', 'signed int', 'signed short', 'signed long'},
    'unsigned': {'uint8', 'uint16', 'uint32', 'uint64', 'unsigned char', 'unsigned int', 'unsigned short', 'unsigned long'},
    'float': {'float32', 'float64', 'float', 'double'},
    'character': {'char'},
    'enum': {},  # any enum type
    'complex': {},  # complex floating types
}

# Mapping from less safe to more safe within the same category
# Adding casts for common patterns
COMMON_PATTERNS = [
    # (signed_type) << shift_amount  → need cast on shift result
    # uint16 | uint32  → need cast to wider type
]


def parse_violations(raw_file, rule='10.4'):
    """Parse raw cppcheck output for rule violations grouped by file."""
    file_lines = {}
    with open(raw_file, 'r') as f:
        for line in f:
            if f'misra-c2012-{rule}' in line and '/yuleASR/src/' in line:
                m = re.search(r'/yuleASR/(src/[^:]+):(\d+):(\d+):', line)
                if m:
                    filepath = m.group(1)
                    line_num = int(m.group(2))
                    col_num = int(m.group(3))
                    if filepath not in file_lines:
                        file_lines[filepath] = set()
                    file_lines[filepath].add((line_num, col_num))
    
    result = {}
    for fp, entries in file_lines.items():
        result[fp] = sorted(entries)
    return result


def is_lin_reg_assign(line_orig, col):
    """Check if this is a LIN register assignment (e.g., u32RegVal |= 0x80u)."""
    line = line_orig.strip()
    # LIN register assignments
    if 'Reg' in line or 'reg' in line or 'REG' in line:
        return True
    # LIN PID computation
    if 'Pid' in line or 'PID' in line or '_Pid' in line:
        return True
    return False


def fix_lin_register_ops(lines, violations, dry_run=False):
    """
    Fix LIN module 10.4 violations by adding explicit casts for register operations.
    These are HW register mixing patterns that require explicit casts.
    """
    fixes_applied = 0
    for filepath, (line_num, col) in violations:
        # This function is called per-violation
        pass
    return fixes_applied


def fix_file_operations(filepath, violations_list, dry_run=False):
    """
    Fix 10.4 violations in a single file.
    The main pattern is essential type mixing in arithmetic/boolean expressions.
    """
    full_path = os.path.join(ROOT, filepath)
    if not os.path.isfile(full_path):
        return 0, 0
    
    with open(full_path, 'r') as f:
        content = f.read()
    
    lines = content.split('\n')
    lines_nl = [l + '\n' for l in lines]
    lines_nl[-1] = lines[-1]
    
    fixes = 0
    skipped = 0
    
    for line_num, col in sorted(violations_list, reverse=True):
        idx = line_num - 1
        if idx >= len(lines_nl):
            skipped += 1
            continue
        
        line = lines_nl[idx]
        stripped = line.strip()
        
        # Skip if already has cast
        if '(uint' in stripped or '(int' in stripped or '(float' in stripped or '(double' in stripped:
            if 'uint8' in stripped or 'uint16' in stripped or 'uint32' in stripped or 'uint64' in stripped:
                skipped += 1
                continue
        
        # Handle different patterns
        fixed_line = None
        
        # Pattern 1: BIT assignment like `RegVal |= BIT0` or `RegVal = val | BIT3`
        m = re.match(r'^(\s*)((?:\w+\s*)*\w+)\s*(\|=|&=|\^=|=)\s*(.*)', stripped)
        if m:
            indent = m.group(1)
            lhs = m.group(2)
            op = m.group(3)
            rhs = m.group(4)
            
            # For bit operations on registers, add cast to match LHS type
            if 'Reg' in lhs or 'reg' in lhs or 'REG' in lhs:
                # Determine the type from context
                lhs_type = 'uint32'  # Default for registers
                if 'uint16' in line or 'UInt16' in line or 'uint16_t' in line:
                    lhs_type = 'uint16'
                elif 'uint8' in line or 'UInt8' in line or 'uint8_t' in line:
                    lhs_type = 'uint8'
                
                if '|' in rhs or '&' in rhs or '^' in rhs:
                    # Complex RHS expression needs wrapping
                    if op == '=':
                        fixed_line = f'{indent}{lhs} = ({lhs_type})({rhs});\n'
                    else:
                        fixed_line = f'{indent}{lhs} {op} ({lhs_type})({rhs});\n'
                elif rhs.startswith('~'):
                    fixed_line = f'{indent}{lhs} {op} ({lhs_type})({rhs});\n'
        
        # Pattern 2: Shift operations without cast
        if fixed_line is None:
            # Check for `<<` or `>>` in the RHS of an assignment
            m = re.search(r'=\s*(.*)(<<\s*\d+)(.*)', stripped)
            if m:
                prefix = m.group(1)
                shift = m.group(2)
                suffix = m.group(3)
                
                # Only fix if the result is used in a wider context
                if prefix.strip() or suffix.strip():
                    indent = re.match(r'^(\s*)', stripped).group(1)
                    eq_pos = stripped.index('=')
                    lhs = stripped[:eq_pos].strip()
                    rhs = stripped[eq_pos+1:].strip()
                    
                    # Determine LHS type
                    lhs_type = 'uint32'
                    for t in ['uint64', 'uint32', 'uint16', 'uint8']:
                        if t in lhs or t in line:
                            lhs_type = t
                            break
                    
                    fixed_line = f'{indent}{lhs} = ({lhs_type})({rhs});\n'
        
        if fixed_line:
            if not dry_run:
                lines_nl[idx] = fixed_line
            fixes += 1
            action = "DRY-RUN:" if dry_run else "FIXED:"
            print(f"  [{action}] {os.path.basename(filepath)}:{line_num} {stripped[:50]}...")
        else:
            skipped += 1
    
    if fixes > 0 and not dry_run:
        content_new = ''.join(lines_nl)
        with open(full_path, 'w') as f:
            f.write(content_new)
    
    return fixes, skipped


ROOT = os.getcwd()


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA Rule 10.4 violations')
    parser.add_argument('raw_output', help='Raw cppcheck output file')
    parser.add_argument('--dry-run', '-n', action='store_true')
    parser.add_argument('--module', '-m', help='Filter by module')
    args = parser.parse_args()
    
    violations = parse_violations(args.raw_output, '10.4')
    
    if args.module:
        prefix = f'src/bsw/{args.module}/'
        violations = {fp: entries for fp, entries in violations.items() if fp.startswith(prefix)}
    
    total_violations = sum(len(v) for v in violations.values())
    print(f"Found {total_violations} violations across {len(violations)} files")
    
    total_fixes = 0
    for filepath, entries in sorted(violations.items()):
        fixes, skipped = fix_file_operations(filepath, entries, args.dry_run)
        total_fixes += fixes
    
    print(f"\nRule 10.4: {total_fixes} fixes applied")
    return 0


if __name__ == '__main__':
    sys.exit(main())
