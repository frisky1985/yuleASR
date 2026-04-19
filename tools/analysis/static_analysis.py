#!/usr/bin/env python3
"""
YuleTech BSW Static Analysis Tool

集成 MISRA C 检查、圈复杂度分析、代码质量报告
"""

import os
import re
import json
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from collections import defaultdict


@dataclass
class AnalysisResult:
    """分析结果"""
    file_path: str
    line_number: int
    rule_id: str
    severity: str  # error, warning, info
    message: str
    category: str  # misra, complexity, style


@dataclass
class ComplexityMetrics:
    """圈复杂度指标"""
    file_path: str
    function_name: str
    line_number: int
    complexity: int
    lines_of_code: int


@dataclass
class CodeMetrics:
    """代码度量"""
    total_files: int
    total_lines: int
    code_lines: int
    comment_lines: int
    blank_lines: int
    avg_complexity: float
    max_complexity: int
    functions_count: int


class StaticAnalyzer:
    """静态分析器"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.results: List[AnalysisResult] = []
        self.complexity_metrics: List[ComplexityMetrics] = []
        self.metrics: Optional[CodeMetrics] = None
        
        # MISRA C 规则（简化版）
        self.misra_rules = {
            "MISRA-C-1.1": {
                "pattern": r'//.*[^\x00-\x7F]',
                "message": "Code shall not contain unreachable code",
                "severity": "error"
            },
            "MISRA-C-2.1": {
                "pattern": r'goto\s+\w+',
                "message": "Assembly language shall be encapsulated and isolated",
                "severity": "warning"
            },
            "MISRA-C-5.1": {
                "pattern": r'^\s*#define\s+\w{32,}',
                "message": "Identifier length shall not exceed 31 characters",
                "severity": "warning"
            },
            "MISRA-C-8.1": {
                "pattern": r'^\s*(?!.*\b(static|extern)\b).*\b\w+\s*\([^)]*\)\s*\{',
                "message": "Functions shall have explicit return type",
                "severity": "error"
            },
            "MISRA-C-14.4": {
                "pattern": r'if\s*\(\s*[^)]+\s*\)\s*;',
                "message": "The controlling expression of an if statement shall not be empty",
                "severity": "error"
            },
            "MISRA-C-15.5": {
                "pattern": r'return\s+[^;]+;.*?\n.*?return\s+',
                "message": "A function should have a single point of exit",
                "severity": "warning"
            },
            "MISRA-C-17.7": {
                "pattern": r'void\s+\w+\s*\([^)]*\)\s*\{[^}]*\}',
                "message": "The value returned by a function shall be used",
                "severity": "info"
            }
        }
        
    def analyze_file(self, file_path: Path) -> List[AnalysisResult]:
        """分析单个文件"""
        results = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
                
            # MISRA 检查
            for rule_id, rule in self.misra_rules.items():
                for match in re.finditer(rule["pattern"], content, re.MULTILINE):
                    line_num = content[:match.start()].count('\n') + 1
                    results.append(AnalysisResult(
                        file_path=str(file_path),
                        line_number=line_num,
                        rule_id=rule_id,
                        severity=rule["severity"],
                        message=rule["message"],
                        category="misra"
                    ))
                    
            # 圈复杂度分析
            results.extend(self._analyze_complexity(file_path, content, lines))
            
            # 代码风格检查
            results.extend(self._check_style(file_path, lines))
            
        except Exception as e:
            print(f"Error analyzing {file_path}: {e}")
            
        return results
        
    def _analyze_complexity(self, file_path: Path, content: str, lines: List[str]) -> List[AnalysisResult]:
        """分析圈复杂度"""
        results = []
        
        # 查找函数定义
        function_pattern = r'^(\w[\w\s*]+)\s+(\w+)\s*\([^)]*\)\s*\{'
        
        for match in re.finditer(function_pattern, content, re.MULTILINE):
            func_start = match.start()
            line_num = content[:func_start].count('\n') + 1
            func_name = match.group(2)
            
            # 计算圈复杂度
            complexity = 1
            func_end = self._find_function_end(content, func_start)
            func_content = content[func_start:func_end]
            
            # 增加复杂度的关键字
            complexity_keywords = ['if', 'while', 'for', 'case', '&&', '||', '?']
            for keyword in complexity_keywords:
                complexity += func_content.count(keyword)
                
            # 记录指标
            self.complexity_metrics.append(ComplexityMetrics(
                file_path=str(file_path),
                function_name=func_name,
                line_number=line_num,
                complexity=complexity,
                lines_of_code=func_content.count('\n')
            ))
            
            # 如果复杂度过高，添加警告
            if complexity > 10:
                results.append(AnalysisResult(
                    file_path=str(file_path),
                    line_number=line_num,
                    rule_id="COMPLEXITY",
                    severity="warning",
                    message=f"Function '{func_name}' has cyclomatic complexity of {complexity} (max recommended: 10)",
                    category="complexity"
                ))
                
        return results
        
    def _find_function_end(self, content: str, start_pos: int) -> int:
        """查找函数结束位置"""
        brace_count = 0
        in_function = False
        
        for i in range(start_pos, len(content)):
            if content[i] == '{':
                brace_count += 1
                in_function = True
            elif content[i] == '}':
                brace_count -= 1
                if in_function and brace_count == 0:
                    return i + 1
                    
        return len(content)
        
    def _check_style(self, file_path: Path, lines: List[str]) -> List[AnalysisResult]:
        """检查代码风格"""
        results = []
        
        for line_num, line in enumerate(lines, 1):
            # 检查行长度
            if len(line) > 120:
                results.append(AnalysisResult(
                    file_path=str(file_path),
                    line_number=line_num,
                    rule_id="STYLE-001",
                    severity="info",
                    message=f"Line exceeds 120 characters ({len(line)})",
                    category="style"
                ))
                
            # 检查尾随空格
            if line.rstrip() != line:
                results.append(AnalysisResult(
                    file_path=str(file_path),
                    line_number=line_num,
                    rule_id="STYLE-002",
                    severity="info",
                    message="Line has trailing whitespace",
                    category="style"
                ))
                
            # 检查 Tab 字符
            if '\t' in line:
                results.append(AnalysisResult(
                    file_path=str(file_path),
                    line_number=line_num,
                    rule_id="STYLE-003",
                    severity="info",
                    message="Line contains tab characters (use spaces)",
                    category="style"
                ))
                
        return results
        
    def analyze_project(self) -> None:
        """分析整个项目"""
        print("YuleTech BSW Static Analysis Tool v1.0.0")
        print("=" * 60)
        
        source_files = []
        
        # 收集源文件
        for pattern in ['**/*.c', '**/*.h']:
            source_files.extend(self.project_root.glob(pattern))
            
        print(f"Found {len(source_files)} source files")
        
        # 分析每个文件
        for file_path in source_files:
            if 'tools' in str(file_path) or 'generated' in str(file_path):
                continue
            print(f"Analyzing: {file_path.relative_to(self.project_root)}")
            results = self.analyze_file(file_path)
            self.results.extend(results)
            
        # 计算度量
        self._calculate_metrics(source_files)
        
    def _calculate_metrics(self, source_files: List[Path]) -> None:
        """计算代码度量"""
        total_lines = 0
        code_lines = 0
        comment_lines = 0
        blank_lines = 0
        
        for file_path in source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    
                total_lines += len(lines)
                
                for line in lines:
                    stripped = line.strip()
                    if not stripped:
                        blank_lines += 1
                    elif stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                        comment_lines += 1
                    else:
                        code_lines += 1
                        
            except Exception as e:
                print(f"Error calculating metrics for {file_path}: {e}")
                
        # 计算平均复杂度
        avg_complexity = 0.0
        max_complexity = 0
        if self.complexity_metrics:
            complexities = [m.complexity for m in self.complexity_metrics]
            avg_complexity = sum(complexities) / len(complexities)
            max_complexity = max(complexities)
            
        self.metrics = CodeMetrics(
            total_files=len(source_files),
            total_lines=total_lines,
            code_lines=code_lines,
            comment_lines=comment_lines,
            blank_lines=blank_lines,
            avg_complexity=avg_complexity,
            max_complexity=max_complexity,
            functions_count=len(self.complexity_metrics)
        )
        
    def generate_report(self, output_path: str) -> None:
        """生成分析报告"""
        report = {
            "summary": {
                "total_issues": len(self.results),
                "errors": len([r for r in self.results if r.severity == "error"]),
                "warnings": len([r for r in self.results if r.severity == "warning"]),
                "info": len([r for r in self.results if r.severity == "info"])
            },
            "metrics": asdict(self.metrics) if self.metrics else {},
            "issues": [asdict(r) for r in self.results],
            "complexity": [asdict(c) for c in self.complexity_metrics]
        }
        
        output_file = Path(output_path)
        output_file.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2)
            
        print(f"\nReport saved to: {output_file}")
        
    def print_summary(self) -> None:
        """打印摘要"""
        print("\n" + "=" * 60)
        print("Analysis Summary")
        print("=" * 60)
        
        if self.metrics:
            print(f"\nCode Metrics:")
            print(f"  Total Files: {self.metrics.total_files}")
            print(f"  Total Lines: {self.metrics.total_lines}")
            print(f"  Code Lines: {self.metrics.code_lines}")
            print(f"  Comment Lines: {self.metrics.comment_lines}")
            print(f"  Blank Lines: {self.metrics.blank_lines}")
            print(f"  Functions: {self.metrics.functions_count}")
            print(f"  Avg Complexity: {self.metrics.avg_complexity:.2f}")
            print(f"  Max Complexity: {self.metrics.max_complexity}")
            
        print(f"\nIssues Found:")
        errors = [r for r in self.results if r.severity == "error"]
        warnings = [r for r in self.results if r.severity == "warning"]
        info = [r for r in self.results if r.severity == "info"]
        
        print(f"  Errors: {len(errors)}")
        print(f"  Warnings: {len(warnings)}")
        print(f"  Info: {len(info)}")
        
        if errors:
            print(f"\nTop 5 Errors:")
            for i, e in enumerate(errors[:5], 1):
                print(f"  {i}. {e.file_path}:{e.line_number} - {e.message}")
                
        if warnings:
            print(f"\nTop 5 Warnings:")
            for i, w in enumerate(warnings[:5], 1):
                print(f"  {i}. {w.file_path}:{w.line_number} - {w.message}")


def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description="YuleTech BSW Static Analysis Tool")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--output", default="analysis_report.json", help="Output report file")
    
    args = parser.parse_args()
    
    analyzer = StaticAnalyzer(args.project_root)
    analyzer.analyze_project()
    analyzer.generate_report(args.output)
    analyzer.print_summary()


if __name__ == "__main__":
    main()
