#!/usr/bin/env python3
"""
Batch fix for cppcheck Advisory-level (style) violations.
"""
import subprocess
import re
import os
import sys

SRC = "src"
PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")

os.chdir(PROJECT)

def run_cppcheck():
    """Run cppcheck and return the full output."""
    result = subprocess.run(
        ["cppcheck", "--enable=warning,style", "--std=c11",
         f"--suppress=*:{SRC}/../.cppcheck_suppressions", SRC],
        capture_output=True, text=True, timeout=300
    )
    return result.stdout + result.stderr

def parse_issues(output):
    """Parse cppcheck output into structured issues."""
    issues = []
    # Expected format:
    # path/file.c:line:col: style: Message [rule]
    pattern = re.compile(
        r'^src/(.+):(\d+):(\d+):\s*style:\s*(.+?)\s*\[(\w+)\]$',
        re.MULTILINE
    )
    for match in pattern.finditer(output):
        relpath, line_str, col_str, msg, rule = match.groups()
        issues.append({
            'file': f"src/{relpath}",
            'line': int(line_str),
            'col': int(col_str),
            'message': msg.strip(),
            'rule': rule
        })
    return issues

def fix_const_variable_pointer(issues):
    """Add const to local pointer variables."""
    fixed = 0
    const_issues = [i for i in issues if i['rule'] == 'constVariablePointer']
    
    for issue in const_issues:
        fpath = issue['file']
        line = issue['line']
        
        if not os.path.exists(fpath):
            print(f"  SKIP (not found): {fpath}")
            continue
            
        with open(fpath, 'r') as f:
            lines = f.readlines()
        
        idx = line - 1
        if idx >= len(lines):
            continue
        
        old = lines[idx]
        # Extract the type name from the message
        # "Variable 'xxx' can be declared as pointer to const"
        msg_match = re.search(r"Variable '(.+?)'", issue['message'])
        if not msg_match:
            continue
        varname = msg_match.group(1)
        
        # Find pointer type: something like "XxxType* varname" or "XxxType *varname"
        # Match: <type> *<varname> or <type>* <varname>
        ptype_match = re.search(
            rf'(\w[\w_]*)\s*\*+\s*{re.escape(varname)}\b', old
        )
        if not ptype_match:
            continue
        
        ptr_type = ptype_match.group(1)
        
        # Check if already const
        if 'const' in old[:old.index(ptr_type)]:
            continue
        
        # Check if variable is modified through pointer
        # Count assignments through pointer in this function
        if is_ptr_written(fpath, lines, idx, varname):
            print(f"  SKIP (writes through ptr): {fpath}:{line} {varname}")
            continue
        
        # Add const before the pointer type
        # Pattern: "Type* varname" -> "const Type* varname"
        new = old.replace(f"{ptr_type}*", f"const {ptr_type}*", 1)
        if new == old:
            new = old.replace(f"{ptr_type} *", f"const {ptr_type} *", 1)
        if new == old:
            continue
        
        lines[idx] = new
        with open(fpath, 'w') as f:
            f.writelines(lines)
        print(f"  FIXED: {fpath}:{line} -> {new.strip()}")
        fixed += 1
    
    return fixed

def is_ptr_written(fpath, lines, start_idx, varname):
    """Check if a pointer variable is used to write through (assignment)."""
    # Walk forward to find the end of the current function or block
    brace_depth = 0
    found_open = False
    
    for i in range(start_idx, min(start_idx + 60, len(lines))):
        line = lines[i]
        # Count braces to find function scope
        if i == start_idx:
            # Find opening brace after variable declaration
            continue
        
        for ch in line:
            if ch == '{':
                brace_depth += 1
                found_open = True
            elif ch == '}':
                brace_depth -= 1
                if found_open and brace_depth < 0:
                    return False  # End of function
                if brace_depth < 0:
                    brace_depth = 0
        
        # Check if varname is assigned to
        # Matches patterns like "varname->something = ..." 
        # but NOT "varname = &something"
        assign_match = re.search(
            rf'{re.escape(varname)}\s*->\s*\w+\s*=', line
        )
        if assign_match:
            return True
    
    return False

def fix_const_parameter_pointer(issues):
    """Add const to function parameters."""
    fixed = 0
    const_issues = [i for i in issues if i['rule'] == 'constParameterPointer']
    
    for issue in const_issues:
        fpath = issue['file']
        line = issue['line']
        
        if not os.path.exists(fpath):
            print(f"  SKIP (not found): {fpath}")
            continue
            
        with open(fpath, 'r') as f:
            content = f.read()
        
        msg_match = re.search(r"Parameter '(.+?)'", issue['message'])
        if not msg_match:
            continue
        param_name = msg_match.group(1)
        
        # Find the parameter declaration in the function signature
        # Pattern like: "Type* paramName" in a function parameter list
        lines = content.split('\n')
        idx = line - 1
        old = lines[idx]
        
        if 'const' in old[:old.find(param_name)]:
            continue
        
        # Replace type declaration
        ptype_match = re.search(
            rf'(\w[\w_]*)\s*\*+\s*{re.escape(param_name)}\b', old
        )
        if not ptype_match:
            continue
        
        ptr_type = ptype_match.group(1)
        if 'const' in old[:old.index(ptr_type)]:
            continue
        
        new = old.replace(f"{ptr_type}*", f"const {ptr_type}*", 1)
        if new == old:
            new = old.replace(f"{ptr_type} *", f"const {ptr_type} *", 1)
        if new == old:
            new = old.replace(f"{ptr_type}  *", f"const {ptr_type} *", 1)
        
        lines[idx] = new
        with open(fpath, 'w') as f:
            f.writelines(lines)
        print(f"  FIXED: {fpath}:{line} -> {new.strip()}")
        fixed += 1
    
    return fixed

