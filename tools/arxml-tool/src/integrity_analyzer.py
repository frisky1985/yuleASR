#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Integrity Analyzer - ARXML完整性分析器

功能：检查ARXML文件的结构和语义完整性
作者：YuleTech AutoSAR Team
版本：1.0.0
"""

import xml.etree.ElementTree as ET
import json
import re
from dataclasses import dataclass, field, asdict
from typing import Dict, List, Set, Optional, Any, Callable, Tuple, Union
from enum import Enum, auto
from pathlib import Path
from datetime import datetime
import uuid


class Severity(Enum):
    """问题严重程度枚举"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


class CheckType(Enum):
    """检查类型枚举"""
    STRUCTURAL = "structural"
    SEMANTIC = "semantic"
    CUSTOM = "custom"


@dataclass
class IntegrityIssue:
    """完整性问题数据类"""
    severity: Severity
    check_type: CheckType
    message: str
    element_path: str
    element_uuid: Optional[str] = None
    suggestion: Optional[str] = None
    rule_id: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "severity": self.severity.value,
            "check_type": self.check_type.value,
            "message": self.message,
            "element_path": self.element_path,
            "element_uuid": self.element_uuid,
            "suggestion": self.suggestion,
            "rule_id": self.rule_id
        }


@dataclass
class CheckRule:
    """检查规则数据类"""
    rule_id: str
    name: str
    description: str
    check_type: CheckType
    severity: Severity
    enabled: bool = True
    strict_only: bool = False  # 仅在严格模式下启用
    custom_checker: Optional[Callable[[ET.Element, str], List[IntegrityIssue]]] = None


@dataclass
class AnalysisReport:
    """分析报告数据类"""
    file_path: str
    analysis_time: str
    mode: str
    total_issues: int = 0
    error_count: int = 0
    warning_count: int = 0
    info_count: int = 0
    issues: List[IntegrityIssue] = field(default_factory=list)
    summary: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "file_path": self.file_path,
            "analysis_time": self.analysis_time,
            "mode": self.mode,
            "total_issues": self.total_issues,
            "error_count": self.error_count,
            "warning_count": self.warning_count,
            "info_count": self.info_count,
            "issues": [issue.to_dict() for issue in self.issues],
            "summary": self.summary
        }


