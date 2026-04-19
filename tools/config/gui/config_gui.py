#!/usr/bin/env python3
"""
YuleTech BSW Configuration GUI

基于 PyQt5 的图形化配置工具
"""

import sys
import json
from pathlib import Path
from typing import Dict, Any, Optional

try:
    from PyQt5.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QLabel, QLineEdit, QPushButton, QComboBox, QSpinBox, QCheckBox,
        QTreeWidget, QTreeWidgetItem, QTabWidget, QTextEdit, QMessageBox,
        QFileDialog, QGroupBox, QFormLayout, QSplitter
    )
    from PyQt5.QtCore import Qt
    from PyQt5.QtGui import QFont
    PYQT_AVAILABLE = True
except ImportError:
    PYQT_AVAILABLE = False
    print("PyQt5 not available, using CLI mode")


class ModuleConfigWidget(QWidget):
    """模块配置控件"""
    
    def __init__(self, module_name: str, parent=None):
        super().__init__(parent)
        self.module_name = module_name
        self.init_ui()
        
    def init_ui(self):
        layout = QFormLayout()
        
        # 模块启用/禁用
        self.enabled_cb = QCheckBox("Enable Module")
        self.enabled_cb.setChecked(True)
        layout.addRow("Status:", self.enabled_cb)
        
        # 版本
        self.version_edit = QLineEdit("1.0.0")
        layout.addRow("Version:", self.version_edit)
        
        self.setLayout(layout)
        
    def get_config(self) -> Dict[str, Any]:
        return {
            "name": self.module_name,
            "enabled": self.enabled_cb.isChecked(),
            "version": self.version_edit.text()
        }
        
    def set_config(self, config: Dict[str, Any]):
        self.enabled_cb.setChecked(config.get("enabled", True))
        self.version_edit.setText(config.get("version", "1.0.0"))


class McuConfigWidget(ModuleConfigWidget):
    """MCU 配置控件"""
    
    def __init__(self, parent=None):
        super().__init__("Mcu", parent)
        self.add_mcu_specific()
        
    def add_mcu_specific(self):
        layout = self.layout()
        
        # 时钟频率
        self.clock_spin = QSpinBox()
        self.clock_spin.setRange(1000000, 1000000000)
        self.clock_spin.setValue(168000000)
        self.clock_spin.setSuffix(" Hz")
        layout.addRow("Clock Frequency:", self.clock_spin)
        
        # 核心数
        self.core_spin = QSpinBox()
        self.core_spin.setRange(1, 8)
        self.core_spin.setValue(4)
        layout.addRow("Core Count:", self.core_spin)
        
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "clock_frequency": self.clock_spin.value(),
            "core_count": self.core_spin.value()
        })
        return config
        
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.clock_spin.setValue(config.get("clock_frequency", 168000000))
        self.core_spin.setValue(config.get("core_count", 4))


class CanConfigWidget(ModuleConfigWidget):
    """CAN 配置控件"""
    
    def __init__(self, parent=None):
        super().__init__("Can", parent)
        self.add_can_specific()
        
    def add_can_specific(self):
        layout = self.layout()
        
        # 波特率
        self.baudrate_combo = QComboBox()
        self.baudrate_combo.addItems(["125000", "250000", "500000", "1000000"])
        self.baudrate_combo.setCurrentText("500000")
        layout.addRow("Baudrate:", self.baudrate_combo)
        
        # 控制器数量
        self.controller_spin = QSpinBox()
        self.controller_spin.setRange(1, 4)
        self.controller_spin.setValue(2)
        layout.addRow("Controller Count:", self.controller_spin)
        
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "baudrate": int(self.baudrate_combo.currentText()),
            "controller_count": self.controller_spin.value()
        })
        return config
        
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        baudrate = str(config.get("baudrate", 500000))
        index = self.baudrate_combo.findText(baudrate)
        if index >= 0:
            self.baudrate_combo.setCurrentIndex(index)
        self.controller_spin.setValue(config.get("controller_count", 2))