def fix_unused_variable(issues):
    """Remove or comment out unused variables."""
    fixed = 0
    unused_issues = [i for i in issues if i['rule'] == 'unusedVariable']
    
    for issue in unused_issues:
        fpath = issue['file']
        line = issue['line']
        
        if not os.path.exists(fpath):
            continue
            
        with open(fpath, 'r') as f:
            lines = f.readlines()
        
        idx = line - 1
        if idx >= len(lines):
            continue
        
        old = lines[idx]
        
        msg_match = re.search(r"Unused variable: (\w+)", issue['message'])
        if not msg_match:
            continue
        varname = msg_match.group(1)
        
        # Check if it's a simple declaration (not function call)
        stripped = old.strip()
        
        # If it's just a variable declaration (no initialization), comment it out
        # Pattern: "type varname;" or "type varname = value;"
        if re.match(r'^\s+\w[\w_]*\s+\w+\s*=\s*.+\s*;\s*(?:\/\/.*)?$', stripped) or \
           re.match(r'^\s+\w[\w_]*\s+\w+\s*;\s*(?:\/\/.*)?$', stripped):
            # Comment it out
            lines[idx] = f"/* {old.rstrip()} */\n"
            print(f"  FIXED: {fpath}:{line} -> commented out {varname}")
            fixed += 1
        elif '=' in stripped:
            # Has initialization but also might be a function call
            # Add (void) cast if there's an assignment from a function
            if re.search(rf'{re.escape(varname)}\s*=\s*\w+\(', stripped):
                lines[idx] = old.replace(
                    f'{varname} = ', f'(void)',
                    1
                ).replace(';', ';', 1)
                print(f"  FIXED: {fpath}:{line} -> (void) cast {varname}")
                fixed += 1
        
        with open(fpath, 'w') as f:
            f.writelines(lines)
    
    return fixed

def fix_unread_variable(issues):
    """Fix variables assigned but never read - add (void) or remove assignment."""
    fixed = 0
    unread_issues = [i for i in issues if i['rule'] == 'unreadVariable']
    
    for issue in unread_issues:
        fpath = issue['file']
        line = issue['line']
        
        if not os.path.exists(fpath):
            continue
            
        with open(fpath, 'r') as f:
            lines = f.readlines()
        
        idx = line - 1
        if idx >= len(lines):
            continue
        
        old = lines[idx]
        stripped = old.strip()
        
        # Skip if already has (void) cast
        if '(void)' in stripped:
            continue
        
        msg_match = re.search(r"Variable '(.+?)'", issue['message'])
        if not msg_match:
            continue
        varname = msg_match.group(1)
        
        # Check if it's on the declaration line with assignment
        # Pattern: "Type varname = value;"
        assign_match = re.search(
            rf'\w[\w_]*\s+\*?{re.escape(varname)}\s*=\s*(.+);',
            stripped
        )
        if assign_match:
            rhs = assign_match.group(1)
            # If the RHS is a simple expression (not function call), comment out the whole line
            if not '(' in rhs:
                lines[idx] = old.replace(f'= {rhs};', ';', 1)
                print(f"  FIXED: {fpath}:{line} -> removed init for {varname}")
                fixed += 1
            else:
                # It's a function call - wrap with (void)
                lines[idx] = old.replace(
                    f'{varname} = ', '(void)',
                    1
                ).replace(';', ';', 1)
                print(f"  FIXED: {fpath}:{line} -> (void) cast {varname}")
                fixed += 1
                continue
        
        with open(fpath, 'w') as f:
            f.writelines(lines)
    
    return fixed

def main():
    print("=== Running cppcheck to get advisory issues ===")
    output = run_cppcheck()
    issues = parse_issues(output)
    
    print(f"\n=== Found {len(issues)} advisory issues ===")
    
    # Count by rule
    rules = {}
    for i in issues:
        rules[i['rule']] = rules.get(i['rule'], 0) + 1
    
    for rule, count in sorted(rules.items(), key=lambda x: -x[1]):
        print(f"  {rule}: {count}")
    
    total_fixed = 0
    
    print("\n=== Fixing constVariablePointer ===")
    fixed = fix_const_variable_pointer(issues)
    print(f"  Fixed: {fixed}")
    total_fixed += fixed
    
    print("\n=== Fixing constParameterPointer ===")
    fixed = fix_const_parameter_pointer(issues)
    print(f"  Fixed: {fixed}")
    total_fixed += fixed
    
    print("\n=== Fixing unusedVariable ===")
    fixed = fix_unused_variable(issues)
    print(f"  Fixed: {fixed}")
    total_fixed += fixed
    
    print("\n=== Fixing unreadVariable ===")
    fixed = fix_unread_variable(issues)
    print(f"  Fixed: {fixed}")
    total_fixed += fixed
    
    print(f"\n=== Total Fixed: {total_fixed} ===")
    
    # Re-run cppcheck to verify
    print("\n=== Verification: re-running cppcheck ===")
    new_output = run_cppcheck()
    new_issues = parse_issues(new_output)
    new_rules = {}
    for i in new_issues:
        new_rules[i['rule']] = new_rules.get(i['rule'], 0) + 1
    
    print(f"Remaining: {len(new_issues)} issues")
    for rule, count in sorted(new_rules.items(), key=lambda x: -x[1]):
        print(f"  {rule}: {count}")

if __name__ == '__main__':
    main()
