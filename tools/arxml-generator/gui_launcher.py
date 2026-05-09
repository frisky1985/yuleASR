#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Generator GUI Launcher

启动可视化ARXML配置工具
"""

import sys
import subprocess
from pathlib import Path

def check_dependencies():
    """检查依赖"""
    try:
        import flask
        import flask_cors
        return True
    except ImportError:
        return False

def install_dependencies():
    "安装依赖"
    print("🔧 正在安装依赖包...")
    subprocess.check_call([
        sys.executable, "-m", "pip", "install", 
        "flask", "flask-cors", "-q"
    ])
    print("✅ 依赖安装完成")

def main():
    """主函数"""
    print("🚀 yuleASR ARXML Generator GUI")
    print("=" * 50)
    
    # 检查并安装依赖
    if not check_dependencies():
        install_dependencies()
    
    # 添加路径
    gui_dir = Path(__file__).parent / "gui"
    sys.path.insert(0, str(gui_dir))
    
    # 启动服务器
    print("\n📊 正在启动Web服务器...")
    print("📁 访问地址: http://localhost:5000")
    print("\n按 Ctrl+C 停止服务\n")
    
    from api.server import app
    app.run(host='0.0.0.0', port=5000, debug=False)

if __name__ == "__main__":
    main()
