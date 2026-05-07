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


class CanTpConfigWidget(ModuleConfigWidget):
    """CAN Transport Protocol (CanTp) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("CanTp", parent)
        self.add_cantp_specific()
    
    def add_cantp_specific(self):
        layout = self.layout()
        
        # Timing Parameters Group
        timing_group = QGroupBox("Timing Parameters (ms) - ISO 15765-2")
        timing_layout = QFormLayout()
        
        # Sender Timeouts
        self.n_as_spin = QSpinBox()
        self.n_as_spin.setRange(1, 10000)
        self.n_as_spin.setValue(1000)
        self.n_as_spin.setSuffix(" ms")
        timing_layout.addRow("N_As (Tx timeout):", self.n_as_spin)
        
        self.n_bs_spin = QSpinBox()
        self.n_bs_spin.setRange(1, 10000)
        self.n_bs_spin.setValue(1000)
        self.n_bs_spin.setSuffix(" ms")
        timing_layout.addRow("N_Bs (FC timeout):", self.n_bs_spin)
        
        # Receiver Timeouts
        self.n_ar_spin = QSpinBox()
        self.n_ar_spin.setRange(1, 10000)
        self.n_ar_spin.setValue(1000)
        self.n_ar_spin.setSuffix(" ms")
        timing_layout.addRow("N_Ar (Rx Tx timeout):", self.n_ar_spin)
        
        self.n_br_spin = QSpinBox()
        self.n_br_spin.setRange(1, 10000)
        self.n_br_spin.setValue(1000)
        self.n_br_spin.setSuffix(" ms")
        timing_layout.addRow("N_Br (Buffer ready):", self.n_br_spin)
        
        self.n_cr_spin = QSpinBox()
        self.n_cr_spin.setRange(1, 10000)
        self.n_cr_spin.setValue(1000)
        self.n_cr_spin.setSuffix(" ms")
        timing_layout.addRow("N_Cr (CF timeout):", self.n_cr_spin)
        
        timing_group.setLayout(timing_layout)
        layout.addRow(timing_group)
        
        # Performance Parameters Group
        perf_group = QGroupBox("Performance Parameters")
        perf_layout = QFormLayout()
        
        self.stmin_spin = QSpinBox()
        self.stmin_spin.setRange(0, 127)
        self.stmin_spin.setValue(20)
        self.stmin_spin.setSuffix(" ms")
        perf_layout.addRow("STmin (Separation Time):", self.stmin_spin)
        
        self.bs_spin = QSpinBox()
        self.bs_spin.setRange(0, 15)
        self.bs_spin.setValue(8)
        perf_layout.addRow("BS (Block Size):", self.bs_spin)
        
        self.wftmax_spin = QSpinBox()
        self.wftmax_spin.setRange(0, 15)
        self.wftmax_spin.setValue(4)
        perf_layout.addRow("WFTmax (Wait Frames):", self.wftmax_spin)
        
        perf_group.setLayout(perf_layout)
        layout.addRow(perf_group)
        
        # Addressing Group
        addr_group = QGroupBox("Addressing Configuration")
        addr_layout = QFormLayout()
        
        self.addr_combo = QComboBox()
        self.addr_combo.addItems(["STANDARD", "EXTENDED", "MIXED"])
        addr_layout.addRow("Addressing Format:", self.addr_combo)
        
        self.padding_cb = QCheckBox("Enable Padding")
        self.padding_cb.setChecked(True)
        addr_layout.addRow("Padding:", self.padding_cb)
        
        addr_group.setLayout(addr_layout)
        layout.addRow(addr_group)
        
        # Channel Configuration
        self.channel_spin = QSpinBox()
        self.channel_spin.setRange(1, 8)
        self.channel_spin.setValue(2)
        layout.addRow("Number of Channels:", self.channel_spin)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "N_As": self.n_as_spin.value(),
            "N_Bs": self.n_bs_spin.value(),
            "N_Ar": self.n_ar_spin.value(),
            "N_Br": self.n_br_spin.value(),
            "N_Cr": self.n_cr_spin.value(),
            "STmin": self.stmin_spin.value(),
            "BS": self.bs_spin.value(),
            "WFTmax": self.wftmax_spin.value(),
            "AddressingFormat": self.addr_combo.currentText(),
            "PaddingActivation": self.padding_cb.isChecked(),
            "ChannelCount": self.channel_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.n_as_spin.setValue(config.get("N_As", 1000))
        self.n_bs_spin.setValue(config.get("N_Bs", 1000))
        self.n_ar_spin.setValue(config.get("N_Ar", 1000))
        self.n_br_spin.setValue(config.get("N_Br", 1000))
        self.n_cr_spin.setValue(config.get("N_Cr", 1000))
        self.stmin_spin.setValue(config.get("STmin", 20))
        self.bs_spin.setValue(config.get("BS", 8))
        self.wftmax_spin.setValue(config.get("WFTmax", 4))
        addr_format = config.get("AddressingFormat", "STANDARD")
        index = self.addr_combo.findText(addr_format)
        if index >= 0:
            self.addr_combo.setCurrentIndex(index)
        self.padding_cb.setChecked(config.get("PaddingActivation", True))
        self.channel_spin.setValue(config.get("ChannelCount", 2))


class PduRConfigWidget(ModuleConfigWidget):
    """PDU Router (PduR) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("PduR", parent)
        self.add_pdur_specific()
    
    def add_pdur_specific(self):
        layout = self.layout()
        
        # General Settings
        general_group = QGroupBox("General Settings")
        general_layout = QFormLayout()
        
        self.zero_cost_cb = QCheckBox("Enable Zero-Cost Operation")
        self.zero_cost_cb.setChecked(False)
        general_layout.addRow("Zero-Cost:", self.zero_cost_cb)
        
        self.lo_tp_cb = QCheckBox("Enable LoTp (Large Data TP)")
        self.lo_tp_cb.setChecked(True)
        general_layout.addRow("LoTp:", self.lo_tp_cb)
        
        general_group.setLayout(general_layout)
        layout.addRow(general_group)
        
        # Routing Tables
        routing_group = QGroupBox("Routing Table Configuration")
        routing_layout = QFormLayout()
        
        self.src_routing_spin = QSpinBox()
        self.src_routing_spin.setRange(1, 256)
        self.src_routing_spin.setValue(16)
        routing_layout.addRow("Max Source Routing Paths:", self.src_routing_spin)
        
        self.dest_routing_spin = QSpinBox()
        self.dest_routing_spin.setRange(1, 256)
        self.dest_routing_spin.setValue(32)
        routing_layout.addRow("Max Destination Routing Paths:", self.dest_routing_spin)
        
        routing_group.setLayout(routing_layout)
        layout.addRow(routing_group)
        
        # Buffer Configuration
        buffer_group = QGroupBox("Buffer Configuration")
        buffer_layout = QFormLayout()
        
        self.tp_buffer_size_spin = QSpinBox()
        self.tp_buffer_size_spin.setRange(8, 4096)
        self.tp_buffer_size_spin.setValue(64)
        self.tp_buffer_size_spin.setSingleStep(8)
        buffer_layout.addRow("Tp Buffer Size (bytes):", self.tp_buffer_size_spin)
        
        self.tp_buffer_count_spin = QSpinBox()
        self.tp_buffer_count_spin.setRange(1, 32)
        self.tp_buffer_count_spin.setValue(4)
        buffer_layout.addRow("Tp Buffer Count:", self.tp_buffer_count_spin)
        
        buffer_group.setLayout(buffer_layout)
        layout.addRow(buffer_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "ZeroCostOperation": self.zero_cost_cb.isChecked(),
            "LoTpEnabled": self.lo_tp_cb.isChecked(),
            "MaxSourceRoutingPaths": self.src_routing_spin.value(),
            "MaxDestinationRoutingPaths": self.dest_routing_spin.value(),
            "TpBufferSize": self.tp_buffer_size_spin.value(),
            "TpBufferCount": self.tp_buffer_count_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.zero_cost_cb.setChecked(config.get("ZeroCostOperation", False))
        self.lo_tp_cb.setChecked(config.get("LoTpEnabled", True))
        self.src_routing_spin.setValue(config.get("MaxSourceRoutingPaths", 16))
        self.dest_routing_spin.setValue(config.get("MaxDestinationRoutingPaths", 32))
        self.tp_buffer_size_spin.setValue(config.get("TpBufferSize", 64))
        self.tp_buffer_count_spin.setValue(config.get("TpBufferCount", 4))


class CanIfConfigWidget(ModuleConfigWidget):
    """CAN Interface (CanIf) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("CanIf", parent)
        self.add_canif_specific()
    
    def add_canif_specific(self):
        layout = self.layout()
        
        # Controller Configuration
        ctrl_group = QGroupBox("Controller Configuration")
        ctrl_layout = QFormLayout()
        
        self.controller_count_spin = QSpinBox()
        self.controller_count_spin.setRange(1, 8)
        self.controller_count_spin.setValue(2)
        ctrl_layout.addRow("Controller Count:", self.controller_count_spin)
        
        self.transceiver_count_spin = QSpinBox()
        self.transceiver_count_spin.setRange(0, 8)
        self.transceiver_count_spin.setValue(2)
        ctrl_layout.addRow("Transceiver Count:", self.transceiver_count_spin)
        
        ctrl_group.setLayout(ctrl_layout)
        layout.addRow(ctrl_group)
        
        # HOH Configuration
        hoh_group = QGroupBox("Hardware Object Handle (HOH)")
        hoh_layout = QFormLayout()
        
        self.hrh_count_spin = QSpinBox()
        self.hrh_count_spin.setRange(1, 64)
        self.hrh_count_spin.setValue(8)
        hoh_layout.addRow("HRH Count (Rx):", self.hrh_count_spin)
        
        self.hth_count_spin = QSpinBox()
        self.hth_count_spin.setRange(1, 64)
        self.hth_count_spin.setValue(8)
        hoh_layout.addRow("HTH Count (Tx):", self.hth_count_spin)
        
        hoh_group.setLayout(hoh_layout)
        layout.addRow(hoh_group)
        
        # Advanced Settings
        advanced_group = QGroupBox("Advanced Settings")
        advanced_layout = QFormLayout()
        
        self.multiplex_tx_cb = QCheckBox("Enable Multiplexed Transmission")
        self.multiplex_tx_cb.setChecked(False)
        advanced_layout.addRow("Multiplexed Tx:", self.multiplex_tx_cb)
        
        self.readtxpdu_cb = QCheckBox("Enable Read Tx PDU Notify Status")
        self.readtxpdu_cb.setChecked(False)
        advanced_layout.addRow("ReadTxPduNotify:", self.readtxpdu_cb)
        
        advanced_group.setLayout(advanced_layout)
        layout.addRow(advanced_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "ControllerCount": self.controller_count_spin.value(),
            "TransceiverCount": self.transceiver_count_spin.value(),
            "HRHCount": self.hrh_count_spin.value(),
            "HTHCount": self.hth_count_spin.value(),
            "MultiplexedTransmission": self.multiplex_tx_cb.isChecked(),
            "ReadTxPduNotifyStatus": self.readtxpdu_cb.isChecked()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.controller_count_spin.setValue(config.get("ControllerCount", 2))
        self.transceiver_count_spin.setValue(config.get("TransceiverCount", 2))
        self.hrh_count_spin.setValue(config.get("HRHCount", 8))
        self.hth_count_spin.setValue(config.get("HTHCount", 8))
        self.multiplex_tx_cb.setChecked(config.get("MultiplexedTransmission", False))
        self.readtxpdu_cb.setChecked(config.get("ReadTxPduNotifyStatus", False))


class ComConfigWidget(ModuleConfigWidget):
    """Communication (Com) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("Com", parent)
        self.add_com_specific()
    
    def add_com_specific(self):
        layout = self.layout()
        
        # Signal Configuration
        signal_group = QGroupBox("Signal Configuration")
        signal_layout = QFormLayout()
        
        self.signal_count_spin = QSpinBox()
        self.signal_count_spin.setRange(1, 512)
        self.signal_count_spin.setValue(64)
        signal_layout.addRow("Signal Count:", self.signal_count_spin)
        
        self.group_signal_count_spin = QSpinBox()
        self.group_signal_count_spin.setRange(0, 64)
        self.group_signal_count_spin.setValue(8)
        signal_layout.addRow("Group Signal Count:", self.group_signal_count_spin)
        
        signal_group.setLayout(signal_layout)
        layout.addRow(signal_group)
        
        # IPDU Configuration
        ipdu_group = QGroupBox("IPDU Configuration")
        ipdu_layout = QFormLayout()
        
        self.ipdu_count_spin = QSpinBox()
        self.ipdu_count_spin.setRange(1, 256)
        self.ipdu_count_spin.setValue(32)
        ipdu_layout.addRow("IPDU Count:", self.ipdu_count_spin)
        
        self.ipdu_group_count_spin = QSpinBox()
        self.ipdu_group_count_spin.setRange(0, 32)
        self.ipdu_group_count_spin.setValue(4)
        ipdu_layout.addRow("IPDU Group Count:", self.ipdu_group_count_spin)
        
        ipdu_group.setLayout(ipdu_layout)
        layout.addRow(ipdu_group)
        
        # Timing Configuration
        timing_group = QGroupBox("Timing Configuration")
        timing_layout = QFormLayout()
        
        self.time_base_spin = QSpinBox()
        self.time_base_spin.setRange(1, 100)
        self.time_base_spin.setValue(10)
        self.time_base_spin.setSuffix(" ms")
        timing_layout.addRow("Time Base:", self.time_base_spin)
        
        timing_group.setLayout(timing_layout)
        layout.addRow(timing_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "SignalCount": self.signal_count_spin.value(),
            "GroupSignalCount": self.group_signal_count_spin.value(),
            "IPDUCount": self.ipdu_count_spin.value(),
            "IPDUGroupCount": self.ipdu_group_count_spin.value(),
            "TimeBase": self.time_base_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.signal_count_spin.setValue(config.get("SignalCount", 64))
        self.group_signal_count_spin.setValue(config.get("GroupSignalCount", 8))
        self.ipdu_count_spin.setValue(config.get("IPDUCount", 32))
        self.ipdu_group_count_spin.setValue(config.get("IPDUGroupCount", 4))
        self.time_base_spin.setValue(config.get("TimeBase", 10))


class DcmConfigWidget(ModuleConfigWidget):
    """Diagnostic Communication Manager (Dcm) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("Dcm", parent)
        self.add_dcm_specific()
    
    def add_dcm_specific(self):
        layout = self.layout()
        
        # Diagnostic Services
        services_group = QGroupBox("Diagnostic Services")
        services_layout = QFormLayout()
        
        self.dsd_service_count_spin = QSpinBox()
        self.dsd_service_count_spin.setRange(1, 64)
        self.dsd_service_count_spin.setValue(16)
        services_layout.addRow("DSD Service Count:", self.dsd_service_count_spin)
        
        self.dsp_did_count_spin = QSpinBox()
        self.dsp_did_count_spin.setRange(0, 256)
        self.dsp_did_count_spin.setValue(32)
        services_layout.addRow("DID Count:", self.dsp_did_count_spin)
        
        self.dsp_rid_count_spin = QSpinBox()
        self.dsp_rid_count_spin.setRange(0, 64)
        self.dsp_rid_count_spin.setValue(8)
        services_layout.addRow("Routine ID Count:", self.dsp_rid_count_spin)
        
        services_group.setLayout(services_layout)
        layout.addRow(services_group)
        
        # Protocol Configuration
        protocol_group = QGroupBox("Protocol Configuration")
        protocol_layout = QFormLayout()
        
        self.protocol_combo = QComboBox()
        self.protocol_combo.addItems(["UDS_ON_CAN", "UDS_ON_FLEXRAY", "UDS_ON_IP", "OBD_ON_CAN"])
        protocol_layout.addRow("Protocol Type:", self.protocol_combo)
        
        self.p2_timeout_spin = QSpinBox()
        self.p2_timeout_spin.setRange(1, 5000)
        self.p2_timeout_spin.setValue(50)
        self.p2_timeout_spin.setSuffix(" ms")
        protocol_layout.addRow("P2 Timeout:", self.p2_timeout_spin)
        
        self.p2_star_spin = QSpinBox()
        self.p2_star_spin.setRange(100, 50000)
        self.p2_star_spin.setValue(5000)
        self.p2_star_spin.setSuffix(" ms")
        protocol_layout.addRow("P2* Timeout:", self.p2_star_spin)
        
        protocol_group.setLayout(protocol_layout)
        layout.addRow(protocol_group)
        
        # Security
        security_group = QGroupBox("Security Configuration")
        security_layout = QFormLayout()
        
        self.security_level_count_spin = QSpinBox()
        self.security_level_count_spin.setRange(0, 16)
        self.security_level_count_spin.setValue(2)
        security_layout.addRow("Security Level Count:", self.security_level_count_spin)
        
        self.session_count_spin = QSpinBox()
        self.session_count_spin.setRange(1, 16)
        self.session_count_spin.setValue(3)
        security_layout.addRow("Session Count:", self.session_count_spin)
        
        security_group.setLayout(security_layout)
        layout.addRow(security_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "DSDServiceCount": self.dsd_service_count_spin.value(),
            "DIDCount": self.dsp_did_count_spin.value(),
            "RIDCount": self.dsp_rid_count_spin.value(),
            "ProtocolType": self.protocol_combo.currentText(),
            "P2Timeout": self.p2_timeout_spin.value(),
            "P2StarTimeout": self.p2_star_spin.value(),
            "SecurityLevelCount": self.security_level_count_spin.value(),
            "SessionCount": self.session_count_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.dsd_service_count_spin.setValue(config.get("DSDServiceCount", 16))
        self.dsp_did_count_spin.setValue(config.get("DIDCount", 32))
        self.dsp_rid_count_spin.setValue(config.get("RIDCount", 8))
        protocol = config.get("ProtocolType", "UDS_ON_CAN")
        index = self.protocol_combo.findText(protocol)
        if index >= 0:
            self.protocol_combo.setCurrentIndex(index)
        self.p2_timeout_spin.setValue(config.get("P2Timeout", 50))
        self.p2_star_spin.setValue(config.get("P2StarTimeout", 5000))
        self.security_level_count_spin.setValue(config.get("SecurityLevelCount", 2))
        self.session_count_spin.setValue(config.get("SessionCount", 3))


class DemConfigWidget(ModuleConfigWidget):
    """Diagnostic Event Manager (Dem) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("Dem", parent)
        self.add_dem_specific()
    
    def add_dem_specific(self):
        layout = self.layout()
        
        # Event Configuration
        event_group = QGroupBox("Event Configuration")
        event_layout = QFormLayout()
        
        self.event_count_spin = QSpinBox()
        self.event_count_spin.setRange(1, 512)
        self.event_count_spin.setValue(64)
        event_layout.addRow("Event Count:", self.event_count_spin)
        
        self.event_dest_count_spin = QSpinBox()
        self.event_dest_count_spin.setRange(1, 8)
        self.event_dest_count_spin.setValue(2)
        event_layout.addRow("Event Destination Count:", self.event_dest_count_spin)
        
        event_group.setLayout(event_layout)
        layout.addRow(event_group)
        
        # Debounce Configuration
        debounce_group = QGroupBox("Debounce Configuration")
        debounce_layout = QFormLayout()
        
        self.debounce_algo_combo = QComboBox()
        self.debounce_algo_combo.addItems(["COUNTER_BASED", "TIME_BASED", "MONITOR_INTERNAL"])
        debounce_layout.addRow("Debounce Algorithm:", self.debounce_algo_combo)
        
        self.debounce_counter_spin = QSpinBox()
        self.debounce_counter_spin.setRange(1, 255)
        self.debounce_counter_spin.setValue(10)
        debounce_layout.addRow("Debounce Counter Threshold:", self.debounce_counter_spin)
        
        self.debounce_time_spin = QSpinBox()
        self.debounce_time_spin.setRange(1, 10000)
        self.debounce_time_spin.setValue(100)
        self.debounce_time_spin.setSuffix(" ms")
        debounce_layout.addRow("Debounce Time Threshold:", self.debounce_time_spin)
        
        debounce_group.setLayout(debounce_layout)
        layout.addRow(debounce_group)
        
        # Memory Configuration
        memory_group = QGroupBox("Memory Configuration")
        memory_layout = QFormLayout()
        
        self.primary_mem_size_spin = QSpinBox()
        self.primary_mem_size_spin.setRange(1, 64)
        self.primary_mem_size_spin.setValue(20)
        memory_layout.addRow("Primary Memory Entries:", self.primary_mem_size_spin)
        
        self.user_mem_size_spin = QSpinBox()
        self.user_mem_size_spin.setRange(0, 32)
        self.user_mem_size_spin.setValue(10)
        memory_layout.addRow("User Memory Entries:", self.user_mem_size_spin)
        
        memory_group.setLayout(memory_layout)
        layout.addRow(memory_group)
        
        # Freeze Frame
        self.freeze_frame_cb = QCheckBox("Enable Freeze Frame Recording")
        self.freeze_frame_cb.setChecked(True)
        layout.addRow("Freeze Frame:", self.freeze_frame_cb)
        
        self.extended_data_cb = QCheckBox("Enable Extended Data Recording")
        self.extended_data_cb.setChecked(True)
        layout.addRow("Extended Data:", self.extended_data_cb)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "EventCount": self.event_count_spin.value(),
            "EventDestinationCount": self.event_dest_count_spin.value(),
            "DebounceAlgorithm": self.debounce_algo_combo.currentText(),
            "DebounceCounterThreshold": self.debounce_counter_spin.value(),
            "DebounceTimeThreshold": self.debounce_time_spin.value(),
            "PrimaryMemorySize": self.primary_mem_size_spin.value(),
            "UserMemorySize": self.user_mem_size_spin.value(),
            "FreezeFrameEnabled": self.freeze_frame_cb.isChecked(),
            "ExtendedDataEnabled": self.extended_data_cb.isChecked()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.event_count_spin.setValue(config.get("EventCount", 64))
        self.event_dest_count_spin.setValue(config.get("EventDestinationCount", 2))
        algo = config.get("DebounceAlgorithm", "COUNTER_BASED")
        index = self.debounce_algo_combo.findText(algo)
        if index >= 0:
            self.debounce_algo_combo.setCurrentIndex(index)
        self.debounce_counter_spin.setValue(config.get("DebounceCounterThreshold", 10))
        self.debounce_time_spin.setValue(config.get("DebounceTimeThreshold", 100))
        self.primary_mem_size_spin.setValue(config.get("PrimaryMemorySize", 20))
        self.user_mem_size_spin.setValue(config.get("UserMemorySize", 10))
        self.freeze_frame_cb.setChecked(config.get("FreezeFrameEnabled", True))
        self.extended_data_cb.setChecked(config.get("ExtendedDataEnabled", True))


class NvMConfigWidget(ModuleConfigWidget):
    """NVRAM Manager (NvM) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("NvM", parent)
        self.add_nvm_specific()
    
    def add_nvm_specific(self):
        layout = self.layout()
        
        # General Settings
        general_group = QGroupBox("General Settings")
        general_layout = QFormLayout()
        
        self.multi_job_cb = QCheckBox("Enable Multi-Job Processing")
        self.multi_job_cb.setChecked(True)
        general_layout.addRow("Multi-Job:", self.multi_job_cb)
        
        self.set_ram_block_cb = QCheckBox("Enable Set RAM Block Status API")
        self.set_ram_block_cb.setChecked(True)
        general_layout.addRow("SetRamBlockStatus API:", self.set_ram_block_cb)
        
        self.calc_crc_cb = QCheckBox("Enable Calc CRC API")
        self.calc_crc_cb.setChecked(True)
        general_layout.addRow("Calc CRC API:", self.calc_crc_cb)
        
        self.comp_crc_cb = QCheckBox("Enable Compare CRC API")
        self.comp_crc_cb.setChecked(True)
        general_layout.addRow("Compare CRC API:", self.comp_crc_cb)
        
        general_group.setLayout(general_layout)
        layout.addRow(general_group)
        
        # Block Configuration
        block_group = QGroupBox("Block Configuration")
        block_layout = QFormLayout()
        
        self.block_count_spin = QSpinBox()
        self.block_count_spin.setRange(1, 256)
        self.block_count_spin.setValue(32)
        block_layout.addRow("Number of Blocks:", self.block_count_spin)
        
        self.dataset_count_spin = QSpinBox()
        self.dataset_count_spin.setRange(0, 32)
        self.dataset_count_spin.setValue(4)
        block_layout.addRow("Max DataSets per Block:", self.dataset_count_spin)
        
        block_group.setLayout(block_layout)
        layout.addRow(block_group)
        
        # Queue Configuration
        queue_group = QGroupBox("Queue Configuration")
        queue_layout = QFormLayout()
        
        self.job_queue_spin = QSpinBox()
        self.job_queue_spin.setRange(1, 64)
        self.job_queue_spin.setValue(16)
        queue_layout.addRow("Job Queue Size:", self.job_queue_spin)
        
        self.immediate_write_spin = QSpinBox()
        self.immediate_write_spin.setRange(0, 32)
        self.immediate_write_spin.setValue(4)
        queue_layout.addRow("Max Immediate Write Jobs:", self.immediate_write_spin)
        
        queue_group.setLayout(queue_layout)
        layout.addRow(queue_group)
        
        # Timing
        timing_group = QGroupBox("Timing Configuration")
        timing_layout = QFormLayout()
        
        self.main_period_spin = QSpinBox()
        self.main_period_spin.setRange(1, 1000)
        self.main_period_spin.setValue(10)
        self.main_period_spin.setSuffix(" ms")
        timing_layout.addRow("Main Function Period:", self.main_period_spin)
        
        timing_group.setLayout(timing_layout)
        layout.addRow(timing_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "MultiJobEnabled": self.multi_job_cb.isChecked(),
            "SetRamBlockStatusApi": self.set_ram_block_cb.isChecked(),
            "CalcCrcApi": self.calc_crc_cb.isChecked(),
            "CompareCrcApi": self.comp_crc_cb.isChecked(),
            "BlockCount": self.block_count_spin.value(),
            "MaxDataSets": self.dataset_count_spin.value(),
            "JobQueueSize": self.job_queue_spin.value(),
            "MaxImmediateWriteJobs": self.immediate_write_spin.value(),
            "MainFunctionPeriod": self.main_period_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.multi_job_cb.setChecked(config.get("MultiJobEnabled", True))
        self.set_ram_block_cb.setChecked(config.get("SetRamBlockStatusApi", True))
        self.calc_crc_cb.setChecked(config.get("CalcCrcApi", True))
        self.comp_crc_cb.setChecked(config.get("CompareCrcApi", True))
        self.block_count_spin.setValue(config.get("BlockCount", 32))
        self.dataset_count_spin.setValue(config.get("MaxDataSets", 4))
        self.job_queue_spin.setValue(config.get("JobQueueSize", 16))
        self.immediate_write_spin.setValue(config.get("MaxImmediateWriteJobs", 4))
        self.main_period_spin.setValue(config.get("MainFunctionPeriod", 10))


class EthIfConfigWidget(ModuleConfigWidget):
    """Ethernet Interface (EthIf) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("EthIf", parent)
        self.add_ethif_specific()
    
    def add_ethif_specific(self):
        layout = self.layout()
        
        # Controller Configuration
        ctrl_group = QGroupBox("Controller Configuration")
        ctrl_layout = QFormLayout()
        
        self.ctrl_count_spin = QSpinBox()
        self.ctrl_count_spin.setRange(1, 8)
        self.ctrl_count_spin.setValue(2)
        ctrl_layout.addRow("Controller Count:", self.ctrl_count_spin)
        
        self.transceiver_count_spin = QSpinBox()
        self.transceiver_count_spin.setRange(0, 8)
        self.transceiver_count_spin.setValue(2)
        ctrl_layout.addRow("Transceiver Count:", self.transceiver_count_spin)
        
        self.switch_count_spin = QSpinBox()
        self.switch_count_spin.setRange(0, 4)
        self.switch_count_spin.setValue(0)
        ctrl_layout.addRow("Switch Count:", self.switch_count_spin)
        
        ctrl_group.setLayout(ctrl_layout)
        layout.addRow(ctrl_group)
        
        # Frame Configuration
        frame_group = QGroupBox("Frame Configuration")
        frame_layout = QFormLayout()
        
        self.frame_count_spin = QSpinBox()
        self.frame_count_spin.setRange(1, 128)
        self.frame_count_spin.setValue(16)
        frame_layout.addRow("Frame Owner Count:", self.frame_count_spin)
        
        self.pdu_count_spin = QSpinBox()
        self.pdu_count_spin.setRange(1, 256)
        self.pdu_count_spin.setValue(32)
        frame_layout.addRow("PDU Count:", self.pdu_count_spin)
        
        frame_group.setLayout(frame_layout)
        layout.addRow(frame_group)
        
        # Buffer Configuration
        buffer_group = QGroupBox("Buffer Configuration")
        buffer_layout = QFormLayout()
        
        self.rx_buf_count_spin = QSpinBox()
        self.rx_buf_count_spin.setRange(1, 64)
        self.rx_buf_count_spin.setValue(8)
        buffer_layout.addRow("Rx Buffer Count:", self.rx_buf_count_spin)
        
        self.rx_buf_size_spin = QSpinBox()
        self.rx_buf_size_spin.setRange(256, 16000)
        self.rx_buf_size_spin.setValue(1536)
        self.rx_buf_size_spin.setSingleStep(256)
        buffer_layout.addRow("Rx Buffer Size (bytes):", self.rx_buf_size_spin)
        
        buffer_group.setLayout(buffer_layout)
        layout.addRow(buffer_group)
        
        # Advanced
        advanced_group = QGroupBox("Advanced Features")
        advanced_layout = QFormLayout()
        
        self.timestamp_cb = QCheckBox("Enable TimeStamp Support")
        self.timestamp_cb.setChecked(True)
        advanced_layout.addRow("TimeStamp:", self.timestamp_cb)
        
        self.mac_sec_cb = QCheckBox("Enable MACsec Support")
        self.mac_sec_cb.setChecked(False)
        advanced_layout.addRow("MACsec:", self.mac_sec_cb)
        
        self.wakeup_cb = QCheckBox("Enable Wake-up Support")
        self.wakeup_cb.setChecked(True)
        advanced_layout.addRow("Wake-up:", self.wakeup_cb)
        
        advanced_group.setLayout(advanced_layout)
        layout.addRow(advanced_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "ControllerCount": self.ctrl_count_spin.value(),
            "TransceiverCount": self.transceiver_count_spin.value(),
            "SwitchCount": self.switch_count_spin.value(),
            "FrameOwnerCount": self.frame_count_spin.value(),
            "PDUCount": self.pdu_count_spin.value(),
            "RxBufferCount": self.rx_buf_count_spin.value(),
            "RxBufferSize": self.rx_buf_size_spin.value(),
            "TimeStampSupport": self.timestamp_cb.isChecked(),
            "MACsecSupport": self.mac_sec_cb.isChecked(),
            "WakeUpSupport": self.wakeup_cb.isChecked()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.ctrl_count_spin.setValue(config.get("ControllerCount", 2))
        self.transceiver_count_spin.setValue(config.get("TransceiverCount", 2))
        self.switch_count_spin.setValue(config.get("SwitchCount", 0))
        self.frame_count_spin.setValue(config.get("FrameOwnerCount", 16))
        self.pdu_count_spin.setValue(config.get("PDUCount", 32))
        self.rx_buf_count_spin.setValue(config.get("RxBufferCount", 8))
        self.rx_buf_size_spin.setValue(config.get("RxBufferSize", 1536))
        self.timestamp_cb.setChecked(config.get("TimeStampSupport", True))
        self.mac_sec_cb.setChecked(config.get("MACsecSupport", False))
        self.wakeup_cb.setChecked(config.get("WakeUpSupport", True))


class FeeConfigWidget(ModuleConfigWidget):
    """Flash EEPROM Emulation (Fee) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("Fee", parent)
        self.add_fee_specific()
    
    def add_fee_specific(self):
        layout = self.layout()
        
        # Block Configuration
        block_group = QGroupBox("Block Configuration")
        block_layout = QFormLayout()
        
        self.block_count_spin = QSpinBox()
        self.block_count_spin.setRange(1, 256)
        self.block_count_spin.setValue(32)
        block_layout.addRow("Number of Blocks:", self.block_count_spin)
        
        self.max_block_size_spin = QSpinBox()
        self.max_block_size_spin.setRange(8, 4096)
        self.max_block_size_spin.setValue(256)
        self.max_block_size_spin.setSingleStep(8)
        block_layout.addRow("Max Block Size (bytes):", self.max_block_size_spin)
        
        block_group.setLayout(block_layout)
        layout.addRow(block_group)
        
        # Virtual Page Configuration
        page_group = QGroupBox("Virtual Page Configuration")
        page_layout = QFormLayout()
        
        self.page_size_spin = QSpinBox()
        self.page_size_spin.setRange(1, 64)
        self.page_size_spin.setValue(8)
        page_layout.addRow("Virtual Page Size (bytes):", self.page_size_spin)
        
        self.page_count_spin = QSpinBox()
        self.page_count_spin.setRange(1, 64)
        self.page_count_spin.setValue(4)
        page_layout.addRow("Pages per Block:", self.page_count_spin)
        
        page_group.setLayout(page_layout)
        layout.addRow(page_group)
        
        # Garbage Collection
        gc_group = QGroupBox("Garbage Collection")
        gc_layout = QFormLayout()
        
        self.gc_threshold_spin = QSpinBox()
        self.gc_threshold_spin.setRange(10, 90)
        self.gc_threshold_spin.setValue(80)
        self.gc_threshold_spin.setSuffix(" %")
        gc_layout.addRow("GC Threshold:", self.gc_threshold_spin)
        
        self.gc_limit_spin = QSpinBox()
        self.gc_limit_spin.setRange(1, 32)
        self.gc_limit_spin.setValue(8)
        gc_layout.addRow("GC Erase Limit:", self.gc_limit_spin)
        
        gc_group.setLayout(gc_layout)
        layout.addRow(gc_group)
        
        # API Configuration
        api_group = QGroupBox("API Configuration")
        api_layout = QFormLayout()
        
        self.quick_erase_cb = QCheckBox("Enable Quick Erase Mode")
        self.quick_erase_cb.setChecked(False)
        api_layout.addRow("Quick Erase:", self.quick_erase_cb)
        
        self.get_cycle_cb = QCheckBox("Enable Get Cycle Count API")
        self.get_cycle_cb.setChecked(False)
        api_layout.addRow("Get Cycle Count:", self.get_cycle_cb)
        
        api_group.setLayout(api_layout)
        layout.addRow(api_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "BlockCount": self.block_count_spin.value(),
            "MaxBlockSize": self.max_block_size_spin.value(),
            "VirtualPageSize": self.page_size_spin.value(),
            "PagesPerBlock": self.page_count_spin.value(),
            "GCThreshold": self.gc_threshold_spin.value(),
            "GCEraseLimit": self.gc_limit_spin.value(),
            "QuickEraseMode": self.quick_erase_cb.isChecked(),
            "GetCycleCountApi": self.get_cycle_cb.isChecked()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.block_count_spin.setValue(config.get("BlockCount", 32))
        self.max_block_size_spin.setValue(config.get("MaxBlockSize", 256))
        self.page_size_spin.setValue(config.get("VirtualPageSize", 8))
        self.page_count_spin.setValue(config.get("PagesPerBlock", 4))
        self.gc_threshold_spin.setValue(config.get("GCThreshold", 80))
        self.gc_limit_spin.setValue(config.get("GCEraseLimit", 8))
        self.quick_erase_cb.setChecked(config.get("QuickEraseMode", False))
        self.get_cycle_cb.setChecked(config.get("GetCycleCountApi", False))


class EaConfigWidget(ModuleConfigWidget):
    """EEPROM Abstraction (Ea) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("Ea", parent)
        self.add_ea_specific()
    
    def add_ea_specific(self):
        layout = self.layout()
        
        # Block Configuration
        block_group = QGroupBox("Block Configuration")
        block_layout = QFormLayout()
        
        self.block_count_spin = QSpinBox()
        self.block_count_spin.setRange(1, 256)
        self.block_count_spin.setValue(32)
        block_layout.addRow("Number of Blocks:", self.block_count_spin)
        
        self.max_block_size_spin = QSpinBox()
        self.max_block_size_spin.setRange(1, 4096)
        self.max_block_size_spin.setValue(256)
        block_layout.addRow("Max Block Size (bytes):", self.max_block_size_spin)
        
        self.page_size_spin = QSpinBox()
        self.page_size_spin.setRange(1, 256)
        self.page_size_spin.setValue(32)
        block_layout.addRow("Page Size (bytes):", self.page_size_spin)
        
        block_group.setLayout(block_layout)
        layout.addRow(block_group)
        
        # Driver Selection
        driver_group = QGroupBox("Driver Configuration")
        driver_layout = QFormLayout()
        
        self.eep_driver_combo = QComboBox()
        self.eep_driver_combo.addItems(["Eep_17_Dio", "Eep_Int", "Eep_Ext"])
        driver_layout.addRow("EEPROM Driver:", self.eep_driver_combo)
        
        self.eep_base_addr_spin = QSpinBox()
        self.eep_base_addr_spin.setRange(0, 0xFFFFFFFF)
        self.eep_base_addr_spin.setDisplayIntegerBase(16)
        self.eep_base_addr_spin.setPrefix("0x")
        self.eep_base_addr_spin.setValue(0x08080000)
        driver_layout.addRow("EEPROM Base Address:", self.eep_base_addr_spin)
        
        driver_group.setLayout(driver_layout)
        layout.addRow(driver_group)
        
        # API Configuration
        api_group = QGroupBox("API Configuration")
        api_layout = QFormLayout()
        
        self.erase_api_cb = QCheckBox("Enable Erase API")
        self.erase_api_cb.setChecked(True)
        api_layout.addRow("Erase API:", self.erase_api_cb)
        
        self.cancel_api_cb = QCheckBox("Enable Cancel API")
        self.cancel_api_cb.setChecked(False)
        api_layout.addRow("Cancel API:", self.cancel_api_cb)
        
        self.status_api_cb = QCheckBox("Enable Status API")
        self.status_api_cb.setChecked(True)
        api_layout.addRow("Status API:", self.status_api_cb)
        
        api_group.setLayout(api_layout)
        layout.addRow(api_group)
        
        # Job Processing
        job_group = QGroupBox("Job Processing")
        job_layout = QFormLayout()
        
        self.job_processing_cb = QComboBox()
        self.job_processing_cb.addItems(["IMMEDIATE", "DEFERRED"])
        job_layout.addRow("Job Processing Mode:", self.job_processing_cb)
        
        job_group.setLayout(job_layout)
        layout.addRow(job_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "BlockCount": self.block_count_spin.value(),
            "MaxBlockSize": self.max_block_size_spin.value(),
            "PageSize": self.page_size_spin.value(),
            "EepDriver": self.eep_driver_combo.currentText(),
            "EepBaseAddress": self.eep_base_addr_spin.value(),
            "EraseApi": self.erase_api_cb.isChecked(),
            "CancelApi": self.cancel_api_cb.isChecked(),
            "StatusApi": self.status_api_cb.isChecked(),
            "JobProcessingMode": self.job_processing_cb.currentText()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.block_count_spin.setValue(config.get("BlockCount", 32))
        self.max_block_size_spin.setValue(config.get("MaxBlockSize", 256))
        self.page_size_spin.setValue(config.get("PageSize", 32))
        driver = config.get("EepDriver", "Eep_17_Dio")
        index = self.eep_driver_combo.findText(driver)
        if index >= 0:
            self.eep_driver_combo.setCurrentIndex(index)
        self.eep_base_addr_spin.setValue(config.get("EepBaseAddress", 0x08080000))
        self.erase_api_cb.setChecked(config.get("EraseApi", True))
        self.cancel_api_cb.setChecked(config.get("CancelApi", False))
        self.status_api_cb.setChecked(config.get("StatusApi", True))
        mode = config.get("JobProcessingMode", "IMMEDIATE")
        index = self.job_processing_cb.findText(mode)
        if index >= 0:
            self.job_processing_cb.setCurrentIndex(index)


class IoHwAbConfigWidget(ModuleConfigWidget):
    """I/O Hardware Abstraction (IoHwAb) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("IoHwAb", parent)
        self.add_iohwab_specific()
    
    def add_iohwab_specific(self):
        layout = self.layout()
        
        # Signal Configuration
        signal_group = QGroupBox("Signal Configuration")
        signal_layout = QFormLayout()
        
        self.signal_count_spin = QSpinBox()
        self.signal_count_spin.setRange(1, 512)
        self.signal_count_spin.setValue(64)
        signal_layout.addRow("Number of Signals:", self.signal_count_spin)
        
        self.group_count_spin = QSpinBox()
        self.group_count_spin.setRange(0, 64)
        self.group_count_spin.setValue(8)
        signal_layout.addRow("Number of Groups:", self.group_count_spin)
        
        self.channel_count_spin = QSpinBox()
        self.channel_count_spin.setRange(1, 256)
        self.channel_count_spin.setValue(32)
        signal_layout.addRow("Number of Channels:", self.channel_count_spin)
        
        signal_group.setLayout(signal_layout)
        layout.addRow(signal_group)
        
        # Driver Interfaces
        driver_group = QGroupBox("Driver Interfaces")
        driver_layout = QFormLayout()
        
        self.use_adc_cb = QCheckBox("Use ADC Interface")
        self.use_adc_cb.setChecked(True)
        driver_layout.addRow("ADC:", self.use_adc_cb)
        
        self.use_dio_cb = QCheckBox("Use DIO Interface")
        self.use_dio_cb.setChecked(True)
        driver_layout.addRow("DIO:", self.use_dio_cb)
        
        self.use_pwm_cb = QCheckBox("Use PWM Interface")
        self.use_pwm_cb.setChecked(True)
        driver_layout.addRow("PWM:", self.use_pwm_cb)
        
        self.use_icu_cb = QCheckBox("Use ICU Interface")
        self.use_icu_cb.setChecked(False)
        driver_layout.addRow("ICU:", self.use_icu_cb)
        
        self.use_ocu_cb = QCheckBox("Use OCU Interface")
        self.use_ocu_cb.setChecked(False)
        driver_layout.addRow("OCU:", self.use_ocu_cb)
        
        driver_group.setLayout(driver_layout)
        layout.addRow(driver_group)
        
        # API Configuration
        api_group = QGroupBox("API Configuration")
        api_layout = QFormLayout()
        
        self.version_api_cb = QCheckBox("Enable Version Info API")
        self.version_api_cb.setChecked(True)
        api_layout.addRow("Version API:", self.version_api_cb)
        
        self.dev_error_cb = QCheckBox("Enable Development Error Detection")
        self.dev_error_cb.setChecked(True)
        api_layout.addRow("Dev Error Detect:", self.dev_error_cb)
        
        api_group.setLayout(api_layout)
        layout.addRow(api_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "SignalCount": self.signal_count_spin.value(),
            "GroupCount": self.group_count_spin.value(),
            "ChannelCount": self.channel_count_spin.value(),
            "UseAdc": self.use_adc_cb.isChecked(),
            "UseDio": self.use_dio_cb.isChecked(),
            "UsePwm": self.use_pwm_cb.isChecked(),
            "UseIcu": self.use_icu_cb.isChecked(),
            "UseOcu": self.use_ocu_cb.isChecked(),
            "VersionApi": self.version_api_cb.isChecked(),
            "DevErrorDetect": self.dev_error_cb.isChecked()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.signal_count_spin.setValue(config.get("SignalCount", 64))
        self.group_count_spin.setValue(config.get("GroupCount", 8))
        self.channel_count_spin.setValue(config.get("ChannelCount", 32))
        self.use_adc_cb.setChecked(config.get("UseAdc", True))
        self.use_dio_cb.setChecked(config.get("UseDio", True))
        self.use_pwm_cb.setChecked(config.get("UsePwm", True))
        self.use_icu_cb.setChecked(config.get("UseIcu", False))
        self.use_ocu_cb.setChecked(config.get("UseOcu", False))
        self.version_api_cb.setChecked(config.get("VersionApi", True))
        self.dev_error_cb.setChecked(config.get("DevErrorDetect", True))


class MemIfConfigWidget(ModuleConfigWidget):
    """Memory Interface (MemIf) Configuration Widget"""
    
    def __init__(self, parent=None):
        super().__init__("MemIf", parent)
        self.add_memif_specific()
    
    def add_memif_specific(self):
        layout = self.layout()
        
        # Device Configuration
        device_group = QGroupBox("Device Configuration")
        device_layout = QFormLayout()
        
        self.fee_device_count_spin = QSpinBox()
        self.fee_device_count_spin.setRange(0, 4)
        self.fee_device_count_spin.setValue(1)
        device_layout.addRow("FEE Device Count:", self.fee_device_count_spin)
        
        self.ea_device_count_spin = QSpinBox()
        self.ea_device_count_spin.setRange(0, 4)
        self.ea_device_count_spin.setValue(1)
        device_layout.addRow("EA Device Count:", self.ea_device_count_spin)
        
        self.total_devices_spin = QSpinBox()
        self.total_devices_spin.setRange(1, 8)
        self.total_devices_spin.setValue(2)
        device_layout.addRow("Total Devices:", self.total_devices_spin)
        
        device_group.setLayout(device_layout)
        layout.addRow(device_group)
        
        # API Configuration
        api_group = QGroupBox("API Configuration")
        api_layout = QFormLayout()
        
        self.version_api_cb = QCheckBox("Enable Version Info API")
        self.version_api_cb.setChecked(True)
        api_layout.addRow("Version API:", self.version_api_cb)
        
        self.dev_error_cb = QCheckBox("Enable Development Error Detection")
        self.dev_error_cb.setChecked(True)
        api_layout.addRow("Dev Error Detect:", self.dev_error_cb)
        
        self.wa_api_cb = QCheckBox("Enable Write API")
        self.wa_api_cb.setChecked(True)
        api_layout.addRow("Write API:", self.wa_api_cb)
        
        api_group.setLayout(api_layout)
        layout.addRow(api_group)
        
        # Buffer Configuration
        buffer_group = QGroupBox("Buffer Configuration")
        buffer_layout = QFormLayout()
        
        self.main_period_spin = QSpinBox()
        self.main_period_spin.setRange(1, 1000)
        self.main_period_spin.setValue(10)
        self.main_period_spin.setSuffix(" ms")
        buffer_layout.addRow("Main Function Period:", self.main_period_spin)
        
        buffer_group.setLayout(buffer_layout)
        layout.addRow(buffer_group)
    
    def get_config(self) -> Dict[str, Any]:
        config = super().get_config()
        config.update({
            "FeeDeviceCount": self.fee_device_count_spin.value(),
            "EaDeviceCount": self.ea_device_count_spin.value(),
            "TotalDevices": self.total_devices_spin.value(),
            "VersionApi": self.version_api_cb.isChecked(),
            "DevErrorDetect": self.dev_error_cb.isChecked(),
            "WriteApi": self.wa_api_cb.isChecked(),
            "MainFunctionPeriod": self.main_period_spin.value()
        })
        return config
    
    def set_config(self, config: Dict[str, Any]):
        super().set_config(config)
        self.fee_device_count_spin.setValue(config.get("FeeDeviceCount", 1))
        self.ea_device_count_spin.setValue(config.get("EaDeviceCount", 1))
        self.total_devices_spin.setValue(config.get("TotalDevices", 2))
        self.version_api_cb.setChecked(config.get("VersionApi", True))
        self.dev_error_cb.setChecked(config.get("DevErrorDetect", True))
        self.wa_api_cb.setChecked(config.get("WriteApi", True))
        self.main_period_spin.setValue(config.get("MainFunctionPeriod", 10))


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
        # MCAL Modules
        # Mcu
        mcu_widget = McuConfigWidget()
        self.module_tabs["Mcu"] = mcu_widget
        self.tab_widget.addTab(mcu_widget, "Mcu")
        
        # Can
        can_widget = CanConfigWidget()
        self.module_tabs["Can"] = can_widget
        self.tab_widget.addTab(can_widget, "Can")
        
        # ECUAL Modules
        # CanIf
        canif_widget = CanIfConfigWidget()
        self.module_tabs["CanIf"] = canif_widget
        self.tab_widget.addTab(canif_widget, "CanIf")
        
        # CanTp
        cantp_widget = CanTpConfigWidget()
        self.module_tabs["CanTp"] = cantp_widget
        self.tab_widget.addTab(cantp_widget, "CanTp")
        
        # Service Modules
        # PduR
        pdur_widget = PduRConfigWidget()
        self.module_tabs["PduR"] = pdur_widget
        self.tab_widget.addTab(pdur_widget, "PduR")
        
        # Com
        com_widget = ComConfigWidget()
        self.module_tabs["Com"] = com_widget
        self.tab_widget.addTab(com_widget, "Com")
        
        # Dcm
        dcm_widget = DcmConfigWidget()
        self.module_tabs["Dcm"] = dcm_widget
        self.tab_widget.addTab(dcm_widget, "Dcm")
        
        # Dem
        dem_widget = DemConfigWidget()
        self.module_tabs["Dem"] = dem_widget
        self.tab_widget.addTab(dem_widget, "Dem")
        
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
