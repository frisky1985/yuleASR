#!/usr/bin/env python3
"""
DLT配置工具
用于生成和修改AutoSAR DLT模块配置文件

用法:
    python dlt_config_tool.py generate --output dlt_config.json
    python dlt_config_tool.py modify --input dlt_config.json --level DEBUG
"""

import json
import argparse
import sys
from dataclasses import dataclass, asdict
from typing import List, Optional
from pathlib import Path


@dataclass
class DltContextConfig:
    """DLT上下文配置"""
    app_id: str
    context_id: str
    description: str
    log_level: str = "INFO"
    trace_status: bool = True


@dataclass
class DltOutputConfig:
    """DLT输出通道配置"""
    enable_udp: bool = True
    udp_port: int = 3490
    udp_address: str = "239.255.42.99"
    enable_tcp: bool = False
    tcp_port: int = 3490
    enable_serial: bool = False
    serial_port: str = "/dev/ttyUSB0"
    enable_file: bool = True
    file_path: str = "/tmp/dlt.log"
    file_max_size: int = 10485760  # 10MB
    file_max_count: int = 5


@dataclass
class DltBufferConfig:
    """DLT缓冲区配置"""
    buffer_size: int = 65536  # 64KB
    overflow_strategy: str = "DROP_OLD"  # DROP_OLD or DROP_NEW
    flush_interval_ms: int = 100


@dataclass
class DltModuleConfig:
    """DLT模块完整配置"""
    version: str = "1.0.0"
    ecu_id: str = "ECU1"
    default_log_level: str = "INFO"
    enable_timestamp: bool = True
    enable_ecu_id: bool = True
    enable_session_id: bool = True
    enable_extended_header: bool = True
    
    contexts: List[DltContextConfig] = None
    output: DltOutputConfig = None
    buffer: DltBufferConfig = None
    
    def __post_init__(self):
        if self.contexts is None:
            self.contexts = [
                DltContextConfig("SYS", "MAIN", "System"),
                DltContextConfig("DDS", "COMM", "DDS Communication"),
                DltContextConfig("ETH", "LINK", "Ethernet Link"),
                DltContextConfig("UDS", "DIAG", "Diagnostic Services"),
            ]
        if self.output is None:
            self.output = DltOutputConfig()
        if self.buffer is None:
            self.buffer = DltBufferConfig()


def generate_default_config() -> DltModuleConfig:
    """生成默认配置"""
    return DltModuleConfig()


def config_to_dict(config: DltModuleConfig) -> dict:
    """将配置转换为字典"""
    return {
        "version": config.version,
        "ecu_id": config.ecu_id,
        "default_log_level": config.default_log_level,
        "enable_timestamp": config.enable_timestamp,
        "enable_ecu_id": config.enable_ecu_id,
        "enable_session_id": config.enable_session_id,
        "enable_extended_header": config.enable_extended_header,
        "contexts": [
            {
                "app_id": ctx.app_id,
                "context_id": ctx.context_id,
                "description": ctx.description,
                "log_level": ctx.log_level,
                "trace_status": ctx.trace_status
            }
            for ctx in config.contexts
        ],
        "output": asdict(config.output),
        "buffer": asdict(config.buffer)
    }


def dict_to_config(data: dict) -> DltModuleConfig:
    """从字典创建配置"""
    config = DltModuleConfig()
    config.version = data.get("version", "1.0.0")
    config.ecu_id = data.get("ecu_id", "ECU1")
    config.default_log_level = data.get("default_log_level", "INFO")
    config.enable_timestamp = data.get("enable_timestamp", True)
    config.enable_ecu_id = data.get("enable_ecu_id", True)
    config.enable_session_id = data.get("enable_session_id", True)
    config.enable_extended_header = data.get("enable_extended_header", True)
    
    # Parse contexts
    contexts_data = data.get("contexts", [])
    config.contexts = [
        DltContextConfig(
            ctx["app_id"],
            ctx["context_id"],
            ctx.get("description", ""),
            ctx.get("log_level", "INFO"),
            ctx.get("trace_status", True)
        )
        for ctx in contexts_data
    ]
    
    # Parse output config
    output_data = data.get("output", {})
    config.output = DltOutputConfig(
        enable_udp=output_data.get("enable_udp", True),
        udp_port=output_data.get("udp_port", 3490),
        udp_address=output_data.get("udp_address", "239.255.42.99"),
        enable_tcp=output_data.get("enable_tcp", False),
        tcp_port=output_data.get("tcp_port", 3490),
        enable_serial=output_data.get("enable_serial", False),
        serial_port=output_data.get("serial_port", "/dev/ttyUSB0"),
        enable_file=output_data.get("enable_file", True),
        file_path=output_data.get("file_path", "/tmp/dlt.log"),
        file_max_size=output_data.get("file_max_size", 10485760),
        file_max_count=output_data.get("file_max_count", 5)
    )
    
    # Parse buffer config
    buffer_data = data.get("buffer", {})
    config.buffer = DltBufferConfig(
        buffer_size=buffer_data.get("buffer_size", 65536),
        overflow_strategy=buffer_data.get("overflow_strategy", "DROP_OLD"),
        flush_interval_ms=buffer_data.get("flush_interval_ms", 100)
    )
    
    return config


