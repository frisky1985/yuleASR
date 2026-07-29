#!/usr/bin/env python3
"""
yuleASR ARXML Tool - Unified CLI Entry Point

ARXML解析、配置生成和完整性分析的统一命令行工具

Usage:
    arxml-tool.py parse <file> [options]
    arxml-tool.py generate <file> [options]
    arxml-tool.py analyze <file> [options]
    arxml-tool.py validate <file> [options]

Author: YuleTech
Version: 1.0.0
"""

import sys
import os
import json
import argparse
import logging
from pathlib import Path
from typing import Optional, Dict, Any, List
from datetime import datetime

# 添加src目录到Python路径
SCRIPT_DIR = Path(__file__).parent
SRC_DIR = SCRIPT_DIR / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

# 导入功能模块
try:
    from arxml_parser import ArxmlParser, ArxmlQueryEngine
    from config_generator import ConfigGenerator, ModuleConfig, EcucContainer, EcucParameter
    from integrity_analyzer import IntegrityAnalyzer, ReportGenerator, Severity, CheckType
except ImportError as e:
    print(f"Error importing modules: {e}")
    print(f"Make sure the tool is installed correctly.")
    sys.exit(1)


# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)


class OutputFormatter:
    """统一的输出格式管理器"""
    
    @staticmethod
    def format_as_json(data: Dict[str, Any], pretty: bool = True) -> str:
        """格式化为JSON"""
        indent = 2 if pretty else None
        return json.dumps(data, indent=indent, ensure_ascii=False)
    
    @staticmethod
    def format_as_markdown(title: str, data: Dict[str, Any]) -> str:
        """格式化为Markdown"""
        lines = [
            f"# {title}",
            "",
            f"**生成时间**: {datetime.now().isoformat()}",
            "",
            "## 内容",
            "",
            "```json",
            json.dumps(data, indent=2, ensure_ascii=False),
            "```",
            ""
        ]
        return "\n".join(lines)
    
    @staticmethod
    def format_as_console(title: str, data: Dict[str, Any]) -> str:
        """格式化为控制台输出"""
        lines = [
            "=" * 60,
            title,
            "=" * 60,
            ""
        ]
        
        def format_value(v, indent=0):
            prefix = "  " * indent
            if isinstance(v, dict):
                result = []
                for key, val in v.items():
                    if isinstance(val, (dict, list)):
                        result.append(f"{prefix}{key}:")
                        result.append(format_value(val, indent + 1))
                    else:
                        result.append(f"{prefix}{key}: {val}")
                return "\n".join(result)
            elif isinstance(v, list):
                result = []
                for i, item in enumerate(v):
                    if isinstance(item, (dict, list)):
                        result.append(f"{prefix}[{i}]:")
                        result.append(format_value(item, indent + 1))
                    else:
                        result.append(f"{prefix}  - {item}")
                return "\n".join(result)
            else:
                return f"{prefix}{v}"
        
        lines.append(format_value(data))
        lines.append("")
        lines.append("=" * 60)
        return "\n".join(lines)


def load_config(config_path: Optional[str]) -> Dict[str, Any]:
    """加载配置文件"""
    if not config_path:
        return {}
    
    config_file = Path(config_path)
    if not config_file.exists():
        logger.warning(f"Config file not found: {config_path}")
        return {}
    
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        logger.error(f"Invalid JSON in config file: {e}")
        return {}


def write_output(content: str, output_path: Optional[str] = None) -> bool:
    """写入输出文件或打印到控制台"""
    if output_path:
        try:
            output_file = Path(output_path)
            output_file.parent.mkdir(parents=True, exist_ok=True)
            output_file.write_text(content, encoding='utf-8')
            logger.info(f"Output written to: {output_path}")
            return True
        except Exception as e:
            logger.error(f"Failed to write output: {e}")
            return False
    else:
        print(content)
        return True


