#!/usr/bin/env python3
"""
Fix variableScope advisories.
Strategy: Move variable declarations closer to first usage by relocating
them to the innermost block that contains the first use.
"""
import re
import os

WORKSPACE = os.path.expanduser("~/.openclaw/workspace/yuleASR")

def read_file(path):
    full = os.path.join(WORKSPACE, path)
    with open(full) as f:
        return f.readlines()

def write_file(path, lines):
    full = os.path.join(WORKSPACE, path)
    with open(full, 'w') as f:
        f.writelines(lines)

def fix_variable_scope(lines, line_no, var_name):
    """Move variable declaration closer to its first use."""
    decl_idx = line_no - 1
    if decl_idx >= len(lines):
        return False
    
    decl_line = lines[decl_idx]
    stripped = decl_line.strip()
    indent = decl_line[:len(decl_line) - len(stripped)]
    
    # Find the first usage of var_name after declaration (not in the declaration itself)
    first_use_line = None
    first_use_block_end = None
    brace_depth = 0
    block_start = None
    
    for i in range(decl_idx + 1, len(lines)):
        line = lines[i]
        # Count braces to track block depth
        for ch in line:
            if ch == '{':
                if brace_depth == 0:
                    block_start = i
                brace_depth += 1
            elif ch == '}':
                brace_depth -= 1
                if brace_depth == 0 and block_start is not None:
                    first_use_block_end = i
                    block_start = None
        
        # Check if var_name is used on this line
        if var_name in line:
            # Skip if it's part of the declaration itself
            if '=' in line and var_name in line.split('=')[0] and '(' not in line and ')' not in line:
                continue
            # Skip simple comments about the variable
            if line.strip().startswith('//') or line.strip().startswith('*'):
                continue
            # Found first use
            first_use_line = i
            break
    
    if first_use_line is None:
        return False
    
    # Find the nearest enclosing block start before the first use
    # Search backwards from first_use_line to find the { that begins the block
    block_open_idx = None
    depth = 0
    for i in range(first_use_line, decl_idx, -1):
        for ch in reversed(lines[i]):
            if ch == '}':
                depth += 1
            elif ch == '{':
                if depth == 0:
                    block_open_idx = i
                    break
                depth -= 1
        if block_open_idx is not None:
            break
    
    if block_open_idx is None:
        return False
    
    # Only move if the block is within a reasonable range from the declaration
    if block_open_idx <= decl_idx + 1:
        return False  # Very close - not worth
    
    # Move the declaration: remove it from current position and add before the block
    # Extract variable type and name from declaration
    m = re.match(r'^(\s*)(?:static\s+|LOCAL_INLINE\s+)?((?:const\s+)?\w+(?:\s*\*+)?\s+)\w+\b', decl_line)
    if not m:
        # Try simpler pattern
        m2 = re.search(r'\b(\w+(?:\s*\*+\s*)+\w+|\w+\s+\w+)\s*;', decl_line)
        if not m2:
            return False
    
    # Get the indentation of the block
    block_indent = lines[block_open_idx][:len(lines[block_open_idx]) - len(lines[block_open_idx].lstrip())]
    
    # Add declaration inside the block, just after the opening brace
    indent_inside = block_indent + "    "
    
    # Check if this declaration is a multi-decl line (e.g., "int a, b;")
    if ',' in stripped.rstrip(';').strip():
        # Complex: multiple variables in one declaration
        # Safer to just leave it
        return False
    
    # Move single declaration
    lines.insert(block_open_idx + 1, f"{indent_inside}{stripped}\n")
    # Remove old declaration (now at decl_idx if no insert before it, or decl_idx+1 if insert before)
    if block_open_idx < decl_idx:
        old_idx = decl_idx + 1  # because we inserted before
    else:
        old_idx = decl_idx
    
    # But we need to be careful - if the original declaration was part of a group
    # Just comment it out instead
    indent_orig = lines[decl_idx][:len(lines[decl_idx]) - len(lines[decl_idx].lstrip())]
    
    # Check: only move if this variable hasn't already been moved
    stripped_line = lines[decl_idx].strip()
    if '/* MOVED:' in stripped_line or var_name not in stripped_line:
        return False
    
    # Actually, let me use a safer approach: just comment out the original declaration
    # and add a new one in the block
    lines[decl_idx] = f"{indent_orig}/* MOVED: {stripped_line} */\n"
    lines.insert(block_open_idx + 1, f"{indent_inside}{stripped_line}\n")
    
    return True

# Parse violations
with open('/tmp/advisory_v2.txt') as f:
    lines = f.readlines()

by_file = {}
for v in lines:
    m = re.match(r'^([^:]+):(\d+):\d+:\s+style:\s+(.*)\[([^\]]+)\]', v.strip())
    if m:
        f = m.group(1)
        ln = int(m.group(2))
        msg = m.group(3).strip()
        rule = m.group(4).strip()
        if rule == 'variableScope':
            var_match = re.search(r"variable '([^']+)'", msg)
            if var_match:
                vname = var_match.group(1)
                by_file.setdefault(f, []).append((ln, vname))

total_fixed = 0
for f, vios in sorted(by_file.items()):
    lines_content = read_file(f)
    if lines_content is None:
        continue
    changed = False
    for ln, vname in sorted(vios, key=lambda x: -x[0]):
        if fix_variable_scope(lines_content, ln, vname):
            total_fixed += 1
            changed = True
    if changed:
        write_file(f, lines_content)
        print(f"  Fixed in {f}")

print(f"\nTotal variableScope fixed: {total_fixed}")