def generate_c_header(config: DltModuleConfig, output_path: str):
    """生成C头文件"""
    header = f"""/*
 * @file dlt_generated_config.h
 * @brief Auto-generated DLT configuration
 * @version {config.version}
 * 
 * DO NOT EDIT - Generated by dlt_config_tool.py
 */

#ifndef DLT_GENERATED_CONFIG_H
#define DLT_GENERATED_CONFIG_H

#define DLT_ECU_ID                  "{config.ecu_id}"
#define DLT_DEFAULT_LOG_LEVEL       DLT_LOG_{config.default_log_level}
#define DLT_ENABLE_TIMESTAMP        {1 if config.enable_timestamp else 0}
#define DLT_ENABLE_ECU_ID           {1 if config.enable_ecu_id else 0}
#define DLT_ENABLE_SESSION_ID       {1 if config.enable_session_id else 0}
#define DLT_ENABLE_EXTENDED_HEADER  {1 if config.enable_extended_header else 0}

#define DLT_BUFFER_SIZE             {config.buffer.buffer_size}
#define DLT_OVERFLOW_STRATEGY       DLT_OVERFLOW_{config.buffer.overflow_strategy}
#define DLT_FLUSH_INTERVAL_MS       {config.buffer.flush_interval_ms}

#define DLT_ENABLE_UDP              {1 if config.output.enable_udp else 0}
#define DLT_UDP_PORT                {config.output.udp_port}
#define DLT_UDP_ADDRESS             "{config.output.udp_address}"

#define DLT_ENABLE_FILE             {1 if config.output.enable_file else 0}
#define DLT_FILE_PATH               "{config.output.file_path}"
#define DLT_FILE_MAX_SIZE           {config.output.file_max_size}
#define DLT_FILE_MAX_COUNT          {config.output.file_max_count}

/* Registered Contexts */
#define DLT_CONTEXT_COUNT           {len(config.contexts)}

"""
    
    for i, ctx in enumerate(config.contexts):
        header += f"""
#define DLT_CTX_{ctx.app_id}_{ctx.context_id}_APPID      "{ctx.app_id}"
#define DLT_CTX_{ctx.app_id}_{ctx.context_id}_CTID       "{ctx.context_id}"
#define DLT_CTX_{ctx.app_id}_{ctx.context_id}_LEVEL      DLT_LOG_{ctx.log_level}
"""
    
    header += """
#endif /* DLT_GENERATED_CONFIG_H */
"""
    
    with open(output_path, 'w') as f:
        f.write(header)
    print(f"Generated C header: {output_path}")


def generate_dlt_viewer_config(config: DltModuleConfig, output_path: str):
    """生成DLT Viewer配置文件"""
    viewer_config = {
        "ECU": config.ecu_id,
        "Project": {
            "version": config.version,
            "contexts": [
                {
                    "appId": ctx.app_id,
                    "ctxId": ctx.context_id,
                    "description": ctx.description,
                    "logLevel": ctx.log_level
                }
                for ctx in config.contexts
            ]
        },
        "Filters": {
            "defaultLevel": config.default_log_level,
            "enableFilters": True
        }
    }
    
    with open(output_path, 'w') as f:
        json.dump(viewer_config, f, indent=2)
    print(f"Generated DLT Viewer config: {output_path}")


def cmd_generate(args):
    """生成配置命令"""
    config = generate_default_config()
    
    # 输出JSON配置
    output_path = Path(args.output)
    with open(output_path, 'w') as f:
        json.dump(config_to_dict(config), f, indent=2)
    print(f"Generated config: {output_path}")
    
    # 同时生成C头文件
    header_path = output_path.with_suffix('.h')
    generate_c_header(config, str(header_path))
    
    # 生成DLT Viewer配置
    if args.viewer_config:
        viewer_path = output_path.parent / "dlt_viewer_config.json"
        generate_dlt_viewer_config(config, str(viewer_path))


def cmd_modify(args):
    """修改配置命令"""
    # 读取现有配置
    with open(args.input, 'r') as f:
        data = json.load(f)
    
    config = dict_to_config(data)
    
    # 应用修改
    if args.level:
        config.default_log_level = args.level.upper()
        for ctx in config.contexts:
            ctx.log_level = args.level.upper()
        print(f"Set default log level to: {args.level.upper()}")
    
    if args.ecu:
        config.ecu_id = args.ecu
        print(f"Set ECU ID to: {args.ecu}")
    
    if args.buffer_size:
        config.buffer.buffer_size = args.buffer_size
        print(f"Set buffer size to: {args.buffer_size}")
    
    # 保存修改后的配置
    output_path = args.output if args.output else args.input
    with open(output_path, 'w') as f:
        json.dump(config_to_dict(config), f, indent=2)
    print(f"Saved config to: {output_path}")
    
    # 重新生成C头文件
    header_path = Path(output_path).with_suffix('.h')
    generate_c_header(config, str(header_path))


