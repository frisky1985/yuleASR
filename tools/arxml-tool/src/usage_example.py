#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML完整性分析器 - 使用示例

展示如何使用IntegrityAnalyzer类进行自定义检查
"""

import xml.etree.ElementTree as ET
from integrity_analyzer import (
    IntegrityAnalyzer, 
    CheckRule, 
    IntegrityIssue,
    Severity,
    CheckType,
    ReportGenerator
)


def custom_checker_example():
    """自定义检查示例: 检查所有短名称长度不超过32个字符"""
    
    def check_short_name_length(root: ET.Element, path: str) -> list:
        issues = []
        
        for elem in root.iter():
            if elem.tag.endswith("}SHORT-NAME") and elem.text:
                if len(elem.text) > 32:
                    issues.append(IntegrityIssue(
                        severity=Severity.WARNING,
                        check_type=CheckType.CUSTOM,
                        message=f"短名称超过32个字符: {elem.text[:20]}...",
                        element_path=path,
                        suggestion="将短名称缩短到32个字符以内"
                    ))
        
        return issues
    
    return check_short_name_length


def main():
    print("=" * 60)
    print("ARXML Integrity Analyzer - 使用示例")
    print("=" * 60)
    
    # 示例1: 基本分析
    print("\n【示例1】基本分析模式")
    print("-" * 60)
    analyzer = IntegrityAnalyzer(strict_mode=False)
    report = analyzer.analyze("example.arxml")
    
    print(f"文件: {report.file_path}")
    print(f"问题总数: {report.total_issues}")
    print(f"  - 错误: {report.error_count}")
    print(f"  - 警告: {report.warning_count}")
    print(f"  - 信息: {report.info_count}")
    
    # 示例2: 严格模式分析
    print("\n【示例2】严格模式分析")
    print("-" * 60)
    strict_analyzer = IntegrityAnalyzer(strict_mode=True)
    strict_report = strict_analyzer.analyze("test_invalid.arxml")
    
    print(f"文件: {strict_report.file_path}")
    print(f"问题总数: {strict_report.total_issues}")
    print(f"结构性问题: {strict_report.summary['structural_issues']}")
    print(f"语义性问题: {strict_report.summary['semantic_issues']}")
    
    # 示例3: 自定义检查规则
    print("\n【示例3】添加自定义检查规则")
    print("-" * 60)
    
    custom_analyzer = IntegrityAnalyzer(strict_mode=False)
    
    # 创建自定义规则
    custom_rule = CheckRule(
        rule_id="CUSTOM-001",
        name="Short Name Length Check",
        description="检查短名称长度不超过32个字符",
        check_type=CheckType.CUSTOM,
        severity=Severity.WARNING,
        custom_checker=custom_checker_example()
    )
    
    # 添加自定义规则
    custom_analyzer.add_custom_rule(custom_rule)
    
    print(f"已添加自定义规则: {custom_rule.name}")
    print(f"规则ID: {custom_rule.rule_id}")
    print(f"规则描述: {custom_rule.description}")
    
    # 示例4: 禁用某些规则
    print("\n【示例4】禁用特定规则")
    print("-" * 60)
    
    filtered_analyzer = IntegrityAnalyzer(strict_mode=False)
    
    # 禁用ECU映射检查
    filtered_analyzer.enable_rule("RULE-005", enabled=False)
    
    print("已禁用规则: RULE-005 (ECU Mapping Completeness)")
    print(f"当前启用的规则数: {sum(1 for r in filtered_analyzer.rules.values() if r.enabled)}")
    
    filtered_report = filtered_analyzer.analyze("example.arxml")
    print(f"过滤后问题总数: {filtered_report.total_issues}")
    
    # 示例5: 生成报告
    print("\n【示例5】生成报告")
    print("-" * 60)
    
    generator = ReportGenerator()
    
    # 生成JSON报告
    json_report = generator.to_json(report)
    print(f"JSON报告长度: {len(json_report)} 字节")
    
    # 生成Markdown报告
    md_report = generator.to_markdown(report)
    print(f"Markdown报告长度: {len(md_report)} 字节")
    
    # 示例6: 获取详细信息
    print("\n【示例6】问题详情展示")
    print("-" * 60)
    
    if strict_report.issues:
        issue = strict_report.issues[0]
        print(f"问题1:")
        print(f"  严重程度: {issue.severity.value}")
        print(f"  检查类型: {issue.check_type.value}")
        print(f"  消息: {issue.message}")
        print(f"  路径: {issue.element_path}")
        print(f"  UUID: {issue.element_uuid}")
        print(f"  规则ID: {issue.rule_id}")
        print(f"  建议: {issue.suggestion}")
    
    print("\n" + "=" * 60)
    print("示例完成")
    print("=" * 60)


if __name__ == "__main__":
    main()
