#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Integrity Checker - AUTOSAR XML文件完整性分析器

功能:
1. XML语法正确性检查
2. 必需元素检查
3. 引用关系验证
4. 数据类型定义检查
5. 配置参数范围检查
6. 生成详细报告 (ERROR/WARNING/INFO)

作者: yuleASR Team
版本: 1.0.0
"""

import xml.etree.ElementTree as ET
import re
import os
import sys
import argparse
from dataclasses import dataclass, field
from enum import Enum
from typing import List, Dict, Set, Optional, Tuple, Any
from collections import defaultdict


class SeverityLevel(Enum):
    """错误严重级别"""
    ERROR = "ERROR"
    WARNING = "WARNING"
    INFO = "INFO"


@dataclass
class CheckResult:
    """检查结果条目"""
    severity: SeverityLevel
    line: int
    message: str
    element: Optional[str] = None
    suggestion: Optional[str] = None
    
    def __str__(self) -> str:
        base = f"[{self.severity.value}] Line {self.line}: {self.message}"
        if self.element:
            base += f" [Element: {self.element}]"
        if self.suggestion:
            base += f"\n    Suggestion: {self.suggestion}"
        return base


@dataclass
class IntegrityReport:
    """完整性检查报告"""
    file_path: str
    errors: List[CheckResult] = field(default_factory=list)
    warnings: List[CheckResult] = field(default_factory=list)
    infos: List[CheckResult] = field(default_factory=list)
    
    def add_result(self, result: CheckResult) -> None:
        """添加检查结果"""
        if result.severity == SeverityLevel.ERROR:
            self.errors.append(result)
        elif result.severity == SeverityLevel.WARNING:
            self.warnings.append(result)
        else:
            self.infos.append(result)
    
    @property
    def total_issues(self) -> int:
        return len(self.errors) + len(self.warnings) + len(self.infos)
    
    def to_string(self) -> str:
        """生成报告字符串"""
        lines = [
            "=" * 60,
            "ARXML Integrity Check Report",
            "=" * 60,
            f"File: {self.file_path}",
            f"Errors: {len(self.errors)}",
            f"Warnings: {len(self.warnings)}",
            f"Infos: {len(self.infos)}",
            "-" * 60
        ]
        
        if self.errors:
            lines.append("\n[ERRORS]")
            for err in self.errors:
                lines.append(str(err))
        
        if self.warnings:
            lines.append("\n[WARNINGS]")
            for warn in self.warnings:
                lines.append(str(warn))
        
        if self.infos:
            lines.append("\n[INFOS]")
            for info in self.infos:
                lines.append(str(info))
        
        lines.extend([
            "-" * 60,
            f"Check completed: {'PASSED' if not self.errors else 'FAILED'}"
        ])
        
        return "\n".join(lines)


class ArxmlIntegrityChecker:
    """ARXML完整性检查器"""
    
    # AUTOSAR标准命名空间
    AUTOSAR_NS = "http://autosar.org/schema/r4.0"
    
    # 必需的核心元素
    REQUIRED_ELEMENTS = {
        "AUTOSAR",
        "AR-PACKAGES",
        "AR-PACKAGE",
        "SHORT-NAME",
        "ELEMENTS"
    }
    
    # 常见的引用属性
    REF_ATTRIBUTES = {
        "DEST",
        "UUID-REF",
        "TYPE-TREF",
        "COMPONENT-REF",
        "PORT-REF",
        "INTERFACE-REF",
        "DATA-TYPE-REF",
        "UNIT-REF"
    }
    
    # 数据类型定义检查
    DATA_TYPE_ELEMENTS = {
        "APPLICATION-PRIMITIVE-DATA-TYPE",
        "APPLICATION-ARRAY-DATA-TYPE",
        "APPLICATION-RECORD-DATA-TYPE",
        "IMPLEMENTATION-DATA-TYPE",
        "SW-BASE-TYPE",
        "COMPU-METHOD",
        "UNIT",
        "DATA-CONSTR"
    }
    
    def __init__(self, file_path: str):
        self.file_path = file_path
        self.report = IntegrityReport(file_path)
        self.root: Optional[ET.Element] = None
        self.namespaces: Dict[str, str] = {}
        self.line_map: Dict[int, str] = {}
        self.element_ids: Set[str] = set()
        self.element_refs: List[Tuple[str, int, str]] = []  # (ref_value, line, context)
        
    def check(self) -> IntegrityReport:
        """执行所有检查"""
        # 1. XML语法检查
        if not self._check_xml_syntax():
            return self.report
        
        # 2. 必需元素检查
        self._check_required_elements()
        
        # 3. 引用关系检查
        self._check_references()
        
        # 4. 数据类型定义检查
        self._check_data_types()
        
        # 5. 配置参数范围检查
        self._check_parameter_ranges()
        
        # 6. 命名空间检查
        self._check_namespaces()
        
        # 7. UUID唯一性检查
        self._check_uuid_uniqueness()
        
        return self.report
    
    def _check_xml_syntax(self) -> bool:
        """检查XML语法正确性"""
        try:
            # 读取文件内容用于行号映射
            with open(self.file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                self._build_line_map(content)
            
            # 解析XML
            tree = ET.parse(self.file_path)
            self.root = tree.getroot()
            
            # 提取命名空间
            self._extract_namespaces(content)
            
            # 记录成功信息
            self.report.add_result(CheckResult(
                severity=SeverityLevel.INFO,
                line=1,
                message="XML syntax validation passed"
            ))
            return True
            
        except ET.ParseError as e:
            line_num = self._extract_line_from_error(str(e))
            self.report.add_result(CheckResult(
                severity=SeverityLevel.ERROR,
                line=line_num,
                message=f"XML syntax error: {str(e)}",
                suggestion="Check XML structure, ensure all tags are properly closed"
            ))
            return False
        except Exception as e:
            self.report.add_result(CheckResult(
                severity=SeverityLevel.ERROR,
                line=0,
                message=f"Failed to read file: {str(e)}"
            ))
            return False
    
    def _build_line_map(self, content: str) -> None:
        """构建行号到内容的映射"""
        lines = content.split('\n')
        for i, line in enumerate(lines, 1):
            self.line_map[i] = line
    
    def _extract_line_from_error(self, error_msg: str) -> int:
        """从错误消息中提取行号"""
        match = re.search(r'line\s*(\d+)', error_msg, re.IGNORECASE)
        if match:
            return int(match.group(1))
        return 0
    
    def _extract_namespaces(self, content: str) -> None:
        """提取XML命名空间"""
        ns_pattern = r'xmlns(?::(\w+))?="([^"]+)"'
        for match in re.finditer(ns_pattern, content):
            prefix = match.group(1) or ""
            uri = match.group(2)
            self.namespaces[prefix] = uri
            # 检查是否为AUTOSAR标准命名空间
            if "autosar" in uri.lower():
                self.report.add_result(CheckResult(
                    severity=SeverityLevel.INFO,
                    line=self._find_line_for_pattern(f'xmlns:{prefix}'),
                    message=f"Found AUTOSAR namespace: {uri}"
                ))
    
    def _find_line_for_pattern(self, pattern: str) -> int:
        """查找模式出现的行号"""
        for line_num, content in self.line_map.items():
            if pattern in content:
                return line_num
        return 1
    
    def _check_required_elements(self) -> None:
        """检查必需元素是否存在"""
        if self.root is None:
            return
        
        # 检查根元素
        root_tag = self._get_local_tag(self.root.tag)
        if root_tag != "AUTOSAR":
            self.report.add_result(CheckResult(
                severity=SeverityLevel.ERROR,
                line=1,
                message=f"Root element must be 'AUTOSAR', found '{root_tag}'",
                element=root_tag,
                suggestion="ARXML文件根元素必须是<AUTOSAR>"
            ))
        
        # 递归检查必需元素
        self._scan_elements(self.root, 1)
    
    def _get_local_tag(self, tag: str) -> str:
        """获取本地标签名(去除命名空间)"""
        if '}' in tag:
            return tag.split('}')[1]
        return tag
    
    def _scan_elements(self, element: ET.Element, depth: int) -> None:
        """递归扫描元素"""
        tag = self._get_local_tag(element.tag)
        line = element.get("_line", 0)
        
        # 检查SHORT-NAME不能为空
        if tag == "SHORT-NAME":
            if not element.text or not element.text.strip():
                self.report.add_result(CheckResult(
                    severity=SeverityLevel.ERROR,
                    line=line,
                    message="SHORT-NAME element cannot be empty",
                    element=tag,
                    suggestion="Provide a valid short name for the element"
                ))
        
        # 收集UUID用于后续唯一性检查
        uuid_attr = "{http://autosar.org/schema/r4.0}UUID"
        if uuid_attr in element.attrib:
            self.element_ids.add(element.attrib[uuid_attr])
        
        # 收集引用
        for attr_name, attr_value in element.attrib.items():
            if any(ref in attr_name.upper() for ref in self.REF_ATTRIBUTES):
                self.element_refs.append((attr_value, line, tag))
        
        # 递归处理子元素
        for child in element:
            self._scan_elements(child, depth + 1)
    
    def _check_references(self) -> None:
        """检查引用关系有效性"""
        if not self.element_refs:
            return
        
        # 提取所有定义的UUID
        defined_uuids = self._extract_all_uuids()
        
        for ref_value, line, context in self.element_refs:
            # 检查空引用
            if not ref_value or not ref_value.strip():
                self.report.add_result(CheckResult(
                    severity=SeverityLevel.WARNING,
                    line=line,
                    message=f"Empty reference found in {context}",
                    element=context,
                    suggestion="Remove empty reference or provide valid reference value"
                ))
            # 检查UUID引用是否存在
            elif ref_value.startswith("UUID:"):
                uuid_val = ref_value[5:]
                if uuid_val not in defined_uuids:
                    self.report.add_result(CheckResult(
                        severity=SeverityLevel.ERROR,
                        line=line,
                        message=f"Referenced UUID not found: {uuid_val}",
                        element=context,
                        suggestion="Ensure the referenced element exists in the file"
                    ))
    
    def _extract_all_uuids(self) -> Set[str]:
        """提取所有定义的UUID"""
        uuids = set()
        if self.root is None:
            return uuids
        
        # UUID属性可能有多种格式
        uuid_attrs = ["UUID", "{http://autosar.org/schema/r4.0}UUID"]
        for elem in self.root.iter():
            for attr in uuid_attrs:
                if attr in elem.attrib:
                    uuids.add(elem.attrib[attr])
        return uuids
    
    def _check_data_types(self) -> None:
        """检查数据类型定义"""
        if self.root is None:
            return
        
        found_types = set()
        undefined_refs = []
        
        for elem in self.root.iter():
            tag = self._get_local_tag(elem.tag)
            
            # 记录定义的数据类型
            if tag in self.DATA_TYPE_ELEMENTS:
                short_name_elem = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                if short_name_elem is not None and short_name_elem.text:
                    found_types.add(short_name_elem.text)
            
            # 检查数据类型引用
            if tag in ["TYPE-TREF", "BASE-TYPE-REF", "COMPU-METHOD-REF"]:
                if elem.text and elem.text not in found_types:
                    # 延迟检查，因为定义可能在后面
                    undefined_refs.append((elem.text, elem))
        
        # 报告未定义的数据类型引用
        for type_name, elem in undefined_refs:
            if type_name not in found_types:
                line = self._get_element_line(elem)
                self.report.add_result(CheckResult(
                    severity=SeverityLevel.WARNING,
                    line=line,
                    message=f"Potentially undefined data type reference: {type_name}",
                    element=self._get_local_tag(elem.tag),
                    suggestion="Ensure the data type is defined in this or imported ARXML"
                ))
    
    def _get_element_line(self, element: ET.Element) -> int:
        """获取元素的行号(估计值)"""
        # 由于ElementTree不提供行号，使用元素特征搜索
        return 0
    
    def _check_parameter_ranges(self) -> None:
        """检查配置参数范围"""
        if self.root is None:
            return
        
        # 检查常见的数值参数
        range_checks = {
            "LENGTH": (0, 65535, "Data length must be positive"),
            "SIZE": (0, 4294967295, "Size must be positive"),
            "PRIORITY": (0, 255, "Priority should be in range 0-255"),
            "PERIOD": (0, 3600000, "Period should be in valid range (ms)"),
            "TIMEOUT": (0, 3600000, "Timeout should be in valid range (ms)")
        }
        
        for elem in self.root.iter():
            tag = self._get_local_tag(elem.tag)
            
            for check_name, (min_val, max_val, msg) in range_checks.items():
                if check_name in tag.upper():
                    if elem.text:
                        try:
                            value = int(elem.text)
                            if value < min_val or value > max_val:
                                self.report.add_result(CheckResult(
                                    severity=SeverityLevel.WARNING,
                                    line=0,
                                    message=f"{msg}: {value}",
                                    element=tag,
                                    suggestion=f"Value should be in range [{min_val}, {max_val}]"
                                ))
                        except ValueError:
                            self.report.add_result(CheckResult(
                                severity=SeverityLevel.ERROR,
                                line=0,
                                message=f"Invalid numeric value for {tag}: '{elem.text}'",
                                element=tag,
                                suggestion="Provide a valid integer value"
                            ))
    
    def _check_namespaces(self) -> None:
        """检查命名空间声明"""
        has_autosar_ns = False
        
        for prefix, uri in self.namespaces.items():
            if self.AUTOSAR_NS in uri or "autosar" in uri.lower():
                has_autosar_ns = True
                break
        
        if not has_autosar_ns:
            self.report.add_result(CheckResult(
                severity=SeverityLevel.WARNING,
                line=1,
                message="AUTOSAR namespace not declared",
                suggestion=f"Add xmlns attribute with {self.AUTOSAR_NS}"
            ))
    
    def _check_uuid_uniqueness(self) -> None:
        """检查UUID唯一性"""
        if self.root is None:
            return
        
        uuid_counts = defaultdict(list)
        uuid_attrs = ["UUID", "{http://autosar.org/schema/r4.0}UUID"]
        
        for elem in self.root.iter():
            for uuid_attr in uuid_attrs:
                if uuid_attr in elem.attrib:
                    uuid_val = elem.attrib[uuid_attr]
                    tag = self._get_local_tag(elem.tag)
                    uuid_counts[uuid_val].append(tag)
        
        for uuid_val, elements in uuid_counts.items():
            if len(elements) > 1:
                self.report.add_result(CheckResult(
                    severity=SeverityLevel.ERROR,
                    line=0,
                    message=f"Duplicate UUID detected: {uuid_val}",
                    element=", ".join(elements),
                    suggestion="UUID must be unique across all elements"
                ))


class ArxmlBatchChecker:
    """批量ARXML文件检查器"""
    
    def __init__(self, paths: List[str], recursive: bool = False):
        self.paths = paths
        self.recursive = recursive
        self.files_to_check: List[str] = []
        
    def find_files(self) -> None:
        """查找所有ARXML文件"""
        for path in self.paths:
            if os.path.isfile(path):
                if path.endswith('.arxml'):
                    self.files_to_check.append(path)
            elif os.path.isdir(path):
                if self.recursive:
                    for root, _, files in os.walk(path):
                        for f in files:
                            if f.endswith('.arxml'):
                                self.files_to_check.append(os.path.join(root, f))
                else:
                    for f in os.listdir(path):
                        if f.endswith('.arxml'):
                            self.files_to_check.append(os.path.join(path, f))
    
    def check_all(self) -> List[IntegrityReport]:
        """检查所有文件"""
        self.find_files()
        reports = []
        
        for file_path in self.files_to_check:
            print(f"Checking: {file_path}...")
            checker = ArxmlIntegrityChecker(file_path)
            report = checker.check()
            reports.append(report)
        
        return reports


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="ARXML Integrity Checker - 验证AUTOSAR XML文件完整性",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python integrity_checker.py file.arxml
  python integrity_checker.py -r ./arxml_files/
  python integrity_checker.py file1.arxml file2.arxml -o report.txt
        """
    )
    
    parser.add_argument(
        "paths",
        nargs="+",
        help="ARXML文件或目录路径"
    )
    
    parser.add_argument(
        "-r", "--recursive",
        action="store_true",
        help="递归检查子目录"
    )
    
    parser.add_argument(
        "-o", "--output",
        help="输出报告到文件"
    )
    
    parser.add_argument(
        "-s", "--summary-only",
        action="store_true",
        help="仅显示摘要"
    )
    
    args = parser.parse_args()
    
    # 执行检查
    batch_checker = ArxmlBatchChecker(args.paths, args.recursive)
    reports = batch_checker.check_all()
    
    if not reports:
        print("No ARXML files found to check.")
        sys.exit(1)
    
    # 生成报告
    total_errors = sum(len(r.errors) for r in reports)
    total_warnings = sum(len(r.warnings) for r in reports)
    total_infos = sum(len(r.infos) for r in reports)
    
    output_lines = []
    
    # 报告头部
    output_lines.extend([
        "=" * 70,
        "ARXML Integrity Check Summary",
        "=" * 70,
        f"Files checked: {len(reports)}",
        f"Total errors: {total_errors}",
        f"Total warnings: {total_warnings}",
        f"Total infos: {total_infos}",
        "=" * 70
    ])
    
    # 详细报告
    if not args.summary_only:
        for report in reports:
            output_lines.append(report.to_string())
            output_lines.append("")
    else:
        # 仅显示有问题文件的摘要
        for report in reports:
            if report.errors or report.warnings:
                status = "FAILED" if report.errors else "WARNING"
                output_lines.append(f"[{status}] {report.file_path}: "
                                  f"{len(report.errors)} errors, "
                                  f"{len(report.warnings)} warnings")
    
    # 最终结果
    output_lines.extend([
        "=" * 70,
        f"Overall result: {'PASSED' if total_errors == 0 else 'FAILED'}"
    ])
    
    output_text = "\n".join(output_lines)
    
    # 输出
    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(output_text)
        print(f"Report saved to: {args.output}")
    else:
        print(output_text)
    
    # 返回码
    sys.exit(0 if total_errors == 0 else 1)


if __name__ == "__main__":
    main()
