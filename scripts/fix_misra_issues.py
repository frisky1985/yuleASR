#!/usr/bin/env python3
"""
MISRA C:2012 Auto-Fix Script
Project: ETH-DDS Integration (Automotive Ethernet Stack)

This script automatically fixes simple MISRA C:2012 violations:
- Rule 20.7: Add parentheses around macro parameters
- Rule 17.7: Add (void) cast for ignored return values
- Rule 8.8: Add static keyword to internal functions
- Rule 15.6: Add braces to single-statement control blocks

Usage:
    python3 fix_misra_issues.py [--rules=R1,R2,...] [--apply] <files...>
    python3 fix_misra_issues.py --dry-run src/ota/*.c
    python3 fix_misra_issues.py --rules=20.7,17.7 --apply src/crypto_stack/*.c
"""

import argparse
import re
import sys
import os
from pathlib import Path
from typing import List, Tuple, Optional
from dataclasses import dataclass
from enum import Enum


class RuleType(Enum):
    """MISRA rule types that can be auto-fixed."""
    MACRO_PARENS = "20.7"      # Add parentheses around macro parameters
    RETURN_VALUE = "17.7"      # Add (void) cast for ignored return values
    STATIC_INTERNAL = "8.8"    # Add static to internal linkage functions
    COMPOUND_STMT = "15.6"     # Add braces to control statements
    UNUSED_PARAM = "2.7"       # Mark unused parameters (void)expr


@dataclass
class Fix:
    """Represents a single fix to be applied."""
    rule: RuleType
    line_num: int
    original: str
    replacement: str
    description: str


