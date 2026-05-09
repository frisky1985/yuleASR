#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Generator - PyQt6 GUI

Vector Configurator风格的桌面可视化配置工具
"""

import sys
import json
from pathlib import Path
from typing import Dict, Any

# 添加src到路径
sys.path.insert(0, str(Path(__file__).parent / "src"))

try:
    from PyQt6.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QLabel, QPushButton, QLineEdit, QComboBox, QCheckBox, QSpinBox,
        QDoubleSpinBox, QTreeWidget, QTreeWidgetItem, QTextEdit, QMessageBox,
        QFileDialog, QGroupBox, QFormLayout, QSplitter, QTabWidget,
        QScrollArea, QFrame, QToolBar, QStatusBar
    )
    from PyQt6.QtCore import Qt, QSize
    from PyQt6.QtGui import QFont, QIcon, QAction
    GUI_AVAILABLE = True
except ImportError:
    GUI_AVAILABLE = False
    print("❌ 需要安装PyQt6: pip3 install PyQt6")
    sys.exit(1)

from mcal_config_generator import (
    create_mcu_config, create_port_config, create_can_config
)
from bsw_config_generator import create_com_config, create_nvm_config


class ArxmlGeneratorGUI(QMainWindow):
    """ARXML生成器主窗口"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("yuleASR ARXML Generator - 可视化配置工具")
        self.setMinimumSize(1400, 900)
        
        # 当前状态
        self.current_module = None
        self.module_config = {}
        self.generated_arxml = ""
        
        # 初始化UI
        self.init_ui()
        self.init_toolbar()
        self.init_statusbar()
        
    def init_ui(self):
        """初始化界面"""
        # 主分割器
        main_splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # 左侧模块选择面板
        self.module_panel = self.create_module_panel()
        main_splitter.addWidget(self.module_panel)
        
        # 中间配置面板
        self.config_panel = self.create_config_panel()
        main_splitter.addWidget(self.config_panel)
        
        # 右侧预览面板
        self.preview_panel = self.create_preview_panel()
        main_splitter.addWidget(self.preview_panel)
        
        # 设置分割比例
        main_splitter.setSizes([250, 600, 550])
        
        self.setCentralWidget(main_splitter)
        
    def create_module_panel(self) -> QWidget:
        """创建模块选择面板"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # 标题
        title = QLabel("📤 模块选择")
        title.setFont(QFont("Microsoft YaHei", 12, QFont.Weight.Bold))
        layout.addWidget(title)
        
        # MCAL 模块
        mcal_group = QGroupBox("MCAL 微控制器驱动")
        mcal_layout = QVBoxLayout(mcal_group)
        
        mcal_modules = [
            ("Mcu", "🔫 微控制器驱动"),
            ("Port", "🔗 GPIO配置"),
            ("Can", "📡 CAN通信"),
            ("Spi", "🔄 SPI通信"),
            ("Gpt", "⏰ 通用定时器"),
            ("Pwm", "📈 脉宽调制"),
            ("Adc", "📊 模拟转换"),
        ]
        
        for mod_id, mod_name in mcal_modules:
            btn = QPushButton(mod_name)
            btn.setStyleSheet("""
                QPushButton {
                    text-align: left;
                    padding: 10px;
                    border: none;
                    border-radius: 5px;
                    background: #f3f4f6;
                }
                QPushButton:hover {
                    background: #e5e7eb;
                }
                QPushButton:pressed {
                    background: #3b82f6;
                    color: white;
                }
            """)
            btn.clicked.connect(lambda checked, m=mod_id: self.on_module_selected(m))
            mcal_layout.addWidget(btn)
            
        layout.addWidget(mcal_group)
        
        # BSW 模块
        bsw_group = QGroupBox("BSW 基础软件")
        bsw_layout = QVBoxLayout(bsw_group)
        
        bsw_modules = [
            ("Com", "💬 通信服务"),
            ("PduR", "📋 PDU路由"),
            ("NvM", "💾 NVRAM管理"),
        ]
        
        for mod_id, mod_name in bsw_modules:
            btn = QPushButton(mod_name)
            btn.setStyleSheet("""
                QPushButton {
                    text-align: left;
                    padding: 10px;
                    border: none;
                    border-radius: 5px;
                    background: #ecfdf5;
                }
                QPushButton:hover {
                    background: #d1fae5;
                }
                QPushButton:pressed {
                    background: #10b981;
                    color: white;
                }
            """)
            btn.clicked.connect(lambda checked, m=mod_id: self.on_module_selected(m))
            bsw_layout.addWidget(btn)
            
        layout.addWidget(bsw_group)
        layout.addStretch()
        
        return panel
        
    def create_config_panel(self) -> QWidget:
        """创建配置面板"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # 标题栏
        header = QHBoxLayout()
        self.config_title = QLabel("请选择模块")
        self.config_title.setFont(QFont("Microsoft YaHei", 14, QFont.Weight.Bold))
        header.addWidget(self.config_title)
        
        # ECU名称
        header.addWidget(QLabel("ECU:"))
        self.ecu_input = QLineEdit("ECU0")
        self.ecu_input.setMaximumWidth(150)
        header.addWidget(self.ecu_input)
        
        header.addStretch()
        layout.addLayout(header)
        
        # 滚动区域
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        
        self.config_container = QWidget()
        self.config_layout = QVBoxLayout(self.config_container)
        self.config_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        
        # 默认提示
        hint = QLabel("👆 请从左侧选择一个模块进行配置")
        hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        hint.setStyleSheet("color: #9ca3af; font-size: 16px; margin-top: 50px;")
        self.config_layout.addWidget(hint)
        
        scroll.setWidget(self.config_container)
        layout.addWidget(scroll)
        
        # 生成按钮
        self.generate_btn = QPushButton("🚀 生成ARXML")
        self.generate_btn.setStyleSheet("""
            QPushButton {
                background: #3b82f6;
                color: white;
                padding: 12px 24px;
                font-size: 14px;
                font-weight: bold;
                border: none;
                border-radius: 6px;
            }
            QPushButton:hover {
                background: #2563eb;
            }
            QPushButton:disabled {
                background: #9ca3af;
            }
        """)
        self.generate_btn.setEnabled(False)
        self.generate_btn.clicked.connect(self.generate_arxml)
        layout.addWidget(self.generate_btn)
        
        return panel
        
    def create_preview_panel(self) -> QWidget:
        """创建预览面板"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # 标题栏
        header = QHBoxLayout()
        title = QLabel("📄 ARXML 预览")
        title.setFont(QFont("Microsoft YaHei", 12, QFont.Weight.Bold))
        header.addWidget(title)
        
        # 工具按钮
        copy_btn = QPushButton("📄 复制")
        copy_btn.clicked.connect(self.copy_to_clipboard)
        header.addWidget(copy_btn)
        
        download_btn = QPushButton("📥 下载")
        download_btn.clicked.connect(self.download_arxml)
        header.addWidget(download_btn)
        
        layout.addLayout(header)
        
        # 预览编辑器
        self.preview_edit = QTextEdit()
        self.preview_edit.setFont(QFont("Consolas", 10))
        self.preview_edit.setStyleSheet("""
            QTextEdit {
                background: #1f2937;
                color: #4ade80;
                border: 1px solid #374151;
                border-radius: 6px;
                padding: 10px;
            }
        """)
        self.preview_edit.setPlaceholderText("<!-- 选择模块并配置后，点击'生成ARXML'查看结果 -->")
        layout.addWidget(self.preview_edit)
        
        return panel
        
    def init_toolbar(self):
        """初始化工具栏"""
        toolbar = QToolBar()
        self.addToolBar(toolbar)
        
        new_action = QAction("📁 新建", self)
        new_action.triggered.connect(self.reset_config)
        toolbar.addAction(new_action)
        
        toolbar.addSeparator()
        
        help_action = QAction("❓ 帮助", self)
        help_action.triggered.connect(self.show_help)
        toolbar.addAction(help_action)
        
    def init_statusbar(self):
        """初始化状态栏"""
        self.statusbar = QStatusBar()
        self.setStatusBar(self.statusbar)
        self.statusbar.showMessage("就绪 - 请选择模块")
        
    def on_module_selected(self, module_id: str):
        """模块选择事件"""
        self.current_module = module_id
        self.config_title.setText(f"配置: {module_id}")
        self.generate_btn.setEnabled(True)
        
        # 清除旧配置
        while self.config_layout.count():
            child = self.config_layout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
                
        # 创建模块配置表单
        self.create_module_config_form(module_id)
        self.statusbar.showMessage(f"当前模块: {module_id}")
        
    def create_module_config_form(self, module_id: str):
        """创建模块配置表单"""
        self.module_config = {}
        
        if module_id == "Mcu":
            self.create_mcu_form()
        elif module_id == "Port":
            self.create_port_form()
        elif module_id == "Can":
            self.create_can_form()
        elif module_id == "Com":
            self.create_com_form()
        elif module_id == "NvM":
            self.create_nvm_form()
        else:
            # 默认表单
            label = QLabel(f"暂未实现 {module_id} 的可视化配置")
            label.setStyleSheet("color: #ef4444;")
            self.config_layout.addWidget(label)
            
    def create_mcu_form(self):
        """创建MCU配置表单"""
        # 通用配置组
        general_group = QGroupBox("通用配置")
        general_layout = QFormLayout(general_group)
        
        self.mcu_dev_error = QCheckBox("启用开发错误检测")
        self.mcu_dev_error.setChecked(True)
        general_layout.addRow(self.mcu_dev_error)
        
        self.mcu_init_clock = QCheckBox("初始化时钟")
        self.mcu_init_clock.setChecked(True)
        general_layout.addRow(self.mcu_init_clock)
        
        self.mcu_version_api = QCheckBox("启用版本信息API")
        general_layout.addRow(self.mcu_version_api)
        
        self.config_layout.addWidget(general_group)
        
        # 时钟配置组
        clock_group = QGroupBox("时钟配置")
        clock_layout = QFormLayout(clock_group)
        
        self.mcu_cpu_clock = QSpinBox()
        self.mcu_cpu_clock.setRange(1000000, 200000000)
        self.mcu_cpu_clock.setValue(80000000)
        self.mcu_cpu_clock.setSuffix(" Hz")
        self.mcu_cpu_clock.setSingleStep(1000000)
        clock_layout.addRow("CPU时钟频率:", self.mcu_cpu_clock)
        
        self.mcu_periph_clock = QSpinBox()
        self.mcu_periph_clock.setRange(1000000, 100000000)
        self.mcu_periph_clock.setValue(40000000)
        self.mcu_periph_clock.setSuffix(" Hz")
        self.mcu_periph_clock.setSingleStep(1000000)
        clock_layout.addRow("外设时钟频率:", self.mcu_periph_clock)
        
        self.config_layout.addWidget(clock_group)
        self.config_layout.addStretch()
        
    def create_port_form(self):
        """创建Port配置表单"""
        # 通用配置
        general_group = QGroupBox("通用配置")
        general_layout = QFormLayout(general_group)
        
        self.port_dev_error = QCheckBox("启用开发错误检测")
        self.port_dev_error.setChecked(True)
        general_layout.addRow(self.port_dev_error)
        
        self.port_set_dir_api = QCheckBox("启用设置引脚方向API")
        self.port_set_dir_api.setChecked(True)
        general_layout.addRow(self.port_set_dir_api)
        
        self.config_layout.addWidget(general_group)
        
        # 引脚配置
        pin_group = QGroupBox("引脚配置")
        pin_layout = QVBoxLayout(pin_group)
        
        self.pin_configs = []
        for i in range(4):
            pin_frame = QFrame()
            pin_frame.setFrameShape(QFrame.Shape.StyledPanel)
            pin_layout_h = QHBoxLayout(pin_frame)
            
            pin_name = QLineEdit(f"PortPin_{i}")
            pin_name.setMaximumWidth(120)
            pin_layout_h.addWidget(QLabel(f"引脚{i}:"))
            pin_layout_h.addWidget(pin_name)
            
            pin_dir = QComboBox()
            pin_dir.addItems(["PORT_PIN_OUT", "PORT_PIN_IN"])
            pin_layout_h.addWidget(pin_dir)
            
            pin_mode = QComboBox()
            pin_mode.addItems(["PORT_PIN_MODE_GPIO", "PORT_PIN_MODE_CAN", "PORT_PIN_MODE_SPI"])
            pin_layout_h.addWidget(pin_mode)
            
            self.pin_configs.append((pin_name, pin_dir, pin_mode))
            pin_layout.addWidget(pin_frame)
            
        self.config_layout.addWidget(pin_group)
        self.config_layout.addStretch()
        
    def create_can_form(self):
        """创建CAN配置表单"""
        # 通用配置
        general_group = QGroupBox("通用配置")
        general_layout = QFormLayout(general_group)
        
        self.can_dev_error = QCheckBox("启用开发错误检测")
        self.can_dev_error.setChecked(True)
        general_layout.addRow(self.can_dev_error)
        
        self.can_index = QSpinBox()
        self.can_index.setRange(0, 255)
        general_layout.addRow("控制器索引:", self.can_index)
        
        self.can_period = QDoubleSpinBox()
        self.can_period.setRange(1.0, 1000.0)
        self.can_period.setValue(10.0)
        self.can_period.setSuffix(" ms")
        general_layout.addRow("主函数周期:", self.can_period)
        
        self.config_layout.addWidget(general_group)
        
        # 控制器配置
        ctrl_group = QGroupBox("控制器配置")
        ctrl_layout = QFormLayout(ctrl_group)
        
        self.can_baudrate = QSpinBox()
        self.can_baudrate.setRange(10000, 1000000)
        self.can_baudrate.setValue(500000)
        self.can_baudrate.setSuffix(" bps")
        self.can_baudrate.setSingleStep(100000)
        ctrl_layout.addRow("波特率:", self.can_baudrate)
        
        self.can_tx_objects = QSpinBox()
        self.can_tx_objects.setRange(1, 64)
        self.can_tx_objects.setValue(4)
        ctrl_layout.addRow("Tx对象数量:", self.can_tx_objects)
        
        self.can_rx_objects = QSpinBox()
        self.can_rx_objects.setRange(1, 64)
        self.can_rx_objects.setValue(2)
        ctrl_layout.addRow("Rx对象数量:", self.can_rx_objects)
        
        self.config_layout.addWidget(ctrl_group)
        self.config_layout.addStretch()
        
    def create_com_form(self):
        """创建COM配置表单"""
        general_group = QGroupBox("通用配置")
        layout = QFormLayout(general_group)
        
        self.com_dev_error = QCheckBox("启用开发错误检测")
        self.com_dev_error.setChecked(True)
        layout.addRow(self.com_dev_error)
        
        self.config_layout.addWidget(general_group)
        self.config_layout.addStretch()
        
    def create_nvm_form(self):
        """创建NvM配置表单"""
        common_group = QGroupBox("通用配置")
        layout = QFormLayout(common_group)
        
        self.nvm_blocks = QSpinBox()
        self.nvm_blocks.setRange(1, 256)
        self.nvm_blocks.setValue(8)
        layout.addRow("NVRAM块数量:", self.nvm_blocks)
        
        self.nvm_crc = QComboBox()
        self.nvm_crc.addItems(["NVM_CRC32", "NVM_CRC16", "NVM_CRC8", "NVM_CRC_NONE"])
        layout.addRow("CRC类型:", self.nvm_crc)
        
        self.config_layout.addWidget(common_group)
        self.config_layout.addStretch()
        
    def generate_arxml(self):
        """生成ARXML"""
        if not self.current_module:
            QMessageBox.warning(self, "警告", "请先选择一个模块")
            return
            
        ecu_name = self.ecu_input.text() or "ECU0"
        
        try:
            if self.current_module == "Mcu":
                gen = create_mcu_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.mcu_dev_error.isChecked(),
                    init_clock=self.mcu_init_clock.isChecked(),
                    version_info_api=self.mcu_version_api.isChecked()
                )
                gen.add_clock_config(
                    cpu_clock=self.mcu_cpu_clock.value(),
                    peripheral_clock=self.mcu_periph_clock.value()
                )
                
            elif self.current_module == "Port":
                gen = create_port_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.port_dev_error.isChecked(),
                    set_pin_direction_api=self.port_set_dir_api.isChecked()
                )
                # 添加引脚配置
                for i, (name, direction, mode) in enumerate(self.pin_configs):
                    gen.add_pin_config(
                        pin_name=name.text(),
                        pin_id=i,
                        direction=direction.currentText(),
                        mode=mode.currentText()
                    )
                    
            elif self.current_module == "Can":
                gen = create_can_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.can_dev_error.isChecked(),
                    index=self.can_index.value(),
                    main_function_period=self.can_period.value()
                )
                gen.add_controller_config(
                    controller_id=0,
                    baudrate=self.can_baudrate.value(),
                    tx_objects=self.can_tx_objects.value(),
                    rx_objects=self.can_rx_objects.value()
                )
                
            elif self.current_module == "Com":
                gen = create_com_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.com_dev_error.isChecked()
                )
                
            elif self.current_module == "NvM":
                gen = create_nvm_config(ecu_name)
                crc_map = {"NVM_CRC8": 1, "NVM_CRC16": 2, "NVM_CRC32": 4, "NVM_CRC_NONE": 0}
                crc_bytes = crc_map.get(self.nvm_crc.currentText(), 4)
                gen.add_common_config(crc_num_bytes=crc_bytes)
                
                # 添加NVRAM块
                for i in range(self.nvm_blocks.value()):
                    gen.add_block_descriptor(
                        block_name=f"NvMBlockDescriptor_{i}",
                        block_id=i,
                        block_size=32 + i * 16,
                        crc_type=self.nvm_crc.currentText()
                    )
            else:
                QMessageBox.information(self, "提示", f"{self.current_module} 暂未实现")
                return
                
            self.generated_arxml = gen.to_arxml()
            self.preview_edit.setText(self.generated_arxml)
            self.statusbar.showMessage(f"✓ {self.current_module} 配置生成成功")
            
        except Exception as e:
            QMessageBox.critical(self, "错误", f"生成ARXML失败:\n{str(e)}")
            
    def copy_to_clipboard(self):
        """复制到剪贴板"""
        if not self.generated_arxml:
            QMessageBox.warning(self, "警告", "请先生成ARXML")
            return
            
        clipboard = QApplication.clipboard()
        clipboard.setText(self.generated_arxml)
        self.statusbar.showMessage("✓ 已复制到剪贴板", 2000)
        
    def download_arxml(self):
        """下载ARXML"""
        if not self.generated_arxml:
            QMessageBox.warning(self, "警告", "请先生成ARXML")
            return
            
        filename, _ = QFileDialog.getSaveFileName(
            self, "保存ARXML文件",
            f"{self.current_module}.arxml",
            "ARXML Files (*.arxml);;All Files (*.*)"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(self.generated_arxml)
                self.statusbar.showMessage(f"✓ 已保存到 {filename}", 3000)
            except Exception as e:
                QMessageBox.critical(self, "错误", f"保存失败:\n{str(e)}")
                
    def reset_config(self):
        """重置配置"""
        self.current_module = None
        self.module_config = {}
        self.generated_arxml = ""
        self.preview_edit.clear()
        self.config_title.setText("请选择模块")
        self.generate_btn.setEnabled(False)
        
        # 清除配置表单
        while self.config_layout.count():
            child = self.config_layout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
                
        # 添加默认提示
        hint = QLabel("👆 请从左侧选择一个模块进行配置")
        hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        hint.setStyleSheet("color: #9ca3af; font-size: 16px; margin-top: 50px;")
        self.config_layout.addWidget(hint)
        
        self.statusbar.showMessage("已重置")
        
    def show_help(self):
        """显示帮助"""
        QMessageBox.information(self, "帮助", """
<b>yuleASR ARXML Generator</b>

快速开始:
1. 从左侧选择需要配置的模块
2. 在中间面板填写配置参数
3. 点击"生成ARXML"按钮
4. 在右侧预览区查看结果
5. 使用复制或下载按钮保存配置

支持的模块:
- MCAL: Mcu, Port, CAN, SPI, GPT, PWM, ADC
- BSW: COM, PduR, NvM

版本: 1.0.0
        """)


def main():
    """主函数"""
    if not GUI_AVAILABLE:
        print("❌ 需要安装PyQt6:")
        print("   pip3 install PyQt6")
        sys.exit(1)
        
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    # 设置应用样式
    app.setStyleSheet("""
        QMainWindow {
            background: #f3f4f6;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 12px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    """)
    
    window = ArxmlGeneratorGUI()
    window.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
