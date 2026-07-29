#!/usr/bin/env python3
"""
完整性分析器测试模块

测试内容:
1. 规则检查
2. 问题检测
3. 报告生成
4. 自定义规则
"""

import sys
import os
import tempfile
from pathlib import Path

# 添加src目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

import pytest
from integrity_analyzer import (
    IntegrityAnalyzer, IntegrityIssue, CheckRule, AnalysisReport,
    Severity, CheckType, ReportGenerator
)


class TestSeverity:
    """测试严重程度枚举"""
    
    def test_severity_values(self):
        """测试严重程度值"""
        assert Severity.ERROR.value == "error"
        assert Severity.WARNING.value == "warning"
        assert Severity.INFO.value == "info"


class TestCheckType:
    """测试检查类型枚举"""
    
    def test_check_type_values(self):
        """测试检查类型值"""
        assert CheckType.STRUCTURAL.value == "structural"
        assert CheckType.SEMANTIC.value == "semantic"
        assert CheckType.CUSTOM.value == "custom"


class TestIntegrityIssue:
    """测试完整性问题类"""
    
    def test_issue_creation(self):
        """测试创建问题"""
        issue = IntegrityIssue(
            severity=Severity.ERROR,
            check_type=CheckType.STRUCTURAL,
            message="Test error message",
            element_path="/AUTOSAR/Test",
            element_uuid="test-uuid-001",
            suggestion="Fix this issue",
            rule_id="RULE-TEST-001"
        )
        
        assert issue.severity == Severity.ERROR
        assert issue.check_type == CheckType.STRUCTURAL
        assert issue.message == "Test error message"
        assert issue.element_path == "/AUTOSAR/Test"
        assert issue.element_uuid == "test-uuid-001"
        assert issue.suggestion == "Fix this issue"
        assert issue.rule_id == "RULE-TEST-001"
    
    def test_issue_to_dict(self):
        """测试转换为dict"""
        issue = IntegrityIssue(
            severity=Severity.WARNING,
            check_type=CheckType.SEMANTIC,
            message="Test warning",
            element_path="/Test"
        )
        
        d = issue.to_dict()
        assert d["severity"] == "warning"
        assert d["check_type"] == "semantic"
        assert d["message"] == "Test warning"
        assert d["element_path"] == "/Test"


class TestCheckRule:
    """测试检查规则类"""
    
    def test_rule_creation(self):
        """测试创建规则"""
        rule = CheckRule(
            rule_id="RULE-001",
            name="Test Rule",
            description="A test rule",
            check_type=CheckType.STRUCTURAL,
            severity=Severity.ERROR
        )
        
        assert rule.rule_id == "RULE-001"
        assert rule.name == "Test Rule"
        assert rule.description == "A test rule"
        assert rule.check_type == CheckType.STRUCTURAL
        assert rule.severity == Severity.ERROR
        assert rule.enabled is True
        assert rule.strict_only is False


class TestAnalysisReport:
    """测试分析报告类"""
    
    def test_report_creation(self):
        """测试创建报告"""
        report = AnalysisReport(
            file_path="test.arxml",
            analysis_time="2024-01-01T00:00:00",
            mode="strict"
        )
        
        assert report.file_path == "test.arxml"
        assert report.analysis_time == "2024-01-01T00:00:00"
        assert report.mode == "strict"
        assert report.total_issues == 0
        assert report.error_count == 0
        assert report.warning_count == 0
        assert report.info_count == 0
        assert report.issues == []
    
    def test_report_with_issues(self):
        """测试带问题的报告"""
        issues = [
            IntegrityIssue(Severity.ERROR, CheckType.STRUCTURAL, "Error", "/Test1"),
            IntegrityIssue(Severity.WARNING, CheckType.SEMANTIC, "Warning", "/Test2"),
            IntegrityIssue(Severity.INFO, CheckType.CUSTOM, "Info", "/Test3")
        ]
        
        report = AnalysisReport(
            file_path="test.arxml",
            analysis_time="2024-01-01T00:00:00",
            mode="loose",
            total_issues=len(issues),
            error_count=1,
            warning_count=1,
            info_count=1,
            issues=issues
        )
        
        assert report.total_issues == 3
        assert report.error_count == 1
        assert report.warning_count == 1
        assert report.info_count == 1