class ConfigMainWindow(QMainWindow):
    """主窗口"""
    
    def __init__(self):
        super().__init__()
        self.config_data = {"version": "1.0.0", "modules": {}}
        self.module_widgets = {}
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("YuleTech BSW Configuration Tool")
        self.setGeometry(100, 100, 1200, 800)
        
        # 中央部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 主布局
        main_layout = QHBoxLayout(central_widget)
        
        # 分割器
        splitter = QSplitter(Qt.Horizontal)
        main_layout.addWidget(splitter)
        
        # 左侧：模块树
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        
        # 模块树
        self.module_tree = QTreeWidget()
        self.module_tree.setHeaderLabel("Modules")
        self.module_tree.itemClicked.connect(self.on_module_selected)
        left_layout.addWidget(self.module_tree)
        
        # 初始化模块树
        self.init_module_tree()
        
        splitter.addWidget(left_widget)
        
        # 右侧：配置区域
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        
        # 标签页
        self.tab_widget = QTabWidget()
        
        # 通用配置页
        self.general_tab = QWidget()
        self.init_general_tab()
        self.tab_widget.addTab(self.general_tab, "General")
        
        # 模块配置页
        self.module_tabs = {}
        self.init_module_tabs()
        
        right_layout.addWidget(self.tab_widget)
        
        # 按钮区域
        button_layout = QHBoxLayout()
        
        self.new_btn = QPushButton("New")
        self.new_btn.clicked.connect(self.new_config)
        button_layout.addWidget(self.new_btn)
        
        self.open_btn = QPushButton("Open")
        self.open_btn.clicked.connect(self.open_config)
        button_layout.addWidget(self.open_btn)
        
        self.save_btn = QPushButton("Save")
        self.save_btn.clicked.connect(self.save_config)
        button_layout.addWidget(self.save_btn)
        
        self.validate_btn = QPushButton("Validate")
        self.validate_btn.clicked.connect(self.validate_config)
        button_layout.addWidget(self.validate_btn)
        
        self.generate_btn = QPushButton("Generate Code")
        self.generate_btn.clicked.connect(self.generate_code)
        button_layout.addWidget(self.generate_btn)
        
        right_layout.addLayout(button_layout)
        
        # 日志区域
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(150)
        right_layout.addWidget(self.log_text)
        
        splitter.addWidget(right_widget)
        splitter.setSizes([300, 900])
        
        self.log("YuleTech BSW Configuration Tool v1.0.0")
        self.log("Ready")
        
    def init_module_tree(self):
        """初始化模块树"""
        # MCAL
        mcal_item = QTreeWidgetItem(self.module_tree, ["MCAL"])
        mcal_modules = ["Mcu", "Port", "Dio", "Can", "Spi", "Gpt", "Pwm", "Adc", "Wdg"]
        for module in mcal_modules:
            QTreeWidgetItem(mcal_item, [module])
            
        # ECUAL
        ecual_item = QTreeWidgetItem(self.module_tree, ["ECUAL"])
        ecual_modules = ["CanIf", "IoHwAb", "CanTp", "EthIf", "MemIf", "Fee", "Ea", "FrIf", "LinIf"]
        for module in ecual_modules:
            QTreeWidgetItem(ecual_item, [module])
            
        # Services
        services_item = QTreeWidgetItem(self.module_tree, ["Services"])
        services_modules = ["Com", "PduR", "NvM", "Dcm", "Dem"]
        for module in services_modules:
            QTreeWidgetItem(services_item, [module])
            
        # RTE
        rte_item = QTreeWidgetItem(self.module_tree, ["RTE"])
        QTreeWidgetItem(rte_item, ["Rte"])
        
        self.module_tree.expandAll()
        
    def init_general_tab(self):
        """初始化通用配置页"""
        layout = QFormLayout()
        
        # 项目版本
        self.project_version = QLineEdit("1.0.0")
        layout.addRow("Project Version:", self.project_version)
        
        # 目标平台
        self.target_platform = QComboBox()
        self.target_platform.addItems([
            "i.MX8M Mini",
            "STM32F4xx",
            "STM32H7xx",
            "Generic Cortex-M4",
            "Generic Cortex-M7"
        ])
        layout.addRow("Target Platform:", self.target_platform)
        
        # 编译器
        self.compiler = QComboBox()
        self.compiler.addItems(["GCC", "IAR", "Keil"])
        layout.addRow("Compiler:", self.compiler)
        
        self.general_tab.setLayout(layout)
        
    def init_module_tabs(self):
        """初始化模块配置页"""
        # Mcu
        mcu_widget = McuConfigWidget()
        self.module_tabs["Mcu"] = mcu_widget
        self.tab_widget.addTab(mcu_widget, "Mcu")
        
        # Can
        can_widget = CanConfigWidget()
        self.module_tabs["Can"] = can_widget
        self.tab_widget.addTab(can_widget, "Can")
        
    def on_module_selected(self, item: QTreeWidgetItem, column: int):
        """模块选择事件"""
        module_name = item.text(0)
        if module_name in self.module_tabs:
            self.tab_widget.setCurrentWidget(self.module_tabs[module_name])
            
    def log(self, message: str):
        """添加日志"""
        self.log_text.append(message)
        
    def new_config(self):
        """新建配置"""
        self.config_data = {"version": "1.0.0", "modules": {}}
        self.project_version.setText("1.0.0")
        self.log("Created new configuration")
        
    def open_config(self):
        """打开配置"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Open Configuration", "", "JSON Files (*.json)"
        )
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.config_data = json.load(f)
                self.log(f"Loaded configuration from {file_path}")
                
                # 更新 UI
                for name, config in self.config_data.get("modules", {}).items():
                    if name in self.module_tabs:
                        self.module_tabs[name].set_config(config)
                        
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to load configuration:\n{str(e)}")
                
    def save_config(self):
        """保存配置"""
        # 收集配置
        self.config_data["version"] = self.project_version.text()
        
        for name, widget in self.module_tabs.items():
            self.config_data["modules"][name] = widget.get_config()
            
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Configuration", "bsw_config.json", "JSON Files (*.json)"
        )
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    json.dump(self.config_data, f, indent=2)
                self.log(f"Saved configuration to {file_path}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save configuration:\n{str(e)}")
                
    def validate_config(self):
        """验证配置"""
        errors = []
        
        # 验证版本
        if not self.project_version.text():
            errors.append("Project version is required")
            
        # 验证模块
        for name, widget in self.module_tabs.items():
            config = widget.get_config()
            if config.get("enabled") and not config.get("version"):
                errors.append(f"Module {name}: version is required when enabled")
                
        if errors:
            QMessageBox.warning(self, "Validation Failed", "\n".join(errors))
            self.log("Validation failed")
        else:
            QMessageBox.information(self, "Validation", "Configuration is valid!")
            self.log("Validation passed")
            
    def generate_code(self):
        """生成代码"""
        self.log("Generating code...")
        
        # 这里调用代码生成器
        import subprocess
        import sys
        
        try:
            result = subprocess.run([
                sys.executable,
                "tools/generator/src/code_generator.py",
                "config/bsw_config.json",
                "generated"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                self.log("Code generation completed successfully")
                QMessageBox.information(self, "Code Generation", "Code generated successfully!")
            else:
                self.log(f"Code generation failed: {result.stderr}")
                QMessageBox.critical(self, "Code Generation", f"Failed:\n{result.stderr}")
                
        except Exception as e:
            self.log(f"Code generation error: {str(e)}")
            QMessageBox.critical(self, "Error", f"Code generation failed:\n{str(e)}")


def main():
    """主函数"""
    if not PYQT_AVAILABLE:
        print("Please install PyQt5: pip install PyQt5")
        return 1
        
    app = QApplication(sys.argv)
    
    # 设置字体
    font = QFont("Microsoft YaHei", 10)
    app.setFont(font)
    
    window = ConfigMainWindow()
    window.show()
    
    return app.exec_()


if __name__ == "__main__":
    exit(main())