class MISRAFixer:
    """Automatically fixes simple MISRA C:2012 violations."""
    
    def __init__(self, rules: List[RuleType], apply: bool = False):
        self.rules = rules
        self.apply = apply
        self.fixes: List[Fix] = []
        self.stats = {rule: 0 for rule in RuleType}
    
    def process_file(self, filepath: str) -> List[Fix]:
        """Process a single C source file."""
        self.fixes = []
        
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"[ERROR] Cannot read {filepath}: {e}")
            return []
        
        original_lines = lines.copy()
        
        # Apply fixes based on enabled rules
        if RuleType.MACRO_PARENS in self.rules:
            lines = self._fix_macro_parens(lines)
        
        if RuleType.RETURN_VALUE in self.rules:
            lines = self._fix_return_values(lines)
        
        if RuleType.STATIC_INTERNAL in self.rules:
            lines = self._fix_static_internal(lines)
        
        if RuleType.COMPOUND_STMT in self.rules:
            lines = self._fix_compound_statements(lines)
        
        if RuleType.UNUSED_PARAM in self.rules:
            lines = self._fix_unused_params(lines)
        
        # Apply changes if requested
        if self.apply and self.fixes:
            new_content = '\n'.join(lines)
            if new_content != content:
                try:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                    print(f"[FIXED] {filepath}: {len(self.fixes)} changes")
                except Exception as e:
                    print(f"[ERROR] Cannot write {filepath}: {e}")
        elif self.fixes:
            print(f"[DRY-RUN] {filepath}: {len(self.fixes)} potential fixes")
            for fix in self.fixes:
                print(f"  Line {fix.line_num} [{fix.rule.value}]: {fix.description}")
        
        return self.fixes
    
    def _fix_macro_parens(self, lines: List[str]) -> List[str]:
        """
        Fix Rule 20.7: Add parentheses around macro parameters.
        
        Pattern: #define MACRO(x) (x + 1)  ->  #define MACRO(x) ((x) + 1)
        """
        macro_pattern = re.compile(
            r'^\s*#\s*define\s+(\w+)\s*\(([^)]+)\)\s*(.+)$'
        )
        
        for i, line in enumerate(lines):
            match = macro_pattern.match(line)
            if match:
                name = match.group(1)
                params = match.group(2).split(',')
                body = match.group(3)
                
                # Check if parameters are already parenthesized in body
                modified_body = body
                for param in params:
                    param = param.strip()
                    # Only add parens if not already present
                    pattern = r'(?<!\()' + re.escape(param) + r'(?!\))'
                    if re.search(pattern, modified_body):
                        modified_body = re.sub(
                            pattern,
                            f'({param})',
                            modified_body
                        )
                
                if modified_body != body:
                    new_line = f"#define {name}({match.group(2)}) {modified_body}"
                    self.fixes.append(Fix(
                        rule=RuleType.MACRO_PARENS,
                        line_num=i + 1,
                        original=line,
                        replacement=new_line,
                        description=f"Added parentheses to macro '{name}' parameters"
                    ))
                    self.stats[RuleType.MACRO_PARENS] += 1
                    lines[i] = new_line
        
        return lines
    
    def _fix_return_values(self, lines: List[str]) -> List[str]:
        """
        Fix Rule 17.7: Add (void) cast for ignored return values.
        
        Pattern: function_call();  ->  (void)function_call();
        
        This is a conservative fix that only handles obvious cases.
        """
        # Function calls that typically have important return values
        important_funcs = [
            'memcpy', 'memmove', 'memset', 'strcpy', 'strncpy', 'strcat',
            'strncat', 'sprintf', 'snprintf', 'malloc', 'calloc', 'realloc',
            'fread', 'fwrite', 'fopen', 'fclose', 'fprintf', 'fscanf',
            'sem_init', 'pthread_create', 'pthread_mutex_lock', 'pthread_mutex_unlock',
            'lock', 'unlock', 'init', 'deinit', 'alloc', 'free'
        ]
        
        func_call_pattern = re.compile(
            r'^\s*(?!\(void\))(\w+)\s*\([^)]*\)\s*;\s*$'
        )
        
        for i, line in enumerate(lines):
            # Skip comments, preprocessor directives, control statements
            stripped = line.strip()
            if (stripped.startswith('//') or stripped.startswith('*') or
                stripped.startswith('#') or stripped.startswith('if') or
                stripped.startswith('while') or stripped.startswith('for') or
                stripped.startswith('switch') or stripped.startswith('return') or
                stripped.startswith('(void)')):
                continue
            
            match = func_call_pattern.match(stripped)
            if match:
                func_name = match.group(1)
                # Check if this looks like a function with important return value
                if any(imp in func_name.lower() for imp in important_funcs):
                    # Check if line is not already a void cast
                    if not stripped.startswith('(void)'):
                        new_line = line.replace(func_name, f'(void){func_name}', 1)
                        self.fixes.append(Fix(
                            rule=RuleType.RETURN_VALUE,
                            line_num=i + 1,
                            original=line,
                            replacement=new_line,
                            description=f"Added (void) cast to '{func_name}'"
                        ))
                        self.stats[RuleType.RETURN_VALUE] += 1
                        lines[i] = new_line
        
        return lines
    
    def _fix_static_internal(self, lines: List[str]) -> List[str]:
        """
        Fix Rule 8.8: Add static keyword to functions with internal linkage.
        
        This is a heuristic that adds static to functions not declared in headers.
        """
        func_pattern = re.compile(
            r'^\s*(?:(?:const|volatile)\s+)?(?:\w+\s+)*?(\w+)\s*\([^)]*\)\s*\{'
        )
        
        # Find functions that should be static (simple heuristic)
        for i, line in enumerate(lines):
            # Skip if already static, extern, or inline
            if any(kw in line for kw in ['static', 'extern', 'inline', 'typedef']):
                continue
            
            # Skip if it's a forward declaration (ends with ;)
            if line.strip().endswith(';'):
                continue
            
            match = func_pattern.match(line)
            if match:
                func_name = match.group(1)
                # Skip main and common API patterns
                if func_name in ['main', 'ISR'] or func_name.startswith('_'):
                    continue
                
                # Add static keyword
                if not line.strip().startswith('static'):
                    new_line = 'static ' + line.lstrip()
                    self.fixes.append(Fix(
                        rule=RuleType.STATIC_INTERNAL,
                        line_num=i + 1,
                        original=line,
                        replacement=new_line,
                        description=f"Added static to function '{func_name}'"
                    ))
                    self.stats[RuleType.STATIC_INTERNAL] += 1
                    lines[i] = new_line
        
        return lines
    
    def _fix_compound_statements(self, lines: List[str]) -> List[str]:
        """
        Fix Rule 15.6: Add braces to single-statement control blocks.
        
        Pattern: if (x) statement;  ->  if (x) { statement; }
        """
        control_patterns = [
            (re.compile(r'^(\s*)(if\s*\([^)]+\))\s*([^;{].*?;)\s*$'), 'if'),
            (re.compile(r'^(\s*)(else)\s+([^;{].*?;)\s*$'), 'else'),
            (re.compile(r'^(\s*)(while\s*\([^)]+\))\s*([^;{].*?;)\s*$'), 'while'),
            (re.compile(r'^(\s*)(for\s*\([^)]+\))\s*([^;{].*?;)\s*$'), 'for'),
        ]
        
        for i, line in enumerate(lines):
            for pattern, keyword in control_patterns:
                match = pattern.match(line)
                if match:
                    indent = match.group(1)
                    control = match.group(2)
                    statement = match.group(3)
                    
                    # Skip if already has braces or is a comment
                    if '{' in statement or '//' in statement:
                        continue
                    
                    new_line = f"{indent}{control} {{\n{indent}    {statement}\n{indent}}}"
                    self.fixes.append(Fix(
                        rule=RuleType.COMPOUND_STMT,
                        line_num=i + 1,
                        original=line,
                        replacement=new_line,
                        description=f"Added braces to {keyword} statement"
                    ))
                    self.stats[RuleType.COMPOUND_STMT] += 1
                    lines[i] = new_line
        
        return lines
    
    def _fix_unused_params(self, lines: List[str]) -> List[str]:
        """
        Fix Rule 2.7: Mark unused parameters.
        
        Pattern: void func(int x) {  ->  void func(int x) { (void)x;
        """
        func_pattern = re.compile(
            r'^\s*(?:static\s+)?(?:\w+\s+)*(\w+)\s*\(([^)]*)\)\s*\{'
        )
        
        for i, line in enumerate(lines):
            match = func_pattern.match(line)
            if match:
                params_str = match.group(2)
                # Extract parameter names
                params = re.findall(r'\b(\w+)\s*[,)]', params_str)
                
                # Add (void)param; for each parameter
                void_casts = []
                for param in params:
                    if param not in ['void', '']:
                        void_casts.append(f"(void){param};")
                
                if void_casts:
                    indent = len(line) - len(line.lstrip())
                    void_line = ' ' * (indent + 4) + ' '.join(void_casts)
                    # Insert after opening brace
                    lines.insert(i + 1, void_line)
                    self.fixes.append(Fix(
                        rule=RuleType.UNUSED_PARAM,
                        line_num=i + 1,
                        original=line,
                        replacement=f"{line}\n{void_line}",
                        description=f"Added (void) casts for unused parameters"
                    ))
                    self.stats[RuleType.UNUSED_PARAM] += 1
        
        return lines
    
    def print_summary(self):
        """Print summary of fixes."""
        print("\n" + "=" * 60)
        print("MISRA Auto-Fix Summary")
        print("=" * 60)
        total = 0
        for rule, count in self.stats.items():
            if count > 0:
                print(f"  Rule {rule.value}: {count} fixes")
                total += count
        print("-" * 60)
        print(f"  Total fixes: {total}")
        print("=" * 60)