class IntegrityAnalyzer:
    """
    ARXML完整性分析器
    
    提供结构完整性和语义完整性检查功能
    支持严格/宽松模式
    支持自定义检查规则
    """

    # AutoSAR标准命名空间
    AUTOSAR_NS = "http://autosar.org/schema/r4.0"
    NS_MAP = {"ar": "http://autosar.org/schema/r4.0"}
    
    # 必需元素定义
    REQUIRED_ELEMENTS = {
        "AUTOSAR": ["AR-PACKAGES"],
        "AR-PACKAGE": ["SHORT-NAME"],
        "ECU-CONFIGURATION": ["ECU", "ECU-EXTRACT-REF"],
        "SW-COMPONENT-PROTOTYPE": ["SHORT-NAME", "TYPE-TREF"],
        "P-PORT-PROTOTYPE": ["SHORT-NAME"],
        "R-PORT-PROTOTYPE": ["SHORT-NAME"],
        "ASSEMBLY-SW-CONNECTOR": ["SHORT-NAME", "PROVIDER-IREF", "REQUESTER-IREF"],
        "DELEGATION-SW-CONNECTOR": ["SHORT-NAME", "INNER-PORT-IREF", "OUTER-PORT-IREF"],
        "COMPOSITION-SW-COMPONENT-TYPE": ["SHORT-NAME"],
    }
    
    # 有效数据类型
    VALID_DATA_TYPES = {
        "boolean", "uint8", "uint16", "uint32", "uint64",
        "sint8", "sint16", "sint32", "sint64",
        "float32", "float64"
    }

    def __init__(self, strict_mode: bool = False):
        """
        初始化分析器
        
        Args:
            strict_mode: 是否启用严格模式
        """
        self.strict_mode = strict_mode
        self.issues: List[IntegrityIssue] = []
        self.uuids: Dict[str, Tuple[str, ET.Element]] = {}  # UUID -> (路径, 元素)
        self.references: List[Tuple[str, str, str]] = []  # (引用值, 路径, 引用类型)
        self.root: Optional[ET.Element] = None
        self.file_path: Optional[str] = None
        self.rules: Dict[str, CheckRule] = {}
        
        self._init_default_rules()
    
    def _init_default_rules(self):
        """初始化默认检查规则"""
        default_rules = [
            CheckRule(
                rule_id="RULE-001",
                name="Required Element Check",
                description="检查必需元素是否存在",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.ERROR
            ),
            CheckRule(
                rule_id="RULE-002",
                name="UUID Uniqueness Check",
                description="检查UUID是否唯一",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.ERROR
            ),
            CheckRule(
                rule_id="RULE-003",
                name="Reference Validity Check",
                description="检查引用是否有效",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.ERROR
            ),
            CheckRule(
                rule_id="RULE-004",
                name="Data Type Match Check",
                description="检查数据类型是否匹配",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.WARNING
            ),
            CheckRule(
                rule_id="RULE-005",
                name="ECU Mapping Completeness",
                description="检查ECU映射完整性",
                check_type=CheckType.SEMANTIC,
                severity=Severity.ERROR
            ),
            CheckRule(
                rule_id="RULE-006",
                name="Component Connection Integrity",
                description="检查组件连接完整性",
                check_type=CheckType.SEMANTIC,
                severity=Severity.WARNING
            ),
            CheckRule(
                rule_id="RULE-007",
                name="Interface Compatibility",
                description="检查接口兼容性",
                check_type=CheckType.SEMANTIC,
                severity=Severity.ERROR
            ),
            CheckRule(
                rule_id="RULE-008",
                name="UUID Format Check",
                description="检查UUID格式是否正确",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.WARNING,
                strict_only=True
            ),
            CheckRule(
                rule_id="RULE-009",
                name="Naming Convention Check",
                description="检查命名规范",
                check_type=CheckType.STRUCTURAL,
                severity=Severity.INFO,
                strict_only=True
            ),
            CheckRule(
                rule_id="RULE-010",
                name="Unused Element Check",
                description="检查未使用的元素",
                check_type=CheckType.SEMANTIC,
                severity=Severity.INFO,
                strict_only=True
            ),
        ]
        
        for rule in default_rules:
            self.rules[rule.rule_id] = rule
    
    def add_custom_rule(self, rule: CheckRule):
        """
        添加自定义检查规则
        
        Args:
            rule: 检查规则对象
        """
        self.rules[rule.rule_id] = rule
    
    def remove_rule(self, rule_id: str):
        """
        移除检查规则
        
        Args:
            rule_id: 规则ID
        """
        if rule_id in self.rules:
            del self.rules[rule_id]
    
    def enable_rule(self, rule_id: str, enabled: bool = True):
        """
        启用/禁用规则
        
        Args:
            rule_id: 规则ID
            enabled: 是否启用
        """
        if rule_id in self.rules:
            self.rules[rule_id].enabled = enabled
    
    def analyze(self, file_path: Union[str, Path]) -> AnalysisReport:
        """
        分析ARXML文件完整性
        
        Args:
            file_path: ARXML文件路径
            
        Returns:
            AnalysisReport: 分析报告
        """
        self.file_path = str(file_path)
        self.issues = []
        self.uuids = {}
        self.references = []
        
        try:
            # 解析XML文件
            tree = ET.parse(file_path)
            self.root = tree.getroot()
            
            # 第一阶段：收集所有UUID和引用
            self._collect_uuids_and_references(self.root, "")
            
            # 第二阶段：执行检查
            self._run_checks()
            
        except ET.ParseError as e:
            self._add_issue(
                Severity.ERROR,
                CheckType.STRUCTURAL,
                f"XML解析错误: {str(e)}",
                "",
                rule_id="RULE-XML-001",
                suggestion="检查XML语法是否正确"
            )
        except Exception as e:
            self._add_issue(
                Severity.ERROR,
                CheckType.STRUCTURAL,
                f"分析过程中发生错误: {str(e)}",
                "",
                rule_id="RULE-SYS-001",
                suggestion="检查文件是否存在且有读取权限"
            )
        
        # 生成报告
        return self._generate_report()
    
    def _collect_uuids_and_references(self, element: ET.Element, path: str):
        """
        收集UUID和引用信息
        
        Args:
            element: 当前元素
            path: 当前路径
        """
        tag = self._get_tag_name(element)
        current_path = f"{path}/{tag}" if path else tag
        
        # 收集UUID
        uuid_attr = element.get("UUID")
        if uuid_attr:
            if uuid_attr in self.uuids:
                existing_path, _ = self.uuids[uuid_attr]
                self._add_issue(
                    Severity.ERROR,
                    CheckType.STRUCTURAL,
                    f"重复的UUID: {uuid_attr}",
                    current_path,
                    element_uuid=uuid_attr,
                    rule_id="RULE-002",
                    suggestion=f"该UUID已存在于: {existing_path}"
                )
            else:
                self.uuids[uuid_attr] = (current_path, element)
        
        # 收集引用 (以-TREF结尾的属性或DEST-REF元素)
        for attr_name, attr_value in element.attrib.items():
            if attr_name.endswith("-TREF") or attr_name.endswith("-REF"):
                self.references.append((attr_value, current_path, attr_name))
        
        # 处理DEST-REF元素
        if tag.endswith("-REF") and element.get("DEST"):
            ref_text = element.text.strip() if element.text else ""
            if ref_text:
                self.references.append((ref_text, current_path, tag))
        
        # 递归处理子元素
        for child in element:
            self._collect_uuids_and_references(child, current_path)
    
    def _run_checks(self):
        """运行所有启用的检查规则"""
        for rule_id, rule in self.rules.items():
            if not rule.enabled:
                continue
            if rule.strict_only and not self.strict_mode:
                continue
            
            # 执行内置检查
            if rule.rule_id == "RULE-001":
                self._check_required_elements()
            elif rule.rule_id == "RULE-002":
                self._check_uuid_uniqueness()
            elif rule.rule_id == "RULE-003":
                self._check_references()
            elif rule.rule_id == "RULE-004":
                self._check_data_types()
            elif rule.rule_id == "RULE-005":
                self._check_ecu_mapping()
            elif rule.rule_id == "RULE-006":
                self._check_component_connections()
            elif rule.rule_id == "RULE-007":
                self._check_interface_compatibility()
            elif rule.rule_id == "RULE-008":
                self._check_uuid_format()
            elif rule.rule_id == "RULE-009":
                self._check_naming_convention()
            elif rule.rule_id == "RULE-010":
                self._check_unused_elements()
            
            # 执行自定义检查
            elif rule.custom_checker and self.root is not None:
                issues = rule.custom_checker(self.root, "")
                for issue in issues:
                    issue.rule_id = rule_id
                    self.issues.append(issue)
    
    def _check_required_elements(self):
        """检查必需元素"""
        if self.root is None:
            return
        
        def check_element(element: ET.Element, path: str):
            tag = self._get_tag_name(element)
            current_path = f"{path}/{tag}" if path else tag
            
            # 检查必需元素
            if tag in self.REQUIRED_ELEMENTS:
                required_children = self.REQUIRED_ELEMENTS[tag]
                child_tags = {self._get_tag_name(child) for child in element}
                
                for required in required_children:
                    if required not in child_tags:
                        self._add_issue(
                            Severity.ERROR,
                            CheckType.STRUCTURAL,
                            f"缺少必需元素: {required}",
                            current_path,
                            element_uuid=element.get("UUID"),
                            rule_id="RULE-001",
                            suggestion=f"添加<{required}>元素"
                        )
            
            # 递归检查
            for child in element:
                check_element(child, current_path)
        
        check_element(self.root, "")
    
    def _check_uuid_uniqueness(self):
        """检查UUID唯一性 (已在_collect_uuids_and_references中处理)"""
        pass
    
    def _check_references(self):
        """检查引用有效性"""
        for ref_value, ref_path, ref_type in self.references:
            # 检查是否为UUID引用
            if ref_value in self.uuids:
                continue
            
            # 检查是否为路径引用
            path_found = any(
                ref_value in uuid_path or uuid_path.endswith(ref_value)
                for uuid_path, _ in self.uuids.values()
            )
            
            if not path_found:
                # 提取短名称进行匹配
                short_name = ref_value.split("/")[-1] if "/" in ref_value else ref_value
                if not any(
                    short_name in self._get_tag_name(elem) or 
                    (elem.text and short_name in elem.text)
                    for _, elem in self.uuids.values()
                ):
                    severity = Severity.WARNING if not self.strict_mode else Severity.ERROR
                    self._add_issue(
                        severity,
                        CheckType.STRUCTURAL,
                        f"无法解析的引用: {ref_value} ({ref_type})",
                        ref_path,
                        rule_id="RULE-003",
                        suggestion="检查引用路径或UUID是否正确"
                    )
    
    def _check_data_types(self):
        """检查数据类型匹配"""
        if self.root is None:
            return
        
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            
            # 检查数据类型定义
            if tag == "IMPLEMENTATION-DATA-TYPE":
                type_name = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                if type_name is not None and type_name.text:
                    base_type = type_name.text.lower()
                    if base_type not in self.VALID_DATA_TYPES:
                        # 允许自定义类型，但发出警告
                        if self.strict_mode:
                            self._add_issue(
                                Severity.WARNING,
                                CheckType.STRUCTURAL,
                                f"非标准数据类型: {type_name.text}",
                                self._get_element_path(elem),
                                rule_id="RULE-004",
                                suggestion=f"建议使用标准类型: {', '.join(self.VALID_DATA_TYPES)}"
                            )
    
    def _check_ecu_mapping(self):
        """检查ECU映射完整性"""
        if self.root is None:
            return
        
        # 查找ECU映射
        ecu_mappings = []
        swc_to_ecu: Dict[str, str] = {}
        
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            
            if tag == "SWC-TO-ECU-MAPPING":
                component_ref = elem.find(f".//{{{self.AUTOSAR_NS}}}COMPONENT-REF")
                ecu_ref = elem.find(f".//{{{self.AUTOSAR_NS}}}ECU-REF")
                
                if component_ref is None:
                    self._add_issue(
                        Severity.ERROR,
                        CheckType.SEMANTIC,
                        "SWC-TO-ECU-MAPPING缺少COMPONENT-REF",
                        self._get_element_path(elem),
                        rule_id="RULE-005",
                        suggestion="添加组件引用"
                    )
                
                if ecu_ref is None:
                    self._add_issue(
                        Severity.ERROR,
                        CheckType.SEMANTIC,
                        "SWC-TO-ECU-MAPPING缺少ECU-REF",
                        self._get_element_path(elem),
                        rule_id="RULE-005",
                        suggestion="添加ECU引用"
                    )
                
                if component_ref is not None and ecu_ref is not None:
                    comp_ref_text = component_ref.text or ""
                    if comp_ref_text in swc_to_ecu:
                        self._add_issue(
                            Severity.WARNING,
                            CheckType.SEMANTIC,
                            f"组件被映射到多个ECU: {comp_ref_text}",
                            self._get_element_path(elem),
                            rule_id="RULE-005",
                            suggestion="检查映射配置，一个组件通常只映射到一个ECU"
                        )
                    swc_to_ecu[comp_ref_text] = ecu_ref.text or ""
        
        # 检查所有SWC是否都有映射
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            if tag in ("APPLICATION-SW-COMPONENT-TYPE", "COMPOSITION-SW-COMPONENT-TYPE"):
                short_name = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                if short_name is not None and short_name.text:
                    swc_name = short_name.text
                    found = any(swc_name in ref for ref in swc_to_ecu.keys())
                    if not found:
                        self._add_issue(
                            Severity.WARNING,
                            CheckType.SEMANTIC,
                            f"组件未映射到ECU: {swc_name}",
                            self._get_element_path(elem),
                            element_uuid=elem.get("UUID"),
                            rule_id="RULE-005",
                            suggestion="添加SWC-TO-ECU-MAPPING配置"
                        )
    
    def _check_component_connections(self):
        """检查组件连接完整性"""
        if self.root is None:
            return
        
        # 收集所有端口
        ports: Dict[str, Tuple[str, str]] = {}  # 端口路径 -> (组件名, 端口类型)
        
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            if tag in ("P-PORT-PROTOTYPE", "R-PORT-PROTOTYPE"):
                short_name = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                if short_name is not None:
                    port_path = self._get_element_path(elem)
                    port_type = "P" if tag == "P-PORT-PROTOTYPE" else "R"
                    ports[port_path] = (short_name.text or "", port_type)
        
        # 检查连接器
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            if tag in ("ASSEMBLY-SW-CONNECTOR", "DELEGATION-SW-CONNECTOR"):
                if tag == "ASSEMBLY-SW-CONNECTOR":
                    provider = elem.find(f".//{{{self.AUTOSAR_NS}}}PROVIDER-IREF")
                    requester = elem.find(f".//{{{self.AUTOSAR_NS}}}REQUESTER-IREF")
                    
                    if provider is None:
                        self._add_issue(
                            Severity.ERROR,
                            CheckType.SEMANTIC,
                            "ASSEMBLY-SW-CONNECTOR缺少PROVIDER-IREF",
                            self._get_element_path(elem),
                            rule_id="RULE-006"
                        )
                    
                    if requester is None:
                        self._add_issue(
                            Severity.ERROR,
                            CheckType.SEMANTIC,
                            "ASSEMBLY-SW-CONNECTOR缺少REQUESTER-IREF",
                            self._get_element_path(elem),
                            rule_id="RULE-006"
                        )
    
    def _check_interface_compatibility(self):
        """检查接口兼容性"""
        if self.root is None:
            return
        
        # 收集接口定义
        interfaces: Dict[str, ET.Element] = {}
        
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            if tag in ("SENDER-RECEIVER-INTERFACE", "CLIENT-SERVER-INTERFACE"):
                short_name = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                if short_name is not None and short_name.text:
                    interfaces[short_name.text] = elem
        
        # 检查端口接口引用
        for elem in self.root.iter():
            tag = self._get_tag_name(elem)
            if tag in ("P-PORT-PROTOTYPE", "R-PORT-PROTOTYPE"):
                interface_ref = elem.find(f".//{{{self.AUTOSAR_NS}}}PROVIDED-INTERFACE-TREF")
                if interface_ref is None:
                    interface_ref = elem.find(f".//{{{self.AUTOSAR_NS}}}REQUIRED-INTERFACE-TREF")
                
                if interface_ref is not None:
                    ref_text = interface_ref.text
                    if ref_text and not any(ref_text.endswith(name) for name in interfaces.keys()):
                        self._add_issue(
                            Severity.ERROR,
                            CheckType.SEMANTIC,
                            f"引用的接口不存在: {ref_text}",
                            self._get_element_path(elem),
                            rule_id="RULE-007",
                            suggestion="检查接口定义是否存在"
                        )
    
    def _check_uuid_format(self):
        """检查UUID格式 (严格模式)"""
        if not self.strict_mode:
            return
        
        uuid_pattern = re.compile(
            r'^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$'
        )
        
        for uuid_val, (path, _) in self.uuids.items():
            if not uuid_pattern.match(uuid_val):
                self._add_issue(
                    Severity.WARNING,
                    CheckType.STRUCTURAL,
                    f"UUID格式不符合标准: {uuid_val}",
                    path,
                    element_uuid=uuid_val,
                    rule_id="RULE-008",
                    suggestion="使用标准UUID格式 (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)"
                )
    
    def _check_naming_convention(self):
        """检查命名规范 (严格模式)"""
        if not self.strict_mode:
            return
        
        # AutoSAR短名称规范: 字母数字下划线，不以数字开头
        name_pattern = re.compile(r'^[a-zA-Z][a-zA-Z0-9_]*$')
        
        for elem in self.root.iter() if self.root else []:
            tag = self._get_tag_name(elem)
            if tag == "SHORT-NAME" and elem.text:
                if not name_pattern.match(elem.text):
                    self._add_issue(
                        Severity.INFO,
                        CheckType.STRUCTURAL,
                        f"短名称不符合AutoSAR命名规范: {elem.text}",
                        self._get_element_path(elem),
                        rule_id="RULE-009",
                        suggestion="使用字母开头，仅包含字母、数字和下划线"
                    )
    
    def _check_unused_elements(self):
        """检查未使用的元素 (严格模式)"""
        if not self.strict_mode:
            return
        
        # 收集所有被引用的UUID
        referenced_uuids = set()
        for ref_value, _, _ in self.references:
            if ref_value in self.uuids:
                referenced_uuids.add(ref_value)
        
        # 检查未被引用的重要元素
        important_types = {
            "APPLICATION-SW-COMPONENT-TYPE",
            "COMPOSITION-SW-COMPONENT-TYPE",
            "SENDER-RECEIVER-INTERFACE",
            "CLIENT-SERVER-INTERFACE"
        }
        
        for uuid_val, (path, elem) in self.uuids.items():
            tag = self._get_tag_name(elem)
            if tag in important_types and uuid_val not in referenced_uuids:
                short_name = elem.find(f".//{{{self.AUTOSAR_NS}}}SHORT-NAME")
                name = short_name.text if short_name is not None else "unknown"
                self._add_issue(
                    Severity.INFO,
                    CheckType.SEMANTIC,
                    f"元素未被引用: {name} ({tag})",
                    path,
                    element_uuid=uuid_val,
                    rule_id="RULE-010",
                    suggestion="检查是否需要删除未使用的元素"
                )
    
    def _add_issue(self, severity: Severity, check_type: CheckType, 
                   message: str, element_path: str, element_uuid: Optional[str] = None,
                   suggestion: Optional[str] = None, rule_id: Optional[str] = None):
        """
        添加问题记录
        
        Args:
            severity: 严重程度
            check_type: 检查类型
            message: 问题描述
            element_path: 元素路径
            element_uuid: 元素UUID
            suggestion: 建议
            rule_id: 规则ID
        """
        issue = IntegrityIssue(
            severity=severity,
            check_type=check_type,
            message=message,
            element_path=element_path,
            element_uuid=element_uuid,
            suggestion=suggestion,
            rule_id=rule_id
        )
        self.issues.append(issue)
    
    def _generate_report(self) -> AnalysisReport:
        """
        生成分析报告
        
        Returns:
            AnalysisReport: 分析报告
        """
        error_count = sum(1 for i in self.issues if i.severity == Severity.ERROR)
        warning_count = sum(1 for i in self.issues if i.severity == Severity.WARNING)
        info_count = sum(1 for i in self.issues if i.severity == Severity.INFO)
        
        # 按严重程度排序
        severity_order = {Severity.ERROR: 0, Severity.WARNING: 1, Severity.INFO: 2}
        sorted_issues = sorted(self.issues, key=lambda x: severity_order[x.severity])
        
        # 生成统计摘要
        structural_issues = sum(1 for i in self.issues if i.check_type == CheckType.STRUCTURAL)
        semantic_issues = sum(1 for i in self.issues if i.check_type == CheckType.SEMANTIC)
        custom_issues = sum(1 for i in self.issues if i.check_type == CheckType.CUSTOM)
        
        summary = {
            "structural_issues": structural_issues,
            "semantic_issues": semantic_issues,
            "custom_issues": custom_issues,
            "total_elements_analyzed": len(self.uuids),
            "total_references_checked": len(self.references),
            "analysis_mode": "strict" if self.strict_mode else "loose",
            "rules_enabled": sum(1 for r in self.rules.values() if r.enabled)
        }
        
        return AnalysisReport(
            file_path=self.file_path or "",
            analysis_time=datetime.now().isoformat(),
            mode="strict" if self.strict_mode else "loose",
            total_issues=len(self.issues),
            error_count=error_count,
            warning_count=warning_count,
            info_count=info_count,
            issues=sorted_issues,
            summary=summary
        )
    
    def _get_tag_name(self, element: ET.Element) -> str:
        """
        获取元素标签名（去掉命名空间）
        
        Args:
            element: XML元素
            
        Returns:
            str: 标签名
        """
        tag = element.tag
        if "}" in tag:
            return tag.split("}")[1]
        return tag
    
    def _get_element_path(self, element: ET.Element) -> str:
        """
        获取元素的完整路径
        
        Args:
            element: XML元素
            
        Returns:
            str: 元素路径
        """
        path_parts = []
        current = element
        
        while current is not None:
            tag = self._get_tag_name(current)
            short_name = current.find(f"{{{self.AUTOSAR_NS}}}SHORT-NAME")
            if short_name is not None and short_name.text:
                path_parts.append(f"{tag}[{short_name.text}]")
            else:
                path_parts.append(tag)
            
            # 获取父元素
            parent_map = {c: p for p in self.root.iter() for c in p} if self.root else {}
            current = parent_map.get(current)
        
        return "/".join(reversed(path_parts))


