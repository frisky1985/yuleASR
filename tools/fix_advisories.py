#!/usr/bin/env python3
"""
Fix Advisory cppcheck violations across the project.
Fixes: unusedVariable, unreadVariable, redundantAssignment
"""
import subprocess
import re
import os

PROJECT = os.path.expanduser("~/.openclaw/workspace/yuleASR")
WORKSPACE = PROJECT

def run_cppcheck():
    """Run cppcheck and get advisory violations"""
    result = subprocess.run([
        "cppcheck", "--enable=all",
        "--project", os.path.join(WORKSPACE, "build-test/compile_commands.json"),
        "--suppress=unmatchedSuppression",
        "--suppress=missingIncludeSystem",
        "--suppress=unusedFunction",
        "--inconclusive"
    ], capture_output=True, text=True, cwd=WORKSPACE, timeout=120)
    
    violations = []
    for line in result.stdout.splitlines() + result.stderr.splitlines():
        if any(tag in line for tag in ["[unusedVariable]", "[unreadVariable]", "[redundantAssignment]"]):
            violations.append(line)
    return violations

def parse_violation(line):
    """Parse a cppcheck violation line into (file, line_num, message, rule)"""
    # Format: src/.../file.c:123:45: style: message [rule]
    match = re.match(r'^([^:]+):(\d+):(\d+):\s+style:\s+(.*)\[([^\]]+)\]', line)
    if match:
        return match.group(1), int(match.group(2)), match.group(4).strip(), match.group(5).strip()
    return None, 0, "", ""

def read_file_lines(path):
    full_path = os.path.join(WORKSPACE, path)
    if not os.path.exists(full_path):
        return None
    with open(full_path, 'r') as f:
        return f.readlines()

def write_file_lines(path, lines):
    full_path = os.path.join(WORKSPACE, path)
    with open(full_path, 'w') as f:
        f.writelines(lines)

def fix_unused_variable(lines, line_no, var_name):
    """Fix unusedVariable - remove the declaration line or add (void)"""
    line = lines[line_no - 1]  # 0-indexed
    stripped = line.strip()
    if var_name in stripped:
        # Check if the declaration can be removed
        if stripped.startswith(var_name) or var_name in stripped.split(',')[0]:
            # Simple case: declaration only, e.g. "type var;"
            # Comment it out
            indent = line[:len(line) - len(line.lstrip())]
            lines[line_no - 1] = f"{indent}/* UNUSED: {stripped} */\n"
            return True
    return False

def fix_unread_variable(lines, line_no, var_name):
    """Fix unreadVariable - add (void) cast or comment"""
    line = lines[line_no - 1]
    stripped = line.strip()
    indent = line[:len(line) - len(line.lstrip())]
    
    # If it's a struct member assignment like message.Payload = NULL;
    if '.' in var_name and '=' in stripped:
        # Comment out
        lines[line_no - 1] = f"{indent}/* UNREAD: {stripped} */\n"
        return True
    
    # If it's a simple variable assignment
    if '=' in stripped and var_name in stripped.split('=')[0].strip():
        # Check if RHS is a function call (don't remove)
        rhs = stripped.split('=', 1)[1].strip().rstrip(';')
        if '(' in rhs:
            # Function call - keep the call, remove assignment
            lines[line_no - 1] = f"{indent}(void){rhs};\n"
            return True
        else:
            # Simple value - comment out
            lines[line_no - 1] = f"{indent}/* UNREAD: {stripped} */\n"
            return True
    
    return False

def fix_redundant_assignment(lines, line_no, var_name):
    """Fix redundantAssignment - comment out the redundant line"""
    line = lines[line_no - 1]
    stripped = line.strip()
    indent = line[:len(line) - len(line.lstrip())]
    
    # Comment out the line
    lines[line_no - 1] = f"{indent}/* REDUNDANT: {stripped} */\n"
    return True

def main():
    violations = run_cppcheck()
    print(f"Found {len(violations)} fixable violations")
    
    # Group by file
    file_violations = {}
    for v in violations:
        file_path, line_no, msg, rule = parse_violation(v)
        if file_path and line_no:
            if file_path not in file_violations:
                file_violations[file_path] = []
            file_violations[file_path].append((line_no, msg, rule))
    
    total_fixed = 0
    for file_path, file_vios in file_violations.items():
        lines = read_file_lines(file_path)
        if lines is None:
            print(f"  SKIP (not found): {file_path}")
            continue
        
        file_fixed = 0
        # Process in reverse line order to maintain line numbers
        for line_no, msg, rule in sorted(file_vios, key=lambda x: -x[0]):
            var_name = ""
            if "Variable '" in msg:
                m = re.search(r"Variable '([^']+)'", msg)
                if m:
                    var_name = m.group(1)
            elif "Unused variable:" in msg:
                var_name = msg.split(":")[-1].strip()
            elif "is reassigned" in msg:
                m = re.search(r"Variable '([^']+)'", msg)
                if m:
                    var_name = m.group(1)
            
            if rule == "unusedVariable":
                if fix_unused_variable(lines, line_no, var_name):
                    file_fixed += 1
            elif rule == "unreadVariable":
                if fix_unread_variable(lines, line_no, var_name):
                    file_fixed += 1
            elif rule == "redundantAssignment":
                if fix_redundant_assignment(lines, line_no, var_name):
                    file_fixed += 1
        
        if file_fixed > 0:
            write_file_lines(file_path, lines)
            total_fixed += file_fixed
            print(f"  Fixed {file_fixed} in {file_path}")
    
    print(f"\nTotal fixed: {total_fixed}")
    
    # Now also fix variableScope in the modified files
    # This is more targeted - just move declarations closer to usage
    return total_fixed

if __name__ == '__main__':
    main()
