#!/usr/bin/env python3
"""
DDS配置工具命令行入口
"""

import argparse
import sys
from pathlib import Path

from dds_config_tool import DDSConfigParser, DDSConfigValidator, DDSCodeGenerator


def main():
    parser = argparse.ArgumentParser(
        description="DDS配置工具 - 生成符合microdds API的C代码",
        prog="dds-config-tool"
    )
    
    parser.add_argument(
        "input",
        help="输入配置文件路径 (.xml 或 .json)"
    )
    
    parser.add_argument(
        "-o", "--output",
        default="./generated",
        help="输出目录 (默认: ./generated)"
    )
    
    parser.add_argument(
        "-p", "--prefix",
        default="dds",
        help="生成文件名前缀 (默认: dds)"
    )
    
    parser.add_argument(
        "-t", "--template-dir",
        help="自定义模板目录"
    )
    
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="详细输出"
    )
    
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="仅验证配置文件，不生成代码"
    )
    
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s 1.0.0"
    )
    
    args = parser.parse_args()
    
    # 检查输入文件
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"错误: 配置文件不存在: {input_path}", file=sys.stderr)
        return 1
    
    # 解析配置
    if args.verbose:
        print(f"正在解析配置文件: {input_path}")
    
    config_parser = DDSConfigParser()
    try:
        config = config_parser.parse(input_path)
    except Exception as e:
        print(f"错误: 无法解析配置文件: {e}", file=sys.stderr)
        return 1
    
    if args.verbose:
        print(f"配置名称: {config.name}")
        print(f"版本: {config.version}")
        print(f"域参与者数量: {len(config.domain_participants)}")
    
    # 验证配置
    if args.verbose:
        print("正在验证配置...")
    
    validator = DDSConfigValidator()
    is_valid, issues = validator.validate(config)
    
    # 输出验证结果
    for issue in issues:
        prefix = "错误" if issue.severity == "ERROR" else "警告"
        print(f"{prefix} [{issue.path}]: {issue.message}")
    
    if not is_valid:
        print("配置验证失败", file=sys.stderr)
        return 1
    
    print("配置验证通过")
    
    if args.validate_only:
        return 0
    
    # 生成代码
    if args.verbose:
        print(f"正在生成代码到: {args.output}")
    
    output_dir = Path(args.output)
    
    generator = DDSCodeGenerator(
        template_dir=args.template_dir if args.template_dir else None
    )
    
    try:
        generated_files = generator.generate(config, output_dir, args.prefix)
    except Exception as e:
        print(f"错误: 生成代码失败: {e}", file=sys.stderr)
        return 1
    
    print(f"
生成文件:")
    for f in generated_files:
        print(f"  - {f}")
    
    print(f"
成功生成 {len(generated_files)} 个文件")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
