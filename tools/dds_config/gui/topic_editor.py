#!/usr/bin/env python3
"""
Topic Editor Component

Provides visual editing capabilities for DDS Topic configuration:
    - Topic creation and editing
    - Type definition management
    - QoS assignment
    - Topic visualization

Classes:
    TopicEditor: Main topic editor widget
    TopicEditorDialog: Dialog for editing individual topics
    TopicVisualizer: Graphical representation of topic connections
"""

import sys
from typing import Dict, List, Optional, Any, Callable
from dataclasses import dataclass, field

try:
    from PyQt5.QtWidgets import (
        QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
        QComboBox, QPushButton, QTableWidget, QTableWidgetItem,
        QDialog, QFormLayout, QGroupBox, QSpinBox, QCheckBox,
        QMessageBox, QHeaderView, QMenu, QAction, QSplitter,
        QTreeWidget, QTreeWidgetItem, QTextEdit, QFileDialog
    )
    from PyQt5.QtCore import Qt, pyqtSignal
    from PyQt5.QtGui import QColor, QBrush
    HAS_QT = True
except ImportError:
    HAS_QT = False
    # Stub classes for non-GUI environments
    class QWidget:
        pass
    class QDialog:
        pass
    pyqtSignal = lambda: None


@dataclass
class TopicConfig:
    """Topic configuration data structure"""
    name: str = ""
    type_name: str = ""
    reliability: str = "BEST_EFFORT"
    durability: str = "VOLATILE"
    history_kind: str = "KEEP_LAST"
    history_depth: int = 1
    deadline_sec: int = 0
    deadline_nsec: int = 0
    partitions: List[str] = field(default_factory=list)
    description: str = ""


class TopicEditorDialog(QDialog):
    """Dialog for editing topic configuration"""
    
    topic_saved = pyqtSignal(object)  # Emits TopicConfig
    
    RELIABILITY_OPTIONS = ['BEST_EFFORT', 'RELIABLE']
    DURABILITY_OPTIONS = ['VOLATILE', 'TRANSIENT_LOCAL', 'TRANSIENT', 'PERSISTENT']
    HISTORY_OPTIONS = ['KEEP_LAST', 'KEEP_ALL']
    
    def __init__(self, parent=None, topic: Optional[TopicConfig] = None):
        if not HAS_QT:
            raise RuntimeError("PyQt5 is required for GUI components")
        
        super().__init__(parent)
        self.topic = topic or TopicConfig()
        self.setWindowTitle("Edit Topic Configuration")
        self.setMinimumWidth(500)
        self.setup_ui()
        self.load_topic_data()
    
    def setup_ui(self):
        """Initialize UI components"""
        layout = QVBoxLayout()
        
        # Basic info group
        basic_group = QGroupBox("Basic Information")
        basic_layout = QFormLayout()
        
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("Topic name (e.g., VehicleState)")
        basic_layout.addRow("Topic Name:", self.name_edit)
        
        self.type_edit = QLineEdit()
        self.type_edit.setPlaceholderText("Data type (e.g., VehicleStateData)")
        basic_layout.addRow("Data Type:", self.type_edit)
        
        self.desc_edit = QTextEdit()
        self.desc_edit.setMaximumHeight(60)
        self.desc_edit.setPlaceholderText("Optional description")
        basic_layout.addRow("Description:", self.desc_edit)
        
        basic_group.setLayout(basic_layout)
        layout.addWidget(basic_group)
        
        # QoS group
        qos_group = QGroupBox("QoS Settings")
        qos_layout = QFormLayout()
        
        self.reliability_combo = QComboBox()
        self.reliability_combo.addItems(self.RELIABILITY_OPTIONS)
        self.reliability_combo.setToolTip(
            "RELIABLE: Guaranteed delivery\n"
            "BEST_EFFORT: May drop samples under load"
        )
        qos_layout.addRow("Reliability:", self.reliability_combo)
        
        self.durability_combo = QComboBox()
        self.durability_combo.addItems(self.DURABILITY_OPTIONS)
        self.durability_combo.setToolTip(
            "VOLATILE: No persistence\n"
            "TRANSIENT_LOCAL: Late-joiners receive last value\n"
            "TRANSIENT/PERSISTENT: Persistent storage"
        )
        qos_layout.addRow("Durability:", self.durability_combo)
        
        history_layout = QHBoxLayout()
        self.history_combo = QComboBox()
        self.history_combo.addItems(self.HISTORY_OPTIONS)
        history_layout.addWidget(self.history_combo)
        
        self.history_depth_spin = QSpinBox()
        self.history_depth_spin.setRange(1, 1000)
        self.history_depth_spin.setValue(1)
        history_layout.addWidget(QLabel("Depth:"))
        history_layout.addWidget(self.history_depth_spin)
        history_layout.addStretch()
        
        qos_layout.addRow("History:", history_layout)
        
        # Deadline
        deadline_layout = QHBoxLayout()
        self.deadline_sec_spin = QSpinBox()
        self.deadline_sec_spin.setRange(0, 3600)
        self.deadline_nsec_spin = QSpinBox()
        self.deadline_nsec_spin.setRange(0, 999999999)
        self.deadline_nsec_spin.setSingleStep(1000000)
        
        deadline_layout.addWidget(self.deadline_sec_spin)
        deadline_layout.addWidget(QLabel("sec"))
        deadline_layout.addWidget(self.deadline_nsec_spin)
        deadline_layout.addWidget(QLabel("nsec"))
        deadline_layout.addStretch()
        
        qos_layout.addRow("Deadline:", deadline_layout)
        
        qos_group.setLayout(qos_layout)
        layout.addWidget(qos_group)
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        self.save_btn = QPushButton("Save")
        self.save_btn.setDefault(True)
        self.save_btn.clicked.connect(self.save_topic)
        button_layout.addWidget(self.save_btn)
        
        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(self.cancel_btn)
        
        layout.addLayout(button_layout)
        self.setLayout(layout)
    
    def load_topic_data(self):
        """Load existing topic data into form"""
        self.name_edit.setText(self.topic.name)
        self.type_edit.setText(self.topic.type_name)
        self.desc_edit.setPlainText(self.topic.description)
        
        idx = self.reliability_combo.findText(self.topic.reliability)
        if idx >= 0:
            self.reliability_combo.setCurrentIndex(idx)
        
        idx = self.durability_combo.findText(self.topic.durability)
        if idx >= 0:
            self.durability_combo.setCurrentIndex(idx)
        
        idx = self.history_combo.findText(self.topic.history_kind)
        if idx >= 0:
            self.history_combo.setCurrentIndex(idx)
        
        self.history_depth_spin.setValue(self.topic.history_depth)
        self.deadline_sec_spin.setValue(self.topic.deadline_sec)
        self.deadline_nsec_spin.setValue(self.topic.deadline_nsec)
    
    def save_topic(self):
        """Save topic configuration"""
        name = self.name_edit.text().strip()
        if not name:
            QMessageBox.warning(self, "Validation Error", "Topic name is required")
            return
        
        self.topic.name = name
        self.topic.type_name = self.type_edit.text().strip()
        self.topic.description = self.desc_edit.toPlainText()
        self.topic.reliability = self.reliability_combo.currentText()
        self.topic.durability = self.durability_combo.currentText()
        self.topic.history_kind = self.history_combo.currentText()
        self.topic.history_depth = self.history_depth_spin.value()
        self.topic.deadline_sec = self.deadline_sec_spin.value()
        self.topic.deadline_nsec = self.deadline_nsec_spin.value()
        
        self.topic_saved.emit(self.topic)
        self.accept()


