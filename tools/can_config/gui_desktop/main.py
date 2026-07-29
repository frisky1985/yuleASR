#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN配置工具 - 桌面版GUI

基于PyQt6的桌面应用程序
"""

import sys
import json
import tempfile
import os
from pathlib import Path
from datetime import datetime

try:
    from PyQt6.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QLabel, QPushButton, QLineEdit, QTableWidget, QTableWidgetItem,
        QTabWidget, QTextEdit, QFileDialog, QMessageBox, QGroupBox,
        QSplitter, QHeaderView, QComboBox, QSpinBox, QDoubleSpinBox,
        QDialog, QFormLayout, QDialogButtonBox, QStatusBar, QProgressBar,
        QFrame, QScrollArea, QToolBar, QMenuBar, QMenu, QSizePolicy
    )
    from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize
    from PyQt6.QtGui import QAction, QIcon, QFont, QDragEnterEvent, QDropEvent
    
    PYQT6_AVAILABLE = True
except ImportError:
    PYQT6_AVAILABLE = False
    print("错误: 未安装PyQt6。请运行: pip install PyQt6")
    sys.exit(1)

# 添加工具路径
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from dbc_parser import DbcParser
from can_matrix_parser import CanMatrixParser
from com_config_generator import ComConfigGenerator


class ConfigWorker(QThread):
    """后台工作线程"""
    finished = pyqtSignal(bool, str, dict)
    
    def __init__(self, task, **kwargs):
        super().__init__()
        self.task = task
        self.kwargs = kwargs
    
    def run(self):
        try:
            if self.task == 'parse':
                result = self._parse_file()
            elif self.task == 'generate':
                result = self._generate_config()
            else:
                result = {'success': False, 'error': '未知任务'}
            
            self.finished.emit(
                result.get('success', False),
                result.get('message', ''),
                result
            )
        except Exception as e:
            self.finished.emit(False, str(e), {})
    
    def _parse_file(self):
        filepath = self.kwargs.get('filepath')
        ext = Path(filepath).suffix.lower()
        
        if ext == '.dbc':
            parser = DbcParser()
            network = parser.parse_file(filepath)
            config = parser.to_com_config()
        elif ext == '.csv':
            parser = CanMatrixParser()
            matrix = parser.parse_csv(filepath)
            config = parser.to_com_config()
        elif ext in ('.xlsx', '.xls'):
            parser = CanMatrixParser()
            matrix = parser.parse_excel(filepath)
            config = parser.to_com_config()
        else:
            return {'success': False, 'error': '不支持的文件格式'}
        
        return {
            'success': True,
            'message': f'成功解析文件',
            'config': config,
            'source_type': ext[1:].upper()
        }
    
    def _generate_config(self):
        config = self.kwargs.get('config')
        generator = ComConfigGenerator(config)
        
        output_dir = tempfile.mkdtemp()
        cfg_h_path, cfg_c_path = generator.generate(output_dir)
        
        with open(cfg_h_path, 'r') as f:
            cfg_h_content = f.read()
        with open(cfg_c_path, 'r') as f:
            cfg_c_content = f.read()
        
        # 清理
        os.remove(cfg_h_path)
        os.remove(cfg_c_path)
        os.rmdir(output_dir)
        
        return {
            'success': True,
            'message': '配置文件生成成功',
            'files': {
                'Com_Cfg.h': cfg_h_content,
                'Com_Cfg.c': cfg_c_content
            }
        }


class EditIpduDialog(QDialog):
    """编辑IPDU对话框"""
    def __init__(self, ipdu, parent=None):
        super().__init__(parent)
        self.ipdu = ipdu
        self.setWindowTitle(f"编辑 IPDU - {ipdu['name']}")
        self.setMinimumWidth(400)
        self.setup_ui()
    
    def setup_ui(self):
        layout = QFormLayout(self)
        
        # 名称 (只读)
        self.name_edit = QLineEdit(self.ipdu['name'])
        self.name_edit.setReadOnly(True)
        layout.addRow("名称:", self.name_edit)
        
        # 方向
        self.direction_combo = QComboBox()
        self.direction_combo.addItems(["发送 (SEND)", "接收 (RECEIVE)"])
        self.direction_combo.setCurrentIndex(0 if self.ipdu['direction'] == 'SEND' else 1)
        layout.addRow("方向:", self.direction_combo)
        
        # 周期时间
        self.cycle_spin = QSpinBox()
        self.cycle_spin.setRange(0, 10000)
        self.cycle_spin.setValue(self.ipdu.get('cycle_time', 100))
        self.cycle_spin.setSuffix(" ms")
        layout.addRow("周期时间:", self.cycle_spin)
        
        # 按钮
        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Save | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)
    
    def get_data(self):
        return {
            'direction': 'SEND' if self.direction_combo.currentIndex() == 0 else 'RECEIVE',
            'cycle_time': self.cycle_spin.value()
        }


class EditSignalDialog(QDialog):
    """编辑信号对话框"""
    def __init__(self, signal, parent=None):
        super().__init__(parent)
        self.signal = signal
        self.setWindowTitle(f"编辑 信号 - {signal['name']}")
        self.setMinimumWidth(400)
        self.setup_ui()
    
    def setup_ui(self):
        layout = QFormLayout(self)
        
        # 名称
        self.name_edit = QLineEdit(self.signal['name'])
        self.name_edit.setReadOnly(True)
        layout.addRow("名称:", self.name_edit)
        
        # 数据类型
        self.type_edit = QLineEdit(self.signal['data_type'])
        self.type_edit.setReadOnly(True)
        layout.addRow("数据类型:", self.type_edit)
        
        # 起始位
        self.start_spin = QSpinBox()
        self.start_spin.setRange(0, 63)
        self.start_spin.setValue(self.signal['start_bit'])
        layout.addRow("起始位:", self.start_spin)
        
        # 长度
        self.length_spin = QSpinBox()
        self.length_spin.setRange(1, 64)
        self.length_spin.setValue(self.signal['bit_length'])
        layout.addRow("长度:", self.length_spin)
        
        # 因子
        self.factor_spin = QDoubleSpinBox()
        self.factor_spin.setRange(-1000000, 1000000)
        self.factor_spin.setDecimals(6)
        self.factor_spin.setValue(self.signal['factor'])
        layout.addRow("因子 (Factor):", self.factor_spin)
        
        # 偏移
        self.offset_spin = QDoubleSpinBox()
        self.offset_spin.setRange(-1000000, 1000000)
        self.offset_spin.setDecimals(6)
        self.offset_spin.setValue(self.signal['offset'])
        layout.addRow("偏移 (Offset):", self.offset_spin)
        
        # 初始值
        self.init_spin = QSpinBox()
        self.init_spin.setRange(-2147483648, 2147483647)
        self.init_spin.setValue(self.signal['init_value'])
        layout.addRow("初始值:", self.init_spin)
        
        # 按钮
        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Save | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)
    
    def get_data(self):
        return {
            'start_bit': self.start_spin.value(),
            'bit_length': self.length_spin.value(),
            'factor': self.factor_spin.value(),
            'offset': self.offset_spin.value(),
            'init_value': self.init_spin.value()
        }


class MainWindow(QMainWindow):
    """主窗口"""
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CAN配置工具 - yuleASR")
        self.setMinimumSize(1400, 900)
        
        # 配置状态
        self.config = {
            'ecu_name': 'ECU0',
            'ipdus': [],
            'signals': [],
            'signal_groups': []
        }
        self.generated_files = None
        
        self.setup_ui()
        self.setup_menu()
        self.setup_toolbar()
        self.setup_statusbar()
    
    def setup_ui(self):
        """设置UI"""
        # 中央部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 主分割器
        main_splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # 左侧面板
        left_panel = self.create_left_panel()
        main_splitter.addWidget(left_panel)
        
        # 右侧面板
        right_panel = self.create_right_panel()
        main_splitter.addWidget(right_panel)
        
        # 设置分割比例
        main_splitter.setSizes([350, 1050])
        
        # 布局
        layout = QHBoxLayout(central_widget)
        layout.addWidget(main_splitter)
        layout.setContentsMargins(10, 10, 10, 10)
    
    def create_left_panel(self):
        """创建左侧面板"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setSpacing(15)
        
        # 文件导入组
        file_group = QGroupBox("📁 文件导入")
        file_layout = QVBoxLayout(file_group)
        
        # 文件拖放区域
        self.drop_frame = QFrame()
        self.drop_frame.setFrameStyle(QFrame.Shape.StyledPanel | QFrame.Shadow.Sunken)
        self.drop_frame.setAcceptDrops(True)
        self.drop_frame.setMinimumHeight(120)
        self.drop_frame.setStyleSheet("""
            QFrame {
                background-color: #f8f9fa;
                border: 2px dashed #d0d7de;
                border-radius: 10px;
            }
            QFrame:hover {
                border-color: #667eea;
                background-color: #f0f3ff;
            }
        """)
        self.drop_frame.dragEnterEvent = self.dragEnterEvent
        self.drop_frame.dropEvent = self.dropEvent
        
        drop_layout = QVBoxLayout(self.drop_frame)
        drop_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        drop_icon = QLabel("📤")
        drop_icon.setStyleSheet("font-size: 36px;")
        drop_icon.setAlignment(Qt.AlignmentFlag.AlignCenter)
        drop_layout.addWidget(drop_icon)
        
        drop_text = QLabel("点击或拖拽文件到此处")
        drop_text.setAlignment(Qt.AlignmentFlag.AlignCenter)
        drop_text.setStyleSheet("color: #666; margin: 5px;")
        drop_layout.addWidget(drop_text)
        
        drop_hint = QLabel("支持 DBC, CSV, Excel (.xlsx)")
        drop_hint.setAlignment(Qt.AlignmentFlag.AlignCenter)
        drop_hint.setStyleSheet("color: #999; font-size: 12px;")
        drop_layout.addWidget(drop_hint)
        
        # 点击上传
        self.drop_frame.mousePressEvent = lambda e: self.open_file_dialog()
        
        file_layout.addWidget(self.drop_frame)
        
        # 或按钮
        or_label = QLabel("或")
        or_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        or_label.setStyleSheet("color: #999; margin: 5px;")
        file_layout.addWidget(or_label)
        
        btn_browse = QPushButton("📂 浏览文件...")
        btn_browse.setMinimumHeight(40)
        btn_browse.clicked.connect(self.open_file_dialog)
        file_layout.addWidget(btn_browse)
        
        layout.addWidget(file_group)
        
        # 全局配置组
        config_group = QGroupBox("⚙️ 全局配置")
        config_layout = QFormLayout(config_group)
        
        self.ecu_edit = QLineEdit("ECU0")
        self.ecu_edit.textChanged.connect(self.update_ecu_name)
        config_layout.addRow("ECU名称:", self.ecu_edit)
        
        self.source_file_label = QLabel("暂未导入")
        self.source_file_label.setStyleSheet("color: #999; font-style: italic;")
        config_layout.addRow("源文件:", self.source_file_label)
        
        self.source_type_label = QLabel("-")
        self.source_type_label.setStyleSheet("color: #999;")
        config_layout.addRow("文件类型:", self.source_type_label)
        
        layout.addWidget(config_group)
        
        # 操作组
        action_group = QGroupBox("🔧 操作")
        action_layout = QVBoxLayout(action_group)
        
        self.btn_generate = QPushButton("🚀 生成配置文件")
        self.btn_generate.setMinimumHeight(45)
        self.btn_generate.setEnabled(False)
        self.btn_generate.setStyleSheet("""
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                    stop:0 #667eea, stop:1 #764ba2);
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: bold;
                font-size: 14px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                    stop:0 #5a6fd6, stop:1 #6a4190);
            }
            QPushButton:disabled {
                background: #ccc;
            }
        """)
        self.btn_generate.clicked.connect(self.generate_config)
        action_layout.addWidget(self.btn_generate)
        
        self.btn_reset = QPushButton("🔄 重置")
        self.btn_reset.setMinimumHeight(40)
        self.btn_reset.clicked.connect(self.reset_config)
        action_layout.addWidget(self.btn_reset)
        
        layout.addWidget(action_group)
        
        # 统计组
        stats_group = QGroupBox("📊 统计信息")
        stats_layout = QFormLayout(stats_group)
        
        self.stat_ipdu = QLabel("0")
        self.stat_ipdu.setStyleSheet("font-size: 20px; font-weight: bold; color: #667eea;")
        stats_layout.addRow("IPDU数量:", self.stat_ipdu)
        
        self.stat_signals = QLabel("0")
        self.stat_signals.setStyleSheet("font-size: 20px; font-weight: bold; color: #667eea;")
        stats_layout.addRow("信号数量:", self.stat_signals)
        
        self.stat_rx = QLabel("0")
        self.stat_rx.setStyleSheet("font-size: 20px; font-weight: bold; color: #27ae60;")
        stats_layout.addRow("接收消息:", self.stat_rx)
        
        self.stat_tx = QLabel("0")
        self.stat_tx.setStyleSheet("font-size: 20px; font-weight: bold; color: #e74c3c;")
        stats_layout.addRow("发送消息:", self.stat_tx)
        
        layout.addWidget(stats_group)
        
        layout.addStretch()
        
        scroll = QScrollArea()
        scroll.setWidget(panel)
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        
        return scroll
    
    def create_right_panel(self):
        """创建右侧面板"""
        panel = QTabWidget()
        
        # IPDU列表选项卡
        self.tab_ipdu = QWidget()
        self.setup_ipdu_tab()
        panel.addTab(self.tab_ipdu, "IPDU列表")
        
        # 信号列表选项卡
        self.tab_signals = QWidget()
        self.setup_signals_tab()
        panel.addTab(self.tab_signals, "信号列表")
        
        # 代码预览选项卡
        self.tab_preview = QWidget()
        self.setup_preview_tab()
        panel.addTab(self.tab_preview, "代码预览")
        
        return panel
    
    def setup_ipdu_tab(self):
        """设置IPDU选项卡"""
        layout = QVBoxLayout(self.tab_ipdu)
        
        # 提示文字
        self.ipdu_empty_label = QLabel(
            "\n\n\n📤\n\n暂无数据\n\n请先导入DBC、CSV或Excel文件\n\n")
        self.ipdu_empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.ipdu_empty_label.setStyleSheet("font-size: 16px; color: #999;")
        layout.addWidget(self.ipdu_empty_label)
        
        # 表格
        self.ipdu_table = QTableWidget()
        self.ipdu_table.setColumnCount(6)
        self.ipdu_table.setHorizontalHeaderLabels([
            "名称", "CAN ID", "DLC", "方向", "信号数", "操作"
        ])
        self.ipdu_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.ipdu_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        self.ipdu_table.horizontalHeader().setSectionResizeMode(5, QHeaderView.ResizeMode.ResizeToContents)
        self.ipdu_table.setAlternatingRowColors(True)
        self.ipdu_table.setStyleSheet("""
            QTableWidget {
                border: 1px solid #ddd;
                border-radius: 8px;
            }
            QTableWidget::item {
                padding: 8px;
            }
            QHeaderView::section {
                background: #f8f9fa;
                padding: 10px;
                font-weight: bold;
                border: none;
                border-bottom: 2px solid #ddd;
            }
        """)
        self.ipdu_table.hide()
        layout.addWidget(self.ipdu_table)
    
    def setup_signals_tab(self):
        """设置信号选项卡"""
        layout = QVBoxLayout(self.tab_signals)
        
        # 提示文字
        self.signals_empty_label = QLabel(
            "\n\n\n📤\n\n暂无数据\n\n请先导入DBC、CSV或Excel文件\n\n")
        self.signals_empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.signals_empty_label.setStyleSheet("font-size: 16px; color: #999;")
        layout.addWidget(self.signals_empty_label)
        
        # 表格
        self.signals_table = QTableWidget()
        self.signals_table.setColumnCount(8)
        self.signals_table.setHorizontalHeaderLabels([
            "名称", "数据类型", "起始位", "长度", "因子", "偏移", "初始值", "操作"
        ])
        self.signals_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.signals_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        self.signals_table.horizontalHeader().setSectionResizeMode(7, QHeaderView.ResizeMode.ResizeToContents)
        self.signals_table.setAlternatingRowColors(True)
        self.signals_table.setStyleSheet("""
            QTableWidget {
                border: 1px solid #ddd;
                border-radius: 8px;
            }
            QTableWidget::item {
                padding: 8px;
            }
            QHeaderView::section {
                background: #f8f9fa;
                padding: 10px;
                font-weight: bold;
                border: none;
                border-bottom: 2px solid #ddd;
            }
        """)
        self.signals_table.hide()
        layout.addWidget(self.signals_table)
    
    def setup_preview_tab(self):
        """设置预览选项卡"""
        layout = QVBoxLayout(self.tab_preview)
        
        # 工具栏
        toolbar = QHBoxLayout()
        
        self.btn_preview_h = QPushButton("Com_Cfg.h")
        self.btn_preview_h.setCheckable(True)
        self.btn_preview_h.setChecked(True)
        self.btn_preview_h.clicked.connect(lambda: self.switch_preview('h'))
        toolbar.addWidget(self.btn_preview_h)
        
        self.btn_preview_c = QPushButton("Com_Cfg.c")
        self.btn_preview_c.setCheckable(True)
        self.btn_preview_c.clicked.connect(lambda: self.switch_preview('c'))
        toolbar.addWidget(self.btn_preview_c)
        
        toolbar.addStretch()
        
        self.btn_download_h = QPushButton("💾 下载 .h")
        self.btn_download_h.clicked.connect(lambda: self.download_file('Com_Cfg.h'))
        self.btn_download_h.setEnabled(False)
        toolbar.addWidget(self.btn_download_h)
        
        self.btn_download_c = QPushButton("💾 下载 .c")
        self.btn_download_c.clicked.connect(lambda: self.download_file('Com_Cfg.c'))
        self.btn_download_c.setEnabled(False)
        toolbar.addWidget(self.btn_download_c)
        
        layout.addLayout(toolbar)
        
        # 预览空状态
        self.preview_empty_label = QLabel(
            "\n\n\n📄\n\n暂无预览\n\n点击'生成配置文件'查看代码预览\n\n")
        self.preview_empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview_empty_label.setStyleSheet("font-size: 16px; color: #999;")
        layout.addWidget(self.preview_empty_label)
        
        # 代码编辑器
        self.preview_edit = QTextEdit()
        self.preview_edit.setFont(QFont("Consolas", 10))
        self.preview_edit.setReadOnly(True)
        self.preview_edit.setStyleSheet("""
            QTextEdit {
                background-color: #1e1e1e;
                color: #d4d4d4;
                border-radius: 8px;
                padding: 15px;
            }
        """)
        self.preview_edit.hide()
        layout.addWidget(self.preview_edit)
    
    def setup_menu(self):
        """设置菜单"""
        menubar = self.menuBar()
        
        # 文件菜单
        file_menu = menubar.addMenu("文件(&F)")
        
        open_action = QAction("打开(&O)...", self)
        open_action.setShortcut("Ctrl+O")
        open_action.triggered.connect(self.open_file_dialog)
        file_menu.addAction(open_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("退出(&X)", self)
        exit_action.setShortcut("Alt+F4")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # 工具菜单
        tools_menu = menubar.addMenu("工具(&T)")
        
        gen_action = QAction("生成配置(&G)", self)
        gen_action.setShortcut("Ctrl+G")
        gen_action.triggered.connect(self.generate_config)
        tools_menu.addAction(gen_action)
        
        # 帮助菜单
        help_menu = menubar.addMenu("帮助(&H)")
        
        about_action = QAction("关于(&A)", self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
    
    def setup_toolbar(self):
        """设置工具栏"""
        toolbar = QToolBar()
        self.addToolBar(toolbar)
        
        open_action = QAction("📁 打开", self)
        open_action.triggered.connect(self.open_file_dialog)
        toolbar.addAction(open_action)
        
        toolbar.addSeparator()
        
        gen_action = QAction("🚀 生成", self)
        gen_action.triggered.connect(self.generate_config)
        toolbar.addAction(gen_action)
    
    def setup_statusbar(self):
        """设置状态栏"""
        self.statusbar = QStatusBar()
        self.setStatusBar(self.statusbar)
        self.statusbar.showMessage("就绪")
    
    def dragEnterEvent(self, event: QDragEnterEvent):
        """拖拽进入"""
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
    
    def dropEvent(self, event: QDropEvent):
        """拖拽放置"""
        urls = event.mimeData().urls()
        if urls:
            filepath = urls[0].toLocalFile()
            if filepath.lower().endswith(('.dbc', '.csv', '.xlsx', '.xls')):
                self.parse_file(filepath)
    
    def open_file_dialog(self):
        """打开文件对话框"""
        filepath, _ = QFileDialog.getOpenFileName(
            self,
            "选择文件",
            "",
            "CAN配置文件 (*.dbc *.csv *.xlsx *.xls);;"
            "DBC文件 (*.dbc);;"
            "CSV文件 (*.csv);;"
            "Excel文件 (*.xlsx *.xls);;"
            "所有文件 (*.*)"
        )
        
        if filepath:
            self.parse_file(filepath)
    
    def parse_file(self, filepath):
        """解析文件"""
        self.statusbar.showMessage(f"正在解析: {filepath}...")
        
        # 启动工作线程
        self.worker = ConfigWorker('parse', filepath=filepath)
        self.worker.finished.connect(self.on_parse_finished)
        self.worker.start()
    
    def on_parse_finished(self, success, message, data):
        """解析完成回调"""
        if success:
            self.config.update(data['config'])
            self.ecu_edit.setText(self.config['ecu_name'])
            self.source_file_label.setText(Path(data['config'].get('source_file', '')).name)
            self.source_type_label.setText(data.get('source_type', '-'))
            self.source_file_label.setStyleSheet("color: #333;")
            
            self.update_ui()
            self.btn_generate.setEnabled(True)
            self.statusbar.showMessage(f"✓ 解析成功: {message}", 5000)
        else:
            QMessageBox.critical(self, "解析失败", message)
            self.statusbar.showMessage("解析失败")
    
    def update_ecu_name(self, name):
        """更新ECU名称"""
        self.config['ecu_name'] = name
    
    def update_ui(self):
        """更新UI"""
        # 统计
        self.stat_ipdu.setText(str(len(self.config['ipdus'])))
        self.stat_signals.setText(str(len(self.config['signals'])))
        
        rx_count = sum(1 for ipdu in self.config['ipdus'] if ipdu['direction'] == 'RECEIVE')
        tx_count = sum(1 for ipdu in self.config['ipdus'] if ipdu['direction'] == 'SEND')
        self.stat_rx.setText(str(rx_count))
        self.stat_tx.setText(str(tx_count))
        
        # IPDU表格
        self.update_ipdu_table()
        
        # 信号表格
        self.update_signals_table()
    
    def update_ipdu_table(self):
        """更新IPDU表格"""
        if not self.config['ipdus']:
            self.ipdu_empty_label.show()
            self.ipdu_table.hide()
            return
        
        self.ipdu_empty_label.hide()
        self.ipdu_table.show()
        
        self.ipdu_table.setRowCount(len(self.config['ipdus']))
        
        for row, ipdu in enumerate(self.config['ipdus']):
            self.ipdu_table.setItem(row, 0, QTableWidgetItem(ipdu['name']))
            self.ipdu_table.setItem(row, 1, 
                QTableWidgetItem(f"0x{ipdu['message_id']:04X}"))
            self.ipdu_table.setItem(row, 2, QTableWidgetItem(str(ipdu['dlc'])))
            
            direction_item = QTableWidgetItem(
                "🔴 发送" if ipdu['direction'] == 'SEND' else "🟢 接收"
            )
            self.ipdu_table.setItem(row, 3, direction_item)
            
            signal_count = len(ipdu.get('signals', []))
            self.ipdu_table.setItem(row, 4, QTableWidgetItem(str(signal_count)))
            
            btn_edit = QPushButton("✏️ 编辑")
            btn_edit.clicked.connect(lambda checked, r=row: self.edit_ipdu(r))
            self.ipdu_table.setCellWidget(row, 5, btn_edit)
    
    def update_signals_table(self):
        """更新信号表格"""
        if not self.config['signals']:
            self.signals_empty_label.show()
            self.signals_table.hide()
            return
        
        self.signals_empty_label.hide()
        self.signals_table.show()
        
        self.signals_table.setRowCount(len(self.config['signals']))
        
        for row, sig in enumerate(self.config['signals']):
            self.signals_table.setItem(row, 0, QTableWidgetItem(sig['name']))
            self.signals_table.setItem(row, 1, QTableWidgetItem(sig['data_type']))
            self.signals_table.setItem(row, 2, QTableWidgetItem(str(sig['start_bit'])))
            self.signals_table.setItem(row, 3, QTableWidgetItem(str(sig['bit_length'])))
            self.signals_table.setItem(row, 4, QTableWidgetItem(str(sig['factor'])))
            self.signals_table.setItem(row, 5, QTableWidgetItem(str(sig['offset'])))
            self.signals_table.setItem(row, 6, QTableWidgetItem(str(sig['init_value'])))
            
            btn_edit = QPushButton("✏️ 编辑")
            btn_edit.clicked.connect(lambda checked, r=row: self.edit_signal(r))
            self.signals_table.setCellWidget(row, 7, btn_edit)
    
    def edit_ipdu(self, row):
        """编辑IPDU"""
        dialog = EditIpduDialog(self.config['ipdus'][row], self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            data = dialog.get_data()
            self.config['ipdus'][row].update(data)
            self.update_ui()
    
    def edit_signal(self, row):
        """编辑信号"""
        dialog = EditSignalDialog(self.config['signals'][row], self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            data = dialog.get_data()
            self.config['signals'][row].update(data)
            self.update_ui()
    
    def generate_config(self):
        """生成配置文件"""
        self.btn_generate.setEnabled(False)
        self.btn_generate.setText("生成中...")
        
        self.worker = ConfigWorker('generate', config=self.config)
        self.worker.finished.connect(self.on_generate_finished)
        self.worker.start()
    
    def on_generate_finished(self, success, message, data):
        """生成完成回调"""
        self.btn_generate.setEnabled(True)
        self.btn_generate.setText("🚀 生成配置文件")
        
        if success:
            self.generated_files = data['files']
            
            # 显示预览
            self.preview_empty_label.hide()
            self.preview_edit.show()
            self.switch_preview('h')
            
            # 启用下载按钮
            self.btn_download_h.setEnabled(True)
            self.btn_download_c.setEnabled(True)
            
            # 切换到预览选项卡
            self.tab_preview.parent().setCurrentWidget(self.tab_preview)
            
            self.statusbar.showMessage("✓ 配置文件生成成功", 5000)
        else:
            QMessageBox.critical(self, "生成失败", message)
            self.statusbar.showMessage("生成失败")
    
    def switch_preview(self, file_type):
        """切换预览"""
        if not self.generated_files:
            return
        
        if file_type == 'h':
            self.btn_preview_h.setChecked(True)
            self.btn_preview_c.setChecked(False)
            self.preview_edit.setPlainText(self.generated_files['Com_Cfg.h'])
        else:
            self.btn_preview_h.setChecked(False)
            self.btn_preview_c.setChecked(True)
            self.preview_edit.setPlainText(self.generated_files['Com_Cfg.c'])
    
    def download_file(self, filename):
        """下载文件"""
        if not self.generated_files or filename not in self.generated_files:
            QMessageBox.warning(self, "警告", "请先生成配置文件")
            return
        
        filepath, _ = QFileDialog.getSaveFileName(
            self,
            f"保存 {filename}",
            filename,
            "C文件 (*.c);;头文件 (*.h);;所有文件 (*.*)"
        )
        
        if filepath:
            try:
                with open(filepath, 'w') as f:
                    f.write(self.generated_files[filename])
                self.statusbar.showMessage(f"✓ 已保存: {filepath}", 5000)
            except Exception as e:
                QMessageBox.critical(self, "保存失败", str(e))
    
    def reset_config(self):
        """重置配置"""
        self.config = {
            'ecu_name': 'ECU0',
            'ipdus': [],
            'signals': [],
            'signal_groups': []
        }
        self.generated_files = None
        
        self.ecu_edit.setText('ECU0')
        self.source_file_label.setText("暂未导入")
        self.source_file_label.setStyleSheet("color: #999; font-style: italic;")
        self.source_type_label.setText("-")
        
        self.update_ui()
        
        # 重置预览
        self.preview_edit.hide()
        self.preview_empty_label.show()
        self.btn_download_h.setEnabled(False)
        self.btn_download_c.setEnabled(False)
        
        self.btn_generate.setEnabled(False)
        self.statusbar.showMessage("已重置")
    
    def show_about(self):
        """显示关于对话框"""
        QMessageBox.about(
            self,
            "关于 CAN配置工具",
            "<h2>CAN配置工具</h2>"
            "<p>版本: 1.0.0</p>"
            "<p>基于PyQt6的桌面应用程序</p>"
            "<p>支持从DBC、CSV、Excel导入生成AUTOSAR Com配置</p>"
            "<p>yuleASR 工具集</p>"
        )


def main():
    """主函数"""
    app = QApplication(sys.argv)
    
    # 设置应用样式
    app.setStyle('Fusion')
    
    # 创建并显示主窗口
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