def cmd_validate(args):
    """验证配置命令"""
    with open(args.input, 'r') as f:
        data = json.load(f)
    
    config = dict_to_config(data)
    
    errors = []
    warnings = []
    
    # 验证上下文
    for ctx in config.contexts:
        if len(ctx.app_id) != 4:
            errors.append(f"Context {ctx.app_id}/{ctx.context_id}: app_id must be 4 characters")
        if len(ctx.context_id) != 4:
            errors.append(f"Context {ctx.app_id}/{ctx.context_id}: context_id must be 4 characters")
        if ctx.log_level not in ["OFF", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"]:
            errors.append(f"Context {ctx.app_id}/{ctx.context_id}: invalid log level {ctx.log_level}")
    
    # 验证缓冲区大小
    if config.buffer.buffer_size < 1024:
        warnings.append(f"Buffer size {config.buffer.buffer_size} is very small")
    if config.buffer.buffer_size > 1024 * 1024:
        warnings.append(f"Buffer size {config.buffer.buffer_size} is very large")
    
    # 验证输出配置
    if not any([config.output.enable_udp, config.output.enable_tcp, 
                config.output.enable_serial, config.output.enable_file]):
        warnings.append("No output channel is enabled")
    
    # 打印结果
    if errors:
        print("ERRORS:")
        for e in errors:
            print(f"  - {e}")
    
    if warnings:
        print("WARNINGS:")
        for w in warnings:
            print(f"  - {w}")
    
    if not errors and not warnings:
        print("Configuration is valid!")
        return 0
    elif errors:
        return 1
    return 0


def cmd_add_context(args):
    """添加上下文命令"""
    with open(args.input, 'r') as f:
        data = json.load(f)
    
    config = dict_to_config(data)
    
    # 添加新上下文
    new_ctx = DltContextConfig(
        app_id=args.app_id,
        context_id=args.context_id,
        description=args.description,
        log_level=args.level.upper()
    )
    config.contexts.append(new_ctx)
    
    # 保存
    with open(args.input, 'w') as f:
        json.dump(config_to_dict(config), f, indent=2)
    print(f"Added context: {args.app_id}/{args.context_id}")
    
    # 重新生成C头文件
    header_path = Path(args.input).with_suffix('.h')
    generate_c_header(config, str(header_path))


def main():
    parser = argparse.ArgumentParser(
        description="DLT Configuration Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s generate --output dlt_config.json
  %(prog)s modify --input dlt_config.json --level DEBUG
  %(prog)s validate --input dlt_config.json
  %(prog)s add-context --input dlt_config.json --app-id TEST --context-id DEMO --desc "Test context"
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Generate command
    gen_parser = subparsers.add_parser('generate', help='Generate default configuration')
    gen_parser.add_argument('--output', '-o', required=True, help='Output file path')
    gen_parser.add_argument('--viewer-config', '-v', action='store_true', 
                            help='Also generate DLT Viewer configuration')
    
    # Modify command
    mod_parser = subparsers.add_parser('modify', help='Modify existing configuration')
    mod_parser.add_argument('--input', '-i', required=True, help='Input file path')
    mod_parser.add_argument('--output', '-o', help='Output file path (default: overwrite input)')
    mod_parser.add_argument('--level', '-l', choices=['off', 'fatal', 'error', 'warn', 'info', 'debug', 'verbose'],
                            help='Set default log level')
    mod_parser.add_argument('--ecu', '-e', help='Set ECU ID')
    mod_parser.add_argument('--buffer-size', '-b', type=int, help='Set buffer size')
    
    # Validate command
    val_parser = subparsers.add_parser('validate', help='Validate configuration')
    val_parser.add_argument('--input', '-i', required=True, help='Input file path')
    
    # Add context command
    ctx_parser = subparsers.add_parser('add-context', help='Add a new context')
    ctx_parser.add_argument('--input', '-i', required=True, help='Input file path')
    ctx_parser.add_argument('--app-id', '-a', required=True, help='Application ID (4 chars)')
    ctx_parser.add_argument('--context-id', '-c', required=True, help='Context ID (4 chars)')
    ctx_parser.add_argument('--description', '-d', default='', help='Context description')
    ctx_parser.add_argument('--level', '-l', default='INFO', 
                            choices=['off', 'fatal', 'error', 'warn', 'info', 'debug', 'verbose'],
                            help='Log level')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Execute command
    commands = {
        'generate': cmd_generate,
        'modify': cmd_modify,
        'validate': cmd_validate,
        'add-context': cmd_add_context,
    }
    
    try:
        return commands[args.command](args)
    except Exception as e:
        print(f"Error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
