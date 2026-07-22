#!/usr/bin/env python3
"""
MISRA C:2012 Rule 15.7 — Auto-fix script.

Detects if-else-if chains that are missing a final else clause and adds:
    } else {
        /* No action required */
    }

Usage:
    python3 tools/misra/fix_else_clause.py <file_or_dir>
    python3 tools/misra/fix_else_clause.py .yuleosh/fix-tasks/misra-misra-c2023-15.7.md
    python3 tools/misra/fix_else_clause.py src/bsw/mcal/lin/src/
"""

import re
import os
import sys


def find_if_else_if_chains(lines):
    """
    Find if-else-if chains that are missing a final else clause.
    Returns list of (chain_start_line, chain_end_line) tuples.
    """
    chains = []
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Look for 'if (...)' start
        if re.match(r'^(--\s*)?\s*\}\s*else\s+if\s*\(', line) or re.match(r'^if\s*\(', line):
            # Found an if or else-if - check if it starts a chain
            # Count braces to find the end
            brace_count = 0
            start_idx = i
            chain_type = 'if' if line.lstrip().startswith('if') else 'else_if'
            
            if chain_type == 'if':
                # Skip to find the first 'else if' or 'else'
                j = i
                found_else_if = False
                while j < len(lines):
                    l = lines[j].strip()
                    if re.match(r'^--\s*\}?\s*else\s+if\s*\(', l):
                        found_else_if = True
                    if re.match(r'^--\s*\}?\s*else\s*$', l) or l.startswith('} else {'):
                        # Found else - but not else-if. Check if this is the end.
                        # Ensure there's no subsequent else if
                        break
                    if found_else_if and re.match(r'^--\s*\}?\s*else\s*$', l):
                        break
                    j += 1
                
                if found_else_if:
                    chains.append((i, j))
            i += 1
        else:
            i += 1
    
    return chains


def add_else_clause(lines, chain_start, chain_end):
    """
    Find the closing brace after the last else-if in the chain and add an else clause
    before it.
    """
    # Walk from chain_end to find the closing brace
    brace_depth = 0
    insert_pos = -1
    
    for i in range(chain_end, len(lines)):
        stripped = lines[i].strip()
        
        # Count braces
        for ch in stripped:
            if ch == '{':
                brace_depth += 1
            elif ch == '}':
                brace_depth -= 1
        
        # Look for the position where we need to insert
        if brace_depth <= 0 and '}' in stripped:
            # This is the closing of the if-else-if chain
            insert_pos = i
            break
    
    if insert_pos < 0:
        return False
    
    # Check if there's already an else clause
    for k in range(chain_end, insert_pos):
        if re.match(r'^\s*\}\s*else\s*[^{]*$', lines[k].strip()):
            return False  # Already has else clause
    
    # Insert the else clause before the closing
    indent = '    '
    # Try to detect indentation from the line before
    if insert_pos > 0:
        prev_line = lines[insert_pos - 1]
        if prev_line.strip().endswith('}'):
            # Match indentation of the closing brace
            indent_match = re.match(r'^(\s*)', prev_line)
            if indent_match:
                indent = indent_match.group(1)
    
    else_block = [
        f'{indent}}} else {{\n',
        f'{indent}    /* No action required */\n',
        f'{indent}}}\n',
    ]
    
    lines[insert_pos:insert_pos] = else_block
    
    return True


def fix_file_violations(filepath, violations, dry_run=False):
    """Fix violations in a single file by adding else clauses."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    lines = content.split('\n')
    # Add newline back to each line
    lines_with_nl = [l + '\n' for l in lines]
    lines_with_nl[-1] = lines[-1]  # Last line shouldn't have extra newline
    
    fixes = 0
    # Process each violation line
    for v_line in sorted(violations, reverse=True):
        if v_line > len(lines):
            continue
        
        # Read around the violation line to understand the context
        start = max(0, v_line - 10)
        end = min(len(lines), v_line + 50)
        
        # Try to find the if-else-if chain and add else
        if add_else_clause(lines_with_nl, v_line - 1, v_line - 1):
            fixes += 1
    
    if fixes > 0 and not dry_run:
        # Write back
        content_new = ''.join(lines_with_nl)
        with open(filepath, 'w') as f:
            f.write(content_new)
        print(f"  {filepath}: {fixes} else clauses added")
    
    return fixes


def fix_file(filepath, dry_run=False):
    """Smart fix: scan file for if-else-if chains without final else."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    lines = content.split('\n')
    lines_with_nl = [l + '\n' for l in lines]
    lines_with_nl[-1] = lines[-1]
    
    # Find if-else-if chains
    chains = find_if_else_if_chains(lines_with_nl)
    
    fixes = 0
    for start, end in reversed(chains):
        if add_else_clause(lines_with_nl, start, end):
            fixes += 1
    
    if fixes > 0:
        if not dry_run:
            content_new = ''.join(lines_with_nl)
            with open(filepath, 'w') as f:
                f.write(content_new)
        print(f"  {filepath}: {fixes} else clauses added" + (" (dry run)" if dry_run else ""))
    
    return fixes


def find_files(paths):
    """Find all .c and .h files in given paths."""
    files = []
    for p in paths:
        if os.path.isfile(p):
            if p.endswith('.c') or p.endswith('.h'):
                files.append(p)
            elif p.endswith('.md'):
                # Parse fix-task file
                files.extend(parse_fix_task(p))
        elif os.path.isdir(p):
            for root, dirs, fnames in os.walk(p):
                for fn in fnames:
                    if fn.endswith('.c') or fn.endswith('.h'):
                        files.append(os.path.join(root, fn))
    return sorted(set(files))


def parse_fix_task(task_path):
    """Parse a fix-task markdown file to get file paths from violations."""
    files = set()
    with open(task_path) as f:
        in_table = False
        for line in f:
            if '| # | File | Line | Col | Message |' in line:
                in_table = True
                continue
            if in_table and line.strip().startswith('|---'):
                continue
            if not in_table:
                continue
            if not line.strip():
                in_table = False
                continue
            parts = line.split('|')
            if len(parts) >= 4:
                fp = parts[2].strip().strip('`').strip()
                if '/Users/stefan/.openclaw/workspace/yuleASR/' in fp:
                    rel = fp.replace('/Users/stefan/.openclaw/workspace/yuleASR/', '')
                    if os.path.isfile(rel):
                        files.add(rel)
    return list(files)


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Fix MISRA Rule 15.7 violations')
    parser.add_argument('paths', nargs='+', help='Files, directories, or fix-task files')
    parser.add_argument('--dry-run', action='store_true', help='Just print changes')
    args = parser.parse_args()
    
    files = find_files(args.paths)
    if not files:
        print("No files found")
        return 1
    
    total_fixes = 0
    for f in files:
        fixes = fix_file(f, dry_run=args.dry_run)
        if fixes > 0:
            total_fixes += fixes
    
    print(f"\nSummary: {total_fixes} else clauses added across {len(files)} files")
    return 0


if __name__ == '__main__':
    sys.exit(main())