class TopicEditor(QWidget):
    """Main topic editor widget"""
    
    topic_added = pyqtSignal(object)    # Emits TopicConfig
    topic_edited = pyqtSignal(object)   # Emits TopicConfig
    topic_removed = pyqtSignal(str)     # Emits topic name
    
    def __init__(self, parent=None):
        if not HAS_QT:
            raise RuntimeError("PyQt5 is required for GUI components")
        
        super().__init__(parent)
        self.topics: Dict[str, TopicConfig] = {}
        self.setup_ui()
    
    def setup_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        # Toolbar
        toolbar = QHBoxLayout()
        
        self.add_btn = QPushButton("Add Topic")
        self.add_btn.setToolTip("Add a new topic")
        self.add_btn.clicked.connect(self.add_topic)
        toolbar.addWidget(self.add_btn)
        
        self.edit_btn = QPushButton("Edit")
        self.edit_btn.setEnabled(False)
        self.edit_btn.clicked.connect(self.edit_selected_topic)
        toolbar.addWidget(self.edit_btn)
        
        self.remove_btn = QPushButton("Remove")
        self.remove_btn.setEnabled(False)
        self.remove_btn.clicked.connect(self.remove_selected_topic)
        toolbar.addWidget(self.remove_btn)
        
        toolbar.addStretch()
        
        self.import_btn = QPushButton("Import...")
        self.import_btn.clicked.connect(self.import_topics)
        toolbar.addWidget(self.import_btn)
        
        self.export_btn = QPushButton("Export...")
        self.export_btn.clicked.connect(self.export_topics)
        toolbar.addWidget(self.export_btn)
        
        layout.addLayout(toolbar)
        
        # Topics table
        self.table = QTableWidget()
        self.table.setColumnCount(5)
        self.table.setHorizontalHeaderLabels([
            "Topic Name", "Type", "Reliability", "History", "Description"
        ])
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.Stretch)
        self.table.setSelectionBehavior(QTableWidget.SelectRows)
        self.table.setSelectionMode(QTableWidget.SingleSelection)
        self.table.setContextMenuPolicy(Qt.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.show_context_menu)
        self.table.itemSelectionChanged.connect(self.on_selection_changed)
        self.table.doubleClicked.connect(self.edit_selected_topic)
        
        layout.addWidget(self.table)
        
        # Status label
        self.status_label = QLabel("No topics configured")
        layout.addWidget(self.status_label)
        
        self.setLayout(layout)
    
    def add_topic(self):
        """Open dialog to add new topic"""
        dialog = TopicEditorDialog(self)
        dialog.topic_saved.connect(self._on_topic_saved)
        dialog.exec_()
    
    def _on_topic_saved(self, topic: TopicConfig):
        """Handle topic saved from dialog"""
        is_new = topic.name not in self.topics
        self.topics[topic.name] = topic
        self.refresh_table()
        
        if is_new:
            self.topic_added.emit(topic)
        else:
            self.topic_edited.emit(topic)
    
    def edit_selected_topic(self):
        """Edit selected topic"""
        row = self.table.currentRow()
        if row < 0:
            return
        
        topic_name = self.table.item(row, 0).text()
        topic = self.topics.get(topic_name)
        if topic:
            dialog = TopicEditorDialog(self, topic)
            dialog.topic_saved.connect(self._on_topic_saved)
            dialog.exec_()
    
    def remove_selected_topic(self):
        """Remove selected topic"""
        row = self.table.currentRow()
        if row < 0:
            return
        
        topic_name = self.table.item(row, 0).text()
        
        reply = QMessageBox.question(
            self, "Confirm Removal",
            f"Remove topic '{topic_name}'?",
            QMessageBox.Yes | QMessageBox.No
        )
        
        if reply == QMessageBox.Yes:
            del self.topics[topic_name]
            self.refresh_table()
            self.topic_removed.emit(topic_name)
    
    def refresh_table(self):
        """Refresh topics table"""
        self.table.setRowCount(len(self.topics))
        
        for row, (name, topic) in enumerate(sorted(self.topics.items())):
            self.table.setItem(row, 0, QTableWidgetItem(topic.name))
            self.table.setItem(row, 1, QTableWidgetItem(topic.type_name))
            self.table.setItem(row, 2, QTableWidgetItem(topic.reliability))
            
            history_text = f"{topic.history_kind} ({topic.history_depth})"
            self.table.setItem(row, 3, QTableWidgetItem(history_text))
            
            desc = topic.description[:50] + "..." if len(topic.description) > 50 else topic.description
            self.table.setItem(row, 4, QTableWidgetItem(desc))
            
            # Color-code by reliability
            if topic.reliability == 'RELIABLE':
                self.table.item(row, 2).setBackground(QBrush(QColor(200, 255, 200)))
        
        self.update_status()
    
    def update_status(self):
        """Update status label"""
        count = len(self.topics)
        if count == 0:
            self.status_label.setText("No topics configured")
        elif count == 1:
            self.status_label.setText("1 topic configured")
        else:
            self.status_label.setText(f"{count} topics configured")
    
    def on_selection_changed(self):
        """Handle table selection change"""
        has_selection = self.table.currentRow() >= 0
        self.edit_btn.setEnabled(has_selection)
        self.remove_btn.setEnabled(has_selection)
    
    def show_context_menu(self, position):
        """Show context menu"""
        menu = QMenu()
        
        add_action = QAction("Add Topic", self)
        add_action.triggered.connect(self.add_topic)
        menu.addAction(add_action)
        
        if self.table.currentRow() >= 0:
            menu.addSeparator()
            
            edit_action = QAction("Edit", self)
            edit_action.triggered.connect(self.edit_selected_topic)
            menu.addAction(edit_action)
            
            remove_action = QAction("Remove", self)
            remove_action.triggered.connect(self.remove_selected_topic)
            menu.addAction(remove_action)
        
        menu.exec_(self.table.viewport().mapToGlobal(position))
    
    def import_topics(self):
        """Import topics from file"""
        filename, _ = QFileDialog.getOpenFileName(
            self, "Import Topics", "",
            "YAML Files (*.yaml *.yml);;JSON Files (*.json);;All Files (*)"
        )
        
        if not filename:
            return
        
        try:
            import yaml
            with open(filename, 'r') as f:
                data = yaml.safe_load(f)
            
            # Parse topics from data
            topics_data = data.get('topics', [])
            for topic_data in topics_data:
                topic = TopicConfig(
                    name=topic_data.get('name', ''),
                    type_name=topic_data.get('type', ''),
                    reliability=topic_data.get('qos', {}).get('reliability', 'BEST_EFFORT'),
                    durability=topic_data.get('qos', {}).get('durability', 'VOLATILE'),
                    history_kind=topic_data.get('qos', {}).get('history', {}).get('kind', 'KEEP_LAST'),
                    history_depth=topic_data.get('qos', {}).get('history', {}).get('depth', 1)
                )
                self.topics[topic.name] = topic
            
            self.refresh_table()
            QMessageBox.information(self, "Import Complete", f"Imported {len(topics_data)} topics")
        except Exception as e:
            QMessageBox.critical(self, "Import Error", str(e))
    
    def export_topics(self):
        """Export topics to file"""
        filename, _ = QFileDialog.getSaveFileName(
            self, "Export Topics", "topics.yaml",
            "YAML Files (*.yaml *.yml);;JSON Files (*.json)"
        )
        
        if not filename:
            return
        
        try:
            import yaml
            topics_data = []
            for topic in self.topics.values():
                topic_data = {
                    'name': topic.name,
                    'type': topic.type_name,
                    'qos': {
                        'reliability': topic.reliability,
                        'durability': topic.durability,
                        'history': {
                            'kind': topic.history_kind,
                            'depth': topic.history_depth
                        }
                    }
                }
                if topic.deadline_sec > 0 or topic.deadline_nsec > 0:
                    topic_data['qos']['deadline'] = {
                        'sec': topic.deadline_sec,
                        'nanosec': topic.deadline_nsec
                    }
                topics_data.append(topic_data)
            
            with open(filename, 'w') as f:
                yaml.dump({'topics': topics_data}, f, default_flow_style=False)
            
            QMessageBox.information(self, "Export Complete", f"Exported {len(topics_data)} topics")
        except Exception as e:
            QMessageBox.critical(self, "Export Error", str(e))
    
    def get_topics(self) -> Dict[str, TopicConfig]:
        """Get all topics"""
        return self.topics.copy()
    
    def set_topics(self, topics: Dict[str, TopicConfig]):
        """Set topics from external source"""
        self.topics = dict(topics)
        self.refresh_table()


