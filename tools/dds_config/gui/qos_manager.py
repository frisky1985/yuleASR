#!/usr/bin/env python3
"""
QoS Manager Component

Provides visual editing capabilities for DDS QoS profile management:
    - QoS profile creation and editing
    - Policy configuration
    - Profile comparison and validation
    - Profile import/export

Classes:
    QoSManager: Main QoS profile manager widget
    QoSProfileEditor: Dialog for editing QoS profiles
    QoSValidator: Validates QoS policy combinations
"""

import sys
from typing import Dict, List, Optional, Any, Set, Tuple
from dataclasses import dataclass, field, asdict
from enum import Enum

try:
    from PyQt5.QtWidgets import (
        QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
        QComboBox, QPushButton, QTableWidget, QTableWidgetItem,
        QDialog, QFormLayout, QGroupBox, QSpinBox, QDoubleSpinBox,
        QMessageBox, QHeaderView, QMenu, QAction, QTabWidget,
        QTextEdit, QFileDialog, QListWidget, QListWidgetItem,
        QSplitter, QFrame, QScrollArea, QGridLayout, QCheckBox,
        QTreeWidget, QTreeWidgetItem
    )
    from PyQt5.QtCore import Qt, pyqtSignal, QSize
    from PyQt5.QtGui import QColor, QBrush, QFont
    HAS_QT = True
except ImportError:
    HAS_QT = False
    # Stub classes for non-GUI environments
    class QWidget:
        pass
    class QDialog:
        pass
    pyqtSignal = lambda: None


class ReliabilityKind(Enum):
    BEST_EFFORT = "BEST_EFFORT"
    RELIABLE = "RELIABLE"


class DurabilityKind(Enum):
    VOLATILE = "VOLATILE"
    TRANSIENT_LOCAL = "TRANSIENT_LOCAL"
    TRANSIENT = "TRANSIENT"
    PERSISTENT = "PERSISTENT"


class HistoryKind(Enum):
    KEEP_LAST = "KEEP_LAST"
    KEEP_ALL = "KEEP_ALL"


class LivelinessKind(Enum):
    AUTOMATIC = "AUTOMATIC"
    MANUAL_BY_TOPIC = "MANUAL_BY_TOPIC"
    MANUAL_BY_PARTICIPANT = "MANUAL_BY_PARTICIPANT"


class DestinationOrderKind(Enum):
    BY_RECEPTION_TIMESTAMP = "BY_RECEPTION_TIMESTAMP"
    BY_SOURCE_TIMESTAMP = "BY_SOURCE_TIMESTAMP"


@dataclass
class QoSPolicy:
    """Individual QoS policy configuration"""
    name: str
    value: Any
    enabled: bool = True
    description: str = ""


