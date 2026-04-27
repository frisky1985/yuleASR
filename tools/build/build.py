#!/usr/bin/env python3
"""
YuleTech BSW Build Tool

自动化编译脚本
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path


class BuildTool:
    """编译工具"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.build_dir = self.project_root / "build"
        self.tools_dir = self.project_root / "tools" / "build"
        
    def clean(self) -> bool:
        """清理构建目录"""
        print("Cleaning build directory...")
        try:
            if self.build_dir.exists():
                import shutil
                shutil.rmtree(self.build_dir)
            print("✓ Clean completed")
            return True
        except Exception as e:
            print(f"✗ Clean failed: {e}")
            return False
    
    def configure(self) -> bool:
        """配置项目"""
        print("Configuring project...")
        try:
            self.build_dir.mkdir(parents=True, exist_ok=True)
            
            cmd = [
                "cmake",
                "-S", str(self.tools_dir),
                "-B", str(self.build_dir),
                "-DCMAKE_BUILD_TYPE=Release"
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ Configuration completed")
                return True
            else:
                print(f"✗ Configuration failed:\n{result.stderr}")
                return False
        except Exception as e:
            print(f"✗ Configuration error: {e}")
            return False
    
    def build(self) -> bool:
        """编译项目"""
        print("Building project...")
        try:
            cmd = [
                "cmake",
                "--build", str(self.build_dir),
                "--parallel"
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ Build completed")
                return True
            else:
                print(f"✗ Build failed:\n{result.stderr}")
                return False
        except Exception as e:
            print(f"✗ Build error: {e}")
            return False
    
    def install(self) -> bool:
        """安装项目"""
        print("Installing project...")
        try:
            cmd = [
                "cmake",
                "--install", str(self.build_dir)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ Installation completed")
                return True
            else:
                print(f"✗ Installation failed:\n{result.stderr}")
                return False
        except Exception as e:
            print(f"✗ Installation error: {e}")
            return False
    
    def full_build(self) -> bool:
        """完整构建流程"""
        print("YuleTech BSW Build Tool v1.0.0")
        print("=" * 50)
        
        steps = [
            ("Clean", self.clean),
            ("Configure", self.configure),
            ("Build", self.build),
            ("Install", self.install)
        ]
        
        for name, step_func in steps:
            print(f"\n[{name}]")
            if not step_func():
                print(f"\n✗ Build failed at step: {name}")
                return False
        
        print("\n" + "=" * 50)
        print("✓ Full build completed successfully!")
        return True


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="YuleTech BSW Build Tool")
    parser.add_argument("--clean", action="store_true", help="Clean build directory")
    parser.add_argument("--configure", action="store_true", help="Configure only")
    parser.add_argument("--build", action="store_true", help="Build only")
    parser.add_argument("--install", action="store_true", help="Install only")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    
    args = parser.parse_args()
    
    tool = BuildTool(args.project_root)
    
    # 如果没有指定任何选项，执行完整构建
    if not any([args.clean, args.configure, args.build, args.install]):
        if tool.full_build():
            return 0
        else:
            return 1
    
    # 执行指定步骤
    success = True
    
    if args.clean:
        success = success and tool.clean()
    
    if args.configure:
        success = success and tool.configure()
    
    if args.build:
        success = success and tool.build()
    
    if args.install:
        success = success and tool.install()
    
    return 0 if success else 1


if __name__ == "__main__":
    exit(main())
