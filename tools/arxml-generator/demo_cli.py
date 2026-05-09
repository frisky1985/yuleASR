#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Generator - CLI Demo
终端交互式演示脚本
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "src"))

from mcal_config_generator import create_mcu_config, create_port_config, create_can_config
from bsw_config_generator import create_com_config, create_nvm_config


def clear():
    print("\n" * 2)


def print_header(title):
    width = 60
    print("=" * width)
    print(f"  {title}")
    print("=" * width)


def print_step(step_num, desc):
    print(f"\n  ┌─────────────────────────────────────────────────┐")
    print(f"  │ 📅 步骤 {step_num}: {desc}")
    print(f"  └─────────────────────────────────────────────────┘")


def show_gui_mockup():
    """显示GUI界面模拟"""
    print("""
    ┌───────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
    │  yuleASR ARXML Generator - 可视化配置工具 (PyQt6版)            │
    ├───────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
    │                                                                   │
    │  ┌───────────────────┐   ┌──────────────────────────┐   ┌──────────────────────────┐      │
    │  │ MCAL模块      │   │ 配置编辑        │   │ ARXML预览        │      │
    │  ├───────────────────┤   ├──────────────────────────┤   ├──────────────────────────┤      │
    │  │ ┌───────────────┐│   │ ECU名称: ECU0       │   │ ┌─────────────────┐│      │
    │  │ │ ✲ Mcu    ││   │                      │   │ │ ┈ 1 ┈┈┈┈┈┈┈┈┈┈┈┈┈┈││      │
    │  │ │   微控制器 ││   │ ┌───────────────┐ │   │ │ ┈ 2 ┈┈┈┈┈┈┈┈┈┈┈┈┈││      │
    │  │ └───────────────┘│   │ │ ★ 通用配置   │ │   │ │ ┈ 3 ┈┈┈┈┈┈┈┈┈┈┈┈┈││      │
    │  │ ┌───────────────┐│   │ ├───────────────┤ │   │ │ ┈ ┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈││      │
    │  │ │   Port   ││   │ │ [✓] DevError│ │   │ └─────────────────┘│      │
    │  │ │   GPIO配置 ││   │ │ [✓] InitClk  │ │   │                  │      │
    │  │ └───────────────┘│   │ │            │ │   │ [粘贴] [下载]     │      │
    │  │ ┌───────────────┐│   │ ├───────────────┤ │   │                  │      │
    │  │ │   Can    ││   │ │ ★ 时钟配置   │ │   │                  │      │
    │  │ │   CAN通信 ││   │ ├───────────────┤ │   │                  │      │
    │  │ └───────────────┘│   │ │ CPU: 80MHz  │ │   │                  │      │
    │  │ ┌───────────────┐│   │ │ PER: 40MHz  │ │   │                  │      │
    │  │ │   Spi    ││   │ └───────────────┘ │   │                  │      │
    │  │ │   SPI通信 ││   │                      │   │                  │      │
    │  │ └───────────────┘│   │                      │   │                  │      │
    │  │              │   │   [🚀 生成ARXML]     │   │                  │      │
    │  │ BSW模块      │   └──────────────────────────┘   └──────────────────────────┘      │
    │  ├───────────────┤                                              │
    │  │   Com        │   当前模块: Mcu                         │
    │  │   通信服务     │   ECU: ECU0                                  │
    │  └───────────────┘                                              │
    └───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
    """)