@dataclass
class QoSProfile:
    """Complete QoS profile configuration"""
    name: str
    description: str = ""
    
    # Reliability policy
    reliability: ReliabilityKind = ReliabilityKind.BEST_EFFORT
    
    # Durability policy
    durability: DurabilityKind = DurabilityKind.VOLATILE
    
    # History policy
    history_kind: HistoryKind = HistoryKind.KEEP_LAST
    history_depth: int = 1
    
    # Deadline policy
    deadline_sec: int = 0
    deadline_nsec: int = 0
    
    # Latency budget
    latency_budget_sec: int = 0
    latency_budget_nsec: int = 0
    
    # Liveliness policy
    liveliness: LivelinessKind = LivelinessKind.AUTOMATIC
    liveliness_lease_duration_sec: int = 0
    liveliness_lease_duration_nsec: int = 0
    
    # Destination order
    destination_order: DestinationOrderKind = DestinationOrderKind.BY_RECEPTION_TIMESTAMP
    
    # Ownership
    ownership_kind: str = "SHARED"
    
    # Resource limits
    max_samples: int = -1  # -1 means unlimited
    max_instances: int = -1
    max_samples_per_instance: int = -1
    
    # Transport priority
    transport_priority: int = 0
    
    # Lifespan
    lifespan_sec: int = 0
    lifespan_nsec: int = 0
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary"""
        return {
            'reliability': self.reliability.value,
            'durability': self.durability.value,
            'history': {
                'kind': self.history_kind.value,
                'depth': self.history_depth
            },
            'deadline': {
                'sec': self.deadline_sec,
                'nanosec': self.deadline_nsec
            } if self.deadline_sec > 0 or self.deadline_nsec > 0 else None,
            'latency_budget': {
                'sec': self.latency_budget_sec,
                'nanosec': self.latency_budget_nsec
            } if self.latency_budget_sec > 0 or self.latency_budget_nsec > 0 else None,
            'liveliness': {
                'kind': self.liveliness.value,
                'lease_duration': {
                    'sec': self.liveliness_lease_duration_sec,
                    'nanosec': self.liveliness_lease_duration_nsec
                }
            },
            'destination_order': self.destination_order.value,
            'ownership': {'kind': self.ownership_kind},
            'resource_limits': {
                'max_samples': self.max_samples,
                'max_instances': self.max_instances,
                'max_samples_per_instance': self.max_samples_per_instance
            } if self.max_samples > 0 else None,
            'transport_priority': self.transport_priority if self.transport_priority > 0 else None,
            'lifespan': {
                'sec': self.lifespan_sec,
                'nanosec': self.lifespan_nsec
            } if self.lifespan_sec > 0 or self.lifespan_nsec > 0 else None
        }
    
    @classmethod
    def from_dict(cls, name: str, data: Dict[str, Any]) -> 'QoSProfile':
        """Create QoSProfile from dictionary"""
        profile = cls(name=name)
        
        if 'reliability' in data:
            profile.reliability = ReliabilityKind(data['reliability'])
        if 'durability' in data:
            profile.durability = DurabilityKind(data['durability'])
        
        if 'history' in data:
            hist = data['history']
            if isinstance(hist, dict):
                profile.history_kind = HistoryKind(hist.get('kind', 'KEEP_LAST'))
                profile.history_depth = hist.get('depth', 1)
            else:
                profile.history_kind = HistoryKind(hist)
        
        if 'deadline' in data:
            dl = data['deadline']
            profile.deadline_sec = dl.get('sec', 0)
            profile.deadline_nsec = dl.get('nanosec', 0)
        
        return profile


class QoSValidator:
    """Validates QoS policy combinations"""
    
    # Incompatible policy combinations
    INCOMPATIBILITIES: List[Tuple[str, Any, str, Any, str]] = [
        ('durability', DurabilityKind.PERSISTENT, 'reliability', ReliabilityKind.BEST_EFFORT,
         "PERSISTENT durability requires RELIABLE reliability"),
        ('history_kind', HistoryKind.KEEP_ALL, 'resource_limits', None,
         "KEEP_ALL history requires explicit resource limits"),
    ]
    
    # Warnings for suboptimal combinations
    WARNINGS: List[Tuple[str, Any, str, Any, str]] = [
        ('reliability', ReliabilityKind.RELIABLE, 'history_kind', HistoryKind.KEEP_ALL,
         "RELIABLE + KEEP_ALL may cause memory issues with high data rates"),
        ('durability', DurabilityKind.TRANSIENT, 'history_kind', HistoryKind.KEEP_ALL,
         "TRANSIENT durability with KEEP_ALL may cause excessive memory usage"),
    ]
    
    @classmethod
    def validate_profile(cls, profile: QoSProfile) -> Tuple[List[str], List[str]]:
        """
        Validate a QoS profile
        
        Returns:
            Tuple of (errors, warnings)
        """
        errors = []
        warnings = []
        
        # Check incompatible combinations
        for policy1, value1, policy2, value2, message in cls.INCOMPATIBILITIES:
            actual_value1 = getattr(profile, policy1)
            actual_value2 = getattr(profile, policy2) if value2 is not None else None
            
            if isinstance(actual_value1, Enum):
                actual_value1 = actual_value1.value
            if isinstance(actual_value2, Enum):
                actual_value2 = actual_value2.value
            
            if actual_value1 == value1.value if isinstance(value1, Enum) else value1:
                if value2 is None or actual_value2 == value2.value if isinstance(value2, Enum) else value2:
                    errors.append(message)
        
        # Check warnings
        for policy1, value1, policy2, value2, message in cls.WARNINGS:
            actual_value1 = getattr(profile, policy1)
            actual_value2 = getattr(profile, policy2)
            
            if isinstance(actual_value1, Enum):
                actual_value1 = actual_value1.value
            if isinstance(actual_value2, Enum):
                actual_value2 = actual_value2.value
            
            if actual_value1 == (value1.value if isinstance(value1, Enum) else value1):
                if actual_value2 == (value2.value if isinstance(value2, Enum) else value2):
                    warnings.append(message)
        
        # Validate numeric ranges
        if profile.history_depth < 1:
            errors.append("History depth must be at least 1")
        
        if profile.history_depth > 10000:
            warnings.append("Very large history depth may cause memory issues")
        
        return errors, warnings
    
    @classmethod
    def check_compatibility(cls, pub_profile: QoSProfile, sub_profile: QoSProfile) -> Tuple[bool, str]:
        """
        Check if publisher and subscriber QoS are compatible
        
        Returns:
            Tuple of (is_compatible, message)
        """
        # Reliability: Subscriber can request higher reliability than publisher
        if pub_profile.reliability == ReliabilityKind.BEST_EFFORT and \
           sub_profile.reliability == ReliabilityKind.RELIABLE:
            return False, "Publisher BEST_EFFORT incompatible with Subscriber RELIABLE"
        
        # Durability: Subscriber can request higher durability than publisher
        durability_order = [DurabilityKind.VOLATILE, DurabilityKind.TRANSIENT_LOCAL,
                           DurabilityKind.TRANSIENT, DurabilityKind.PERSISTENT]
        pub_dur_idx = durability_order.index(pub_profile.durability)
        sub_dur_idx = durability_order.index(sub_profile.durability)
        
        if sub_dur_idx > pub_dur_idx:
            return False, f"Publisher {pub_profile.durability.value} incompatible with Subscriber {sub_profile.durability.value}"
        
        # Destination order: Must match exactly
        if pub_profile.destination_order != sub_profile.destination_order:
            return False, "Destination order policies must match"
        
        return True, "QoS profiles are compatible"


class QoSProfileEditor(QDialog):
    """Dialog for editing QoS profiles"""
    
    profile_saved = pyqtSignal(object)  # Emits QoSProfile
    
    def __init__(self, parent=None, profile: Optional[QoSProfile] = None):
        if not HAS_QT:
            raise RuntimeError("PyQt5 is required for GUI components")
        
        super().__init__(parent)
        self.profile = profile or QoSProfile(name="")
        self.setWindowTitle("QoS Profile Editor")
        self.setMinimumSize(600, 700)
        self.setup_ui()
        self.load_profile_data()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout()
        
        # Basic info
        basic_group = QGroupBox("Profile Information")
        basic_layout = QFormLayout()
        
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("Profile name (e.g., sensor_high_reliability)")
        basic_layout.addRow("Name:", self.name_edit)
        
        self.desc_edit = QTextEdit()
        self.desc_edit.setMaximumHeight(80)
        self.desc_edit.setPlaceholderText("Optional description of this QoS profile")
        basic_layout.addRow("Description:", self.desc_edit)
        
        basic_group.setLayout(basic_layout)
        layout.addWidget(basic_group)
        
        # Create tabs for policy categories
        tabs = QTabWidget()
        
        # Communication policies tab
        comm_tab = self._create_communication_tab()
        tabs.addTab(comm_tab, "Communication")
        
        # Resource policies tab
        resource_tab = self._create_resource_tab()
        tabs.addTab(resource_tab, "Resources")
        
        # Time-based policies tab
        time_tab = self._create_time_tab()
        tabs.addTab(time_tab, "Timing")
        
        layout.addWidget(tabs)
        
        # Validation results
        self.validation_group = QGroupBox("Validation")
        self.validation_layout = QVBoxLayout()
        self.validation_label = QLabel("Click Validate to check profile")
        self.validation_label.setWordWrap(True)
        self.validation_layout.addWidget(self.validation_label)
        
        validate_btn = QPushButton("Validate Profile")
        validate_btn.clicked.connect(self.validate_profile)
        self.validation_layout.addWidget(validate_btn)
        
        self.validation_group.setLayout(self.validation_layout)
        layout.addWidget(self.validation_group)
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        self.save_btn = QPushButton("Save")
        self.save_btn.setDefault(True)
        self.save_btn.clicked.connect(self.save_profile)
        button_layout.addWidget(self.save_btn)
        
        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(self.cancel_btn)
        
        layout.addLayout(button_layout)
        self.setLayout(layout)
    
    def _create_communication_tab(self) -> QWidget:
        """Create communication policies tab"""
        widget = QWidget()
        layout = QFormLayout()
        
        # Reliability
        self.reliability_combo = QComboBox()
        for r in ReliabilityKind:
            self.reliability_combo.addItem(r.value, r)
        self.reliability_combo.setToolTip(
            "BEST_EFFORT: Samples may be lost under load\n"
            "RELIABLE: Guaranteed delivery with acknowledgments"
        )
        layout.addRow("Reliability:", self.reliability_combo)
        
        # Durability
        self.durability_combo = QComboBox()
        for d in DurabilityKind:
            self.durability_combo.addItem(d.value, d)
        self.durability_combo.setToolTip(
            "VOLATILE: No persistence\n"
            "TRANSIENT_LOCAL: Late-joiners get last value\n"
            "TRANSIENT: Persistent storage (service-based)\n"
            "PERSISTENT: Durable storage"
        )
        layout.addRow("Durability:", self.durability_combo)
        
        # History
        history_layout = QHBoxLayout()
        self.history_combo = QComboBox()
        for h in HistoryKind:
            self.history_combo.addItem(h.value, h)
        history_layout.addWidget(self.history_combo)
        
        self.history_depth_spin = QSpinBox()
        self.history_depth_spin.setRange(1, 100000)
        self.history_depth_spin.setValue(1)
        history_layout.addWidget(QLabel("Depth:"))
        history_layout.addWidget(self.history_depth_spin)
        history_layout.addStretch()
        
        layout.addRow("History:", history_layout)
        
        # Liveliness
        self.liveliness_combo = QComboBox()
        for l in LivelinessKind:
            self.liveliness_combo.addItem(l.value, l)
        layout.addRow("Liveliness:", self.liveliness_combo)
        
        # Destination order
        self.order_combo = QComboBox()
        for o in DestinationOrderKind:
            self.order_combo.addItem(o.value, o)
        layout.addRow("Destination Order:", self.order_combo)
        
        # Ownership
        self.ownership_combo = QComboBox()
        self.ownership_combo.addItems(["SHARED", "EXCLUSIVE"])
        layout.addRow("Ownership:", self.ownership_combo)
        
        widget.setLayout(layout)
        return widget
    
    def _create_resource_tab(self) -> QWidget:
        """Create resource policies tab"""
        widget = QWidget()
        layout = QFormLayout()
        
        # Resource limits
        limits_group = QGroupBox("Resource Limits (-1 = unlimited)")
        limits_layout = QFormLayout()
        
        self.max_samples_spin = QSpinBox()
        self.max_samples_spin.setRange(-1, 1000000)
        self.max_samples_spin.setValue(-1)
        self.max_samples_spin.setSpecialValueText("unlimited")
        limits_layout.addRow("Max Samples:", self.max_samples_spin)
        
        self.max_instances_spin = QSpinBox()
        self.max_instances_spin.setRange(-1, 100000)
        self.max_instances_spin.setValue(-1)
        self.max_instances_spin.setSpecialValueText("unlimited")
        limits_layout.addRow("Max Instances:", self.max_instances_spin)
        
        self.max_samples_per_inst_spin = QSpinBox()
        self.max_samples_per_inst_spin.setRange(-1, 100000)
        self.max_samples_per_inst_spin.setValue(-1)
        self.max_samples_per_inst_spin.setSpecialValueText("unlimited")
        limits_layout.addRow("Max Samples/Instance:", self.max_samples_per_inst_spin)
        
        limits_group.setLayout(limits_layout)
        layout.addRow(limits_group)
        
        # Transport priority
        self.priority_spin = QSpinBox()
        self.priority_spin.setRange(0, 100)
        layout.addRow("Transport Priority:", self.priority_spin)
        
        widget.setLayout(layout)
        return widget
    
    def _create_time_tab(self) -> QWidget:
        """Create time-based policies tab"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # Deadline
        deadline_group = QGroupBox("Deadline (maximum expected interval between samples)")
        deadline_layout = QHBoxLayout()
        
        self.deadline_sec_spin = QSpinBox()
        self.deadline_sec_spin.setRange(0, 3600)
        self.deadline_sec_spin.setSuffix(" sec")
        deadline_layout.addWidget(self.deadline_sec_spin)
        
        self.deadline_nsec_spin = QSpinBox()
        self.deadline_nsec_spin.setRange(0, 999999999)
        self.deadline_nsec_spin.setSuffix(" nsec")
        self.deadline_nsec_spin.setSingleStep(1000000)
        deadline_layout.addWidget(self.deadline_nsec_spin)
        deadline_layout.addStretch()
        
        deadline_group.setLayout(deadline_layout)
        layout.addWidget(deadline_group)
        
        # Latency budget
        latency_group = QGroupBox("Latency Budget")
        latency_layout = QHBoxLayout()
        
        self.latency_sec_spin = QSpinBox()
        self.latency_sec_spin.setRange(0, 60)
        self.latency_sec_spin.setSuffix(" sec")
        latency_layout.addWidget(self.latency_sec_spin)
        
        self.latency_nsec_spin = QSpinBox()
        self.latency_nsec_spin.setRange(0, 999999999)
        self.latency_nsec_spin.setSuffix(" nsec")
        latency_layout.addWidget(self.latency_nsec_spin)
        latency_layout.addStretch()
        
        latency_group.setLayout(latency_layout)
        layout.addWidget(latency_group)
        
        # Lifespan
        lifespan_group = QGroupBox("Lifespan (how long samples remain valid)")
        lifespan_layout = QHBoxLayout()
        
        self.lifespan_sec_spin = QSpinBox()
        self.lifespan_sec_spin.setRange(0, 86400)
        self.lifespan_sec_spin.setSuffix(" sec")
        lifespan_layout.addWidget(self.lifespan_sec_spin)
        
        self.lifespan_nsec_spin = QSpinBox()
        self.lifespan_nsec_spin.setRange(0, 999999999)
        self.lifespan_nsec_spin.setSuffix(" nsec")
        lifespan_layout.addWidget(self.lifespan_nsec_spin)
        lifespan_layout.addStretch()
        
        lifespan_group.setLayout(lifespan_layout)
        layout.addWidget(lifespan_group)
        
        # Liveliness lease duration
        lease_group = QGroupBox("Liveliness Lease Duration")
        lease_layout = QHBoxLayout()
        
        self.lease_sec_spin = QSpinBox()
        self.lease_sec_spin.setRange(0, 3600)
        self.lease_sec_spin.setSuffix(" sec")
        lease_layout.addWidget(self.lease_sec_spin)
        
        self.lease_nsec_spin = QSpinBox()
        self.lease_nsec_spin.setRange(0, 999999999)
        self.lease_nsec_spin.setSuffix(" nsec")
        lease_layout.addWidget(self.lease_nsec_spin)
        lease_layout.addStretch()
        
        lease_group.setLayout(lease_layout)
        layout.addWidget(lease_group)
        
        layout.addStretch()
        widget.setLayout(layout)
        return widget
    
    def load_profile_data(self):
        """Load profile data into form"""
        self.name_edit.setText(self.profile.name)
        self.desc_edit.setPlainText(self.profile.description)
        
        self.reliability_combo.setCurrentIndex(
            self.reliability_combo.findData(self.profile.reliability))
        self.durability_combo.setCurrentIndex(
            self.durability_combo.findData(self.profile.durability))
        self.history_combo.setCurrentIndex(
            self.history_combo.findData(self.profile.history_kind))
        
        self.history_depth_spin.setValue(self.profile.history_depth)
        self.deadline_sec_spin.setValue(self.profile.deadline_sec)
        self.deadline_nsec_spin.setValue(self.profile.deadline_nsec)
        
        self.liveliness_combo.setCurrentIndex(
            self.liveliness_combo.findData(self.profile.liveliness))
        self.order_combo.setCurrentIndex(
            self.order_combo.findData(self.profile.destination_order))
        
        idx = self.ownership_combo.findText(self.profile.ownership_kind)
        if idx >= 0:
            self.ownership_combo.setCurrentIndex(idx)
    
    def validate_profile(self):
        """Validate current profile settings"""
        self.update_profile_from_ui()
        errors, warnings = QoSValidator.validate_profile(self.profile)
        
        if errors:
            text = "<span style='color: red;'><b>Errors:</b></span><ul>"
            for e in errors:
                text += f"<li>{e}</li>"
            text += "</ul>"
        elif warnings:
            text = "<span style='color: green;'>✓ Valid</span><br>"
            text += "<span style='color: orange;'><b>Warnings:</b></span><ul>"
            for w in warnings:
                text += f"<li>{w}</li>"
            text += "</ul>"
        else:
            text = "<span style='color: green;'>✓ Profile is valid</span>"
        
        self.validation_label.setText(text)
    
    def update_profile_from_ui(self):
        """Update profile from UI values"""
        self.profile.name = self.name_edit.text().strip()
        self.profile.description = self.desc_edit.toPlainText()
        
        self.profile.reliability = self.reliability_combo.currentData()
        self.profile.durability = self.durability_combo.currentData()
        self.profile.history_kind = self.history_combo.currentData()
        self.profile.history_depth = self.history_depth_spin.value()
        
        self.profile.deadline_sec = self.deadline_sec_spin.value()
        self.profile.deadline_nsec = self.deadline_nsec_spin.value()
        
        self.profile.liveliness = self.liveliness_combo.currentData()
        self.profile.destination_order = self.order_combo.currentData()
        self.profile.ownership_kind = self.ownership_combo.currentText()
    
    def save_profile(self):
        """Save profile"""
        self.update_profile_from_ui()
        
        if not self.profile.name:
            QMessageBox.warning(self, "Validation Error", "Profile name is required")
            return
        
        self.profile_saved.emit(self.profile)
        self.accept()


