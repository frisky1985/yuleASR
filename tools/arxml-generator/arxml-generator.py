#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Configuration Generator

Vector Configurator风格的ARXML配置生成工具
支持MCAL和BSW模块的AUTOSAR标准配置生成

使用示例:
    # 生成MCU配置
    ./arxml-generator.py mcu --ecu ECU0 --clock 80000000 -o Mcu.arxml
    
    # 生成Port配置
    ./arxml-generator.py port --pins "PA0:OUT,PA1:IN" -o Port.arxml
    
    # 生成CAN配置
    ./arxml-generator.py can --baudrate 500000 --controller 0 -o Can.arxml
    
    # 从JSON配置生成
    ./arxml-generator.py from-json config.json -o output.arxml
"""

import sys
import argparse
import json
from pathlib import Path
from typing import Optional, Dict, Any

# 添加src到路径
sys.path.insert(0, str(Path(__file__).parent / "src"))

try:
    from mcal_config_generator import (
        McuConfigGenerator,
        PortConfigGenerator,
        CanConfigGenerator,
        SpiConfigGenerator,
        GptConfigGenerator,
        create_mcu_config,
        create_port_config,
        create_can_config
    )
    from bsw_config_generator import (
        ComConfigGenerator,
        PduRConfigGenerator,
        CanIfConfigGenerator,
        NvMConfigGenerator,
        create_com_config,
        create_pdur_config,
        create_canif_config,
        create_nvm_config
    )
    from arxml_ecuc_generator import ArxmlEcucGenerator
except ImportError as e:
    print(f"错误: 无法导入模块 - {e}")
    sys.exit(1)


def create_mcu_parser(subparsers):
    """MCU配置子命令"""
    parser = subparsers.add_parser(
        "mcu",
        help="生成MCU驱动配置",
        description="生成微控制器驱动的ARXML配置文件"
    )
    parser.add_argument("--ecu", default="ECU0", help="ECU实例名称")
    parser.add_argument("--clock", type=int, default=80000000, help="CPU时钟频率(Hz)")
    parser.add_argument("--periph-clock", type=int, default=40000000, help="外设时钟频率(Hz)")
    parser.add_argument("--dev-error-detect", action="store_true", default=True, help="启用开发错误检测")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_mcu)


def cmd_mcu(args):
    """执行MCU配置生成"""
    print(f"🔧 生成MCU配置...")
    
    gen = create_mcu_config(args.ecu)
    gen.add_general_config(
        dev_error_detect=args.dev_error_detect,
        init_clock=True,
        version_info_api=False
    )
    gen.add_clock_config(
        cpu_clock=args.clock,
        peripheral_clock=args.periph_clock
    )
    gen.add_mode_config()
    gen.add_ram_section()
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ MCU配置已生成: {output_path}")
    print(f"   - CPU时钟: {args.clock/1e6:.1f} MHz")
    print(f"   - 外设时钟: {args.periph_clock/1e6:.1f} MHz")


def create_port_parser(subparsers):
    """Port配置子命令"""
    parser = subparsers.add_parser(
        "port",
        help="生成Port驱动配置",
        description="生成端口驱动的ARXML配置文件"
    )
    parser.add_argument("--ecu", default="ECU0", help="ECU实例名称")
    parser.add_argument("--pins", required=True, 
                       help="引脚配置，格式: PA0:OUT,PA1:IN,PA2:OUT 等")
    parser.add_argument("--dev-error-detect", action="store_true", default=True, help="启用开发错误检测")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_port)


def cmd_port(args):
    """执行Port配置生成"""
    print(f"🔧 生成Port配置...")
    
    gen = create_port_config(args.ecu)
    gen.add_general_config(dev_error_detect=args.dev_error_detect)
    
    # 解析引脚配置
    pins = args.pins.split(",")
    pin_count = 0
    
    for pin_config in pins:
        parts = pin_config.strip().split(":")
        if len(parts) != 2:
            print(f"⚠️ 忽略无效的引脚配置: {pin_config}")
            continue
        
        pin_name = parts[0].strip()
        direction = parts[1].strip().upper()
        
        # 解析端口名称和引脚号
        port_name = pin_name[:2]  # 如 PA
        try:
            pin_number = int(pin_name[2:])  # 如 0, 1, 2
        except ValueError:
            print(f"⚠️ 无法解析引脚号: {pin_name}")
            continue
        
        # 映射方向
        direction_map = {
            "OUT": "PORT_PIN_OUT",
            "IN": "PORT_PIN_IN",
            "OUTPUT": "PORT_PIN_OUT",
            "INPUT": "PORT_PIN_IN"
        }
        
        arxml_direction = direction_map.get(direction, "PORT_PIN_OUT")
        
        gen.add_pin_config(
            pin_name=f"PortPin_{pin_name}",
            port_name=port_name,
            pin_number=pin_number,
            direction=arxml_direction,
            mode="PORT_PIN_MODE_GPIO"
        )
        pin_count += 1
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ Port配置已生成: {output_path}")
    print(f"   - 配置引脚数: {pin_count}")


def create_can_parser(subparsers):
    """CAN配置子命令"""
    parser = subparsers.add_parser(
        "can",
        help="生成CAN驱动配置",
        description="生成CAN驱动的ARXML配置文件"
    )
    parser.add_argument("--ecu", default="ECU0", help="ECU实例名称")
    parser.add_argument("--baudrate", type=int, default=500000, help="波特率(bps)")
    parser.add_argument("--controller", type=int, default=0, help="控制器ID")
    parser.add_argument("--tx-objects", type=int, default=2, help="发送HOH数量")
    parser.add_argument("--rx-objects", type=int, default=2, help="接收HOH数量")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_can)


def cmd_can(args):
    """执行CAN配置生成"""
    print(f"🔧 生成CAN配置...")
    
    gen = create_can_config(args.ecu)
    gen.add_general_config()
    gen.add_controller_config(
        controller_id=args.controller,
        baudrate=args.baudrate
    )
    
    # 添加硬件对象
    for i in range(args.tx_objects):
        gen.add_hardware_object(
            hoh_name=f"CanHardwareObject_Tx_{i}",
            controller_ref=args.controller,
            object_type="TRANSMIT"
        )
    
    for i in range(args.rx_objects):
        gen.add_hardware_object(
            hoh_name=f"CanHardwareObject_Rx_{i}",
            controller_ref=args.controller,
            object_type="RECEIVE"
        )
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ CAN配置已生成: {output_path}")
    print(f"   - 波特率: {args.baudrate/1000:.0f} kbps")
    print(f"   - 控制器ID: {args.controller}")
    print(f"   - Tx HOH: {args.tx_objects}")
    print(f"   - Rx HOH: {args.rx_objects}")


def create_com_parser(subparsers):
    """COM配置子命令"""
    parser = subparsers.add_parser(
        "com",
        help="生成COM通信服务配置",
        description="生成COM模块的ARXML配置文件"
    )
    parser.add_argument("--ecu", default="ECU0", help="ECU实例名称")
    parser.add_argument("--signals", type=int, default=4, help="信号数量")
    parser.add_argument("--ipdus", type=int, default=2, help="IPDU数量")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_com)


def cmd_com(args):
    """执行COM配置生成"""
    print(f"🔧 生成COM配置...")
    
    gen = create_com_config(args.ecu)
    gen.add_general_config()
    
    # 添加IPDU和信号
    for i in range(args.ipdus):
        gen.add_ipdu_config(
            ipdu_name=f"ComIPdu_{i}",
            pdu_id=i,
            length=8,
            direction="SEND" if i % 2 == 0 else "RECEIVE"
        )
    
    for i in range(args.signals):
        gen.add_signal_config(
            signal_name=f"ComSignal_{i}",
            ipdu_ref=f"ComIPdu_{i % args.ipdus}",
            start_bit=(i * 8) % 64,
            bit_length=8
        )
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ COM配置已生成: {output_path}")
    print(f"   - IPDU数量: {args.ipdus}")
    print(f"   - 信号数量: {args.signals}")


def create_nvm_parser(subparsers):
    """NvM配置子命令"""
    parser = subparsers.add_parser(
        "nvm",
        help="生成NvM NVRAM管理器配置",
        description="生成NvM模块的ARXML配置文件"
    )
    parser.add_argument("--ecu", default="ECU0", help="ECU实例名称")
    parser.add_argument("--blocks", type=int, default=4, help="NVRAM块数量")
    parser.add_argument("--crc", default="NVM_CRC32", 
                       choices=["NVM_CRC8", "NVM_CRC16", "NVM_CRC32", "NVM_CRC_NONE"],
                       help="CRC类型")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_nvm)


def cmd_nvm(args):
    """执行NvM配置生成"""
    print(f"🔧 生成NvM配置...")
    
    gen = create_nvm_config(args.ecu)
    gen.add_common_config(crc_type=args.crc)
    
    for i in range(args.blocks):
        gen.add_block_descriptor(
            block_name=f"NvMBlockDescriptor_{i}",
            block_id=i,
            block_size=32 + i * 16,
            crc_type=args.crc
        )
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ NvM配置已生成: {output_path}")
    print(f"   - NVRAM块数量: {args.blocks}")
    print(f"   - CRC类型: {args.crc}")


def create_json_parser(subparsers):
    """JSON导入子命令"""
    parser = subparsers.add_parser(
        "from-json",
        help="从JSON配置生成ARXML",
        description="从JSON配置文件生成ARXML"
    )
    parser.add_argument("config", help="JSON配置文件路径")
    parser.add_argument("-o", "--output", required=True, help="输出ARXML文件路径")
    parser.set_defaults(func=cmd_from_json)


def cmd_from_json(args):
    """从JSON生成配置"""
    print(f"🔧 从JSON生成配置: {args.config}")
    
    config_path = Path(args.config)
    if not config_path.exists():
        print(f"❌ 配置文件不存在: {config_path}")
        sys.exit(1)
    
    with open(config_path, "r", encoding="utf-8") as f:
        config = json.load(f)
    
    # 根据配置类型选择生成器
    module_type = config.get("module_type", "").lower()
    
    generators = {
        "mcu": create_mcu_config,
        "port": create_port_config,
        "can": create_can_config,
        "com": create_com_config,
        "pdur": create_pdur_config,
        "canif": create_canif_config,
        "nvm": create_nvm_config
    }
    
    if module_type not in generators:
        print(f"❌ 不支持的模块类型: {module_type}")
        print(f"支持的类型: {', '.join(generators.keys())}")
        sys.exit(1)
    
    gen = generators[module_type](config.get("ecu", "ECU0"))
    
    # TODO: 根据JSON内容填充配置
    
    arxml_content = gen.to_arxml()
    
    output_path = Path(args.output)
    output_path.write_text(arxml_content, encoding="utf-8")
    
    print(f"✅ 配置已生成: {output_path}")


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        prog="arxml-generator",
        description="yuleASR ARXML配置生成器 - Vector Configurator风格",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  # 生成MCU配置
  %(prog)s mcu --clock 160000000 -o Mcu.arxml
  
  # 生成Port配置
  %(prog)s port --pins "PA0:OUT,PA1:IN,PB5:OUT" -o Port.arxml
  
  # 生成CAN配置
  %(prog)s can --baudrate 500000 --tx-objects 4 -o Can.arxml
  
  # 生成NvM配置
  %(prog)s nvm --blocks 8 --crc NVM_CRC32 -o NvM.arxml
  
支持的模块:
  MCAL: Mcu, Port, Can, Spi, Gpt
  BSW:  Com, PduR, CanIf, NvM
        """
    )
    
    subparsers = parser.add_subparsers(dest="command", help="可用命令")
    
    # 注册子命令
    create_mcu_parser(subparsers)
    create_port_parser(subparsers)
    create_can_parser(subparsers)
    create_com_parser(subparsers)
    create_nvm_parser(subparsers)
    create_json_parser(subparsers)
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    try:
        args.func(args)
    except Exception as e:
        print(f"❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
