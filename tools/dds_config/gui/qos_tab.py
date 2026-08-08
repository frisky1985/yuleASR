"""
DDS Configuration Tool - QoS Tab

Configuration for DDS QoS (Quality of Service) policies including:
- Reliability
- Durability
- Deadline
- Latency Budget
- Liveliness
- Resource Limits
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


class QoSTab(QWidget):
    """QoS configuration tab."""
    
    config_changed = pyqtSignal()
    
    # Predefined QoS profiles
    PREDEFINED_PROFILES = [
        "Default",
        "Volatile_BestEffort",
        "Volatile_Reliable",
        "TransientLocal_BestEffort",
        "TransientLocal_Reliable",
        "Transient_Reliable",
        "Persistent_Reliable",
        "Automotive_Control",
        "Automotive_Sensor",
        "Automotive_Safety",
        "High_Throughput",
        "Low_Latency"
    ]
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.profiles: List[Dict[str, Any]] = []
        self.current_profile_index = -1
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup the UI components."""
        layout = QHBoxLayout(self)
        
        # Splitter
        splitter = QSplitter(Qt.Horizontal)
        
        # Left panel - Profile list
        left_panel = self._create_left_panel()
        splitter.addWidget(left_panel)
        
        # Right panel - Profile editor
        right_panel = self._create_right_panel()
        splitter.addWidget(right_panel)
        
        splitter.setSizes([300, 700])
        layout.addWidget(splitter)
    
    def _create_left_panel(self) -> QWidget:
        """Create left panel with profile list."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Title
        layout.addWidget(QLabel("<b>QoS Profiles</b>"))
        
        # Profile list
        self.profile_list = QListWidget()
        self.profile_list.currentRowChanged.connect(self._on_profile_selected)
        layout.addWidget(self.profile_list)
        
        # Buttons
        btn_layout = QHBoxLayout()
        
        add_btn = QPushButton("Add")
        add_btn.clicked.connect(self._add_profile)
        btn_layout.addWidget(add_btn)
        
        remove_btn = QPushButton("Remove")
        remove_btn.clicked.connect(self._remove_profile)
        btn_layout.addWidget(remove_btn)
        
        duplicate_btn = QPushButton("Duplicate")
        duplicate_btn.clicked.connect(self._duplicate_profile)
        btn_layout.addWidget(duplicate_btn)
        
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Predefined profiles
        predefined_group = QGroupBox("Load Predefined")
        predefined_layout = QVBoxLayout(predefined_group)
        
        self.predefined_combo = QComboBox()
        self.predefined_combo.addItems(self.PREDEFINED_PROFILES)
        predefined_layout.addWidget(self.predefined_combo)
        
        load_btn = QPushButton("Load Profile")
        load_btn.clicked.connect(self._load_predefined)
        predefined_layout.addWidget(load_btn)
        
        layout.addWidget(predefined_group)
        
        return widget
    
    def _create_right_panel(self) -> QWidget:
        """Create right panel with profile editor."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        
        # Profile editor widget
        self.editor_widget = QWidget()
        editor_layout = QVBoxLayout(self.editor_widget)
        
        # Tabs
        tabs = QTabWidget()
        
        # Reliability & Durability
        reliability_tab = self._create_reliability_tab()
        tabs.addTab(reliability_tab, "Reliability & Durability")
        
        # Timing
        timing_tab = self._create_timing_tab()
        tabs.addTab(timing_tab, "Timing")
        
        # History & Resources
        resources_tab = self._create_resources_tab()
        tabs.addTab(resources_tab, "History & Resources")
        
        # Presentation
        presentation_tab = self._create_presentation_tab()
        tabs.addTab(presentation_tab, "Presentation")
        
        # Advanced
        advanced_tab = self._create_advanced_tab()
        tabs.addTab(advanced_tab, "Advanced")
        
        editor_layout.addWidget(tabs)
        
        # Save button
        save_layout = QHBoxLayout()
        save_layout.addStretch()
        save_btn = QPushButton("Apply Changes")
        save_btn.clicked.connect(self._save_current_profile)
        save_layout.addWidget(save_btn)
        editor_layout.addLayout(save_layout)
        
        layout.addWidget(self.editor_widget)
        
        # Empty state
        self.empty_label = QLabel("Select or create a QoS profile to edit")
        self.empty_label.setAlignment(Qt.AlignCenter)
        self.empty_label.setStyleSheet("color: gray; font-size: 14px;")
        layout.addWidget(self.empty_label)
        
        self.editor_widget.hide()
        
        return widget
    
    def _create_reliability_tab(self) -> QWidget:
        """Create reliability and durability tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Profile name
        self.profile_name_edit = QLineEdit()
        self.profile_name_edit.setPlaceholderText("Unique profile name")
        self.profile_name_edit.textChanged.connect(self._mark_dirty)
        layout.addRow("Profile Name:*", self.profile_name_edit)
        
        # Description
        self.profile_desc_edit = QLineEdit()
        self.profile_desc_edit.setPlaceholderText("Profile description")
        self.profile_desc_edit.textChanged.connect(self._mark_dirty)
        layout.addRow("Description:", self.profile_desc_edit)
        
        layout.addRow(QLabel(""))  # Spacer
        
        # Reliability
        reliability_group = QGroupBox("Reliability QoS")
        reliability_layout = QFormLayout(reliability_group)
        
        self.reliability_kind_combo = QComboBox()
        self.reliability_kind_combo.addItems(["BEST_EFFORT", "RELIABLE"])
        self.reliability_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        reliability_layout.addRow("Kind:", self.reliability_kind_combo)
        
        self.max_blocking_time_spin = QDoubleSpinBox()
        self.max_blocking_time_spin.setRange(0, 100)
        self.max_blocking_time_spin.setValue(0.1)
        self.max_blocking_time_spin.setSuffix(" s")
        self.max_blocking_time_spin.setDecimals(3)
        reliability_layout.addRow("Max Blocking Time:", self.max_blocking_time_spin)
        
        self.synchronous_enabled_check = QCheckBox("Synchronous")
        reliability_layout.addRow(self.synchronous_enabled_check)
        
        layout.addRow(reliability_group)
        
        # Durability
        durability_group = QGroupBox("Durability QoS")
        durability_layout = QFormLayout(durability_group)
        
        self.durability_kind_combo = QComboBox()
        self.durability_kind_combo.addItems([
            "VOLATILE", "TRANSIENT_LOCAL", "TRANSIENT", "PERSISTENT"
        ])
        self.durability_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        durability_layout.addRow("Kind:", self.durability_kind_combo)
        
        self.durability_service_edit = QLineEdit()
        self.durability_service_edit.setPlaceholderText("Durability service name")
        durability_layout.addRow("Service:", self.durability_service_edit)
        
        layout.addRow(durability_group)
        
        layout.addStretch()
        return widget
    
    def _create_timing_tab(self) -> QWidget:
        """Create timing tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Deadline
        deadline_group = QGroupBox("Deadline QoS")
        deadline_layout = QFormLayout(deadline_group)
        
        self.deadline_enabled_check = QCheckBox("Enable Deadline")
        self.deadline_enabled_check.stateChanged.connect(self._mark_dirty)
        deadline_layout.addRow(self.deadline_enabled_check)
        
        self.deadline_period_spin = QDoubleSpinBox()
        self.deadline_period_spin.setRange(0.001, 3600)
        self.deadline_period_spin.setValue(1.0)
        self.deadline_period_spin.setSuffix(" s")
        self.deadline_period_spin.setDecimals(3)
        deadline_layout.addRow("Period:", self.deadline_period_spin)
        
        layout.addRow(deadline_group)
        
        # Latency budget
        latency_group = QGroupBox("Latency Budget QoS")
        latency_layout = QFormLayout(latency_group)
        
        self.latency_budget_spin = QDoubleSpinBox()
        self.latency_budget_spin.setRange(0, 10)
        self.latency_budget_spin.setValue(0)
        self.latency_budget_spin.setSuffix(" s")
        self.latency_budget_spin.setDecimals(6)
        latency_layout.addRow("Duration:", self.latency_budget_spin)
        
        layout.addRow(latency_group)
        
        # Lifespan
        lifespan_group = QGroupBox("Lifespan QoS")
        lifespan_layout = QFormLayout(lifespan_group)
        
        self.lifespan_enabled_check = QCheckBox("Enable Lifespan")
        self.lifespan_enabled_check.stateChanged.connect(self._mark_dirty)
        lifespan_layout.addRow(self.lifespan_enabled_check)
        
        self.lifespan_duration_spin = QDoubleSpinBox()
        self.lifespan_duration_spin.setRange(0, 86400)
        self.lifespan_duration_spin.setValue(0)
        self.lifespan_duration_spin.setSuffix(" s")
        self.lifespan_duration_spin.setDecimals(1)
        lifespan_layout.addRow("Duration:", self.lifespan_duration_spin)
        
        layout.addRow(lifespan_group)
        
        # Liveliness
        liveliness_group = QGroupBox("Liveliness QoS")
        liveliness_layout = QFormLayout(liveliness_group)
        
        self.liveliness_kind_combo = QComboBox()
        self.liveliness_kind_combo.addItems([
            "AUTOMATIC", "MANUAL_BY_PARTICIPANT", "MANUAL_BY_TOPIC"
        ])
        self.liveliness_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        liveliness_layout.addRow("Kind:", self.liveliness_kind_combo)
        
        self.liveliness_lease_duration_spin = QDoubleSpinBox()
        self.liveliness_lease_duration_spin.setRange(0, 3600)
        self.liveliness_lease_duration_spin.setValue(10)
        self.liveliness_lease_duration_spin.setSuffix(" s")
        liveliness_layout.addRow("Lease Duration:", self.liveliness_lease_duration_spin)
        
        self.liveliness_assert_period_spin = QDoubleSpinBox()
        self.liveliness_assert_period_spin.setRange(0.001, 3600)
        self.liveliness_assert_period_spin.setValue(0.5)
        self.liveliness_assert_period_spin.setSuffix(" s")
        self.liveliness_assert_period_spin.setDecimals(3)
        liveliness_layout.addRow("Assert Period:", self.liveliness_assert_period_spin)
        
        layout.addRow(liveliness_group)
        
        layout.addStretch()
        return widget
    
    def _create_resources_tab(self) -> QWidget:
        """Create history and resources tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # History
        history_group = QGroupBox("History QoS")
        history_layout = QFormLayout(history_group)
        
        self.history_kind_combo = QComboBox()
        self.history_kind_combo.addItems(["KEEP_LAST", "KEEP_ALL"])
        self.history_kind_combo.currentIndexChanged.connect(self._mark_dirty)
        history_layout.addRow("Kind:", self.history_kind_combo)
        
        self.history_depth_spin = QSpinBox()
        self.history_depth_spin.setRange(1, 10000)
        self.history_depth_spin.setValue(1)
        history_layout.addRow("Depth:", self.history_depth_spin)
        
        layout.addRow(history_group)
        
        # Resource limits
        resources_group = QGroupBox("Resource Limits QoS")
        resources_layout = QFormLayout(resources_group)
        
        self.max_samples_spin = QSpinBox()
        self.max_samples_spin.setRange(1, 1000000)
        self.max_samples_spin.setValue(-1)
        self.max_samples_spin.setSpecialValueText("UNLIMITED")
        resources_layout.addRow("Max Samples:", self.max_samples_spin)
        
        self.max_instances_spin = QSpinBox()
        self.max_instances_spin.setRange(1, 100000)
        self.max_instances_spin.setValue(-1)
        self.max_instances_spin.setSpecialValueText("UNLIMITED")
        resources_layout.addRow("Max Instances:", self.max_instances_spin)
        
        self.max_samples_per_instance_spin = QSpinBox()
        self.max_samples_per_instance_spin.setRange(1, 100000)
        self.max_samples_per_instance_spin.setValue(-1)
        self.max_samples_per_instance_spin.setSpecialValueText("UNLIMITED")
        resources_layout.addRow("Max Samples/Instance:", self.max_samples_per_instance_spin)
        
        layout.addRow(resources_group)
        
        # Reader data lifecycle
        reader_lifecycle_group = QGroupBox("Reader Data Lifecycle")
        rdl_layout = QFormLayout(reader_lifecycle_group)
        
        self.autopurge_nowriter_delay_spin = QDoubleSpinBox()
        self.autopurge_nowriter_delay_spin.setRange(0, 86400)
        self.autopurge_nowriter_delay_spin.setValue(0)
        self.autopurge_nowriter_delay_spin.setSuffix(" s")
        rdl_layout.addRow("Autopurge No Writer Delay:", self.autopurge_nowriter_delay_spin)
        
        self.autopurge_disposed_delay_spin = QDoubleSpinBox()
        self.autopurge_disposed_delay_spin.setRange(0, 86400)
        self.autopurge_disposed_delay_spin.setValue(0)
        self.autopurge_disposed_delay_spin.setSuffix(" s")
        rdl_layout.addRow("Autopurge Disposed Delay:", self.autopurge_disposed_delay_spin)
        
        layout.addRow(reader_lifecycle_group)
        
        # Writer data lifecycle
        writer_lifecycle_group = QGroupBox("Writer Data Lifecycle")
        wdl_layout = QFormLayout(writer_lifecycle_group)
        
        self.autodispose_unregistered_check = QCheckBox("Autodispose Unregistered Instances")
        self.autodispose_unregistered_check.setChecked(True)
        wdl_layout.addRow(self.autodispose_unregistered_check)
        
        layout.addRow(writer_lifecycle_group)
        
        layout.addStretch()
        return widget
    
    def _create_presentation_tab(self) -> QWidget:
        """Create presentation tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Presentation
        presentation_group = QGroupBox("Presentation QoS")
        presentation_layout = QFormLayout(presentation_group)
        
        self.presentation_access_scope_combo = QComboBox()
        self.presentation_access_scope_combo.addItems([
            "INSTANCE", "TOPIC", "GROUP"
        ])
        presentation_layout.addRow("Access Scope:", self.presentation_access_scope_combo)
        
        self.coherent_access_check = QCheckBox("Coherent Access")
        presentation_layout.addRow(self.coherent_access_check)
        
        self.ordered_access_check = QCheckBox("Ordered Access")
        presentation_layout.addRow(self.ordered_access_check)
        
        layout.addRow(presentation_group)
        
        # Destination order
        destination_order_group = QGroupBox("Destination Order QoS")
        do_layout = QFormLayout(destination_order_group)
        
        self.destination_order_combo = QComboBox()
        self.destination_order_combo.addItems(["BY_RECEPTION_TIMESTAMP", "BY_SOURCE_TIMESTAMP"])
        do_layout.addRow("Kind:", self.destination_order_combo)
        
        self.source_timestamp_enabled_check = QCheckBox("Enable Source Timestamp")
        self.source_timestamp_enabled_check.setChecked(True)
        do_layout.addRow(self.source_timestamp_enabled_check)
        
        layout.addRow(destination_order_group)
        
        # Ownership
        ownership_group = QGroupBox("Ownership QoS")
        ownership_layout = QFormLayout(ownership_group)
        
        self.ownership_kind_combo = QComboBox()
        self.ownership_kind_combo.addItems(["SHARED", "EXCLUSIVE"])
        ownership_layout.addRow("Kind:", self.ownership_kind_combo)
        
        self.ownership_strength_spin = QSpinBox()
        self.ownership_strength_spin.setRange(0, 1000000)
        self.ownership_strength_spin.setValue(0)
        ownership_layout.addRow("Strength:", self.ownership_strength_spin)
        
        layout.addRow(ownership_group)
        
        # Transport priority
        priority_group = QGroupBox("Transport Priority QoS")
        priority_layout = QFormLayout(priority_group)
        
        self.transport_priority_spin = QSpinBox()
        self.transport_priority_spin.setRange(0, 255)
        self.transport_priority_spin.setValue(0)
        priority_layout.addRow("Priority:", self.transport_priority_spin)
        
        layout.addRow(priority_group)
        
        # Partition
        partition_group = QGroupBox("Partition QoS")
        partition_layout = QFormLayout(partition_group)
        
        self.partition_name_edit = QLineEdit()
        self.partition_name_edit.setPlaceholderText("Comma-separated partition names")
        partition_layout.addRow("Names:", self.partition_name_edit)
        
        layout.addRow(partition_group)
        
        layout.addStretch()
        return widget
    
    def _create_advanced_tab(self) -> QWidget:
        """Create advanced tab."""
        widget = QWidget()
        layout = QFormLayout(widget)
        
        # Entity factory
        factory_group = QGroupBox("Entity Factory QoS")
        factory_layout = QFormLayout(factory_group)
        
        self.autoenable_created_entities_check = QCheckBox("Auto-enable Created Entities")
        self.autoenable_created_entities_check.setChecked(True)
        factory_layout.addRow(self.autoenable_created_entities_check)
        
        layout.addRow(factory_group)
        
        # Type consistency
        type_consistency_group = QGroupBox("Type Consistency Enforcement")
        tc_layout = QFormLayout(type_consistency_group)
        
        self.type_consistency_kind_combo = QComboBox()
        self.type_consistency_kind_combo.addItems([
            "DISALLOW_TYPE_COERCION", "ALLOW_TYPE_COERCION"
        ])
        tc_layout.addRow("Kind:", self.type_consistency_kind_combo)
        
        self.ignore_sequence_bounds_check = QCheckBox("Ignore Sequence Bounds")
        tc_layout.addRow(self.ignore_sequence_bounds_check)
        
        self.ignore_string_bounds_check = QCheckBox("Ignore String Bounds")
        tc_layout.addRow(self.ignore_string_bounds_check)
        
        self.ignore_member_names_check = QCheckBox("Ignore Member Names")
        tc_layout.addRow(self.ignore_member_names_check)
        
        self.prevent_type_widening_check = QCheckBox("Prevent Type Widening")
        tc_layout.addRow(self.prevent_type_widening_check)
        
        self.force_type_validation_check = QCheckBox("Force Type Validation")
        tc_layout.addRow(self.force_type_validation_check)
        
        layout.addRow(type_consistency_group)
        
        # Data representation
        data_repr_group = QGroupBox("Data Representation")
        dr_layout = QFormLayout(data_repr_group)
        
        self.use_xcdr_v1_check = QCheckBox("XCDR v1 (CDR)")
        self.use_xcdr_v1_check.setChecked(True)
        dr_layout.addRow(self.use_xcdr_v1_check)
        
        self.use_xcdr_v2_check = QCheckBox("XCDR v2 (PL_CDR)")
        self.use_xcdr_v2_check.setChecked(True)
        dr_layout.addRow(self.use_xcdr_v2_check)
        
        layout.addRow(data_repr_group)
        
        # Property
        property_group = QGroupBox("Property QoS")
        property_layout = QFormLayout(property_group)
        
        self.property_edit = QTextEdit()
        self.property_edit.setPlaceholderText("Name=Value pairs, one per line")
        self.property_edit.setMaximumHeight(100)
        property_layout.addRow("Properties:", self.property_edit)
        
        layout.addRow(property_group)
        
        layout.addStretch()
        return widget
    
    def _on_profile_selected(self, index: int):
        """Handle profile selection."""
        if index < 0 or index >= len(self.profiles):
            self.editor_widget.hide()
            self.empty_label.show()
            self.current_profile_index = -1
            return
        
        self.empty_label.hide()
        self.editor_widget.show()
        self.current_profile_index = index
        
        profile = self.profiles[index]
        self._load_profile_to_ui(profile)
    
    def _load_profile_to_ui(self, profile: Dict[str, Any]):
        """Load profile data to UI."""
        # Basic
        self.profile_name_edit.setText(profile.get('name', ''))
        self.profile_desc_edit.setText(profile.get('description', ''))
        
        # Reliability & Durability
        reliability = profile.get('reliability', {})
        self.reliability_kind_combo.setCurrentText(reliability.get('kind', 'RELIABLE'))
        self.max_blocking_time_spin.setValue(reliability.get('max_blocking_time', 0.1))
        self.synchronous_enabled_check.setChecked(reliability.get('synchronous', False))
        
        durability = profile.get('durability', {})
        self.durability_kind_combo.setCurrentText(durability.get('kind', 'VOLATILE'))
        self.durability_service_edit.setText(durability.get('service', ''))
        
        # Timing
        deadline = profile.get('deadline', {})
        self.deadline_enabled_check.setChecked(deadline.get('enabled', False))
        self.deadline_period_spin.setValue(deadline.get('period', 1.0))
        
        self.latency_budget_spin.setValue(profile.get('latency_budget', 0))
        
        lifespan = profile.get('lifespan', {})
        self.lifespan_enabled_check.setChecked(lifespan.get('enabled', False))
        self.lifespan_duration_spin.setValue(lifespan.get('duration', 0))
        
        liveliness = profile.get('liveliness', {})
        self.liveliness_kind_combo.setCurrentText(liveliness.get('kind', 'AUTOMATIC'))
        self.liveliness_lease_duration_spin.setValue(liveliness.get('lease_duration', 10))
        self.liveliness_assert_period_spin.setValue(liveliness.get('assert_period', 0.5))
        
        # History & Resources
        history = profile.get('history', {})
        self.history_kind_combo.setCurrentText(history.get('kind', 'KEEP_LAST'))
        self.history_depth_spin.setValue(history.get('depth', 1))
        
        resources = profile.get('resource_limits', {})
        max_samples = resources.get('max_samples', -1)
        self.max_samples_spin.setValue(max_samples if max_samples > 0 else -1)
        max_instances = resources.get('max_instances', -1)
        self.max_instances_spin.setValue(max_instances if max_instances > 0 else -1)
        max_spi = resources.get('max_samples_per_instance', -1)
        self.max_samples_per_instance_spin.setValue(max_spi if max_spi > 0 else -1)
        
        # Reader/Writer lifecycle
        reader_lifecycle = profile.get('reader_data_lifecycle', {})
        self.autopurge_nowriter_delay_spin.setValue(
            reader_lifecycle.get('autopurge_nowriter_delay', 0))
        self.autopurge_disposed_delay_spin.setValue(
            reader_lifecycle.get('autopurge_disposed_delay', 0))
        
        writer_lifecycle = profile.get('writer_data_lifecycle', {})
        self.autodispose_unregistered_check.setChecked(
            writer_lifecycle.get('autodispose_unregistered', True))
        
        # Presentation
        presentation = profile.get('presentation', {})
        self.presentation_access_scope_combo.setCurrentText(
            presentation.get('access_scope', 'INSTANCE'))
        self.coherent_access_check.setChecked(presentation.get('coherent_access', False))
        self.ordered_access_check.setChecked(presentation.get('ordered_access', False))
        
        # Destination order
        self.destination_order_combo.setCurrentText(profile.get('destination_order', 'BY_RECEPTION_TIMESTAMP'))
        self.source_timestamp_enabled_check.setChecked(profile.get('source_timestamp', True))
        
        # Ownership
        ownership = profile.get('ownership', {})
        self.ownership_kind_combo.setCurrentText(ownership.get('kind', 'SHARED'))
        self.ownership_strength_spin.setValue(ownership.get('strength', 0))
        
        # Transport priority
        self.transport_priority_spin.setValue(profile.get('transport_priority', 0))
        
        # Partition
        partition = profile.get('partition', {})
        names = partition.get('names', [])
        self.partition_name_edit.setText(','.join(names) if isinstance(names, list) else str(names))
        
        # Entity factory
        entity_factory = profile.get('entity_factory', {})
        self.autoenable_created_entities_check.setChecked(
            entity_factory.get('autoenable_created_entities', True))
    
    def _save_current_profile(self):
        """Save current profile data."""
        if self.current_profile_index < 0:
            return
        
        profile = {
            'name': self.profile_name_edit.text(),
            'description': self.profile_desc_edit.text(),
            'reliability': {
                'kind': self.reliability_kind_combo.currentText(),
                'max_blocking_time': self.max_blocking_time_spin.value(),
                'synchronous': self.synchronous_enabled_check.isChecked()
            },
            'durability': {
                'kind': self.durability_kind_combo.currentText(),
                'service': self.durability_service_edit.text()
            },
            'deadline': {
                'enabled': self.deadline_enabled_check.isChecked(),
                'period': self.deadline_period_spin.value()
            },
            'latency_budget': self.latency_budget_spin.value(),
            'lifespan': {
                'enabled': self.lifespan_enabled_check.isChecked(),
                'duration': self.lifespan_duration_spin.value()
            },
            'liveliness': {
                'kind': self.liveliness_kind_combo.currentText(),
                'lease_duration': self.liveliness_lease_duration_spin.value(),
                'assert_period': self.liveliness_assert_period_spin.value()
            },
            'history': {
                'kind': self.history_kind_combo.currentText(),
                'depth': self.history_depth_spin.value()
            },
            'resource_limits': {
                'max_samples': self.max_samples_spin.value() if self.max_samples_spin.value() > 0 else -1,
                'max_instances': self.max_instances_spin.value() if self.max_instances_spin.value() > 0 else -1,
                'max_samples_per_instance': self.max_samples_per_instance_spin.value() if self.max_samples_per_instance_spin.value() > 0 else -1
            },
            'reader_data_lifecycle': {
                'autopurge_nowriter_delay': self.autopurge_nowriter_delay_spin.value(),
                'autopurge_disposed_delay': self.autopurge_disposed_delay_spin.value()
            },
            'writer_data_lifecycle': {
                'autodispose_unregistered': self.autodispose_unregistered_check.isChecked()
            },
            'presentation': {
                'access_scope': self.presentation_access_scope_combo.currentText(),
                'coherent_access': self.coherent_access_check.isChecked(),
                'ordered_access': self.ordered_access_check.isChecked()
            },
            'destination_order': self.destination_order_combo.currentText(),
            'source_timestamp': self.source_timestamp_enabled_check.isChecked(),
            'ownership': {
                'kind': self.ownership_kind_combo.currentText(),
                'strength': self.ownership_strength_spin.value()
            },
            'transport_priority': self.transport_priority_spin.value(),
            'partition': {
                'names': [n.strip() for n in self.partition_name_edit.text().split(',') if n.strip()]
            },
            'entity_factory': {
                'autoenable_created_entities': self.autoenable_created_entities_check.isChecked()
            }
        }
        
        self.profiles[self.current_profile_index] = profile
        self._update_profile_list()
        self.config_changed.emit()
    
    def _add_profile(self):
        """Add new QoS profile."""
        profile = {
            'name': f"Profile_{len(self.profiles) + 1}",
            'description': '',
            'reliability': {'kind': 'RELIABLE'},
            'durability': {'kind': 'VOLATILE'},
            'history': {'kind': 'KEEP_LAST', 'depth': 1}
        }
        self.profiles.append(profile)
        self._update_profile_list()
        self.profile_list.setCurrentRow(len(self.profiles) - 1)
        self.config_changed.emit()
    
    def _remove_profile(self):
        """Remove selected profile."""
        index = self.profile_list.currentRow()
        if index >= 0:
            self.profiles.pop(index)
            self._update_profile_list()
            self.config_changed.emit()
    
    def _duplicate_profile(self):
        """Duplicate selected profile."""
        index = self.profile_list.currentRow()
        if index >= 0:
            profile = self.profiles[index].copy()
            profile['name'] = f"{profile['name']}_copy"
            self.profiles.insert(index + 1, profile)
            self._update_profile_list()
            self.profile_list.setCurrentRow(index + 1)
            self.config_changed.emit()
    
    def _load_predefined(self):
        """Load predefined QoS profile."""
        profile_name = self.predefined_combo.currentText()
        
        # Predefined profiles data
        profiles = {
            "Default": {
                'reliability': {'kind': 'RELIABLE'},
                'durability': {'kind': 'VOLATILE'},
                'history': {'kind': 'KEEP_LAST', 'depth': 1}
            },
            "Volatile_BestEffort": {
                'reliability': {'kind': 'BEST_EFFORT'},
                'durability': {'kind': 'VOLATILE'},
                'history': {'kind': 'KEEP_LAST', 'depth': 1}
            },
            "Volatile_Reliable": {
                'reliability': {'kind': 'RELIABLE'},
                'durability': {'kind': 'VOLATILE'},
                'history': {'kind': 'KEEP_LAST', 'depth': 1}
            },
            "TransientLocal_Reliable": {
                'reliability': {'kind': 'RELIABLE'},
                'durability': {'kind': 'TRANSIENT_LOCAL'},
                'history': {'kind': 'KEEP_LAST', 'depth': 10}
            },
            "Automotive_Control": {
                'reliability': {'kind': 'RELIABLE'},
                'durability': {'kind': 'TRANSIENT_LOCAL'},
                'history': {'kind': 'KEEP_LAST', 'depth': 1},
                'deadline': {'enabled': True, 'period': 0.01},
                'latency_budget': 0.001,
                'transport_priority': 255
            },
            "Automotive_Safety": {
                'reliability': {'kind': 'RELIABLE'},
                'durability': {'kind': 'TRANSIENT_LOCAL'},
                'history': {'kind': 'KEEP_ALL'},
                'deadline': {'enabled': True, 'period': 0.005},
                'latency_budget': 0.0005,
                'liveliness': {'kind': 'AUTOMATIC', 'lease_duration': 0.1},
                'transport_priority': 255
            }
        }
        
        data = profiles.get(profile_name, {})
        
        profile = {
            'name': profile_name,
            'description': f"Predefined {profile_name} profile",
            **data
        }
        
        self.profiles.append(profile)
        self._update_profile_list()
        self.profile_list.setCurrentRow(len(self.profiles) - 1)
        self.config_changed.emit()
    
    def _update_profile_list(self):
        """Update profile list widget."""
        self.profile_list.clear()
        for profile in self.profiles:
            name = profile.get('name', 'Unnamed')
            desc = profile.get('description', '')
            item = QListWidgetItem(name)
            item.setToolTip(desc if desc else name)
            self.profile_list.addItem(item)
    
    def _mark_dirty(self):
        """Mark configuration as modified."""
        pass  # Changes applied on button click
    
    def get_config(self) -> List[Dict[str, Any]]:
        """Get QoS profiles configuration."""
        if self.current_profile_index >= 0:
            self._save_current_profile()
        return self.profiles
    
    def set_config(self, profiles: List[Dict[str, Any]]):
        """Set QoS profiles configuration."""
        self.profiles = profiles if isinstance(profiles, list) else []
        self.current_profile_index = -1
        self._update_profile_list()
