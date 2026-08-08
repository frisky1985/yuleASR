"""
DDS Configuration Tool - Domain Tab

Configuration for DDS Domain settings including:
- Domain ID
- Discovery protocols (Simple/Static/Dynamic)
- Discovery configuration
- Participant settings
"""

from typing import Dict, Any, List

try:
    from PyQt5.QtWidgets import (
        QWidget, QVBoxLayout, QHBoxLayout, QFormLayout,
        QLabel, QLineEdit, QComboBox, QSpinBox, QCheckBox,
        QGroupBox, QPushButton, QListWidget, QTextEdit,
        QTabWidget, QTableWidget, QTableWidgetItem, QHeaderView
    )
    from PyQt5.QtCore import Qt, pyqtSignal
except ImportError:
    raise ImportError("PyQt5 is required for GUI mode")


class DomainTab(QWidget):
    """Domain configuration tab."""
    
    config_changed = pyqtSignal()
    
    # Discovery protocols
    DISCOVERY_PROTOCOLS = [
        "SIMPLE",       # Simple discovery
        "STATIC",       # Static discovery
        "DYNAMIC",      # Dynamic discovery
        "SERVER",       # Server-based discovery
        "RTPS_AUTO"     # Automatic RTPS discovery
    ]
    
    # Transport modes
    TRANSPORT_MODES = [
        "UDPv4",
        "UDPv6",
        "SHM",
        "TCPv4",
        "TCPv6",
        "SHM+UDP",
        "INTRA_PROCESS"
    ]
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup the UI components."""
        layout = QVBoxLayout(self)
        
        # Create subtabs
        subtab = QTabWidget()
        
        # Basic settings tab
        basic_tab = self._create_basic_tab()
        subtab.addTab(basic_tab, "Basic Settings")
        
        # Discovery tab
        discovery_tab = self._create_discovery_tab()
        subtab.addTab(discovery_tab, "Discovery")
        
        # Participants tab
        participants_tab = self._create_participants_tab()
        subtab.addTab(participants_tab, "Participants")
        
        # Advanced tab
        advanced_tab = self._create_advanced_tab()
        subtab.addTab(advanced_tab, "Advanced")
        
        layout.addWidget(subtab)
    
    def _create_basic_tab(self) -> QWidget:
        """Create basic settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Domain ID
        self.domain_id_spin = QSpinBox()
        self.domain_id_spin.setRange(0, 232)
        self.domain_id_spin.setValue(0)
        self.domain_id_spin.setToolTip("DDS Domain ID (0-232)")
        self.domain_id_spin.valueChanged.connect(self.config_changed.emit)
        layout.addRow("Domain ID:", self.domain_id_spin)
        
        # Domain name
        self.domain_name_edit = QLineEdit()
        self.domain_name_edit.setPlaceholderText("Enter domain name")
        self.domain_name_edit.setToolTip("Human-readable domain name")
        self.domain_name_edit.textChanged.connect(self.config_changed.emit)
        layout.addRow("Domain Name:", self.domain_name_edit)
        
        # Description
        self.description_edit = QTextEdit()
        self.description_edit.setPlaceholderText("Enter domain description")
        self.description_edit.setMaximumHeight(60)
        self.description_edit.textChanged.connect(self.config_changed.emit)
        layout.addRow("Description:", self.description_edit)
        
        layout.addRow(QLabel(""))  # Spacer
        
        # Transport settings group
        transport_group = QGroupBox("Default Transport")
        transport_layout = QFormLayout(transport_group)
        
        self.transport_combo = QComboBox()
        self.transport_combo.addItems(self.TRANSPORT_MODES)
        self.transport_combo.setToolTip("Default transport mode")
        self.transport_combo.currentIndexChanged.connect(self.config_changed.emit)
        transport_layout.addRow("Transport:", self.transport_combo)
        
        self.shm_enabled_check = QCheckBox("Enable Shared Memory")
        self.shm_enabled_check.setChecked(True)
        self.shm_enabled_check.stateChanged.connect(self.config_changed.emit)
        transport_layout.addRow(self.shm_enabled_check)
        
        self.intra_process_check = QCheckBox("Enable Intra-Process Communication")
        self.intra_process_check.setChecked(True)
        self.intra_process_check.stateChanged.connect(self.config_changed.emit)
        transport_layout.addRow(self.intra_process_check)
        
        layout.addRow(transport_group)
        
        layout.addStretch()
        return widget
    
    def _create_discovery_tab(self) -> QWidget:
        """Create discovery settings tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Discovery protocol group
        protocol_group = QGroupBox("Discovery Protocol")
        protocol_layout = QFormLayout(protocol_group)
        
        self.discovery_combo = QComboBox()
        self.discovery_combo.addItems(self.DISCOVERY_PROTOCOLS)
        self.discovery_combo.setToolTip("Discovery protocol type")
        self.discovery_combo.currentIndexChanged.connect(self._on_discovery_changed)
        protocol_layout.addRow("Protocol:", self.discovery_combo)
        
        # Simple discovery settings
        self.simple_settings_widget = QWidget()
        simple_layout = QFormLayout(self.simple_settings_widget)
        
        self.lease_duration_spin = QSpinBox()
        self.lease_duration_spin.setRange(1, 3600)
        self.lease_duration_spin.setValue(10)
        self.lease_duration_spin.setSuffix(" s")
        simple_layout.addRow("Lease Duration:", self.lease_duration_spin)
        
        self.announce_period_spin = QSpinBox()
        self.announce_period_spin.setRange(1, 300)
        self.announce_period_spin.setValue(5)
        self.announce_period_spin.setSuffix(" s")
        simple_layout.addRow("Announce Period:", self.announce_period_spin)
        
        self.resend_data_spin = QSpinBox()
        self.resend_data_spin.setRange(1, 100)
        self.resend_data_spin.setValue(2)
        self.resend_data_spin.setSuffix(" s")
        simple_layout.addRow("Resend Data Period:", self.resend_data_spin)
        
        protocol_layout.addRow(self.simple_settings_widget)
        
        # Static discovery settings
        self.static_settings_widget = QWidget()
        static_layout = QVBoxLayout(self.static_settings_widget)
        
        static_info = QLabel("Configure static peer endpoints in the table below")
        static_layout.addWidget(static_info)
        
        self.peers_table = QTableWidget()
        self.peers_table.setColumnCount(4)
        self.peers_table.setHorizontalHeaderLabels(["Participant ID", "Locator", "Kind", "Actions"])
        self.peers_table.horizontalHeader().setStretchLastSection(True)
        static_layout.addWidget(self.peers_table)
        
        peers_btn_layout = QHBoxLayout()
        add_peer_btn = QPushButton("Add Peer")
        add_peer_btn.clicked.connect(self._add_peer)
        peers_btn_layout.addWidget(add_peer_btn)
        
        remove_peer_btn = QPushButton("Remove Selected")
        remove_peer_btn.clicked.connect(self._remove_peer)
        peers_btn_layout.addWidget(remove_peer_btn)
        
        peers_btn_layout.addStretch()
        static_layout.addLayout(peers_btn_layout)
        
        self.static_settings_widget.hide()
        protocol_layout.addRow(self.static_settings_widget)
        
        layout.addWidget(protocol_group)
        
        # Multicast settings
        multicast_group = QGroupBox("Multicast Settings")
        multicast_layout = QFormLayout(multicast_group)
        
        self.multicast_enabled_check = QCheckBox("Enable Multicast Discovery")
        self.multicast_enabled_check.setChecked(True)
        multicast_layout.addRow(self.multicast_enabled_check)
        
        self.multicast_addr_edit = QLineEdit()
        self.multicast_addr_edit.setText("239.255.0.1")
        multicast_layout.addRow("Multicast Address:", self.multicast_addr_edit)
        
        self.multicast_port_spin = QSpinBox()
        self.multicast_port_spin.setRange(1024, 65535)
        self.multicast_port_spin.setValue(7400)
        multicast_layout.addRow("Multicast Port:", self.multicast_port_spin)
        
        layout.addWidget(multicast_group)
        
        # Initial peers
        peers_group = QGroupBox("Initial Peers")
        peers_layout = QVBoxLayout(peers_group)
        
        self.initial_peers_list = QListWidget()
        peers_layout.addWidget(self.initial_peers_list)
        
        peers_btn_layout = QHBoxLayout()
        add_btn = QPushButton("Add")
        add_btn.clicked.connect(self._add_initial_peer)
        peers_btn_layout.addWidget(add_btn)
        
        remove_btn = QPushButton("Remove")
        remove_btn.clicked.connect(self._remove_initial_peer)
        peers_btn_layout.addWidget(remove_btn)
        
        peers_btn_layout.addStretch()
        peers_layout.addLayout(peers_btn_layout)
        
        layout.addWidget(peers_group)
        
        layout.addStretch()
        return widget
    
    def _create_participants_tab(self) -> QWidget:
        """Create participants configuration tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Participants table
        self.participants_table = QTableWidget()
        self.participants_table.setColumnCount(5)
        self.participants_table.setHorizontalHeaderLabels([
            "Participant Name", "Participant ID", "App ID", "Priority", "Actions"
        ])
        self.participants_table.horizontalHeader().setStretchLastSection(True)
        layout.addWidget(self.participants_table)
        
        # Buttons
        btn_layout = QHBoxLayout()
        
        add_btn = QPushButton("Add Participant")
        add_btn.clicked.connect(self._add_participant)
        btn_layout.addWidget(add_btn)
        
        remove_btn = QPushButton("Remove Selected")
        remove_btn.clicked.connect(self._remove_participant)
        btn_layout.addWidget(remove_btn)
        
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Participant settings group
        settings_group = QGroupBox("Default Participant Settings")
        settings_layout = QFormLayout(settings_group)
        
        self.entity_factory_check = QCheckBox("Auto-enable Created Entities")
        self.entity_factory_check.setChecked(True)
        settings_layout.addRow(self.entity_factory_check)
        
        self.user_data_edit = QLineEdit()
        self.user_data_edit.setPlaceholderText("Optional user data (hex)")
        settings_layout.addRow("User Data:", self.user_data_edit)
        
        self.property_policy_edit = QTextEdit()
        self.property_policy_edit.setPlaceholderText("Property policy (key=value per line)")
        self.property_policy_edit.setMaximumHeight(80)
        settings_layout.addRow("Properties:", self.property_policy_edit)
        
        layout.addWidget(settings_group)
        
        return widget
    
    def _create_advanced_tab(self) -> QWidget:
        """Create advanced settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Timing settings
        timing_group = QGroupBox("Timing Configuration")
        timing_layout = QFormLayout(timing_group)
        
        self.spin_freq_spin = QSpinBox()
        self.spin_freq_spin.setRange(10, 1000)
        self.spin_freq_spin.setValue(100)
        self.spin_freq_spin.setSuffix(" Hz")
        timing_layout.addRow("Spin Frequency:", self.spin_freq_spin)
        
        self.max_blocking_time_spin = QSpinBox()
        self.max_blocking_time_spin.setRange(1, 10000)
        self.max_blocking_time_spin.setValue(1000)
        self.max_blocking_time_spin.setSuffix(" ms")
        timing_layout.addRow("Max Blocking Time:", self.max_blocking_time_spin)
        
        layout.addRow(timing_group)
        
        # Resource limits
        resources_group = QGroupBox("Resource Limits")
        resources_layout = QFormLayout(resources_group)
        
        self.max_participants_spin = QSpinBox()
        self.max_participants_spin.setRange(1, 100)
        self.max_participants_spin.setValue(10)
        resources_layout.addRow("Max Participants:", self.max_participants_spin)
        
        self.max_domains_spin = QSpinBox()
        self.max_domains_spin.setRange(1, 10)
        self.max_domains_spin.setValue(1)
        resources_layout.addRow("Max Domains per Process:", self.max_domains_spin)
        
        layout.addRow(resources_group)
        
        # Memory settings
        memory_group = QGroupBox("Memory Management")
        memory_layout = QFormLayout(memory_group)
        
        self.shm_size_spin = QSpinBox()
        self.shm_size_spin.setRange(1024, 1048576)
        self.shm_size_spin.setValue(65536)
        self.shm_size_spin.setSuffix(" KB")
        memory_layout.addRow("SHM Segment Size:", self.shm_size_spin)
        
        self.shm_segments_spin = QSpinBox()
        self.shm_segments_spin.setRange(1, 100)
        self.shm_segments_spin.setValue(10)
        memory_layout.addRow("SHM Segments:", self.shm_segments_spin)
        
        layout.addRow(memory_group)
        
        # Logging
        logging_group = QGroupBox("Logging")
        logging_layout = QFormLayout(logging_group)
        
        self.log_level_combo = QComboBox()
        self.log_level_combo.addItems(["ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"])
        logging_layout.addRow("Log Level:", self.log_level_combo)
        
        self.log_file_edit = QLineEdit()
        self.log_file_edit.setPlaceholderText("Log file path (optional)")
        logging_layout.addRow("Log File:", self.log_file_edit)
        
        layout.addRow(logging_group)
        
        layout.addStretch()
        return widget
    
    def _on_discovery_changed(self):
        """Handle discovery protocol change."""
        protocol = self.discovery_combo.currentText()
        
        # Show/hide appropriate settings
        if protocol == "SIMPLE":
            self.simple_settings_widget.show()
            self.static_settings_widget.hide()
        elif protocol == "STATIC":
            self.simple_settings_widget.hide()
            self.static_settings_widget.show()
        else:
            self.simple_settings_widget.hide()
            self.static_settings_widget.hide()
        
        self.config_changed.emit()
    
    def _add_peer(self):
        """Add static peer."""
        row = self.peers_table.rowCount()
        self.peers_table.insertRow(row)
        self.config_changed.emit()
    
    def _remove_peer(self):
        """Remove selected peer."""
        row = self.peers_table.currentRow()
        if row >= 0:
            self.peers_table.removeRow(row)
            self.config_changed.emit()
    
    def _add_initial_peer(self):
        """Add initial peer."""
        # TODO: Implement with dialog
        pass
    
    def _remove_initial_peer(self):
        """Remove selected initial peer."""
        row = self.initial_peers_list.currentRow()
        if row >= 0:
            self.initial_peers_list.takeItem(row)
            self.config_changed.emit()
    
    def _add_participant(self):
        """Add participant."""
        row = self.participants_table.rowCount()
        self.participants_table.insertRow(row)
        self.config_changed.emit()
    
    def _remove_participant(self):
        """Remove selected participant."""
        row = self.participants_table.currentRow()
        if row >= 0:
            self.participants_table.removeRow(row)
            self.config_changed.emit()
    
    def get_config(self) -> Dict[str, Any]:
        """Get domain configuration."""
        peers = []
        for i in range(self.peers_table.rowCount()):
            peer = {
                'participant_id': self.peers_table.item(i, 0).text() if self.peers_table.item(i, 0) else "",
                'locator': self.peers_table.item(i, 1).text() if self.peers_table.item(i, 1) else "",
                'kind': self.peers_table.item(i, 2).text() if self.peers_table.item(i, 2) else ""
            }
            peers.append(peer)
        
        participants = []
        for i in range(self.participants_table.rowCount()):
            p = {
                'name': self.participants_table.item(i, 0).text() if self.participants_table.item(i, 0) else "",
                'id': self.participants_table.item(i, 1).text() if self.participants_table.item(i, 1) else "",
                'app_id': self.participants_table.item(i, 2).text() if self.participants_table.item(i, 2) else "",
                'priority': self.participants_table.item(i, 3).text() if self.participants_table.item(i, 3) else ""
            }
            participants.append(p)
        
        initial_peers = []
        for i in range(self.initial_peers_list.count()):
            initial_peers.append(self.initial_peers_list.item(i).text())
        
        return {
            'domain_id': self.domain_id_spin.value(),
            'domain_name': self.domain_name_edit.text(),
            'description': self.description_edit.toPlainText(),
            'transport': {
                'default': self.transport_combo.currentText(),
                'shm_enabled': self.shm_enabled_check.isChecked(),
                'intra_process': self.intra_process_check.isChecked()
            },
            'discovery': {
                'protocol': self.discovery_combo.currentText(),
                'lease_duration': self.lease_duration_spin.value(),
                'announce_period': self.announce_period_spin.value(),
                'resend_data_period': self.resend_data_spin.value(),
                'static_peers': peers
            },
            'multicast': {
                'enabled': self.multicast_enabled_check.isChecked(),
                'address': self.multicast_addr_edit.text(),
                'port': self.multicast_port_spin.value()
            },
            'initial_peers': initial_peers,
            'participants': participants,
            'timing': {
                'spin_frequency': self.spin_freq_spin.value(),
                'max_blocking_time': self.max_blocking_time_spin.value()
            },
            'resources': {
                'max_participants': self.max_participants_spin.value(),
                'max_domains': self.max_domains_spin.value(),
                'shm_size_kb': self.shm_size_spin.value(),
                'shm_segments': self.shm_segments_spin.value()
            },
            'logging': {
                'level': self.log_level_combo.currentText(),
                'file': self.log_file_edit.text()
            }
        }
    
    def set_config(self, config: Dict[str, Any]):
        """Set domain configuration."""
        self.domain_id_spin.setValue(config.get('domain_id', 0))
        self.domain_name_edit.setText(config.get('domain_name', ''))
        self.description_edit.setPlainText(config.get('description', ''))
        
        transport = config.get('transport', {})
        self.transport_combo.setCurrentText(transport.get('default', 'UDPv4'))
        self.shm_enabled_check.setChecked(transport.get('shm_enabled', True))
        self.intra_process_check.setChecked(transport.get('intra_process', True))
        
        discovery = config.get('discovery', {})
        self.discovery_combo.setCurrentText(discovery.get('protocol', 'SIMPLE'))
        self.lease_duration_spin.setValue(discovery.get('lease_duration', 10))
        self.announce_period_spin.setValue(discovery.get('announce_period', 5))
        self.resend_data_spin.setValue(discovery.get('resend_data_period', 2))
        
        multicast = config.get('multicast', {})
        self.multicast_enabled_check.setChecked(multicast.get('enabled', True))
        self.multicast_addr_edit.setText(multicast.get('address', '239.255.0.1'))
        self.multicast_port_spin.setValue(multicast.get('port', 7400))