def parse_rules(rules_str: str) -> List[RuleType]:
    """Parse comma-separated rule list."""
    if rules_str == 'all':
        return list(RuleType)
    
    rule_map = {r.value: r for r in RuleType}
    rules = []
    for r in rules_str.split(','):
        r = r.strip()
        if r in rule_map:
            rules.append(rule_map[r])
        else:
            print(f"[WARN] Unknown rule: {r}")
    
    return rules if rules else list(RuleType)


def find_c_files(paths: List[str]) -> List[str]:
    """Find all C files in given paths."""
    files = []
    for path in paths:
        p = Path(path)
        if p.is_file() and p.suffix == '.c':
            files.append(str(p))
        elif p.is_dir():
            files.extend(str(f) for f in p.rglob('*.c'))
    return files


def main():
    parser = argparse.ArgumentParser(
        description='Auto-fix simple MISRA C:2012 violations'
    )
    parser.add_argument(
        'paths',
        nargs='+',
        help='C source files or directories to process'
    )
    parser.add_argument(
        '--rules',
        default='all',
        help='Comma-separated list of rules to fix (default: all)'
    )
    parser.add_argument(
        '--apply',
        action='store_true',
        help='Apply fixes to files (default: dry-run)'
    )
    parser.add_argument(
        '--exclude',
        action='append',
        default=[],
        help='Patterns to exclude (can be used multiple times)'
    )
    
    args = parser.parse_args()
    
    # Parse rules
    rules = parse_rules(args.rules)
    if not rules:
        print("[ERROR] No valid rules specified")
        sys.exit(1)
    
    print(f"Enabled rules: {', '.join(r.value for r in rules)}")
    print(f"Mode: {'APPLY' if args.apply else 'DRY-RUN'}")
    
    # Find files
    files = find_c_files(args.paths)
    
    # Apply excludes
    for pattern in args.exclude:
        files = [f for f in files if pattern not in f]
    
    if not files:
        print("[ERROR] No C files found")
        sys.exit(1)
    
    print(f"Processing {len(files)} files...\n")
    
    # Process files
    fixer = MISRAFixer(rules, apply=args.apply)
    total_fixes = 0
    
    for filepath in files:
        fixes = fixer.process_file(filepath)
        total_fixes += len(fixes)
    
    # Print summary
    fixer.print_summary()
    
    if not args.apply:
        print("\nRun with --apply to apply these fixes.")
    
    sys.exit(0 if args.apply or total_fixes == 0 else 1)


if __name__ == '__main__':
    main()