class TopicVisualizer:
    """
    Graphical visualization of topic connections between participants
    
    This is a placeholder for a more advanced visualization component
    that could use graphviz or a canvas to show pub/sub relationships.
    """
    
    def __init__(self):
        self.topics: List[TopicConfig] = []
        self.publishers: Dict[str, List[str]] = {}  # topic -> participants
        self.subscribers: Dict[str, List[str]] = {}  # topic -> participants
    
    def add_topic(self, topic: TopicConfig):
        """Add topic to visualization"""
        self.topics.append(topic)
    
    def add_publisher(self, topic_name: str, participant_name: str):
        """Add publisher relationship"""
        if topic_name not in self.publishers:
            self.publishers[topic_name] = []
        self.publishers[topic_name].append(participant_name)
    
    def add_subscriber(self, topic_name: str, participant_name: str):
        """Add subscriber relationship"""
        if topic_name not in self.subscribers:
            self.subscribers[topic_name] = []
        self.subscribers[topic_name].append(participant_name)
    
    def generate_dot_graph(self) -> str:
        """Generate Graphviz DOT representation"""
        lines = ['d DDS_Topics {', '  rankdir=LR;', '  node [shape=box];', '']
        
        # Add topic nodes
        for topic in self.topics:
            lines.append(f'  "{topic.name}" [shape=ellipse, style=filled, fillcolor=lightblue];')
        
        lines.append('')
        
        # Add edges from publishers to topics
        for topic_name, participants in self.publishers.items():
            for participant in participants:
                lines.append(f'  "{participant}" -> "{topic_name}" [color=blue];')
        
        # Add edges from topics to subscribers
        for topic_name, participants in self.subscribers.items():
            for participant in participants:
                lines.append(f'  "{topic_name}" -> "{participant}" [color=green, style=dashed];')
        
        lines.append('}')
        return '\n'.join(lines)


# Example usage
if __name__ == '__main__':
    if not HAS_QT:
        print("PyQt5 is required to run the GUI demo")
        sys.exit(1)
    
    from PyQt5.QtWidgets import QApplication
    
    app = QApplication(sys.argv)
    
    # Create and show topic editor
    editor = TopicEditor()
    editor.setWindowTitle("DDS Topic Editor")
    
    # Add some sample topics
    sample_topics = {
        'VehicleState': TopicConfig(
            name='VehicleState',
            type_name='VehicleStateData',
            reliability='RELIABLE',
            description='Current vehicle state information'
        ),
        'SensorData': TopicConfig(
            name='SensorData',
            type_name='SensorArray',
            reliability='BEST_EFFORT',
            history_kind='KEEP_LAST',
            history_depth=5,
            description='Raw sensor readings'
        )
    }
    editor.set_topics(sample_topics)
    
    editor.show()
    sys.exit(app.exec_())
