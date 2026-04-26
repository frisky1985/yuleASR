"""
DDS Configuration Tool - Transport Tab

Configuration for DDS Transport layer including:
- UDPv4/UDPv6 transport
- SHM (Shared Memory) transport
- TCP transport
- Transport QoS
- Network interfaces
"""

from typing import Dict, Any, List

try:
    from PyQt5.QtWidgets import (
        QWidget, QVBoxLayout, QHBoxLayout, QFormLayout, QGridLayout,
        QLabel, QLineEdit, QComboBox, QSpinBox, QCheckBox, QGroupBox,
        QPushButton, QListWidget, QListWidgetItem, QTextEdit,
        QTableWidget, QTableWidgetItem, QHeaderView, QSplitter,
        QTabWidget, QDoubleSpinBox
    )
    from PyQt5.QtCore import Qt, pyqtSignal
except ImportError:
    raise ImportError("PyQt5 is required for GUI mode")


class TransportTab(QWidget):
    """Transport configuration tab."""
    
    config_changed = pyqtSignal()
    
    # Transport kinds
    TRANSPORT_KINDS = [
        "UDPv4",
        "UDPv6", 
        "SHM",
        "TCPv4",
        "TCPv6",
        "INTRA",
        "SHM+UDP"
    ]
    
    # SHM modes
    SHM_MODES = [
        "POSIX",
        "SYSV",
        "Windows",
        "Boost"
    ]
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.transports: List[Dict[str, Any]] = []
        self.current_transport_index = -1
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup the UI components."""
        layout = QHBoxLayout(self)
        
        # Splitter
        splitter = QSplitter(Qt.Horizontal)
        
        # Left panel - Transport list
        left_panel = self._create_left_panel()
        splitter.addWidget(left_panel)
        
        # Right panel - Transport editor
        right_panel = self._create_right_panel()
        splitter.addWidget(right_panel)
        
        splitter.setSizes([300, 700])
        layout.addWidget(splitter)
    
    def _create_left_panel(self) -> QWidget:
        """Create left panel with transport list."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Title
        layout.addWidget(QLabel("<b>Transports</b>"))
        
        # Transport list
        self.transport_list = QListWidget()
        self.transport_list.currentRowChanged.connect(self._on_transport_selected)
        layout.addWidget(self.transport_list)
        
        # Buttons
        btn_layout = QHBoxLayout()
        
        add_btn = QPushButton("Add")
        add_btn.clicked.connect(self._add_transport)
        btn_layout.addWidget(add_btn)
        
        remove_btn = QPushButton("Remove")
        remove_btn.clicked.connect(self._remove_transport)
        btn_layout.addWidget(remove_btn)
        
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Quick add buttons
        quick_group = QGroupBox("Quick Add")
        quick_layout = QVBoxLayout(quick_group)
        
        quick_udp_btn = QPushButton("Add UDPv4 Default")
        quick_udp_btn.clicked.connect(self._add_udp_default)
        quick_layout.addWidget(quick_udp_btn)
        
        quick_shm_btn = QPushButton("Add SHM Default")
        quick_shm_btn.clicked.connect(self._add_shm_default)
        quick_layout.addWidget(quick_shm_btn)
        
        quick_tcp_btn = QPushButton("Add TCP Default")
        quick_tcp_btn.clicked.connect(self._add_tcp_default)
        quick_layout.addWidget(quick_tcp_btn)
        
        layout.addWidget(quick_group)
        
        return widget
    
    def _create_right_panel(self) -> QWidget:
        """Create right panel with transport editor."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Transport editor widget
        self.editor_widget = QWidget()
        editor_layout = QVBoxLayout(self.editor_widget)
        
        # Transport type selector
        type_layout = QFormLayout()
        self.transport_type_combo = QComboBox()
        self.transport_type_combo.addItems(self.TRANSPORT_KINDS)
        self.transport_type_combo.currentIndexChanged.connect(self._on_transport_type_changed)
        type_layout.addRow("Transport Type:", self.transport_type_combo)
        editor_layout.addLayout(type_layout)
        
        # Stacked widget for different transport configs
        self.transport_tabs = QTabWidget()
        
        # Common settings
        common_tab = self._create_common_tab()
        self.transport_tabs.addTab(common_tab, "Common")
        
        # UDP settings
        udp_tab = self._create_udp_tab()
        self.transport_tabs.addTab(udp_tab, "UDP")
        
        # TCP settings
        tcp_tab = self._create_tcp_tab()
        self.transport_tabs.addTab(tcp_tab, "TCP")
        
        # SHM settings
        shm_tab = self._create_shm_tab()
        self.transport_tabs.addTab(shm_tab, "SHM")
        
        # Advanced settings
        advanced_tab = self._create_advanced_tab()
        self.transport_tabs.addTab(advanced_tab, "Advanced")
        
        editor_layout.addWidget(self.transport_tabs)
        
        # Save button
        save_layout = QHBoxLayout()
        save_layout.addStretch()
        save_btn = QPushButton("Apply Changes")
        save_btn.clicked.connect(self._save_current_transport)
        save_layout.addWidget(save_btn)
        editor_layout.addLayout(save_layout)
        
        layout.addWidget(self.editor_widget)
        
        # Empty state
        self.empty_label = QLabel("Select or create a transport to edit")
        self.empty_label.setAlignment(Qt.AlignCenter)
        self.empty_label.setStyleSheet("color: gray; font-size: 14px;")
        layout.addWidget(self.empty_label)
        
        self.editor_widget.hide()
        
        return widget
    
    def _create_common_tab(self) -> QWidget:
        """Create common settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Transport name
        self.transport_name_edit = QLineEdit()
        self.transport_name_edit.setPlaceholderText("Unique transport name")
        self.transport_name_edit.textChanged.connect(self._mark_dirty)
        layout.addRow("Transport Name:*", self.transport_name_edit)
        
        # Enabled
        self.transport_enabled_check = QCheckBox("Enabled")
        self.transport_enabled_check.setChecked(True)
        self.transport_enabled_check.stateChanged.connect(self._mark_dirty)
        layout.addRow(self.transport_enabled_check)
        
        # Drop overflow
        self.drop_overflow_check = QCheckBox("Drop on Overflow")
        self.drop_overflow_check.setChecked(True)
        layout.addRow(self.drop_overflow_check)
        
        # Priority
        self.transport_priority_spin = QSpinBox()
        self.transport_priority_spin.setRange(0, 255)
        self.transport_priority_spin.setValue(0)
        layout.addRow("Priority:", self.transport_priority_spin)
        
        # Interface whitelist
        whitelist_group = QGroupBox("Interface Whitelist")
        whitelist_layout = QVBoxLayout(whitelist_group)
        
        self.whitelist_edit = QTextEdit()
        self.whitelist_edit.setPlaceholderText("Interface names or IPs, one per line")
        self.whitelist_edit.setMaximumHeight(80)
        whitelist_layout.addWidget(self.whitelist_edit)
        
        layout.addRow(whitelist_group)
        
        # Interface blacklist
        blacklist_group = QGroupBox("Interface Blacklist")
        blacklist_layout = QVBoxLayout(blacklist_group)
        
        self.blacklist_edit = QTextEdit()
        self.blacklist_edit.setPlaceholderText("Interface names or IPs, one per line")
        self.blacklist_edit.setMaximumHeight(80)
        blacklist_layout.addWidget(self.blacklist_edit)
        
        layout.addRow(blacklist_group)
        
        layout.addStretch()
        return widget
    
    def _create_udp_tab(self) -> QWidget:
        """Create UDP settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # General UDP settings
        udp_group = QGroupBox("UDP Settings")
        udp_layout = QFormLayout(udp_group)
        
        self.udp_port_spin = QSpinBox()
        self.udp_port_spin.setRange(1024, 65535)
        self.udp_port_spin.setValue(7400)
        udp_layout.addRow("Default Port:", self.udp_port_spin)
        
        self.udp_ttl_spin = QSpinBox()
        self.udp_ttl_spin.setRange(1, 255)
        self.udp_ttl_spin.setValue(1)
        udp_layout.addRow("TTL:", self.udp_ttl_spin)
        
        self.udp_recv_buffer_spin = QSpinBox()
        self.udp_recv_buffer_spin.setRange(1024, 104857600)
        self.udp_recv_buffer_spin.setValue(2097152)
        self.udp_recv_buffer_spin.setSuffix(" bytes")
        udp_layout.addRow("Receive Buffer:", self.udp_recv_buffer_spin)
        
        self.udp_send_buffer_spin = QSpinBox()
        self.udp_send_buffer_spin.setRange(1024, 104857600)
        self.udp_send_buffer_spin.setValue(65536)
        self.udp_send_buffer_spin.setSuffix(" bytes")
        udp_layout.addRow("Send Buffer:", self.udp_send_buffer_spin)
        
        layout.addRow(udp_group)
        
        # Multicast settings
        multicast_group = QGroupBox("Multicast Settings")
        multicast_layout = QFormLayout(multicast_group)
        
        self.multicast_ttl_spin = QSpinBox()
        self.multicast_ttl_spin.setRange(0, 255)
        self.multicast_ttl_spin.setValue(1)
        multicast_layout.addRow("Multicast TTL:", self.multicast_ttl_spin)
        
        self.multicast_loopback_check = QCheckBox("Multicast Loopback")
        self.multicast_loopback_check.setChecked(True)
        multicast_layout.addRow(self.multicast_loopback_check)
        
        layout.addRow(multicast_group)
        
        # Binding
        binding_group = QGroupBox("Binding")
        binding_layout = QFormLayout(binding_group)
        
        self.bind_address_edit = QLineEdit()
        self.bind_address_edit.setPlaceholderText("0.0.0.0")
        binding_layout.addRow("Bind Address:", self.bind_address_edit)
        
        self.port_offset_spin = QSpinBox()
        self.port_offset_spin.setRange(0, 1000)
        self.port_offset_spin.setValue(0)
        binding_layout.addRow("Port Offset:", self.port_offset_spin)
        
        layout.addRow(binding_group)
        
        layout.addStretch()
        return widget
    
    def _create_tcp_tab(self) -> QWidget:
        """Create TCP settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Server settings
        server_group = QGroupBox("TCP Server Settings")
        server_layout = QFormLayout(server_group)
        
        self.tcp_server_check = QCheckBox("Act as Server")
        server_layout.addRow(self.tcp_server_check)
        
        self.tcp_listening_port_spin = QSpinBox()
        self.tcp_listening_port_spin.setRange(1024, 65535)
        self.tcp_listening_port_spin.setValue(7400)
        server_layout.addRow("Listening Port:", self.tcp_listening_port_spin)
        
        self.tcp_max_connections_spin = QSpinBox()
        self.tcp_max_connections_spin.setRange(1, 1000)
        self.tcp_max_connections_spin.setValue(100)
        server_layout.addRow("Max Connections:", self.tcp_max_connections_spin)
        
        self.tcp_keepalive_spin = QSpinBox()
        self.tcp_keepalive_spin.setRange(0, 3600)
        self.tcp_keepalive_spin.setValue(60)
        self.tcp_keepalive_spin.setSuffix(" s")
        server_layout.addRow("Keepalive Interval:", self.tcp_keepalive_spin)
        
        layout.addRow(server_group)
        
        # Client settings
        client_group = QGroupBox("TCP Client Settings")
        client_layout = QFormLayout(client_group)
        
        self.tcp_reconnect_spin = QSpinBox()
        self.tcp_reconnect_spin.setRange(0, 3600)
        self.tcp_reconnect_spin.setValue(5)
        self.tcp_reconnect_spin.setSuffix(" s")
        client_layout.addRow("Reconnect Interval:", self.tcp_reconnect_spin)
        
        self.tcp_conn_timeout_spin = QSpinBox()
        self.tcp_conn_timeout_spin.setRange(1, 300)
        self.tcp_conn_timeout_spin.setValue(5)
        self.tcp_conn_timeout_spin.setSuffix(" s")
        client_layout.addRow("Connection Timeout:", self.tcp_conn_timeout_spin)
        
        layout.addRow(client_group)
        
        # Peers
        peers_group = QGroupBox("Static Peers")
        peers_layout = QVBoxLayout(peers_group)
        
        self.tcp_peers_edit = QTextEdit()
        self.tcp_peers_edit.setPlaceholderText("host:port, one per line")
        self.tcp_peers_edit.setMaximumHeight(100)
        peers_layout.addWidget(self.tcp_peers_edit)
        
        layout.addRow(peers_group)
        
        # Security
        security_group = QGroupBox("TCP Security")
        security_layout = QFormLayout(security_group)
        
        self.tcp_tls_check = QCheckBox("Enable TLS")
        security_layout.addRow(self.tcp_tls_check)
        
        self.tcp_cert_edit = QLineEdit()
        self.tcp_cert_edit.setPlaceholderText("Path to certificate file")
        security_layout.addRow("Certificate:", self.tcp_cert_edit)
        
        self.tcp_key_edit = QLineEdit()
        self.tcp_key_edit.setPlaceholderText("Path to private key file")
        security_layout.addRow("Private Key:", self.tcp_key_edit)
        
        layout.addRow(security_group)
        
        layout.addStretch()
        return widget
    
    def _create_shm_tab(self) -> QWidget:
        """Create SHM settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # SHM settings
        shm_group = QGroupBox("Shared Memory Settings")
        shm_layout = QFormLayout(shm_group)
        
        self.shm_mode_combo = QComboBox()
        self.shm_mode_combo.addItems(self.SHM_MODES)
        shm_layout.addRow("SHM Mode:", self.shm_mode_combo)
        
        self.shm_segment_size_spin = QSpinBox()
        self.shm_segment_size_spin.setRange(1024, 1048576)
        self.shm_segment_size_spin.setValue(65536)
        self.shm_segment_size_spin.setSuffix(" KB")
        shm_layout.addRow("Segment Size:", self.shm_segment_size_spin)
        
        self.shm_max_segments_spin = QSpinBox()
        self.shm_max_segments_spin.setRange(1, 1000)
        self.shm_max_segments_spin.setValue(10)
        shm_layout.addRow("Max Segments:", self.shm_max_segments_spin)
        
        self.shm_port_spin = QSpinBox()
        self.shm_port_spin.setRange(1024, 65535)
        self.shm_port_spin.setValue(0)
        self.shm_port_spin.setSpecialValueText("Auto")
        shm_layout.addRow("SHM Port:", self.shm_port_spin)
        
        self.shm_timeout_spin = QSpinBox()
        self.shm_timeout_spin.setRange(0, 60000)
        self.shm_timeout_spin.setValue(5000)
        self.shm_timeout_spin.setSuffix(" ms")
        shm_layout.addRow("Timeout:", self.shm_timeout_spin)
        
        layout.addRow(shm_group)
        
        # SHM options
        options_group = QGroupBox("SHM Options")
        options_layout = QFormLayout(options_group)
        
        self.shm_zero_copy_check = QCheckBox("Enable Zero Copy")
        self.shm_zero_copy_check.setChecked(True)
        options_layout.addRow(self.shm_zero_copy_check)
        
        self.shm_lock_free_check = QCheckBox("Lock-free Mode")
        options_layout.addRow(self.shm_lock_free_check)
        
        self.shm_watchdog_check = QCheckBox("Enable Watchdog")
        self.shm_watchdog_check.setChecked(True)
        options_layout.addRow(self.shm_watchdog_check)
        
        self.shm_clean_check = QCheckBox("Clean on Startup")
        self.shm_clean_check.setChecked(True)
        options_layout.addRow(self.shm_clean_check)
        
        layout.addRow(options_group)
        
        # Debug
        debug_group = QGroupBox("Debug")
        debug_layout = QFormLayout(debug_group)
        
        self.shm_debug_check = QCheckBox("Debug Mode")
        debug_layout.addRow(self.shm_debug_check)
        
        layout.addRow(debug_group)
        
        layout.addStretch()
        return widget
    
    def _create_advanced_tab(self) -> QWidget:
        """Create advanced settings tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Message size
        message_group = QGroupBox("Message Size")
        message_layout = QFormLayout(message_group)
        
        self.max_msg_size_spin = QSpinBox()
        self.max_msg_size_spin.setRange(64, 65536)
        self.max_msg_size_spin.setValue(65536)
        self.max_msg_size_spin.setSuffix(" bytes")
        message_layout.addRow("Max Message Size:", self.max_msg_size_spin)
        
        self.max_init_size_spin = QSpinBox()
        self.max_init_size_spin.setRange(64, 65536)
        self.max_init_size_spin.setValue(65536)
        self.max_init_size_spin.setSuffix(" bytes")
        message_layout.addRow("Max Initial Size:", self.max_init_size_spin)
        
        self.max_gather_spin = QSpinBox()
        self.max_gather_spin.setRange(1, 1000)
        self.max_gather_spin.setValue(16)
        message_layout.addRow("Max Gather:", self.max_gather_spin)
        
        layout.addRow(message_group)
        
        # Fragments
        fragment_group = QGroupBox("Fragmentation")
        fragment_layout = QFormLayout(fragment_group)
        
        self.fragment_size_spin = QSpinBox()
        self.fragment_size_spin.setRange(64, 65536)
        self.fragment_size_spin.setValue(8192)
        self.fragment_size_spin.setSuffix(" bytes")
        fragment_layout.addRow("Fragment Size:", self.fragment_size_spin)
        
        self.fragment_assembly_spin = QSpinBox()
        self.fragment_assembly_spin.setRange(1, 10000)
        self.fragment_assembly_spin.setValue(30)
        self.fragment_assembly_spin.setSuffix(" s")
        fragment_layout.addRow("Assembly Timeout:", self.fragment_assembly_spin)
        
        layout.addRow(fragment_group)
        
        # Flow control
        flow_group = QGroupBox("Flow Control")
        flow_layout = QFormLayout(flow_group)
        
        self.flow_control_check = QCheckBox("Enable Flow Control")
        flow_layout.addRow(self.flow_control_check)
        
        self.flow_limit_spin = QSpinBox()
        self.flow_limit_spin.setRange(0, 1000000)
        self.flow_limit_spin.setValue(0)
        self.flow_limit_spin.setSpecialValueText("Unlimited")
        flow_layout.addRow("Rate Limit (bytes/s):", self.flow_limit_spin)
        
        layout.addRow(flow_group)
        
        # Threading
        thread_group = QGroupBox("Threading")
        thread_layout = QFormLayout(thread_group)
        
        self.recv_thread_count_spin = QSpinBox()
        self.recv_thread_count_spin.setRange(1, 32)
        self.recv_thread_count_spin.setValue(1)
        thread_layout.addRow("Receive Threads:", self.recv_thread_count_spin)
        
        self.send_thread_count_spin = QSpinBox()
        self.send_thread_count_spin.setRange(1, 32)
        self.send_thread_count_spin.setValue(1)
        thread_layout.addRow("Send Threads:", self.send_thread_count_spin)
        
        layout.addRow(thread_group)
        
        layout.addStretch()
        return widget
    
    def _on_transport_selected(self, index: int):
        """Handle transport selection."""
        if index < 0 or index >= len(self.transports):
            self.editor_widget.hide()
            self.empty_label.show()
            self.current_transport_index = -1
            return
        
        self.empty_label.hide()
        self.editor_widget.show()
        self.current_transport_index = index
        
        transport = self.transports[index]
        self._load_transport_to_ui(transport)
    
    def _load_transport_to_ui(self, transport: Dict[str, Any]):
        """Load transport data to UI."""
        # Common
        self.transport_name_edit.setText(transport.get('name', ''))
        self.transport_type_combo.setCurrentText(transport.get('type', 'UDPv4'))
        self.transport_enabled_check.setChecked(transport.get('enabled', True))
        self.drop_overflow_check.setChecked(transport.get('drop_overflow', True))
        self.transport_priority_spin.setValue(transport.get('priority', 0))
        self.whitelist_edit.setPlainText('\n'.join(transport.get('whitelist', [])))
        self.blacklist_edit.setPlainText('\n'.join(transport.get('blacklist', [])))
        
        # UDP
        udp = transport.get('udp', {})
        self.udp_port_spin.setValue(udp.get('port', 7400))
        self.udp_ttl_spin.setValue(udp.get('ttl', 1))
        self.udp_recv_buffer_spin.setValue(udp.get('recv_buffer', 2097152))
        self.udp_send_buffer_spin.setValue(udp.get('send_buffer', 65536))
        self.multicast_ttl_spin.setValue(udp.get('multicast_ttl', 1))
        self.multicast_loopback_check.setChecked(udp.get('multicast_loopback', True))
        self.bind_address_edit.setText(udp.get('bind_address', ''))
        self.port_offset_spin.setValue(udp.get('port_offset', 0))
        
        # TCP
        tcp = transport.get('tcp', {})
        self.tcp_server_check.setChecked(tcp.get('server', False))
        self.tcp_listening_port_spin.setValue(tcp.get('listening_port', 7400))
        self.tcp_max_connections_spin.setValue(tcp.get('max_connections', 100))
        self.tcp_keepalive_spin.setValue(tcp.get('keepalive', 60))
        self.tcp_reconnect_spin.setValue(tcp.get('reconnect', 5))
        self.tcp_conn_timeout_spin.setValue(tcp.get('conn_timeout', 5))
        self.tcp_peers_edit.setPlainText('\n'.join(tcp.get('peers', [])))
        self.tcp_tls_check.setChecked(tcp.get('tls', False))
        self.tcp_cert_edit.setText(tcp.get('cert', ''))
        self.tcp_key_edit.setText(tcp.get('key', ''))
        
        # SHM
        shm = transport.get('shm', {})
        self.shm_mode_combo.setCurrentText(shm.get('mode', 'POSIX'))
        self.shm_segment_size_spin.setValue(shm.get('segment_size', 65536))
        self.shm_max_segments_spin.setValue(shm.get('max_segments', 10))
        self.shm_port_spin.setValue(shm.get('port', 0))
        self.shm_timeout_spin.setValue(shm.get('timeout', 5000))
        self.shm_zero_copy_check.setChecked(shm.get('zero_copy', True))
        self.shm_lock_free_check.setChecked(shm.get('lock_free', False))
        self.shm_watchdog_check.setChecked(shm.get('watchdog', True))
        self.shm_clean_check.setChecked(shm.get('clean', True))
        self.shm_debug_check.setChecked(shm.get('debug', False))
        
        # Advanced
        advanced = transport.get('advanced', {})
        self.max_msg_size_spin.setValue(advanced.get('max_msg_size', 65536))
        self.max_init_size_spin.setValue(advanced.get('max_init_size', 65536))
        self.max_gather_spin.setValue(advanced.get('max_gather', 16))
        self.fragment_size_spin.setValue(advanced.get('fragment_size', 8192))
        self.fragment_assembly_spin.setValue(advanced.get('fragment_assembly', 30))
        self.flow_control_check.setChecked(advanced.get('flow_control', False))
        self.flow_limit_spin.setValue(advanced.get('flow_limit', 0))
        self.recv_thread_count_spin.setValue(advanced.get('recv_threads', 1))
        self.send_thread_count_spin.setValue(advanced.get('send_threads', 1))
        
        # Update tabs based on transport type
        self._on_transport_type_changed()
    
    def _on_transport_type_changed(self):
        """Handle transport type change."""
        transport_type = self.transport_type_combo.currentText()
        
        # Enable/disable relevant tabs
        self.transport_tabs.setTabEnabled(1, 'UDP' in transport_type)  # UDP tab
        self.transport_tabs.setTabEnabled(2, 'TCP' in transport_type)  # TCP tab
        self.transport_tabs.setTabEnabled(3, 'SHM' in transport_type)  # SHM tab
        
        self._mark_dirty()
    
    def _save_current_transport(self):
        """Save current transport data."""
        if self.current_transport_index < 0:
            return
        
        transport = {
            'name': self.transport_name_edit.text(),
            'type': self.transport_type_combo.currentText(),
            'enabled': self.transport_enabled_check.isChecked(),
            'drop_overflow': self.drop_overflow_check.isChecked(),
            'priority': self.transport_priority_spin.value(),
            'whitelist': [w.strip() for w in self.whitelist_edit.toPlainText().split('\n') if w.strip()],
            'blacklist': [b.strip() for b in self.blacklist_edit.toPlainText().split('\n') if b.strip()],
            'udp': {
                'port': self.udp_port_spin.value(),
                'ttl': self.udp_ttl_spin.value(),
                'recv_buffer': self.udp_recv_buffer_spin.value(),
                'send_buffer': self.udp_send_buffer_spin.value(),
                'multicast_ttl': self.multicast_ttl_spin.value(),
                'multicast_loopback': self.multicast_loopback_check.isChecked(),
                'bind_address': self.bind_address_edit.text(),
                'port_offset': self.port_offset_spin.value()
            },
            'tcp': {
                'server': self.tcp_server_check.isChecked(),
                'listening_port': self.tcp_listening_port_spin.value(),
                'max_connections': self.tcp_max_connections_spin.value(),
                'keepalive': self.tcp_keepalive_spin.value(),
                'reconnect': self.tcp_reconnect_spin.value(),
                'conn_timeout': self.tcp_conn_timeout_spin.value(),
                'peers': [p.strip() for p in self.tcp_peers_edit.toPlainText().split('\n') if p.strip()],
                'tls': self.tcp_tls_check.isChecked(),
                'cert': self.tcp_cert_edit.text(),
                'key': self.tcp_key_edit.text()
            },
            'shm': {
                'mode': self.shm_mode_combo.currentText(),
                'segment_size': self.shm_segment_size_spin.value(),
                'max_segments': self.shm_max_segments_spin.value(),
                'port': self.shm_port_spin.value(),
                'timeout': self.shm_timeout_spin.value(),
                'zero_copy': self.shm_zero_copy_check.isChecked(),
                'lock_free': self.shm_lock_free_check.isChecked(),
                'watchdog': self.shm_watchdog_check.isChecked(),
                'clean': self.shm_clean_check.isChecked(),
                'debug': self.shm_debug_check.isChecked()
            },
            'advanced': {
                'max_msg_size': self.max_msg_size_spin.value(),
                'max_init_size': self.max_init_size_spin.value(),
                'max_gather': self.max_gather_spin.value(),
                'fragment_size': self.fragment_size_spin.value(),
                'fragment_assembly': self.fragment_assembly_spin.value(),
                'flow_control': self.flow_control_check.isChecked(),
                'flow_limit': self.flow_limit_spin.value(),
                'recv_threads': self.recv_thread_count_spin.value(),
                'send_threads': self.send_thread_count_spin.value()
            }
        }
        
        self.transports[self.current_transport_index] = transport
        self._update_transport_list()
        self.config_changed.emit()
    
    def _add_transport(self):
        """Add new transport."""
        transport = {
            'name': f"Transport_{len(self.transports) + 1}",
            'type': 'UDPv4',
            'enabled': True,
            'udp': {'port': 7400},
            'shm': {},
            'tcp': {}
        }
        self.transports.append(transport)
        self._update_transport_list()
        self.transport_list.setCurrentRow(len(self.transports) - 1)
        self.config_changed.emit()
    
    def _remove_transport(self):
        """Remove selected transport."""
        index = self.transport_list.currentRow()
        if index >= 0:
            self.transports.pop(index)
            self._update_transport_list()
            self.config_changed.emit()
    
    def _add_udp_default(self):
        """Add default UDP transport."""
        transport = {
            'name': 'udp_default',
            'type': 'UDPv4',
            'enabled': True,
            'udp': {
                'port': 7400,
                'ttl': 1,
                'recv_buffer': 2097152,
                'send_buffer': 65536
            }
        }
        self.transports.append(transport)
        self._update_transport_list()
        self.transport_list.setCurrentRow(len(self.transports) - 1)
        self.config_changed.emit()
    
    def _add_shm_default(self):
        """Add default SHM transport."""
        transport = {
            'name': 'shm_default',
            'type': 'SHM',
            'enabled': True,
            'shm': {
                'mode': 'POSIX',
                'segment_size': 65536,
                'max_segments': 10,
                'zero_copy': True
            }
        }
        self.transports.append(transport)
        self._update_transport_list()
        self.transport_list.setCurrentRow(len(self.transports) - 1)
        self.config_changed.emit()
    
    def _add_tcp_default(self):
        """Add default TCP transport."""
        transport = {
            'name': 'tcp_default',
            'type': 'TCPv4',
            'enabled': True,
            'tcp': {
                'server': True,
                'listening_port': 7400,
                'max_connections': 100,
                'keepalive': 60
            }
        }
        self.transports.append(transport)
        self._update_transport_list()
        self.transport_list.setCurrentRow(len(self.transports) - 1)
        self.config_changed.emit()
    
    def _update_transport_list(self):
        """Update transport list widget."""
        self.transport_list.clear()
        for transport in self.transports:
            name = transport.get('name', 'Unnamed')
            ttype = transport.get('type', 'Unknown')
            enabled = "✓" if transport.get('enabled', True) else "✗"
            item = QListWidgetItem(f"{enabled} {name} ({ttype})")
            self.transport_list.addItem(item)
    
    def _mark_dirty(self):
        """Mark configuration as modified."""
        pass  # Changes applied on button click
    
    def get_config(self) -> List[Dict[str, Any]]:
        """Get transport configuration."""
        if self.current_transport_index >= 0:
            self._save_current_transport()
        return self.transports
    
    def set_config(self, transports: List[Dict[str, Any]]):
        """Set transport configuration."""
        self.transports = transports if isinstance(transports, list) else []
        self.current_transport_index = -1
        self._update_transport_list()
