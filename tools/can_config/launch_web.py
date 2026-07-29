#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN配置工具 - Web版启动脚本
"""

import sys
import os
from pathlib import Path

def main():
    # 添加src到路径
    script_dir = Path(__file__).parent
    sys.path.insert(0, str(script_dir / "src"))
    
    # 导入并启动Flask应用
    from gui.app import app
    
    print("=" * 60)
    print("🚀 CAN配置工具 - Web GUI")
    print("=" * 60)
    print("访问 http://localhost:5000 使用工具")
    print("按 Ctrl+C 停止服务")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)

if __name__ == '__main__':
    main()