class TestIntegrityAnalyzer:
    """测试完整性分析器"""
    
    @pytest.fixture
    def valid_arxml(self):
        """有效的ARXML内容"""
        return '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
          <AR-PACKAGES>
            <AR-PACKAGE UUID="pkg-001">
              <SHORT-NAME>TestPackage</SHORT-NAME>
              <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>TestComponent</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
              </ELEMENTS>
            </AR-PACKAGE>
          </AR-PACKAGES>
        </AUTOSAR>'''
    
    @pytest.fixture
    def invalid_arxml(self):
        """无效的ARXML内容（包含错误）"""
        return '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
          <AR-PACKAGES>
            <!-- 缺少SHORT-NAME -->
            <AR-PACKAGE UUID="pkg-001">
              <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>TestComponent</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
                <!-- 重复UUID -->
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>AnotherComponent</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
              </ELEMENTS>
            </AR-PACKAGE>
          </AR-PACKAGES>
        </AUTOSAR>'''
    
    def test_analyzer_initialization(self):
        """测试分析器初始化"""
        analyzer = IntegrityAnalyzer(strict_mode=False)
        assert analyzer.strict_mode is False
        assert len(analyzer.rules) == 10  # 默认10条规则
        
        analyzer_strict = IntegrityAnalyzer(strict_mode=True)
        assert analyzer_strict.strict_mode is True
    
    def test_analyze_valid_arxml(self, tmp_path, valid_arxml):
        """测试分析有效ARXML"""
        arxml_file = tmp_path / "valid.arxml"
        arxml_file.write_text(valid_arxml)
        
        analyzer = IntegrityAnalyzer()
        report = analyzer.analyze(arxml_file)
        
        assert report.file_path == str(arxml_file)
        assert report.mode == "loose"
        # 宽松模式下可能有一些警告，但不应该有错误
        assert report.error_count == 0
    
    def test_analyze_invalid_arxml(self, tmp_path, invalid_arxml):
        """测试分析无效ARXML"""
        arxml_file = tmp_path / "invalid.arxml"
        arxml_file.write_text(invalid_arxml)
        
        analyzer = IntegrityAnalyzer()
        report = analyzer.analyze(arxml_file)
        
        # 应该检测到重复UUID
        assert report.total_issues > 0
        assert any("UUID" in issue.message for issue in report.issues)
    
    def test_analyze_xml_parse_error(self, tmp_path):
        """测试XML解析错误"""
        arxml_file = tmp_path / "invalid.xml"
        arxml_file.write_text("<invalid>xml content")
        
        analyzer = IntegrityAnalyzer()
        report = analyzer.analyze(arxml_file)
        
        assert report.error_count > 0
        assert any("XML解析" in issue.message or "XML" in issue.message 
                   for issue in report.issues)
    
    def test_custom_rule(self, tmp_path, valid_arxml):
        """测试自定义规则"""
        arxml_file = tmp_path / "valid.arxml"
        arxml_file.write_text(valid_arxml)
        
        analyzer = IntegrityAnalyzer()
        
        # 添加自定义规则
        def custom_checker(root, path):
            return [IntegrityIssue(
                severity=Severity.INFO,
                check_type=CheckType.CUSTOM,
                message="Custom check executed",
                element_path="/AUTOSAR",
                rule_id="CUSTOM-001"
            )]
        
        custom_rule = CheckRule(
            rule_id="CUSTOM-001",
            name="Custom Test Rule",
            description="A custom rule for testing",
            check_type=CheckType.CUSTOM,
            severity=Severity.INFO,
            custom_checker=custom_checker
        )
        
        analyzer.add_custom_rule(custom_rule)
        report = analyzer.analyze(arxml_file)
        
        assert any(issue.rule_id == "CUSTOM-001" for issue in report.issues)
        assert any("Custom check" in issue.message for issue in report.issues)
    
    def test_enable_disable_rule(self):
        """测试启用/禁用规则"""
        analyzer = IntegrityAnalyzer()
        
        # 禁用规则
        analyzer.enable_rule("RULE-001", False)
        assert analyzer.rules["RULE-001"].enabled is False
        
        # 启用规则
        analyzer.enable_rule("RULE-001", True)
        assert analyzer.rules["RULE-001"].enabled is True
    
    def test_remove_rule(self):
        """测试移除规则"""
        analyzer = IntegrityAnalyzer()
        
        assert "RULE-001" in analyzer.rules
        analyzer.remove_rule("RULE-001")
        assert "RULE-001" not in analyzer.rules