class ReportGenerator:
    """报告生成器"""
    
    @staticmethod
    def to_json(report: AnalysisReport, output_path: Optional[str] = None) -> str:
        """
        生成JSON格式报告
        
        Args:
            report: 分析报告
            output_path: 输出文件路径
            
        Returns:
            str: JSON字符串
        """
        json_str = json.dumps(report.to_dict(), indent=2, ensure_ascii=False)
        
        if output_path:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(json_str)
        
        return json_str
    
    @staticmethod
    def to_markdown(report: AnalysisReport, output_path: Optional[str] = None) -> str:
        """
        生成Markdown格式报告
        
        Args:
            report: 分析报告
            output_path: 输出文件路径
            
        Returns:
            str: Markdown字符串
        """
        lines = [
            "# ARXML完整性分析报告",
            "",
            "## 概览",
            "",
            f"- **分析文件**: `{report.file_path}`",
            f"- **分析时间**: {report.analysis_time}",
            f"- **分析模式**: {report.mode}",
            f"- **问题总数**: {report.total_issues}",
            f"- **错误数**: {report.error_count}",
            f"- **警告数**: {report.warning_count}",
            f"- **信息数**: {report.info_count}",
            "",
            "## 统计摘要",
            ""
        ]
        
        for key, value in report.summary.items():
            lines.append(f"- **{key}**: {value}")
        
        lines.extend([
            "",
            "## 问题详情",
            ""
        ])
        
        if not report.issues:
            lines.append("✅ 未发现任何问题")
        else:
            for i, issue in enumerate(report.issues, 1):
                severity_emoji = {
                    Severity.ERROR: "❌",
                    Severity.WARNING: "⚠️",
                    Severity.INFO: "ℹ️"
                }.get(issue.severity, "•")
                
                lines.extend([
                    f"### {i}. {severity_emoji} {issue.message}",
                    "",
                    f"- **严重程度**: {issue.severity.value}",
                    f"- **检查类型**: {issue.check_type.value}",
                    f"- **元素路径**: `{issue.element_path}`",
                ])
                
                if issue.element_uuid:
                    lines.append(f"- **元素UUID**: `{issue.element_uuid}`")
                if issue.rule_id:
                    lines.append(f"- **规则ID**: `{issue.rule_id}`")
                if issue.suggestion:
                    lines.append(f"- **建议**: {issue.suggestion}")
                
                lines.append("")
        
        md_str = "\n".join(lines)
        
        if output_path:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(md_str)
        
        return md_str
    
    @staticmethod
    def to_console(report: AnalysisReport) -> str:
        """
        生成控制台格式报告
        
        Args:
            report: 分析报告
            
        Returns:
            str: 控制台格式字符串
        """
        lines = [
            "=" * 60,
            "ARXML Integrity Analysis Report",
            "=" * 60,
            f"File: {report.file_path}",
            f"Time: {report.analysis_time}",
            f"Mode: {report.mode}",
            "-" * 60,
            f"Total Issues: {report.total_issues}",
            f"  Errors:   {report.error_count}",
            f"  Warnings: {report.warning_count}",
            f"  Info:     {report.info_count}",
            "=" * 60,
            ""
        ]
        
        if not report.issues:
            lines.append("✓ No issues found")
        else:
            for i, issue in enumerate(report.issues, 1):
                prefix = {
                    Severity.ERROR: "[ERROR]",
                    Severity.WARNING: "[WARN] ",
                    Severity.INFO: "[INFO] "
                }.get(issue.severity, "[?]    ")
                
                lines.append(f"{i}. {prefix} {issue.message}")
                lines.append(f"   Path: {issue.element_path}")
                if issue.suggestion:
                    lines.append(f"   Suggestion: {issue.suggestion}")
                lines.append("")
        
        return "\n".join(lines)