def demo_mcu():
    """演示Mcu模块配置"""
    print_header("🚀 演示 1: MCU配置生成")
    
    print_step(1, "创建MCU配置生成器")
    gen = create_mcu_config("ECU0")
    print("  ✓ 创建生成器: ECU0 -> Mcu")
    
    print_step(2, "添加通用配置")
    gen.add_general_config(
        dev_error_detect=True,
        init_clock=True,
        version_info_api=False
    )
    print("  ✓ DevErrorDetect: True")
    print("  ✓ InitClock: True")
    print("  ✓ VersionInfoApi: False")
    
    print_step(3, "添加时钟配置")
    gen.add_clock_config(
        cpu_clock=80000000,
        peripheral_clock=40000000
    )
    print("  ✓ CPU时钟: 80MHz")
    print("  ✓ 外设时钟: 40MHz")
    
    print_step(4, "生成ARXML")
    arxml = gen.to_arxml()
    size_kb = len(arxml) / 1024
    print(f"  ✓ ARXML生成完成: {len(arxml)} 字符 ({size_kb:.1f}KB)")
    
    print_step(5, "预览部分内容")
    lines = arxml.split('\n')[:25]
    print("  ┌" + "─" * 54 + "┐")
    for line in lines:
        print(f"  │ {line[:52]:<52} │")
    print("  │ ... (total: %d lines) ..." % len(arxml.split('\n')))
    print("  └" + "─" * 54 + "┘")
    
    # 保存
    output_path = "/tmp/Mcu_Demo.arxml"
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(arxml)
    print(f"\n  💾 已保存到: {output_path}")


def demo_can():
    """演示Can模块配置"""
    clear()
    print_header("📡 演示 2: CAN配置生成")
    
    print_step(1, "创建CAN配置生成器")
    gen = create_can_config("ECU0")
    print("  ✓ 创建生成器: ECU0 -> Can")
    
    print_step(2, "添加通用配置")
    gen.add_general_config(
        dev_error_detect=True,
        index=0,
        main_function_period=10.0
    )
    print("  ✓ DevErrorDetect: True")
    print("  ✓ Index: 0")
    print("  ✓ MainFunctionPeriod: 10.0ms")
    
    print_step(3, "添加控制器配置")
    gen.add_controller_config(
        controller_id=0,
        baudrate=500000,
        prop_seg=2,
        phase_seg1=6,
        phase_seg2=7
    )
    print("  ✓ 波特率: 500kbps")
    print("  ✓ PropSeg: 2")
    print("  ✓ PhaseSeg1: 6")
    print("  ✓ PhaseSeg2: 7")
    
    print_step(4, "生成ARXML")
    arxml = gen.to_arxml()
    size_kb = len(arxml) / 1024
    print(f"  ✓ ARXML生成完成: {len(arxml)} 字符 ({size_kb:.1f}KB)")
    
    # 保存
    output_path = "/tmp/Can_Demo.arxml"
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(arxml)
    print(f"\n  💾 已保存到: {output_path}")


def demo_port():
    """演示Port模块配置"""
    clear()
    print_header("🔗 演示 3: Port配置生成")
    
    print_step(1, "创建Port配置生成器")
    gen = create_port_config("ECU0")
    print("  ✓ 创建生成器: ECU0 -> Port")
    
    print_step(2, "添加通用配置")
    gen.add_general_config(dev_error_detect=True)
    print("  ✓ DevErrorDetect: True")
    
    print_step(3, "添加引脚配置")
    pins = [
        ("LED_Red", 0, "PORT_PIN_OUT", "PORT_PIN_MODE_GPIO"),
        ("Button_1", 1, "PORT_PIN_IN", "PORT_PIN_MODE_GPIO"),
        ("CAN_TX", 2, "PORT_PIN_OUT", "PORT_PIN_MODE_CAN"),
        ("CAN_RX", 3, "PORT_PIN_IN", "PORT_PIN_MODE_CAN"),
    ]
    for name, pin_id, direction, mode in pins:
        gen.add_pin_config(name, pin_id, direction, mode)
        print(f"  ✓ 引脚: {name} - {direction.split('_')[-1]} - {mode.split('_')[-1]}")
    
    print_step(4, "生成ARXML")
    arxml = gen.to_arxml()
    size_kb = len(arxml) / 1024
    print(f"  ✓ ARXML生成完成: {len(arxml)} 字符 ({size_kb:.1f}KB)")
    
    # 保存
    output_path = "/tmp/Port_Demo.arxml"
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(arxml)
    print(f"\n  💾 已保存到: {output_path}")


