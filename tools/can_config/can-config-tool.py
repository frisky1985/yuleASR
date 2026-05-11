#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR CAN配置工具

功能:
1. 导入DBC文件生成Com配置
2. 导入CAN Matrix Excel生成Com配置
3. 生成AUTOSAR Com模块配置文件(Com_Cfg.h, Com_Cfg.c)

用法:
    # 从DBC文件生成
    python3 can-config-tool.py import-dbc my_network.dbc -o ./output

    # 从CAN Matrix Excel生成
    python3 can-config-tool.py import-excel my_matrix.xlsx -o ./output

    # 从dbc文件生成并指定ECU名称
    python3 can-config-tool.py import-dbc my_network.dbc --ecu ECU1 -o ./output
"""

import sys
import argparse
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "src"))

from dbc_parser import DbcParser, create_example_dbc
from can_matrix_parser import CanMatrixParser, create_example_csv
from com_config_generator import ComConfigGenerator


def print_header():
    """打印标题"""
    print("""
╔══════════════════════════════════════════════════════════════════╗
║  🚀 yuleASR CAN Configuration Tool v1.0                    ║
╠══════════════════════════════════════════════════════════════════╣
║  导入DBC/Excel → 生成AUTOSAR Com配置                     ║
╚══════════════════════════════════════════════════════════════════╝
    """)


def cmd_import_dbc(args):
    """导入DBC文件命令"""
    print(f"\n📁 导入DBC文件: {args.input}")
    
    # 解析DBC
    parser = DbcParser()
    try:
        network = parser.parse_file(args.input)
    except Exception as e:
        print(f"❌ 解析失败: {e}")
        return 1
    
    # 显示解析结果
    print(f"\n  ✅ 解析成功:")
    print(f"     • 网络节点: {', '.join(network.nodes)}")
    print(f"     • 消息数量: {len(network.messages)}")
    
    total_signals = sum(len(m.signals) for m in network.messages)
    print(f"     • 信号数量: {total_signals}")
    
    # 显示消息详情
    if args.verbose:
        print("\n  📤 消息列表:")
        for msg in network.messages:
            print(f"     • 0x{msg.id:04X} {msg.name} (DLC={msg.dlc}, 周期={msg.cycle_time}ms)")
            for sig in msg.signals:
                print(f"       ├─ {sig.name}: {sig.start_bit}|{sig.length}@{sig.factor}")
    
    # 转换为Com配置
    config = parser.to_com_config()
    if args.ecu:
        config['ecu_name'] = args.ecu
    
    # 生成配置文件
    generator = ComConfigGenerator(config)
    
    try:
        cfg_h, cfg_c = generator.generate(args.output, args.prefix)
    except Exception as e:
        print(f"\n❌ 生成配置文件失败: {e}")
        return 1
    
    # 显示摘要
    print("\n" + generator.generate_summary())
    
    print(f"\n  ✅ 配置文件已生成:")
    print(f"     • {cfg_h}")
    print(f"     • {cfg_c}")
    
    return 0


def cmd_import_excel(args):
    """导入Excel文件命令"""
    print(f"\n📁 导入CAN Matrix: {args.input}")
    
    # 解析Excel
    parser = CanMatrixParser()
    try:
        matrix = parser.parse_excel(args.input, args.sheet)
    except ImportError as e:
        print(f"\n❌ {e}")
        print("   请运行: pip3 install pandas openpyxl")
        return 1
    except Exception as e:
        print(f"\n❌ 解析失败: {e}")
        return 1
    
    # 显示解析结果
    print(f"\n  ✅ 解析成功:")
    print(f"     • 网络节点: {', '.join(matrix.nodes)}")
    print(f"     • 消息数量: {len(matrix.messages)}")
    
    total_signals = sum(len(m.signals) for m in matrix.messages)
    print(f"     • 信号数量: {total_signals}")
    
    # 转换为Com配置
    config = parser.to_com_config()
    if args.ecu:
        config['ecu_name'] = args.ecu
    
    # 生成配置文件
    generator = ComConfigGenerator(config)
    
    try:
        cfg_h, cfg_c = generator.generate(args.output, args.prefix)
    except Exception as e:
        print(f"\n❌ 生成配置文件失败: {e}")
        return 1
    
    # 显示摘要
    print("\n" + generator.generate_summary())
    
    print(f"\n  ✅ 配置文件已生成:")
    print(f"     • {cfg_h}")
    print(f"     • {cfg_c}")
    
    return 0


def cmd_import_csv(args):
    """导入CSV文件命令"""
    print(f"\n📁 导入CAN Matrix CSV: {args.input}")
    
    # 解析CSV
    parser = CanMatrixParser()
    try:
        matrix = parser.parse_csv(args.input)
    except Exception as e:
        print(f"\n❌ 解析失败: {e}")
        return 1
    
    # 显示解析结果
    print(f"\n  ✅ 解析成功:")
    print(f"     • 网络节点: {', '.join(matrix.nodes)}")
    print(f"     • 消息数量: {len(matrix.messages)}")
    
    total_signals = sum(len(m.signals) for m in matrix.messages)
    print(f"     • 信号数量: {total_signals}")
    
    # 转换为Com配置
    config = parser.to_com_config()
    if args.ecu:
        config['ecu_name'] = args.ecu
    
    # 生成配置文件
    generator = ComConfigGenerator(config)
    
    try:
        cfg_h, cfg_c = generator.generate(args.output, args.prefix)
    except Exception as e:
        print(f"\n❌ 生成配置文件失败: {e}")
        return 1
    
    # 显示摘要
    print("\n" + generator.generate_summary())
    
    print(f"\n  ✅ 配置文件已生成:")
    print(f"     • {cfg_h}")
    print(f"     • {cfg_c}")
    
    return 0


def cmd_demo(args):
    """运行演示"""
    print_header()
    print("\n🎯 生成示例数据并生成配置...")
    
    import tempfile
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # 创建示例DBC文件
        dbc_content = create_example_dbc()
        dbc_path = Path(tmpdir) / "example.dbc"
        with open(dbc_path, 'w') as f:
            f.write(dbc_content)
        
        print(f"\n  📁 示例DBC文件: {dbc_path}")
        
        # 解析DBC
        parser = DbcParser()
        network = parser.parse_file(str(dbc_path))
        
        print(f"\n  📤 DBC内容:")
        print(f"     • 节点: {', '.join(network.nodes)}")
        print(f"     • 消息: {len(network.messages)}")
        
        for msg in network.messages:
            print(f"\n     📬 {msg.name} (ID=0x{msg.id:X})")
            print(f"        发送方: {msg.sender}, 周期: {msg.cycle_time}ms")
            for sig in msg.signals:
                endian = "Motorola" if sig.byte_order == 0 else "Intel"
                signed = "有符号" if sig.is_signed else "无符号"
                print(f"        │─ {sig.name}: Start={sig.start_bit}, Len={sig.length}")
                print(f"        │        {endian}, {signed}, Factor={sig.factor}, Offset={sig.offset}")
        
        # 生成配置
        config = parser.to_com_config()
        generator = ComConfigGenerator(config)
        
        output_dir = args.output or tmpdir
        cfg_h, cfg_c = generator.generate(output_dir)
        
        print("\n" + generator.generate_summary())
        
        print(f"\n  ✅ 配置文件已生成:")
        print(f"     • {cfg_h}")
        print(f"     • {cfg_c}")
        
        # 显示Com_Cfg.h预览
        print("\n  📄 Com_Cfg.h 预览:")
        print("-" * 60)
        with open(cfg_h, 'r') as f:
            content = f.read()
            print(content[:1200])
        print("...")
        
        # 显示Com_Cfg.c预览
        print("\n  📄 Com_Cfg.c 预览:")
        print("-" * 60)
        with open(cfg_c, 'r') as f:
            lines = f.readlines()
            for line in lines[:30]:
                print(line.rstrip())
        print("...")
    
    return 0


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        prog='can-config-tool',
        description='yuleASR CAN配置工具 - 从DBC/Excel生成AUTOSAR Com配置',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 从DBC生成Com配置
  python3 can-config-tool.py import-dbc vehicle.dbc -o ./output

  # 从Excel生成并指定ECU名称
  python3 can-config-tool.py import-excel matrix.xlsx --ecu BodyECU -o ./output

  # 从CSV生成
  python3 can-config-tool.py import-csv matrix.csv -o ./output

  # 运行演示
  python3 can-config-tool.py demo
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='可用命令')
    
    # import-dbc命令
    dbc_parser = subparsers.add_parser('import-dbc', help='从DBC文件导入')
    dbc_parser.add_argument('input', help='DBC文件路径')
    dbc_parser.add_argument('-o', '--output', default='./output', help='输出目录 (默认: ./output)')
    dbc_parser.add_argument('--ecu', help='ECU名称')
    dbc_parser.add_argument('--prefix', default='', help='文件前缀')
    dbc_parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    # import-excel命令
    excel_parser = subparsers.add_parser('import-excel', help='从Excel文件导入')
    excel_parser.add_argument('input', help='Excel文件路径')
    excel_parser.add_argument('-s', '--sheet', help='工作表名称')
    excel_parser.add_argument('-o', '--output', default='./output', help='输出目录')
    excel_parser.add_argument('--ecu', help='ECU名称')
    excel_parser.add_argument('--prefix', default='', help='文件前缀')
    excel_parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    # import-csv命令
    csv_parser = subparsers.add_parser('import-csv', help='从CSV文件导入')
    csv_parser.add_argument('input', help='CSV文件路径')
    csv_parser.add_argument('-o', '--output', default='./output', help='输出目录')
    csv_parser.add_argument('--ecu', help='ECU名称')
    csv_parser.add_argument('--prefix', default='', help='文件前缀')
    csv_parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    # demo命令
    demo_parser = subparsers.add_parser('demo', help='运行演示')
    demo_parser.add_argument('-o', '--output', help='输出目录 (默认临时目录)')
    
    args = parser.parse_args()
    
    if not args.command:
        print_header()
        parser.print_help()
        return 0
    
    # 执行命令
    if args.command == 'import-dbc':
        return cmd_import_dbc(args)
    elif args.command == 'import-excel':
        return cmd_import_excel(args)
    elif args.command == 'import-csv':
        return cmd_import_csv(args)
    elif args.command == 'demo':
        return cmd_demo(args)
    else:
        parser.print_help()
        return 0


if __name__ == "__main__":
    sys.exit(main())
