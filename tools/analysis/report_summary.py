#!/usr/bin/env python3
"""静态分析报告摘要工具"""

import json
from pathlib import Path

def main():
    report_path = Path("reports/static_analysis_report.json")
    
    if not report_path.exists():
        print("报告文件不存在")
        return
        
    with open(report_path, 'r', encoding='utf-8') as f:
        report = json.load(f)
    
    print("=" * 70)
    print("静态分析报告详情")
    print("=" * 70)
    
    print("\n【概览统计】")
    print(f"  总问题数: {report['summary']['total_issues']}")
    print(f"  错误: {report['summary']['errors']}")
    print(f"  警告: {report['summary']['warnings']}")
    print(f"  信息: {report['summary']['info']}")
    
    print("\n【代码度量】")
    metrics = report['metrics']
    print(f"  分析文件数: {metrics['total_files']}")
    print(f"  总行数: {metrics['total_lines']:,}")
    print(f"  代码行: {metrics['code_lines']:,}")
    print(f"  注释行: {metrics['comment_lines']:,}")
    print(f"  空行: {metrics['blank_lines']:,}")
    print(f"  注释率: {metrics['comment_lines']/metrics['total_lines']*100:.1f}%")
    print(f"  函数数量: {metrics['functions_count']}")
    print(f"  平均圈复杂度: {metrics['avg_complexity']:.2f}")
    print(f"  最大圈复杂度: {metrics['max_complexity']}")
    
    # 按类别统计
    print("\n【问题分类统计】")
    categories = {}
    for issue in report['issues']:
        cat = issue['category']
        if cat not in categories:
            categories[cat] = {'count': 0, 'errors': 0, 'warnings': 0, 'info': 0}
        categories[cat]['count'] += 1
        severity = issue['severity']
        if severity in categories[cat]:
            categories[cat][severity] += 1
    
    for cat, stats in sorted(categories.items()):
        print(f"  {cat.upper()}: {stats['count']} (错误:{stats['errors']}, 警告:{stats['warnings']}, 信息:{stats['info']})")
    
    # 按规则统计
    print("\n【Top 10 规则违规】")
    rule_counts = {}
    for issue in report['issues']:
        rule = issue['rule_id']
        if rule not in rule_counts:
            rule_counts[rule] = 0
        rule_counts[rule] += 1
    
    for rule, count in sorted(rule_counts.items(), key=lambda x: -x[1])[:10]:
        print(f"  {rule}: {count} 次")
    
    # 高复杂度函数
    print("\n【Top 10 高复杂度函数】")
    complex_funcs = sorted(report['complexity'], key=lambda x: -x['complexity'])[:10]
    for func in complex_funcs:
        filename = func['file_path'].split('\\')[-1]
        print(f"  {func['function_name']} ({filename}:{func['line_number']}): 复杂度 {func['complexity']}")
    
    # 问题最多的文件
    print("\n【Top 10 问题文件】")
    file_counts = {}
    for issue in report['issues']:
        filepath = issue['file_path']
        if filepath not in file_counts:
            file_counts[filepath] = 0
        file_counts[filepath] += 1
    
    for filepath, count in sorted(file_counts.items(), key=lambda x: -x[1])[:10]:
        filename = filepath.split('\\')[-1]
        print(f"  {filename}: {count} 个问题")
    
    print("\n" + "=" * 70)

if __name__ == "__main__":
    main()