def demo_nvm():
    """演示NvM模块配置"""
    clear()
    print_header("💾 演示 4: NvM配置生成")
    
    print_step(1, "创建NvM配置生成器")
    gen = create_nvm_config("ECU0")
    print("  ✓ 创建生成器: ECU0 -> NvM")
    
    print_step(2, "添加通用配置")
    gen.add_common_config(crc_num_bytes=4)
    print("  ✓ CRC字节: 4 (CRC32)")
    
    print_step(3, "添加NVRAM块")
    blocks = [
        ("NvMBlockDescriptor_NvMConfigId", 0, 4, "NVM_CRC32"),
        ("NvMBlockDescriptor_ECU_Configuration", 1, 32, "NVM_CRC32"),
        ("NvMBlockDescriptor_ECU_Configuration2", 2, 32, "NVM_CRC32"),
        ("NvMBlockDescriptor_ECU_Configuration3", 3, 32, "NVM_CRC32"),
    ]
    for name, block_id, size, crc in blocks:
        gen.add_block_descriptor(name, block_id, size, crc)
        print(f"  ✓ 块: {name} (ID={block_id}, Size={size})")
    
    print_step(4, "生成ARXML")
    arxml = gen.to_arxml()
    size_kb = len(arxml) / 1024
    print(f"  ✓ ARXML生成完成: {len(arxml)} 字符 ({size_kb:.1f}KB)")
    
    # 保存
    output_path = "/tmp/NvM_Demo.arxml"
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(arxml)
    print(f"\n  💾 已保存到: {output_path}")


def show_summary():
    """显示总结"""
    clear()
    print_header("📋 演示总结")
    
    print("""
  ┌────────────────────────────────────────────────────────────────────────────────────┐
  │  ✅ 生成的ARXML文件                                       │
  ├────────────────────────────────────────────────────────────────────────────────────┤
  │  🔫 /tmp/Mcu_Demo.arxml    - MCU配置 (CPU 80MHz)            │
  │  📡 /tmp/Can_Demo.arxml    - CAN配置 (500kbps)              │
  │  🔗 /tmp/Port_Demo.arxml   - Port配置 (4引脚)              │
  │  💾 /tmp/NvM_Demo.arxml    - NvM配置 (4块)                 │
  └────────────────────────────────────────────────────────────────────────────────────┘

  ┌────────────────────────────────────────────────────────────────────────────────────┐
  │  🎯 GUI功能特点                                          │
  ├────────────────────────────────────────────────────────────────────────────────────┤
  │  • 三栏式布局: 模块选择 | 配置编辑 | ARXML预览           │
  │  • 可视化配置: 选择模块后自动显示对应配置表单              │
  │  • 实时预览: 生成后立即在右侧显示ARXML代码                │
  │  • 一键导出: 支持下载为.arxml文件                      │
  │  • 支持模块: Mcu, Port, Can, Spi, Com, PduR, NvM         │
  └────────────────────────────────────────────────────────────────────────────────────┘

  💡 启动GUI的方式:

    # Web版 (功能完整，需flask)
    pip3 install flask flask-cors
    cd /home/admin/yuleASR/tools/arxml-generator
    python3 gui_launcher.py
    # 打开浏览器访问 http://localhost:5000

    # 桌面版 (独立运行，需PyQt6)
    pip3 install PyQt6
    python3 gui_qt.py
    """)


def main():
    """主函数"""
    print("\n")
    print_header("🚀 yuleASR ARXML Generator - 可视化配置工具演示")
    
    print("""
    📚 这是一个类似 Vector Configurator 风格的可视化配置工具
    支持以下模块: Mcu, Port, Can, Spi, Com, PduR, NvM
    """)
    
    input("  按回车键开始演示...")
    
    # 显示GUI模拟
    show_gui_mockup()
    input("  按回车键继续...")
    
    # 演示MCU
    demo_mcu()
    input("  按回车键继续...")
    
    # 演示CAN
    demo_can()
    input("  按回车键继续...")
    
    # 演示Port
    demo_port()
    input("  按回车键继续...")
    
    # 演示NvM
    demo_nvm()
    input("  按回车键查看总结...")
    
    # 显示总结
    show_summary()


if __name__ == "__main__":
    main()