def main():
    """主函数 - 命令行接口"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="ARXML完整性分析器",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s input.arxml                    # 基本分析
  %(prog)s input.arxml --strict           # 严格模式分析
  %(prog)s input.arxml -o report.json     # 输出JSON报告
  %(prog)s input.arxml -o report.md       # 输出Markdown报告
        """
    )
    
    parser.add_argument("input", help="输入ARXML文件路径")
    parser.add_argument("-o", "--output", help="输出报告文件路径")
    parser.add_argument("--strict", action="store_true", help="启用严格模式")
    parser.add_argument("--format", choices=["json", "md", "markdown", "console"],
                       default="console", help="输出格式")
    parser.add_argument("--quiet", "-q", action="store_true", help="静默模式")
    
    args = parser.parse_args()
    
    # 创建分析器
    analyzer = IntegrityAnalyzer(strict_mode=args.strict)
    
    # 执行分析
    report = analyzer.analyze(args.input)
    
    # 生成报告
    generator = ReportGenerator()
    
    if args.output:
        if args.output.endswith('.json') or args.format == 'json':
            generator.to_json(report, args.output)
        else:
            generator.to_markdown(report, args.output)
        if not args.quiet:
            print(f"Report saved to: {args.output}")
    else:
        print(generator.to_console(report))
    
    # 返回退出码
    return 1 if report.error_count > 0 else 0


if __name__ == "__main__":
    exit(main())