def cmd_parse(args) -> int:
    """解析ARXML命令"""
    input_file = Path(args.input)
    if not input_file.exists():
        logger.error(f"Input file not found: {input_file}")
        return 1
    
    logger.info(f"Parsing ARXML file: {input_file}")
    
    try:
        # 创建解析器
        parser = ArxmlParser(schema_path=args.schema)
        parser.parse(input_file)
        
        # 构建输出数据
        data = {
            "file": str(input_file),
            "summary": parser.get_summary(),
            "software_components": [
                {
                    "name": swc.name,
                    "type": swc.component_type,
                    "ports": [
                        {
                            "name": p.name,
                            "type": p.port_type,
                            "interface": p.interface_name
                        }
                        for p in swc.ports
                    ]
                }
                for swc in parser.get_software_components()
            ],
            "interfaces": [
                {
                    "name": iface.name,
                    "type": iface.interface_type.value,
                    "data_elements": [
                        {"name": de.name, "type": de.type_ref}
                        for de in iface.data_elements
                    ]
                }
                for iface in parser.get_interfaces()
            ],
            "data_types": [
                {"name": dt.name, "category": dt.type_category}
                for dt in parser.get_data_types()
            ],
            "ecu_configurations": [
                {"name": ecu.name, "id": ecu.ecu_id, "modules": ecu.modules}
                for ecu in parser.get_ecu_configurations()
            ]
        }
        
        # 添加查询结果（如果指定了查询）
        if args.query:
            query_engine = ArxmlQueryEngine(parser)
            query_type, query_value = args.query.split(':', 1)
            
            if query_type == 'swc':
                swc = parser.find_swc_by_name(query_value)
                data['query_result'] = {"software_component": swc.__dict__ if swc else None}
            elif query_type == 'interface':
                iface = parser.find_interface_by_name(query_value)
                data['query_result'] = {"interface": iface.__dict__ if iface else None}
            elif query_type == 'connected':
                connected = query_engine.find_connected_components(query_value)
                data['query_result'] = {"connected_components": connected}
            elif query_type == 'ports':
                ports = query_engine.find_ports_with_interface(query_value)
                data['query_result'] = {"ports": ports}
        
        # 格式化输出
        formatter = OutputFormatter()
        if args.format == 'json':
            output = formatter.format_as_json(data)
        elif args.format in ('md', 'markdown'):
            output = formatter.format_as_markdown("ARXML解析报告", data)
        else:
            output = formatter.format_as_console("ARXML解析结果", data)
        
        return 0 if write_output(output, args.output) else 1
        
    except Exception as e:
        logger.error(f"Parse failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


def cmd_generate(args) -> int:
    """生成配置命令"""
    input_file = Path(args.input)
    if not input_file.exists():
        logger.error(f"Input file not found: {input_file}")
        return 1
    
    output_dir = Path(args.output) if args.output else Path("./generated")
    logger.info(f"Generating configs to: {output_dir}")
    
    try:
        # 创建生成器
        generator = ConfigGenerator(
            output_dir=output_dir,
            template_dir=Path(args.templates) if args.templates else None
        )
        
        # 根据输入格式生成
        if args.format == 'json' or input_file.suffix.lower() == '.json':
            generated = generator.generate_from_json(input_file)
        else:
            generated = generator.generate_from_arxml(input_file, args.module)
        
        # 生成结果报告
        data = {
            "input_file": str(input_file),
            "output_directory": str(output_dir),
            "generated_files": [str(f) for f in generated]
        }
        
        if args.verbose:
            formatter = OutputFormatter()
            print(formatter.format_as_console("生成结果", data))
        else:
            print(f"Generated {len(generated)} files:")
            for f in generated:
                print(f"  - {f}")
        
        return 0
        
    except Exception as e:
        logger.error(f"Generate failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


def cmd_analyze(args) -> int:
    """分析完整性命令"""
    input_file = Path(args.input)
    if not input_file.exists():
        logger.error(f"Input file not found: {input_file}")
        return 1
    
    logger.info(f"Analyzing ARXML file: {input_file}")
    
    try:
        # 创建分析器
        analyzer = IntegrityAnalyzer(strict_mode=args.strict)
        
        # 禁用指定规则
        if args.disable_rules:
            for rule_id in args.disable_rules.split(','):
                analyzer.enable_rule(rule_id.strip(), enabled=False)
        
        # 执行分析
        report = analyzer.analyze(input_file)
        
        # 生成报告
        generator = ReportGenerator()
        
        if args.output:
            if args.output.endswith('.json') or args.format == 'json':
                generator.to_json(report, args.output)
            else:
                generator.to_markdown(report, args.output)
            logger.info(f"Report saved to: {args.output}")
        else:
            if args.format == 'json':
                print(generator.to_json(report))
            elif args.format in ('md', 'markdown'):
                print(generator.to_markdown(report))
            else:
                print(generator.to_console(report))
        
        # 返回退出码
        return 1 if (report.error_count > 0 and args.fail_on_error) else 0
        
    except Exception as e:
        logger.error(f"Analysis failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


def cmd_validate(args) -> int:
    """验证Schema命令"""
    input_file = Path(args.input)
    if not input_file.exists():
        logger.error(f"Input file not found: {input_file}")
        return 1
    
    if not args.schema:
        logger.error("Schema file is required for validation")
        return 1
    
    schema_file = Path(args.schema)
    if not schema_file.exists():
        logger.error(f"Schema file not found: {schema_file}")
        return 1
    
    logger.info(f"Validating {input_file} against schema {schema_file}")
    
    try:
        # 创建解析器并验证
        parser = ArxmlParser(schema_path=str(schema_file))
        
        if args.check_wellformed:
            # 仅检查良构性
            try:
                parser.parse(input_file)
                logger.info("XML is well-formed")
                data = {
                    "valid": True,
                    "file": str(input_file),
                    "check_type": "well-formed"
                }
            except Exception as e:
                logger.error(f"XML is not well-formed: {e}")
                data = {
                    "valid": False,
                    "file": str(input_file),
                    "error": str(e),
                    "check_type": "well-formed"
                }
        else:
            # 完整schema验证
            parser.parse(input_file)
            valid = parser.validate()
            
            data = {
                "valid": valid,
                "file": str(input_file),
                "schema": str(schema_file),
                "check_type": "schema"
            }
        
        # 格式化输出
        formatter = OutputFormatter()
        if args.format == 'json':
            output = formatter.format_as_json(data)
        elif args.format in ('md', 'markdown'):
            output = formatter.format_as_markdown("ARXML验证报告", data)
        else:
            output = formatter.format_as_console("ARXML验证结果", data)
        
        return 0 if write_output(output, args.output) else 1
        
    except Exception as e:
        logger.error(f"Validation failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


def main():
    """主入口函数"""
    parser = argparse.ArgumentParser(
        prog='arxml-tool',
        description='yuleASR ARXML Tool - 统一的ARXML处理工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  # 解析ARXML并输出JSON
  arxml-tool.py parse system.arxml --format json -o output.json
  
  # 生成配置文件
  arxml-tool.py generate system.arxml -o ./generated --module Can
  
  # 分析完整性（严格模式）
  arxml-tool.py analyze system.arxml --strict --format md -o report.md
  
  # 验证Schema
  arxml-tool.py validate system.arxml --schema autosar.xsd
  
环境变量:
  ARXML_TOOL_CONFIG    默认配置文件路径
  ARXML_TOOL_SCHEMA    默认Schema文件路径
        '''
    )
    
    # 全局选项
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='启用详细输出')
    parser.add_argument('-c', '--config', 
                       default=os.environ.get('ARXML_TOOL_CONFIG'),
                       help='配置文件路径')
    
    # 子命令
    subparsers = parser.add_subparsers(dest='command', help='可用命令')
    
    # parse 命令
    parse_parser = subparsers.add_parser(
        'parse', 
        help='解析ARXML文件',
        description='解析ARXML文件并输出组件、接口、数据类型等信息'
    )
    parse_parser.add_argument('input', help='输入ARXML文件路径')
    parse_parser.add_argument('-o', '--output', help='输出文件路径')
    parse_parser.add_argument('-f', '--format', 
                             choices=['json', 'md', 'markdown', 'console'],
                             default='console',
                             help='输出格式 (默认: console)')
    parse_parser.add_argument('-s', '--schema', 
                             default=os.environ.get('ARXML_TOOL_SCHEMA'),
                             help='XSD schema文件路径（用于验证）')
    parse_parser.add_argument('-q', '--query', 
                             help='执行查询 (格式: type:value, 如 swc:MyComponent)')
    parse_parser.set_defaults(func=cmd_parse)
    
    # generate 命令
    generate_parser = subparsers.add_parser(
        'generate',
        help='生成配置文件',
        description='从ARXML或JSON生成AUTOSAR BSW配置文件'
    )
    generate_parser.add_argument('input', help='输入文件路径 (ARXML或JSON)')
    generate_parser.add_argument('-o', '--output', required=True,
                                help='输出目录路径')
    generate_parser.add_argument('-f', '--format',
                                choices=['arxml', 'json', 'auto'],
                                default='auto',
                                help='输入文件格式 (默认: auto)')
    generate_parser.add_argument('-m', '--module',
                                help='指定模块名 (仅用于ARXML)')
    generate_parser.add_argument('-t', '--templates',
                                help='自定义模板目录')
    generate_parser.set_defaults(func=cmd_generate)
    
    # analyze 命令
    analyze_parser = subparsers.add_parser(
        'analyze',
        help='分析ARXML完整性',
        description='检查ARXML文件的结构和语义完整性'
    )
    analyze_parser.add_argument('input', help='输入ARXML文件路径')
    analyze_parser.add_argument('-o', '--output', help='输出报告文件路径')
    analyze_parser.add_argument('-f', '--format',
                               choices=['json', 'md', 'markdown', 'console'],
                               default='console',
                               help='报告格式 (默认: console)')
    analyze_parser.add_argument('--strict', action='store_true',
                               help='启用严格模式')
    analyze_parser.add_argument('--disable-rules',
                               help='禁用的规则ID列表，逗号分隔')
    analyze_parser.add_argument('--fail-on-error', action='store_true',
                               help='发现错误时返回非零退出码')
    analyze_parser.set_defaults(func=cmd_analyze)
    
    # validate 命令
    validate_parser = subparsers.add_parser(
        'validate',
        help='验证ARXML Schema',
        description='验证ARXML文件是否符合XSD Schema'
    )
    validate_parser.add_argument('input', help='输入ARXML文件路径')
    validate_parser.add_argument('-s', '--schema',
                                default=os.environ.get('ARXML_TOOL_SCHEMA'),
                                help='XSD schema文件路径')
    validate_parser.add_argument('-o', '--output', help='输出文件路径')
    validate_parser.add_argument('-f', '--format',
                                choices=['json', 'md', 'markdown', 'console'],
                                default='console',
                                help='输出格式 (默认: console)')
    validate_parser.add_argument('--check-wellformed', action='store_true',
                                help='仅检查XML良构性，不进行Schema验证')
    validate_parser.set_defaults(func=cmd_validate)
    
    # 解析参数
    args = parser.parse_args()
    
    # 设置日志级别
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # 加载配置文件
    config = load_config(args.config)
    
    # 执行命令
    if hasattr(args, 'func'):
        return args.func(args)
    else:
        parser.print_help()
        return 0


if __name__ == '__main__':
    sys.exit(main())