class TestReportGenerator:
    """测试报告生成器"""
    
    @pytest.fixture
    def sample_report(self):
        """创建示例报告"""
        issues = [
            IntegrityIssue(
                severity=Severity.ERROR,
                check_type=CheckType.STRUCTURAL,
                message="Duplicate UUID",
                element_path="/AUTOSAR/PACKAGE[Test]/COMPONENT[TestComponent]",
                element_uuid="duplicate-uuid",
                suggestion="Use unique UUIDs",
                rule_id="RULE-002"
            ),
            IntegrityIssue(
                severity=Severity.WARNING,
                check_type=CheckType.SEMANTIC,
                message="Component not mapped",
                element_path="/AUTOSAR/PACKAGE[Test]/COMPONENT[Unmapped]",
                suggestion="Add SWC-TO-ECU-MAPPING"
            ),
            IntegrityIssue(
                severity=Severity.INFO,
                check_type=CheckType.STRUCTURAL,
                message="Naming convention issue",
                element_path="/AUTOSAR/PACKAGE[Test]/COMPONENT[123Name]"
            )
        ]
        
        return AnalysisReport(
            file_path="test.arxml",
            analysis_time="2024-01-01T12:00:00",
            mode="strict",
            total_issues=len(issues),
            error_count=1,
            warning_count=1,
            info_count=1,
            issues=issues,
            summary={
                "structural_issues": 2,
                "semantic_issues": 1,
                "total_elements": 10
            }
        )
    
    def test_to_json(self, sample_report, tmp_path):
        """测试JSON报告生成"""
        output_file = tmp_path / "report.json"
        json_str = ReportGenerator.to_json(sample_report, str(output_file))
        
        assert output_file.exists()
        assert "test.arxml" in json_str
        assert "error" in json_str
        assert "Duplicate UUID" in json_str
        
        # 验证JSON格式
        import json
        data = json.loads(json_str)
        assert data["file_path"] == "test.arxml"
        assert data["total_issues"] == 3
        assert len(data["issues"]) == 3
    
    def test_to_markdown(self, sample_report, tmp_path):
        """测试Markdown报告生成"""
        output_file = tmp_path / "report.md"
        md_str = ReportGenerator.to_markdown(sample_report, str(output_file))
        
        assert output_file.exists()
        assert "# ARXML完整性分析报告" in md_str
        assert "test.arxml" in md_str
        assert "Duplicate UUID" in md_str
        assert "3" in md_str  # total issues
    
    def test_to_console(self, sample_report):
        """测试控制台报告生成"""
        console_str = ReportGenerator.to_console(sample_report)
        
        assert "ARXML Integrity Analysis Report" in console_str
        assert "test.arxml" in console_str
        assert "Total Issues: 3" in console_str
        assert "Errors:   1" in console_str
        assert "Warnings: 1" in console_str
        assert "Info:     1" in console_str


class TestIntegrityAnalyzerStrictMode:
    """严格模式下的分析器测试"""
    
    @pytest.fixture
    def arxml_with_naming_issues(self):
        """包含命名规范问题的ARXML"""
        return '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
          <AR-PACKAGES>
            <AR-PACKAGE UUID="pkg-001">
              <SHORT-NAME>TestPackage</SHORT-NAME>
              <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>123InvalidName</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
              </ELEMENTS>
            </AR-PACKAGE>
          </AR-PACKAGES>
        </AUTOSAR>'''
    
    def test_strict_mode_detects_naming_issues(self, tmp_path, arxml_with_naming_issues):
        """严格模式检测命名规范问题"""
        arxml_file = tmp_path / "naming.arxml"
        arxml_file.write_text(arxml_with_naming_issues)
        
        # 宽松模式不检测命名规范
        analyzer_loose = IntegrityAnalyzer(strict_mode=False)
        report_loose = analyzer_loose.analyze(arxml_file)
        loose_naming_issues = sum(1 for i in report_loose.issues if "命名规范" in i.message)
        
        # 严格模式检测命名规范
        analyzer_strict = IntegrityAnalyzer(strict_mode=True)
        report_strict = analyzer_strict.analyze(arxml_file)
        strict_naming_issues = sum(1 for i in report_strict.issues if "命名规范" in i.message)
        
        # 严格模式应该发现更多问题
        assert strict_naming_issues >= loose_naming_issues


class TestIntegrityAnalyzerIntegration:
    """完整性分析器集成测试"""
    
    def test_analyze_sample_files(self):
        """测试分析示例文件"""
        examples_dir = Path(__file__).parent.parent / "examples"
        
        # 检查示例文件是否存在
        simple_ecu = examples_dir / "simple_ecu.arxml"
        invalid_test = examples_dir / "invalid_test.arxml"
        
        if simple_ecu.exists():
            analyzer = IntegrityAnalyzer()
            report = analyzer.analyze(simple_ecu)
            
            assert report.file_path == str(simple_ecu)
            assert report.mode == "loose"
            print(f"\n简单ECU配置分析结果: {report.total_issues} 个问题")
            print(f"  - 错误: {report.error_count}")
            print(f"  - 警告: {report.warning_count}")
            print(f"  - 信息: {report.info_count}")
        
        if invalid_test.exists():
            analyzer = IntegrityAnalyzer()
            report = analyzer.analyze(invalid_test)
            
            assert report.total_issues > 0  # 应该发现错误
            print(f"\n无效测试文件分析结果: {report.total_issues} 个问题")
            print(f"  - 错误: {report.error_count}")
            print(f"  - 警告: {report.warning_count}")


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
