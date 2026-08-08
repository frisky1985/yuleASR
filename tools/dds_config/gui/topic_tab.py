"""
DDS Configuration Tool - Topic Tab

Configuration for DDS Topics including:
- Topic name and data type
- QoS policies
- Security settings
- AUTOSAR mappings
"""

from typing import Dict, Any, List, Optional

try:
    from PyQt5.QtWidgets import (
        QWidget, QVBoxLayout, QHBoxLayout, QFormLayout, QGridLayout,
        QLabel, QLineEdit, QComboBox, QSpinBox, QCheckBox, QGroupBox,
        QPushButton, QListWidget, QListWidgetItem, QTextEdit,
        QTableWidget, QTableWidgetItem, QHeaderView, QSplitter,
        QDialog, QDialogButtonBox, QTabWidget
    )
    from PyQt5.QtCore import Qt, pyqtSignal
except ImportError:
    raise ImportError("PyQt5 is required for GUI mode")


class TopicTab(QWidget):
    """Topic configuration tab."""
    
    config_changed = pyqtSignal()
    
    # DDS built-in types
    BUILTIN_TYPES = [
        "int8", "uint8", "int16", "uint16", "int32", "uint32",
        "int64", "uint64", "float32", "float64", "char", "wchar",
        "boolean", "octet", "string", "wstring", "sequence", "array"
    ]
    
    # AUTOSAR data types
    AUTOSAR_TYPES = [
        "boolean", "sint8", "uint8", "sint16", "uint16",
        "sint32", "uint32", "float32", "float64"
    ]
    
    # Topic kinds
    TOPIC_KINDS = [
        "STANDARD",       # Regular topic
        "SIGNAL",         # Signal-based topic
        "SERVICE",        # Service topic
        "EVENT",          # Event topic
        "FIELD"           # Field topic
    ]
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.topics: List[Dict[str, Any]] = []
        self.current_topic_index = -1
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup the UI components."""
        layout = QHBoxLayout(self)
        
        # Splitter
        splitter = QSplitter(Qt.Horizontal)
        
        # Left panel - Topic list
        left_panel = self._create_left_panel()
        splitter.addWidget(left_panel)
        
        # Right panel - Topic editor
        right_panel = self._create_right_panel()
        splitter.addWidget(right_panel)
        
        splitter.setSizes([300, 700])
        layout.addWidget(splitter)
    
    def _create_left_panel(self) -> QWidget:
        """Create left panel with topic list."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Title
        layout.addWidget(QLabel("<b>Topics</b>"))
        
        # Topic list
        self.topic_list = QListWidget()
        self.topic_list.currentRowChanged.connect(self._on_topic_selected)
        layout.addWidget(self.topic_list)
        
        # Buttons
        btn_layout = QHBoxLayout()
        
        add_btn = QPushButton("Add")
        add_btn.clicked.connect(self._add_topic)
        btn_layout.addWidget(add_btn)
        
        remove_btn = QPushButton("Remove")
        remove_btn.clicked.connect(self._remove_topic)
        btn_layout.addWidget(remove_btn)
        
        duplicate_btn = QPushButton("Duplicate")
        duplicate_btn.clicked.connect(self._duplicate_topic)
        btn_layout.addWidget(duplicate_btn)
        
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Filter
        filter_layout = QHBoxLayout()
        filter_layout.addWidget(QLabel("Filter:"))
        self.filter_edit = QLineEdit()
        self.filter_edit.setPlaceholderText("Filter topics...")
        self.filter_edit.textChanged.connect(self._filter_topics)
        filter_layout.addWidget(self.filter_edit)
        layout.addLayout(filter_layout)
        
        return widget
    
    def _create_right_panel(self) -> QWidget:
        """Create right panel with topic editor."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Topic editor widget (disabled when no topic selected)
        self.editor_widget = QWidget()
        editor_layout = QVBoxLayout(self.editor_widget)
        
        # Tabs
        tabs = QTabWidget()
        
        # Basic tab
        basic_tab = self._create_basic_tab()
        tabs.addTab(basic_tab, "Basic")
        
        # QoS tab
        qos_tab = self._create_qos_tab()
        tabs.addTab(qos_tab, "QoS")
        
        # Security tab
        security_tab = self._create_security_tab()
        tabs.addTab(security_tab, "Security")
        
        # AUTOSAR tab
        autosar_tab = self._create_autosar_tab()
        tabs.addTab(autosar_tab, "AUTOSAR")
        
        # Advanced tab
        advanced_tab = self._create_advanced_tab()
        tabs.addTab(advanced_tab, "Advanced")
        
        editor_layout.addWidget(tabs)
        
        # Save button
        save_layout = QHBoxLayout()
        save_layout.addStretch()
        save_btn = QPushButton("Apply Changes")
        save_btn.clicked.connect(self._save_current_topic)
        save_layout.addWidget(save_btn)
        editor_layout.addLayout(save_layout)
        
        layout.addWidget(self.editor_widget)
        
        # Empty state
        self.empty_label = QLabel("Select or create a topic to edit")
        self.empty_label.setAlignment(Qt.AlignCenter)
        self.empty_label.setStyleSheet("color: gray; font-size: 14px;")
        layout.addWidget(self.empty_label)
        
        self.editor_widget.hide()
        
        return widget
    
    def _create_basic_tab(self) -> QWidget:
        """Create basic settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Topic name
        self.topic_name_edit = QLineEdit()
        self.topic_name_edit.setPlaceholderText("Unique topic name")
        self.topic_name_edit.textChanged.connect(self._mark_dirty)
        layout.addRow("Topic Name:*", self.topic_name_edit)
        
        # Topic kind
        self.topic_kind_combo = QComboBox()
        self.topic_kind_combo.addItems(self.TOPIC_KINDS)
        self.topic_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        layout.addRow("Topic Kind:", self.topic_kind_combo)
        
        # Data type
        self.data_type_combo = QComboBox()
        self.data_type_combo.setEditable(True)
        self.data_type_combo.addItem("<Select or enter type>", "")
        self.data_type_combo.addItem("--- Built-in Types ---")
        for t in self.BUILTIN_TYPES:
            self.data_type_combo.addItem(t, f"builtin:{t}")
        self.data_type_combo.addItem("--- AUTOSAR Types ---")
        for t in self.AUTOSAR_TYPES:
            self.data_type_combo.addItem(t, f"autosar:{t}")
        self.data_type_combo.addItem("--- Custom Types ---")
        self.data_type_combo.currentIndexChanged.connect(self._mark_dirty)
        layout.addRow("Data Type:*", self.data_type_combo)
        
        # Description
        self.topic_desc_edit = QTextEdit()
        self.topic_desc_edit.setPlaceholderText("Topic description")
        self.topic_desc_edit.setMaximumHeight(80)
        self.topic_desc_edit.textChanged.connect(self._mark_dirty)
        layout.addRow("Description:", self.topic_desc_edit)
        
        # Instance settings
        instance_group = QGroupBox("Instance Configuration")
        instance_layout = QFormLayout(instance_group)
        
        self.instance_key_edit = QLineEdit()
        self.instance_key_edit.setPlaceholderText("Comma-separated key fields")
        self.instance_key_edit.textChanged.connect(self._mark_dirty)
        instance_layout.addRow("Key Fields:", self.instance_key_edit)
        
        self.max_instances_spin = QSpinBox()
        self.max_instances_spin.setRange(1, 10000)
        self.max_instances_spin.setValue(1)
        self.max_instances_spin.valueChanged.connect(self._mark_dirty)
        instance_layout.addRow("Max Instances:", self.max_instances_spin)
        
        layout.addRow(instance_group)
        
        layout.addStretch()
        return widget
    
    def _create_qos_tab(self) -> QWidget:
        """Create QoS settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Reliability
        reliability_group = QGroupBox("Reliability")
        reliability_layout = QFormLayout(reliability_group)
        
        self.reliability_combo = QComboBox()
        self.reliability_combo.addItems(["BEST_EFFORT", "RELIABLE"])
        self.reliability_combo.currentIndexChanged.connect(self._mark_dirty)
        reliability_layout.addRow("Kind:", self.reliability_combo)
        
        self.max_blocking_time_spin = QSpinBox()
        self.max_blocking_time_spin.setRange(0, 10000)
        self.max_blocking_time_spin.setValue(100)
        self.max_blocking_time_spin.setSuffix(" ms")
        reliability_layout.addRow("Max Blocking Time:", self.max_blocking_time_spin)
        
        layout.addRow(reliability_group)
        
        # Durability
        durability_group = QGroupBox("Durability")
        durability_layout = QFormLayout(durability_group)
        
        self.durability_combo = QComboBox()
        self.durability_combo.addItems(["VOLATILE", "TRANSIENT_LOCAL", "TRANSIENT", "PERSISTENT"])
        self.durability_combo.currentIndexChanged.connect(self._mark_dirty)
        durability_layout.addRow("Kind:", self.durability_combo)
        
        layout.addRow(durability_group)
        
        # History
        history_group = QGroupBox("History")
        history_layout = QFormLayout(history_group)
        
        self.history_kind_combo = QComboBox()
        self.history_kind_combo.addItems(["KEEP_LAST", "KEEP_ALL"])
        self.history_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        history_layout.addRow("Kind:", self.history_kind_combo)
        
        self.history_depth_spin = QSpinBox()
        self.history_depth_spin.setRange(1, 1000)
        self.history_depth_spin.setValue(1)
        history_layout.addRow("Depth:", self.history_depth_spin)
        
        layout.addRow(history_group)
        
        # Deadline
        deadline_group = QGroupBox("Deadline")
        deadline_layout = QFormLayout(deadline_group)
        
        self.deadline_enabled_check = QCheckBox("Enable Deadline")
        self.deadline_enabled_check.stateChanged.connect(self._mark_dirty)
        deadline_layout.addRow(self.deadline_enabled_check)
        
        self.deadline_period_spin = QSpinBox()
        self.deadline_period_spin.setRange(1, 10000)
        self.deadline_period_spin.setValue(1000)
        self.deadline_period_spin.setSuffix(" ms")
        deadline_layout.addRow("Period:", self.deadline_period_spin)
        
        layout.addRow(deadline_group)
        
        # Latency budget
        latency_group = QGroupBox("Latency Budget")
        latency_layout = QFormLayout(latency_group)
        
        self.latency_budget_spin = QSpinBox()
        self.latency_budget_spin.setRange(0, 10000)
        self.latency_budget_spin.setValue(0)
        self.latency_budget_spin.setSuffix(" ms")
        latency_layout.addRow("Duration:", self.latency_budget_spin)
        
        layout.addRow(latency_group)
        
        # Liveliness
        liveliness_group = QGroupBox("Liveliness")
        liveliness_layout = QFormLayout(liveliness_group)
        
        self.liveliness_combo = QComboBox()
        self.liveliness_combo.addItems(["AUTOMATIC", "MANUAL_BY_PARTICIPANT", "MANUAL_BY_TOPIC"])
        liveliness_layout.addRow("Kind:", self.liveliness_combo)
        
        self.lease_duration_spin = QSpinBox()
        self.lease_duration_spin.setRange(0, 3600)
        self.lease_duration_spin.setValue(10)
        self.lease_duration_spin.setSuffix(" s")
        liveliness_layout.addRow("Lease Duration:", self.lease_duration_spin)
        
        layout.addRow(liveliness_group)
        
        layout.addStretch()
        return widget
    
    def _create_security_tab(self) -> QWidget:
        """Create security settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # E2E Protection
        e2e_group = QGroupBox("E2E Protection")
        e2e_layout = QFormLayout(e2e_group)
        
        self.e2e_enabled_check = QCheckBox("Enable E2E Protection")
        self.e2e_enabled_check.stateChanged.connect(self._e2e_toggled)
        e2e_layout.addRow(self.e2e_enabled_check)
        
        self.e2e_profile_combo = QComboBox()
        self.e2e_profile_combo.addItems([
            "P01", "P02", "P04", "P05", "P06", "P07", "P11", "P22"
        ])
        self.e2e_profile_combo.currentIndexChanged.connect(self._mark_dirty)
        e2e_layout.addRow("E2E Profile:", self.e2e_profile_combo)
        
        self.e2e_data_id_spin = QSpinBox()
        self.e2e_data_id_spin.setRange(0, 65535)
        e2e_layout.addRow("Data ID:", self.e2e_data_id_spin)
        
        layout.addRow(e2e_group)
        
        # Encryption
        encryption_group = QGroupBox("Encryption")
        encryption_layout = QFormLayout(encryption_group)
        
        self.encryption_enabled_check = QCheckBox("Enable Encryption")
        self.encryption_enabled_check.stateChanged.connect(self._mark_dirty)
        encryption_layout.addRow(self.encryption_enabled_check)
        
        self.encryption_algo_combo = QComboBox()
        self.encryption_algo_combo.addItems(["AES-128-GCM", "AES-256-GCM"])
        encryption_layout.addRow("Algorithm:", self.encryption_algo_combo)
        
        layout.addRow(encryption_group)
        
        # Authentication
        auth_group = QGroupBox("Authentication")
        auth_layout = QFormLayout(auth_group)
        
        self.auth_enabled_check = QCheckBox("Enable Authentication")
        self.auth_enabled_check.stateChanged.connect(self._mark_dirty)
        auth_layout.addRow(self.auth_enabled_check)
        
        self.auth_plugin_combo = QComboBox()
        self.auth_plugin_combo.addItems(["DDS:Auth:PKI-DH", "DDS:Auth:PSK"])
        auth_layout.addRow("Plugin:", self.auth_plugin_combo)
        
        layout.addRow(auth_group)
        
        layout.addStretch()
        return widget
    
    def _create_autosar_tab(self) -> QWidget:
        """Create AUTOSAR settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # General AUTOSAR settings
        autosar_group = QGroupBox("AUTOSAR Integration")
        autosar_layout = QFormLayout(autosar_group)
        
        self.autosar_enabled_check = QCheckBox("Enable AUTOSAR Integration")
        self.autosar_enabled_check.stateChanged.connect(self._autosar_toggled)
        autosar_layout.addRow(self.autosar_enabled_check)
        
        self.ecu_id_edit = QLineEdit()
        self.ecu_id_edit.setPlaceholderText("ECU identifier")
        autosar_layout.addRow("ECU ID:", self.ecu_id_edit)
        
        self.sw_component_edit = QLineEdit()
        self.sw_component_edit.setPlaceholderText("Software component name")
        autosar_layout.addRow("SW Component:", self.sw_component_edit)
        
        layout.addRow(autosar_group)
        
        # SoAd mapping
        soad_group = QGroupBox("SoAd Mapping")
        soad_layout = QFormLayout(soad_group)
        
        self.soad_socket_edit = QLineEdit()
        self.soad_socket_edit.setPlaceholderText("Socket connection name")
        soad_layout.addRow("Socket Connection:", self.soad_socket_edit)
        
        self.soad_pdu_group_spin = QSpinBox()
        self.soad_pdu_group_spin.setRange(0, 65535)
        soad_layout.addRow("PDU Group ID:", self.soad_pdu_group_spin)
        
        layout.addRow(soad_group)
        
        # PduR mapping
        pdur_group = QGroupBox("PduR Mapping")
        pdur_layout = QFormLayout(pdur_group)
        
        self.pdur_src_pdu_edit = QLineEdit()
        self.pdur_src_pdu_edit.setPlaceholderText("Source PDU ID")
        pdur_layout.addRow("Source PDU:", self.pdur_src_pdu_edit)
        
        self.pdur_dst_pdu_edit = QLineEdit()
        self.pdur_dst_pdu_edit.setPlaceholderText("Destination PDU ID")
        pdur_layout.addRow("Destination PDU:", self.pdur_dst_pdu_edit)
        
        layout.addRow(pdur_group)
        
        # BswM mapping
        bswm_group = QGroupBox("BswM Rules")
        bswm_layout = QFormLayout(bswm_group)
        
        self.bswm_mode_req_edit = QLineEdit()
        self.bswm_mode_req_edit.setPlaceholderText("Mode request port")
        bswm_layout.addRow("Mode Request:", self.bswm_mode_req_edit)
        
        layout.addRow(bswm_group)
        
        layout.addStretch()
        return widget
    
    def _create_advanced_tab(self) -> QWidget:
        """Create advanced settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Resource limits
        resources_group = QGroupBox("Resource Limits")
        resources_layout = QFormLayout(resources_group)
        
        self.max_samples_spin = QSpinBox()
        self.max_samples_spin.setRange(1, 100000)
        self.max_samples_spin.setValue(1000)
        resources_layout.addRow("Max Samples:", self.max_samples_spin)
        
        self.max_samples_per_instance_spin = QSpinBox()
        self.max_samples_per_instance_spin.setRange(1, 10000)
        self.max_samples_per_instance_spin.setValue(100)
        resources_layout.addRow("Max Samples/Instance:", self.max_samples_per_instance_spin)
        
        layout.addRow(resources_group)
        
        # Lifespan
        lifespan_group = QGroupBox("Lifespan")
        lifespan_layout = QFormLayout(lifespan_group)
        
        self.lifespan_enabled_check = QCheckBox("Enable Lifespan")
        lifespan_layout.addRow(self.lifespan_enabled_check)
        
        self.lifespan_duration_spin = QSpinBox()
        self.lifespan_duration_spin.setRange(0, 86400)
        self.lifespan_duration_spin.setValue(0)
        self.lifespan_duration_spin.setSuffix(" s")
        lifespan_layout.addRow("Duration:", self.lifespan_duration_spin)
        
        layout.addRow(lifespan_group)
        
        # Transport priority
        priority_group = QGroupBox("Transport Priority")
        priority_layout = QFormLayout(priority_group)
        
        self.priority_spin = QSpinBox()
        self.priority_spin.setRange(0, 255)
        self.priority_spin.setValue(0)
        priority_layout.addRow("Priority:", self.priority_spin)
        
        layout.addRow(priority_group)
        
        # Ownership
        ownership_group = QGroupBox("Ownership")
        ownership_layout = QFormLayout(ownership_group)
        
        self.ownership_combo = QComboBox()
        self.ownership_combo.addItems(["SHARED", "EXCLUSIVE"])
        ownership_layout.addRow("Kind:", self.ownership_combo)
        
        self.ownership_strength_spin = QSpinBox()
        self.ownership_strength_spin.setRange(0, 1000)
        ownership_layout.addRow("Strength:", self.ownership_strength_spin)
        
        layout.addRow(ownership_group)
        
        layout.addStretch()
        return widget
    
    def _on_topic_selected(self, index: int):
        """Handle topic selection."""
        if index < 0 or index >= len(self.topics):
            self.editor_widget.hide()
            self.empty_label.show()
            self.current_topic_index = -1
            return
        
        self.empty_label.hide()
        self.editor_widget.show()
        self.current_topic_index = index
        
        topic = self.topics[index]
        self._load_topic_to_ui(topic)
    
    def _load_topic_to_ui(self, topic: Dict[str, Any]):
        """Load topic data to UI."""
        # Basic
        self.topic_name_edit.setText(topic.get('name', ''))
        self.topic_kind_combo.setCurrentText(topic.get('kind', 'STANDARD'))
        self.data_type_combo.setCurrentText(topic.get('data_type', ''))
        self.topic_desc_edit.setPlainText(topic.get('description', ''))
        self.instance_key_edit.setText(topic.get('key_fields', ''))
        self.max_instances_spin.setValue(topic.get('max_instances', 1))
        
        # QoS
        qos = topic.get('qos', {})
        self.reliability_combo.setCurrentText(qos.get('reliability', 'RELIABLE'))
        self.durability_combo.setCurrentText(qos.get('durability', 'VOLATILE'))
        self.history_kind_combo.setCurrentText(qos.get('history_kind', 'KEEP_LAST'))
        self.history_depth_spin.setValue(qos.get('history_depth', 1))
        self.deadline_enabled_check.setChecked(qos.get('deadline_enabled', False))
        self.deadline_period_spin.setValue(qos.get('deadline_period', 1000))
        self.latency_budget_spin.setValue(qos.get('latency_budget', 0))
        self.liveliness_combo.setCurrentText(qos.get('liveliness', 'AUTOMATIC'))
        self.lease_duration_spin.setValue(qos.get('lease_duration', 10))
        
        # Security
        security = topic.get('security', {})
        self.e2e_enabled_check.setChecked(security.get('e2e_enabled', False))
        self.e2e_profile_combo.setCurrentText(security.get('e2e_profile', 'P01'))
        self.e2e_data_id_spin.setValue(security.get('e2e_data_id', 0))
        self.encryption_enabled_check.setChecked(security.get('encryption_enabled', False))
        self.encryption_algo_combo.setCurrentText(security.get('encryption_algo', 'AES-128-GCM'))
        self.auth_enabled_check.setChecked(security.get('auth_enabled', False))
        
        # AUTOSAR
        autosar = topic.get('autosar', {})
        self.autosar_enabled_check.setChecked(autosar.get('enabled', False))
        self.ecu_id_edit.setText(autosar.get('ecu_id', ''))
        self.soad_socket_edit.setText(autosar.get('soad_socket', ''))
        self.pdur_src_pdu_edit.setText(autosar.get('pdur_src_pdu', ''))
    
    def _save_current_topic(self):
        """Save current topic data."""
        if self.current_topic_index < 0:
            return
        
        topic = {
            'name': self.topic_name_edit.text(),
            'kind': self.topic_kind_combo.currentText(),
            'data_type': self.data_type_combo.currentText(),
            'description': self.topic_desc_edit.toPlainText(),
            'key_fields': self.instance_key_edit.text(),
            'max_instances': self.max_instances_spin.value(),
            'qos': {
                'reliability': self.reliability_combo.currentText(),
                'durability': self.durability_combo.currentText(),
                'history_kind': self.history_kind_combo.currentText(),
                'history_depth': self.history_depth_spin.value(),
                'deadline_enabled': self.deadline_enabled_check.isChecked(),
                'deadline_period': self.deadline_period_spin.value(),
                'latency_budget': self.latency_budget_spin.value(),
                'liveliness': self.liveliness_combo.currentText(),
                'lease_duration': self.lease_duration_spin.value()
            },
            'security': {
                'e2e_enabled': self.e2e_enabled_check.isChecked(),
                'e2e_profile': self.e2e_profile_combo.currentText(),
                'e2e_data_id': self.e2e_data_id_spin.value(),
                'encryption_enabled': self.encryption_enabled_check.isChecked(),
                'encryption_algo': self.encryption_algo_combo.currentText(),
                'auth_enabled': self.auth_enabled_check.isChecked()
            },
            'autosar': {
                'enabled': self.autosar_enabled_check.isChecked(),
                'ecu_id': self.ecu_id_edit.text(),
                'soad_socket': self.soad_socket_edit.text(),
                'pdur_src_pdu': self.pdur_src_pdu_edit.text()
            }
        }
        
        self.topics[self.current_topic_index] = topic
        self._update_topic_list()
        self.config_changed.emit()
    
    def _add_topic(self):
        """Add new topic."""
        topic = {
            'name': f"Topic_{len(self.topics) + 1}",
            'kind': 'STANDARD',
            'data_type': '',
            'description': '',
            'qos': {},
            'security': {},
            'autosar': {}
        }
        self.topics.append(topic)
        self._update_topic_list()
        self.topic_list.setCurrentRow(len(self.topics) - 1)
        self.config_changed.emit()
    
    def _remove_topic(self):
        """Remove selected topic."""
        index = self.topic_list.currentRow()
        if index >= 0:
            self.topics.pop(index)
            self._update_topic_list()
            self.config_changed.emit()
    
    def _duplicate_topic(self):
        """Duplicate selected topic."""
        index = self.topic_list.currentRow()
        if index >= 0:
            topic = self.topics[index].copy()
            topic['name'] = f"{topic['name']}_copy"
            self.topics.insert(index + 1, topic)
            self._update_topic_list()
            self.topic_list.setCurrentRow(index + 1)
            self.config_changed.emit()
    
    def _update_topic_list(self):
        """Update topic list widget."""
        self.topic_list.clear()
        for topic in self.topics:
            name = topic.get('name', 'Unnamed')
            data_type = topic.get('data_type', 'No type')
            item = QListWidgetItem(f"{name}\n({data_type})")
            item.setToolTip(f"Type: {data_type}")
            self.topic_list.addItem(item)
    
    def _filter_topics(self, filter_text: str):
        """Filter topic list."""
        for i in range(self.topic_list.count()):
            item = self.topic_list.item(i)
            if filter_text.lower() in item.text().lower():
                item.setHidden(False)
            else:
                item.setHidden(True)
    
    def _mark_dirty(self):
        """Mark configuration as modified."""
        # Just emit signal - actual save happens on Apply
        pass
    
    def _e2e_toggled(self, state):
        """Handle E2E toggle."""
        enabled = state == Qt.Checked
        self.e2e_profile_combo.setEnabled(enabled)
        self.e2e_data_id_spin.setEnabled(enabled)
        self._mark_dirty()
    
    def _autosar_toggled(self, state):
        """Handle AUTOSAR toggle."""
        enabled = state == Qt.Checked
        self.ecu_id_edit.setEnabled(enabled)
        self.soad_socket_edit.setEnabled(enabled)
        self.pdur_src_pdu_edit.setEnabled(enabled)
        self._mark_dirty()
    
    def get_config(self) -> List[Dict[str, Any]]:
        """Get topics configuration."""
        # Save current topic first
        if self.current_topic_index >= 0:
            self._save_current_topic()
        return self.topics
    
    def set_config(self, topics: List[Dict[str, Any]]):
        """Set topics configuration."""
        self.topics = topics if isinstance(topics, list) else []
        self.current_topic_index = -1
        self._update_topic_list()