class QoSManager(QWidget):
    """Main QoS profile manager widget"""
    
    profile_added = pyqtSignal(object)
    profile_edited = pyqtSignal(object)
    profile_removed = pyqtSignal(str)
    
    def __init__(self, parent=None):
        if not HAS_QT:
            raise RuntimeError("PyQt5 is required for GUI components")
        
        super().__init__(parent)
        self.profiles: Dict[str, QoSProfile] = {}
        self.setup_ui()
    
    def setup_ui(self):
        """Initialize UI"""
        layout = QHBoxLayout()
        
        # Left panel - profile list
        left_panel = QWidget()
        left_layout = QVBoxLayout()
        
        # Toolbar
        toolbar = QHBoxLayout()
        
        self.add_btn = QPushButton("Add Profile")
        self.add_btn.clicked.connect(self.add_profile)
        toolbar.addWidget(self.add_btn)
        
        self.edit_btn = QPushButton("Edit")
        self.edit_btn.setEnabled(False)
        self.edit_btn.clicked.connect(self.edit_selected_profile)
        toolbar.addWidget(self.edit_btn)
        
        self.remove_btn = QPushButton("Remove")
        self.remove_btn.setEnabled(False)
        self.remove_btn.clicked.connect(self.remove_selected_profile)
        toolbar.addWidget(self.remove_btn)
        
        toolbar.addStretch()
        left_layout.addLayout(toolbar)
        
        # Profile list
        self.profile_list = QListWidget()
        self.profile_list.setMaximumWidth(250)
        self.profile_list.itemSelectionChanged.connect(self.on_selection_changed)
        self.profile_list.itemDoubleClicked.connect(self.edit_selected_profile)
        left_layout.addWidget(self.profile_list)
        
        left_panel.setLayout(left_layout)
        layout.addWidget(left_panel)
        
        # Right panel - profile details
        self.details_panel = QGroupBox("Profile Details")
        self.details_layout = QVBoxLayout()
        self.details_text = QTextEdit()
        self.details_text.setReadOnly(True)
        self.details_layout.addWidget(self.details_text)
        
        # Comparison section
        self.compare_btn = QPushButton("Compare with...")
        self.compare_btn.setEnabled(False)
        self.compare_btn.clicked.connect(self.compare_profiles)
        self.details_layout.addWidget(self.compare_btn)
        
        self.details_panel.setLayout(self.details_layout)
        layout.addWidget(self.details_panel, 1)
        
        self.setLayout(layout)
    
    def add_profile(self):
        """Add new profile"""
        dialog = QoSProfileEditor(self)
        dialog.profile_saved.connect(self._on_profile_saved)
        dialog.exec_()
    
    def _on_profile_saved(self, profile: QoSProfile):
        """Handle profile saved"""
        is_new = profile.name not in self.profiles
        self.profiles[profile.name] = profile
        self.refresh_list()
        
        if is_new:
            self.profile_added.emit(profile)
        else:
            self.profile_edited.emit(profile)
    
    def edit_selected_profile(self):
        """Edit selected profile"""
        item = self.profile_list.currentItem()
        if not item:
            return
        
        profile_name = item.text()
        profile = self.profiles.get(profile_name)
        if profile:
            dialog = QoSProfileEditor(self, profile)
            dialog.profile_saved.connect(self._on_profile_saved)
            dialog.exec_()
    
    def remove_selected_profile(self):
        """Remove selected profile"""
        item = self.profile_list.currentItem()
        if not item:
            return
        
        profile_name = item.text()
        
        reply = QMessageBox.question(
            self, "Confirm Removal",
            f"Remove QoS profile '{profile_name}'?",
            QMessageBox.Yes | QMessageBox.No
        )
        
        if reply == QMessageBox.Yes:
            del self.profiles[profile_name]
            self.refresh_list()
            self.profile_removed.emit(profile_name)
    
    def refresh_list(self):
        """Refresh profile list"""
        self.profile_list.clear()
        for name in sorted(self.profiles.keys()):
            self.profile_list.addItem(name)
    
    def on_selection_changed(self):
        """Handle selection change"""
        item = self.profile_list.currentItem()
        has_selection = item is not None
        
        self.edit_btn.setEnabled(has_selection)
        self.remove_btn.setEnabled(has_selection)
        self.compare_btn.setEnabled(has_selection)
        
        if has_selection:
            profile = self.profiles.get(item.text())
            self.update_details(profile)
        else:
            self.details_text.clear()
    
    def update_details(self, profile: QoSProfile):
        """Update details panel"""
        if not profile:
            return
        
        text = f"<h3>{profile.name}</h3>"
        if profile.description:
            text += f"<p><i>{profile.description}</i></p>"
        
        text += "<h4>Policies:</h4><ul>"
        text += f"<li><b>Reliability:</b> {profile.reliability.value}</li>"
        text += f"<li><b>Durability:</b> {profile.durability.value}</li>"
        text += f"<li><b>History:</b> {profile.history_kind.value} (depth: {profile.history_depth})</li>"
        text += f"<li><b>Liveliness:</b> {profile.liveliness.value}</li>"
        text += f"<li><b>Destination Order:</b> {profile.destination_order.value}</li>"
        text += f"<li><b>Ownership:</b> {profile.ownership_kind}</li>"
        
        if profile.deadline_sec > 0 or profile.deadline_nsec > 0:
            text += f"<li><b>Deadline:</b> {profile.deadline_sec}s {profile.deadline_nsec}ns</li>"
        
        if profile.max_samples > 0:
            text += f"<li><b>Max Samples:</b> {profile.max_samples}</li>"
        
        text += "</ul>"
        
        # Add validation info
        errors, warnings = QoSValidator.validate_profile(profile)
        if errors:
            text += "<h4 style='color: red;'>Validation Errors:</h4><ul>"
            for e in errors:
                text += f"<li style='color: red;'>{e}</li>"
            text += "</ul>"
        elif warnings:
            text += "<h4 style='color: orange;'>Warnings:</h4><ul>"
            for w in warnings:
                text += f"<li style='color: orange;'>{w}</li>"
            text += "</ul>"
        else:
            text += "<p style='color: green;'>✓ Valid profile</p>"
        
        self.details_text.setHtml(text)
    
    def compare_profiles(self):
        """Compare two profiles"""
        # TODO: Implement profile comparison dialog
        QMessageBox.information(self, "Compare", "Profile comparison not yet implemented")
    
    def get_profiles(self) -> Dict[str, QoSProfile]:
        """Get all profiles"""
        return self.profiles.copy()
    
    def set_profiles(self, profiles: Dict[str, QoSProfile]):
        """Set profiles from external source"""
        self.profiles = dict(profiles)
        self.refresh_list()


# Example usage
if __name__ == '__main__':
    if not HAS_QT:
        print("PyQt5 is required to run the GUI demo")
        sys.exit(1)
    
    from PyQt5.QtWidgets import QApplication
    
    app = QApplication(sys.argv)
    
    # Create and show QoS manager
    manager = QoSManager()
    manager.setWindowTitle("DDS QoS Manager")
    manager.resize(800, 600)
    
    # Add sample profiles
    sample_profiles = {
        'default': QoSProfile(
            name='default',
            description='Default QoS profile',
            reliability=ReliabilityKind.BEST_EFFORT
        ),
        'reliable': QoSProfile(
            name='reliable',
            description='Reliable communication profile',
            reliability=ReliabilityKind.RELIABLE,
            durability=DurabilityKind.TRANSIENT_LOCAL,
            history_kind=HistoryKind.KEEP_LAST,
            history_depth=10
        ),
        'realtime': QoSProfile(
            name='realtime',
            description='Real-time profile with deadline',
            reliability=ReliabilityKind.RELIABLE,
            deadline_sec=0,
            deadline_nsec=100000000
        )
    }
    manager.set_profiles(sample_profiles)
    
    manager.show()
    sys.exit(app.exec_())
