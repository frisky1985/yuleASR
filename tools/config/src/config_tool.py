#!/usr/bin/env python3
"""
YuleTech BSW Configuration Tool

基于AutoSAR标准的BSW配置工具
"""

import json
import os
from typing import Dict, Any, Optional
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class ModuleConfig:
    """模块配置基类"""
    name: str
    enabled: bool = True
    version: str = "1.0.0"
    parameters: Dict[str, Any] = None
    
    def __post_init__(self):
        if self.parameters is None:
            self.parameters = {}


@dataclass
class McuConfig(ModuleConfig):
    """MCU配置"""
    clock_frequency: int = 800000000  # 800MHz
    core_count: int = 4
    
    def __init__(self):
        super().__init__(name="Mcu", version="1.0.0")


@dataclass
class CanConfig(ModuleConfig):
    """CAN配置"""
    baudrate: int = 500000
    controller_count: int = 2
    
    def __init__(self):
        super().__init__(name="Can", version="1.0.0")


class ConfigManager:
    """配置管理器"""
    
    def __init__(self, project_path: str):
        self.project_path = Path(project_path)
        self.config_file = self.project_path / "config" / "bsw_config.json"
        self.modules: Dict[str, ModuleConfig] = {}
        
    def load_config(self) -> bool:
        """加载配置"""
        if not self.config_file.exists():
            return False
            
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                
            for name, config_data in data.get('modules', {}).items():
                if name == "Mcu":
                    self.modules[name] = McuConfig()
                elif name == "Can":
                    self.modules[name] = CanConfig()
                else:
                    self.modules[name] = ModuleConfig(name=name)
                    
                # 更新参数
                for key, value in config_data.items():
                    if hasattr(self.modules[name], key):
                        setattr(self.modules[name], key, value)
                        
            return True
        except Exception as e:
            print(f"Error loading config: {e}")
            return False
    
    def save_config(self) -> bool:
        """保存配置"""
        try:
            self.config_file.parent.mkdir(parents=True, exist_ok=True)
            
            data = {
                'version': '1.0.0',
                'modules': {}
            }
            
            for name, module in self.modules.items():
                data['modules'][name] = asdict(module)
                
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                
            return True
        except Exception as e:
            print(f"Error saving config: {e}")
            return False
    
    def add_module(self, module: ModuleConfig):
        """添加模块"""
        self.modules[module.name] = module
        
    def get_module(self, name: str) -> Optional[ModuleConfig]:
        """获取模块配置"""
        return self.modules.get(name)
    
    def validate_config(self) -> bool:
        """验证配置"""
        for name, module in self.modules.items():
            if not module.enabled:
                continue
                
            # 验证必需参数
            if not module.version:
                print(f"Module {name}: version is required")
                return False
                
        return True


def main():
    """主函数"""
    print("YuleTech BSW Configuration Tool v1.0.0")
    print("=" * 50)
    
    # 创建默认配置
    config_mgr = ConfigManager(".")
    
    # 添加默认模块
    config_mgr.add_module(McuConfig())
    config_mgr.add_module(CanConfig())
    
    # 保存配置
    if config_mgr.save_config():
        print("[OK] Configuration saved successfully")
    else:
        print("[FAIL] Failed to save configuration")
        return 1
        
    # 验证配置
    if config_mgr.validate_config():
        print("[OK] Configuration validation passed")
    else:
        print("[FAIL] Configuration validation failed")
        return 1
        
    print("\nConfiguration complete!")
    return 0


if __name__ == "__main__":
    exit(main())
