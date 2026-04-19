#!/usr/bin/env python3
"""自动修复代码风格问题"""

import re
from pathlib import Path

def fix_trailing_whitespace(file_path):
    """修复行尾空格"""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    
    fixed = False
    new_lines = []
    for line in lines:
        new_line = line.rstrip() + '\n' if line.endswith('\n') else line.rstrip()
        if new_line != line:
            fixed = True
        new_lines.append(new_line)
    
    # 确保文件以换行符结尾
    if new_lines and not new_lines[-1].endswith('\n'):
        new_lines[-1] += '\n'
        fixed = True
    
    if fixed:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        return True
    return False

def fix_tabs(file_path):
    """将Tab替换为空格"""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    if '\t' in content:
        content = content.replace('\t', '    ')
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

def main():
    """主函数"""
    print("YuleTech BSW Style Fix Tool v1.0.0")
    print("=" * 60)
    
    src_dir = Path("src")
    fixed_files = []
    
    for file_path in src_dir.rglob("*.c"):
        if fix_trailing_whitespace(file_path):
            fixed_files.append(f"{file_path}: trailing whitespace")
    
    for file_path in src_dir.rglob("*.h"):
        if fix_trailing_whitespace(file_path):
            fixed_files.append(f"{file_path}: trailing whitespace")
    
    print(f"\nFixed {len(fixed_files)} files:")
    for f in fixed_files[:10]:
        print(f"  {f}")
    if len(fixed_files) > 10:
        print(f"  ... and {len(fixed_files) - 10} more")

if __name__ == "__main__":
    main()
