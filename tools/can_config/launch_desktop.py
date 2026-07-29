#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN配置工具 - 桌面版启动脚本
"""

import sys
import os
from pathlib import Path

def main():
    # 添加src到路径
    script_dir = Path(__file__).parent
    sys.path.insert(0, str(script_dir / "src"))
    sys.path.insert(0, str(script_dir / "gui_desktop"))
    
    try:
        from main import main as desktop_main
        desktop_main()
    except ImportError as e:
        print(f"错误: 未安装必要的依赖: {e}")
        print("请运行: pip install PyQt6")
        sys.exit(1)

if __name__ == '__main__':
    main()
