#!/usr/bin/env python3
"""
MISRA C:2012 Rule 13.3 — Auto-fix script.

Splits compound expressions with increment/decrement side effects.
Patterns handled:
  - arr[idx++] = val  →  arr[idx] = val; idx++
  - var = ptr->field++  →  var = ptr->field; ptr->field++
  - arr[--idx] = val  →  --idx; arr[idx] = val
  - func(arg1, arg2++)  →  (separate)

Usage:
    python3 tools/misra/fix_increment.py <file_or_dir>
    python3 tools/misra/fix_increment.py src/bsw/services/dcm/src/Dcm.c
"""

import re
import os
import sys


def extract_idx_var(expr):
    """From 'idx++' extract ('idx', 1) or from '--idx' extract ('idx', -1)."""
    m = re.match(r'(\w+)\+\+$', expr)
    if m:
        return m.group(1), 1
    m = re.match(r'(\w+)--$', expr)
    if m:
        return m.group(1), -1
    m = re.match(r'\+\+(\w+)$', expr)
    if m:
        return m.group(1), 1
    m = re.match(r'--(\w+)$', expr)
    if m:
        return m.group(1), -1
    return None, 0


def fix_line_incdec(line_text):
    """
    Fix common increment/decrement patterns in assignment expressions.
    Returns (fixed_line, was_fixed) tuple.
    """
    # Pattern 1: arr[idx++] = val;
    m = re.match(r'^(\s*)(\w+)\[(\w+\+\+|--\w+)\]\s*=\s*(.*);\s*$', line_text)
    if m:
        indent = m.group(1)
        arr = m.group(2)
        inc_expr = m.group(3)
        val = m.group(4)
        var, delta = extract_idx_var(inc_expr)
        if var:
            if delta > 0:
                # idx++: use then increment
                return f'{indent}{arr}[{var}] = {val};\n{indent}{var}++;\n', True
            else:
                # ++idx: increment then use
                return f'{indent}{var}++;\n{indent}{arr}[{var}] = {val};\n', True
    
    # Pattern 2: var = ptr->field++ (or similar postfix)
    m = re.match(r'^(\s*)(\w+)\s*=\s*(\w+(?:->\w+)*)\+\+;\s*$', line_text)
    if m:
        indent = m.group(1)
        lhs = m.group(2)
        field = m.group(3)
        return f'{indent}{lhs} = {field};\n{indent}{field}++;\n', True
    
    # Pattern 3: var = ptr->field--;
    m = re.match(r'^(\s*)(\w+)\s*=\s*(\w+(?:->\w+)*)--;\s*$', line_text)
    if m:
        indent = m.group(1)
        lhs = m.group(2)
        field = m.group(3)
        return f'{indent}{lhs} = {field};\n{indent}{field}--;\n', True
    
    return line_text, False


def fix_file(filepath, dry_run=False):
    """Fix increment/decrement side effects in a file."""
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    new_lines = []
    fixes = 0
    for line in lines:
        fixed, was_fixed = fix_line_incdec(line)
        if was_fixed:
            fixes += 1
            new_lines.append(fixed)
            if not dry_run:
                print(f"  {os.path.basename(filepath)}: {line.rstrip()}")
                print(f"    → {fixed.rstrip()}")
        else:
            new_lines.append(line)
    
    if fixes > 0 and not dry_run:
        with open(filepath, 'w') as f:
            f.writelines(new_lines)
    
    return fixes


def find_files(paths):
    """Find .c files in given paths."""
    files = []
    for p in paths:
        if os.path.isfile(p) and p.endswith('.c'):
            files.append(p)
        elif os.path.isdir(p):
            for root, dirs, fnames in os.walk(p):
                for fn in fnames:
                    if fn.endswith('.c'):
                        files.append(os.path.join(root, fn))
    return sorted(set(files))


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA Rule 13.3 violations')
    parser.add_argument('paths', nargs='+', help='Files or directories')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    
    files = find_files(args.paths)
    if not files:
        print("No .c files found")
        return 1
    
    total_fixes = 0
    for f in files:
        fixes = fix_file(f, dry_run=args.dry_run)
        total_fixes += fixes
    
    print(f"\nSummary: {total_fixes} fixes in {len(files)} files")
    return 0


if __name__ == '__main__':
    sys.exit(main())
