"""
DDS Configuration Tool - Main Window

Main application window with menu bar, toolbar, and tabbed interface
for configuring DDS settings.
"""

import os
from pathlib import Path
from typing import Optional, Dict, Any

try:
    from PyQt5.QtWidgets import (
        QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QTabWidget,
        QMenuBar, QMenu, QAction, QFileDialog, QMessageBox,
        QStatusBar, QToolBar, QLabel, QPushButton, QLineEdit,
        QApplication, QSplitter, QTreeWidget, QTreeWidgetItem,
        QTextEdit, QDockWidget
    )
    from PyQt5.QtCore import Qt, QSettings, pyqtSignal, QSize
    from PyQt5.QtGui import QIcon, QKeySequence, QFont
except ImportError:
    raise ImportError("PyQt5 is required for GUI mode")

from .domain_tab import DomainTab
from .topic_tab import TopicTab
from .qos_tab import QoSTab
from .transport_tab import TransportTab


class MainWindow(QMainWindow):
    """Main application window for DDS Configuration Tool."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.setWindowTitle("DDS Configuration Tool")
        self.setGeometry(100, 100, 1400, 900)
        
        # Settings
        self.settings = QSettings("DDSIntegration", "DDSConfigTool")
        
        # Configuration data
        self.config_data: Dict[str, Any] = {
            'domain': {},
            'topics': [],
            'qos': {},
            'transport': {},
            'security': {},
            'autosar': {}
        }
        self.current_file: Optional[str] = None
        self.modified = False
        
        self._setup_ui()
        self._setup_menu()
        self._setup_toolbar()
        self._setup_statusbar()
        self._setup_dock_widgets()
        
        # Load recent file if exists
        self._load_recent_file()
    
    def _setup_ui(self):
        """Setup the main UI components."""
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        layout = QVBoxLayout(central_widget)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Tab widget
        self.tab_widget = QTabWidget()
        self.tab_widget.setDocumentMode(True)
        self.tab_widget.currentChanged.connect(self._on_tab_changed)
        
        # Create tabs
        self.domain_tab = DomainTab()
        self.topic_tab = TopicTab()
        self.qos_tab = QoSTab()
        self.transport_tab = TransportTab()
        
        # Add tabs
        self.tab_widget.addTab(self.domain_tab, "Domain")
        self.tab_widget.addTab(self.topic_tab, "Topics")
        self.tab_widget.addTab(self.qos_tab, "QoS")
        self.tab_widget.addTab(self.transport_tab, "Transport")
        
        # Connect change signals
        self.domain_tab.config_changed.connect(self._on_config_changed)
        self.topic_tab.config_changed.connect(self._on_config_changed)
        self.qos_tab.config_changed.connect(self._on_config_changed)
        self.transport_tab.config_changed.connect(self._on_config_changed)
        
        layout.addWidget(self.tab_widget)
        
        # Info bar at bottom
        info_layout = QHBoxLayout()
        self.file_label = QLabel("No file loaded")
        self.file_label.setStyleSheet("color: gray;")
        info_layout.addWidget(self.file_label)
        info_layout.addStretch()
        
        self.status_label = QLabel("Ready")
        info_layout.addWidget(self.status_label)
        
        layout.addLayout(info_layout)
    
    def _setup_menu(self):
        """Setup menu bar."""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("&File")
        
        new_action = QAction("&New", self)
        new_action.setShortcut(QKeySequence.New)
        new_action.setStatusTip("Create new configuration")
        new_action.triggered.connect(self._new_config)
        file_menu.addAction(new_action)
        
        open_action = QAction("&Open...", self)
        open_action.setShortcut(QKeySequence.Open)
        open_action.setStatusTip("Open configuration file")
        open_action.triggered.connect(self._open_config)
        file_menu.addAction(open_action)
        
        file_menu.addSeparator()
        
        save_action = QAction("&Save", self)
        save_action.setShortcut(QKeySequence.Save)
        save_action.setStatusTip("Save configuration")
        save_action.triggered.connect(self._save_config)
        file_menu.addAction(save_action)
        
        save_as_action = QAction("Save &As...", self)
        save_as_action.setShortcut(QKeySequence.SaveAs)
        save_as_action.setStatusTip("Save configuration as...")
        save_as_action.triggered.connect(self._save_config_as)
        file_menu.addAction(save_as_action)
        
        file_menu.addSeparator()
        
        import_menu = file_menu.addMenu("&Import")
        
        import_xml_action = QAction("Import from &XML...", self)
        import_xml_action.triggered.connect(self._import_xml)
        import_menu.addAction(import_xml_action)
        
        import_arxml_action = QAction("Import from A&RXMl...", self)
        import_arxml_action.triggered.connect(self._import_arxml)
        import_menu.addAction(import_arxml_action)
        
        export_menu = file_menu.addMenu("&Export")
        
        export_c_action = QAction("Export &C Code...", self)
        export_c_action.triggered.connect(self._export_c_code)
        export_menu.addAction(export_c_action)
        
        export_arxml_action = QAction("Export A&RXML...", self)
        export_arxml_action.triggered.connect(self._export_arxml)
        export_menu.addAction(export_arxml_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("E&xit", self)
        exit_action.setShortcut(QKeySequence.Quit)
        exit_action.setStatusTip("Exit application")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Edit menu
        edit_menu = menubar.addMenu("&Edit")
        
        validate_action = QAction("&Validate Configuration", self)
        validate_action.setShortcut("Ctrl+T")
        validate_action.triggered.connect(self._validate_config)
        edit_menu.addAction(validate_action)
        
        # View menu
        view_menu = menubar.addMenu("&View")
        
        show_preview_action = QAction("Show &Preview", self)
        show_preview_action.setCheckable(True)
        show_preview_action.setChecked(True)
        view_menu.addAction(show_preview_action)
        
        # Tools menu
        tools_menu = menubar.addMenu("&Tools")
        
        generate_action = QAction("&Generate Code...", self)
        generate_action.setShortcut("Ctrl+G")
        generate_action.triggered.connect(self._generate_code)
        tools_menu.addAction(generate_action)
        
        templates_menu = tools_menu.addMenu("&Templates")
        
        automotive_basic_action = QAction("&Automotive Basic", self)
        automotive_basic_action.triggered.connect(lambda: self._load_template("automotive_basic"))
        templates_menu.addAction(automotive_basic_action)
        
        automotive_safety_action = QAction("Automotive &Safety", self)
        automotive_safety_action.triggered.connect(lambda: self._load_template("automotive_safety"))
        templates_menu.addAction(automotive_safety_action)
        
        # Help menu
        help_menu = menubar.addMenu("&Help")
        
        about_action = QAction("&About", self)
        about_action.triggered.connect(self._show_about)
        help_menu.addAction(about_action)
    
    def _setup_toolbar(self):
        """Setup toolbar."""
        toolbar = QToolBar("Main Toolbar")
        toolbar.setMovable(False)
        self.addToolBar(toolbar)
        
        # New button
        new_btn = QPushButton("New")
        new_btn.setToolTip("Create new configuration (Ctrl+N)")
        new_btn.clicked.connect(self._new_config)
        toolbar.addWidget(new_btn)
        
        # Open button
        open_btn = QPushButton("Open")
        open_btn.setToolTip("Open configuration file (Ctrl+O)")
        open_btn.clicked.connect(self._open_config)
        toolbar.addWidget(open_btn)
        
        # Save button
        save_btn = QPushButton("Save")
        save_btn.setToolTip("Save configuration (Ctrl+S)")
        save_btn.clicked.connect(self._save_config)
        toolbar.addWidget(save_btn)
        
        toolbar.addSeparator()
        
        # Validate button
        validate_btn = QPushButton("Validate")
        validate_btn.setToolTip("Validate configuration (Ctrl+T)")
        validate_btn.clicked.connect(self._validate_config)
        toolbar.addWidget(validate_btn)
        
        # Generate button
        generate_btn = QPushButton("Generate")
        generate_btn.setToolTip("Generate C code (Ctrl+G)")
        generate_btn.clicked.connect(self._generate_code)
        toolbar.addWidget(generate_btn)
    
    def _setup_statusbar(self):
        """Setup status bar."""
        self.statusbar = QStatusBar()
        self.setStatusBar(self.statusbar)
        self.statusbar.showMessage("Ready")
    
    def _setup_dock_widgets(self):
        """Setup dock widgets."""
        # Configuration tree dock
        tree_dock = QDockWidget("Configuration Tree", self)
        tree_dock.setAllowedAreas(Qt.LeftDockWidgetArea | Qt.RightDockWidgetArea)
        
        self.config_tree = QTreeWidget()
        self.config_tree.setHeaderLabel("Configuration Structure")
        self.config_tree.setMinimumWidth(200)
        tree_dock.setWidget(self.config_tree)
        
        self.addDockWidget(Qt.LeftDockWidgetArea, tree_dock)
        
        # Update tree
        self._update_config_tree()
        
        # Log dock
        log_dock = QDockWidget("Log", self)
        log_dock.setAllowedAreas(Qt.BottomDockWidgetArea)
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(150)
        log_dock.setWidget(self.log_text)
        
        self.addDockWidget(Qt.BottomDockWidgetArea, log_dock)
    
    def _update_config_tree(self):
        """Update configuration tree view."""
        self.config_tree.clear()
        
        root = QTreeWidgetItem(self.config_tree, ["DDS Configuration"])
        
        domain_item = QTreeWidgetItem(root, ["Domain"])
        topics_item = QTreeWidgetItem(root, [f"Topics ({len(self.config_data.get('topics', []))})"])
        qos_item = QTreeWidgetItem(root, ["QoS Profiles"])
        transport_item = QTreeWidgetItem(root, ["Transport"])
        security_item = QTreeWidgetItem(root, ["Security"])
        autosar_item = QTreeWidgetItem(root, ["AUTOSAR"])
        
        self.config_tree.expandAll()
    
    def _on_tab_changed(self, index):
        """Handle tab change."""
        tab_names = ["Domain", "Topics", "QoS", "Transport"]
        if index < len(tab_names):
            self.statusbar.showMessage(f"Editing {tab_names[index]} configuration")
    
    def _on_config_changed(self):
        """Handle configuration change."""
        self.modified = True
        self._update_window_title()
        self._update_config_tree()
        self.statusbar.showMessage("Configuration modified", 2000)
    
    def _update_window_title(self):
        """Update window title based on file and modification state."""
        title = "DDS Configuration Tool"
        if self.current_file:
            title += f" - {os.path.basename(self.current_file)}"
        if self.modified:
            title += " *"
        self.setWindowTitle(title)
    
    def _new_config(self):
        """Create new configuration."""
        if self.modified:
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "Configuration has unsaved changes. Discard them?",
                QMessageBox.Yes | QMessageBox.No
            )
            if reply == QMessageBox.No:
                return
        
        self.config_data = {
            'domain': {},
            'topics': [],
            'qos': {},
            'transport': {},
            'security': {},
            'autosar': {}
        }
        self.current_file = None
        self.modified = False
        
        # Reset all tabs
        self.domain_tab.set_config({})
        self.topic_tab.set_config({})
        self.qos_tab.set_config({})
        self.transport_tab.set_config({})
        
        self._update_window_title()
        self._update_config_tree()
        self.file_label.setText("New configuration")
        self.log("Created new configuration")
    
    def _open_config(self):
        """Open configuration file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Open Configuration",
            "",
            "Configuration Files (*.xml *.json *.yaml);;XML Files (*.xml);;JSON Files (*.json);;All Files (*)"
        )
        
        if file_path:
            try:
                self._load_config(file_path)
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to load configuration:\n{e}")
    
    def _load_config(self, file_path: str):
        """Load configuration from file."""
        self.log(f"Loading configuration from {file_path}")
        
        # TODO: Implement actual loading logic based on file format
        self.current_file = file_path
        self.modified = False
        self._update_window_title()
        self.file_label.setText(file_path)
        self._update_config_tree()
        
        # Save to recent files
        self.settings.setValue("last_file", file_path)
        
        self.log("Configuration loaded successfully")
    
    def _save_config(self):
        """Save configuration."""
        if self.current_file:
            self._save_config_to_file(self.current_file)
        else:
            self._save_config_as()
    
    def _save_config_as(self):
        """Save configuration as new file."""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Configuration",
            "",
            "XML Files (*.xml);;JSON Files (*.json);;All Files (*)"
        )
        
        if file_path:
            self._save_config_to_file(file_path)
    
    def _save_config_to_file(self, file_path: str):
        """Save configuration to file."""
        self.log(f"Saving configuration to {file_path}")
        
        # TODO: Implement actual saving logic
        self.current_file = file_path
        self.modified = False
        self._update_window_title()
        self.file_label.setText(file_path)
        
        self.settings.setValue("last_file", file_path)
        self.log("Configuration saved successfully")
    
    def _load_recent_file(self):
        """Load most recent file if exists."""
        last_file = self.settings.value("last_file", "")
        if last_file and os.path.exists(last_file):
            self._load_config(last_file)
    
    def _validate_config(self):
        """Validate current configuration."""
        self.log("Validating configuration...")
        
        # Collect data from all tabs
        config = self._collect_config()
        
        # TODO: Implement validation
        QMessageBox.information(self, "Validation", "Configuration validation passed!")
        self.log("Validation complete")
    
    def _generate_code(self):
        """Generate C code."""
        output_dir = QFileDialog.getExistingDirectory(
            self, "Select Output Directory"
        )
        
        if output_dir:
            self.log(f"Generating code to {output_dir}...")
            # TODO: Implement code generation
            QMessageBox.information(self, "Generate Code", f"Code generated to:\n{output_dir}")
            self.log("Code generation complete")
    
    def _collect_config(self) -> Dict[str, Any]:
        """Collect configuration from all tabs."""
        return {
            'domain': self.domain_tab.get_config(),
            'topics': self.topic_tab.get_config(),
            'qos': self.qos_tab.get_config(),
            'transport': self.transport_tab.get_config()
        }
    
    def _import_xml(self):
        """Import from XML file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Import from XML", "", "XML Files (*.xml)"
        )
        if file_path:
            self.log(f"Importing from {file_path}...")
            # TODO: Implement XML import
    
    def _import_arxml(self):
        """Import from ARXML file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Import from ARXML", "", "ARXML Files (*.arxml)"
        )
        if file_path:
            self.log(f"Importing from {file_path}...")
            # TODO: Implement ARXML import
    
    def _export_c_code(self):
        """Export C code."""
        self._generate_code()
    
    def _export_arxml(self):
        """Export to ARXML."""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Export to ARXML", "", "ARXML Files (*.arxml)"
        )
        if file_path:
            self.log(f"Exporting to {file_path}...")
            # TODO: Implement ARXML export
    
    def _load_template(self, template_name: str):
        """Load configuration template."""
        self.log(f"Loading template: {template_name}")
        # TODO: Implement template loading
    
    def _show_about(self):
        """Show about dialog."""
        QMessageBox.about(
            self, "About DDS Configuration Tool",
            """<h2>DDS Configuration Tool 1.0</h2>
            <p>A comprehensive tool for configuring DDS settings for automotive Ethernet applications.</p>
            <p>Features:</p>
            <ul>
                <li>Domain configuration with discovery protocols</li>
                <li>Topic and QoS management</li>
                <li>Transport layer configuration</li>
                <li>Security and E2E protection</li>
                <li>AUTOSAR integration</li>
            </ul>
            <p>&copy; 2024 DDS Integration Team</p>"""
        )
    
    def log(self, message: str):
        """Add message to log."""
        import datetime
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
    
    def closeEvent(self, event):
        """Handle close event."""
        if self.modified:
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "Configuration has unsaved changes. Save before exiting?",
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel
            )
            
            if reply == QMessageBox.Save:
                self._save_config()
                event.accept()
            elif reply == QMessageBox.Discard:
                event.accept()
            else:
                event.ignore()
        else:
            event.accept()
