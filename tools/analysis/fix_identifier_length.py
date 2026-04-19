#!/usr/bin/env python3
"""修复标识符长度超过31字符的问题 (MISRA-C-5.1)"""

import re
from pathlib import Path

def find_long_identifiers(file_path):
    """查找长标识符"""
    issues = []
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
        lines = content.split('\n')
    
    # 匹配 #define 定义的宏
    define_pattern = r'#define\s+(\w{32,})'
    
    for line_num, line in enumerate(lines, 1):
        matches = re.finditer(define_pattern, line)
        for match in matches:
            identifier = match.group(1)
            issues.append({
                'line': line_num,
                'identifier': identifier,
                'length': len(identifier)
            })
    
    return issues

def main():
    """主函数"""
    print("YuleTech BSW MISRA-C-5.1 Fix Tool v1.0.0")
    print("=" * 60)
    
    src_dir = Path("src")
    all_issues = []
    
    for file_path in src_dir.rglob("*.c"):
        issues = find_long_identifiers(file_path)
        if issues:
            all_issues.extend([(file_path, i) for i in issues])
            print(f"\n{file_path}:")
            for issue in issues:
                print(f"  Line {issue['line']}: {issue['identifier']} ({issue['length']} chars)")
    
    print(f"\n总共发现 {len(all_issues)} 个长标识符")
    print("\n注意: 自动缩短标识符可能导致命名冲突，建议手动审查")

if __name__ == "__main__":
    main()
