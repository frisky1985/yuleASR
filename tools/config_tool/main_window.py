"""
Main Window for DDS Configuration Tool.

Provides a unified GUI for configuring diagnostic, E2E, and OTA settings.
Integrates with the DDS base configuration (Domain/Topic/QoS).

Author: Hermes Agent
Version: 1.0.0
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from pathlib import Path
from typing import Optional, Dict, List, Any
import json
import sys

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from diagnostic_config import DiagnosticConfig, DCMParams, DiagServiceConfig, DiagServiceID
from diagnostic_config import DIDDefinition, DTCConfig, SecurityPolicy, DoIPConfig
from diagnostic_config import DiagSessionType, DiagPriority, create_common_did
from e2e_config import E2EConfig, E2EProfileConfig, DataIDMapping, E2EProtection
from e2e_config import E2EMonitoring, E2EProfile, E2EDataIDMode, CRCType
from e2e_config import STANDARD_CRC_POLYNOMIALS, get_recommended_profile
from ota_config import OTAConfig, PartitionConfig, ABPartitionLayout, OTACampaign
from ota_config import DownloadParams, SecurityVerification, ECUUpdate, PreCondition
from ota_config import InstallationParams, OTAState, PartitionType
from code_generator import CCodeGenerator, ConfigValidator


class ToolTip:
    """Tooltip helper for tkinter widgets."""
    
    def __init__(self, widget, text):
        self.widget = widget
        self.text = text
        self.tooltip = None
        self.widget.bind("<Enter>", self.show)
        self.widget.bind("<Leave>", self.hide)
    
    def show(self, event=None):
        x, y, _, _ = self.widget.bbox("insert")
        x += self.widget.winfo_rootx() + 25
        y += self.widget.winfo_rooty() + 25
        
        self.tooltip = tk.Toplevel(self.widget)
        self.tooltip.wm_overrideredirect(True)
        self.tooltip.wm_geometry(f"+{x}+{y}")
        
        label = ttk.Label(self.tooltip, text=self.text, background="#ffffe0", 
                         relief="solid", borderwidth=1, padding=5)
        label.pack()
    
    def hide(self, event=None):
        if self.tooltip:
            self.tooltip.destroy()
            self.tooltip = None


class DiagnosticConfigFrame(ttk.LabelFrame):
    """Frame for diagnostic configuration."""
    
    def __init__(self, parent, config: DiagnosticConfig):
        super().__init__(parent, text="Diagnostic Configuration (UDS/DCM)", padding=10)
        self.config = config
        self._create_widgets()
        self._load_config()
    
    def _create_widgets(self):
        """Create GUI widgets."""
        # DCM Parameters
        dcm_frame = ttk.LabelFrame(self, text="DCM Parameters", padding=5)
        dcm_frame.pack(fill="x", pady=5)
        
        ttk.Label(dcm_frame, text="ECU ID:").grid(row=0, column=0, sticky="w", padx=5, pady=2)
        self.ecu_id_var = tk.StringVar(value="0x7E0")
        ttk.Entry(dcm_frame, textvariable=self.ecu_id_var, width=10).grid(row=0, column=1, padx=5, pady=2)
        
        ttk.Label(dcm_frame, text="Logical Address:").grid(row=0, column=2, sticky="w", padx=5, pady=2)
        self.logical_addr_var = tk.StringVar(value="0x0001")
        ttk.Entry(dcm_frame, textvariable=self.logical_addr_var, width=10).grid(row=0, column=3, padx=5, pady=2)
        
        ttk.Label(dcm_frame, text="Functional Address:").grid(row=1, column=0, sticky="w", padx=5, pady=2)
        self.func_addr_var = tk.StringVar(value="0x7DF")
        ttk.Entry(dcm_frame, textvariable=self.func_addr_var, width=10).grid(row=1, column=1, padx=5, pady=2)
        
        ttk.Label(dcm_frame, text="P2 Timeout (ms):").grid(row=1, column=2, sticky="w", padx=5, pady=2)
        self.p2_timeout_var = tk.StringVar(value="50")
        ttk.Entry(dcm_frame, textvariable=self.p2_timeout_var, width=10).grid(row=1, column=3, padx=5, pady=2)
        
        # Services Frame
        services_frame = ttk.LabelFrame(self, text="Diagnostic Services", padding=5)
        services_frame.pack(fill="both", expand=True, pady=5)
        
        # Services Treeview
        columns = ("Service ID", "Name", "Priority", "Enabled", "Sessions", "Security")
        self.services_tree = ttk.Treeview(services_frame, columns=columns, show="headings", height=8)
        for col in columns:
            self.services_tree.heading(col, text=col)
            self.services_tree.column(col, width=80)
        self.services_tree.column("Name", width=150)
        self.services_tree.pack(fill="both", expand=True, side="left")
        
        scrollbar = ttk.Scrollbar(services_frame, orient="vertical", command=self.services_tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.services_tree.configure(yscrollcommand=scrollbar.set)
        
        # Service Buttons
        btn_frame = ttk.Frame(self)
        btn_frame.pack(fill="x", pady=5)
        
        ttk.Button(btn_frame, text="Add Default Services", 
                  command=self._add_default_services).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Clear All", 
                  command=self._clear_services).pack(side="left", padx=5)
        
        # DID Frame
        did_frame = ttk.LabelFrame(self, text="Data Identifiers (DIDs)", padding=5)
        did_frame.pack(fill="both", expand=True, pady=5)
        
        did_columns = ("DID", "Name", "Description", "Length", "Type")
        self.did_tree = ttk.Treeview(did_frame, columns=did_columns, show="headings", height=5)
        for col in did_columns:
            self.did_tree.heading(col, text=col)
            self.did_tree.column(col, width=100)
        self.did_tree.column("Name", width=150)
        self.did_tree.column("Description", width=200)
        self.did_tree.pack(fill="both", expand=True, side="left")
        
        did_scrollbar = ttk.Scrollbar(did_frame, orient="vertical", command=self.did_tree.yview)
        did_scrollbar.pack(side="right", fill="y")
        self.did_tree.configure(yscrollcommand=did_scrollbar.set)
        
        # DID Buttons
        did_btn_frame = ttk.Frame(self)
        did_btn_frame.pack(fill="x", pady=5)
        
        ttk.Button(did_btn_frame, text="Add Common DIDs", 
                  command=self._add_common_dids).pack(side="left", padx=5)
        ttk.Button(did_btn_frame, text="Clear DIDs", 
                  command=self._clear_dids).pack(side="left", padx=5)
        
        # DoIP Configuration
        doip_frame = ttk.LabelFrame(self, text="DoIP Configuration", padding=5)
        doip_frame.pack(fill="x", pady=5)
        
        self.doip_enabled_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(doip_frame, text="Enable DoIP", 
                       variable=self.doip_enabled_var).grid(row=0, column=0, sticky="w", padx=5)
        
        ttk.Label(doip_frame, text="Local Port:").grid(row=0, column=1, sticky="w", padx=5)
        self.doip_port_var = tk.StringVar(value="13400")
        ttk.Entry(doip_frame, textvariable=self.doip_port_var, width=8).grid(row=0, column=2, padx=5)
        
        ttk.Label(doip_frame, text="Max Connections:").grid(row=0, column=3, sticky="w", padx=5)
        self.doip_max_conn_var = tk.StringVar(value="8")
        ttk.Entry(doip_frame, textvariable=self.doip_max_conn_var, width=5).grid(row=0, column=4, padx=5)
    
    def _load_config(self):
        """Load configuration into GUI."""
        if self.config.dcm_params:
            params = self.config.dcm_params
            self.ecu_id_var.set(f"0x{params.ecu_id:04X}")
            self.logical_addr_var.set(f"0x{params.logical_address:04X}")
            self.func_addr_var.set(f"0x{params.functional_address:04X}")
            self.p2_timeout_var.set(str(params.p2_timeout_ms))
        
        # Load services
        for svc in self.config.services:
            sessions = ", ".join(s.name[:3] for s in svc.supported_sessions)
            self.services_tree.insert("", "end", values=(
                f"0x{svc.service_id:02X}", svc.name, svc.priority.value,
                "Yes" if svc.enabled else "No", sessions, svc.security_level_required
            ))
        
        # Load DIDs
        for did in self.config.dids:
            self.did_tree.insert("", "end", values=(
                f"0x{did.did:04X}", did.name, did.description,
                did.data_length, did.data_type
            ))
    
    def _add_default_services(self):
        """Add default diagnostic services."""
        default_services = self.config.get_default_services()
        for svc in default_services:
            self.config.add_service(svc)
            sessions = ", ".join(s.name[:3] for s in svc.supported_sessions)
            self.services_tree.insert("", "end", values=(
                f"0x{svc.service_id:02X}", svc.name, svc.priority.value,
                "Yes" if svc.enabled else "No", sessions, svc.security_level_required
            ))
    
    def _clear_services(self):
        """Clear all services."""
        self.config.services = []
        for item in self.services_tree.get_children():
            self.services_tree.delete(item)
    
    def _add_common_dids(self):
        """Add common DIDs."""
        from diagnostic_config import COMMON_DIDS
        for did_id, (name, desc) in COMMON_DIDS.items():
            did = create_common_did(did_id)
            if did:
                self.config.add_did(did)
                self.did_tree.insert("", "end", values=(
                    f"0x{did.did:04X}", did.name, did.description,
                    did.data_length, did.data_type
                ))
    
    def _clear_dids(self):
        """Clear all DIDs."""
        self.config.dids = []
        for item in self.did_tree.get_children():
            self.did_tree.delete(item)
    
    def save_config(self):
        """Save GUI values to configuration object."""
        # Save DCM params
        self.config.dcm_params = DCMParams(
            ecu_id=int(self.ecu_id_var.get(), 16),
            logical_address=int(self.logical_addr_var.get(), 16),
            functional_address=int(self.func_addr_var.get(), 16),
            p2_timeout_ms=int(self.p2_timeout_var.get())
        )
        
        # Save DoIP config
        self.config.doip_config = DoIPConfig(
            enabled=self.doip_enabled_var.get(),
            local_port=int(self.doip_port_var.get()),
            max_connections=int(self.doip_max_conn_var.get())
        )


class E2EConfigFrame(ttk.LabelFrame):
    """Frame for E2E configuration."""
    
    def __init__(self, parent, config: E2EConfig):
        super().__init__(parent, text="E2E Protection Configuration", padding=10)
        self.config = config
        self._create_widgets()
        self._load_config()
    
    def _create_widgets(self):
        """Create GUI widgets."""
        # E2E Profiles
        profile_frame = ttk.LabelFrame(self, text="E2E Profiles", padding=5)
        profile_frame.pack(fill="x", pady=5)
        
        self.profile_vars = {}
        profiles_grid = [
            ("Profile 1 (CRC8)", E2EProfile.PROFILE_1),
            ("Profile 2 (CRC8+ID)", E2EProfile.PROFILE_2),
            ("Profile 4 (CRC16)", E2EProfile.PROFILE_4),
            ("Profile 5 (CRC16+ID)", E2EProfile.PROFILE_5),
            ("Profile 6 (CRC16+Nibble)", E2EProfile.PROFILE_6),
            ("Profile 7 (CRC32)", E2EProfile.PROFILE_7),
        ]
        
        for i, (label, profile) in enumerate(profiles_grid):
            var = tk.BooleanVar(value=False)
            self.profile_vars[profile] = var
            ttk.Checkbutton(profile_frame, text=label, variable=var).grid(
                row=i//3, column=i%3, sticky="w", padx=10, pady=2)
        
        # Data ID Mappings
        mapping_frame = ttk.LabelFrame(self, text="Data ID Mappings", padding=5)
        mapping_frame.pack(fill="both", expand=True, pady=5)
        
        mapping_columns = ("Data ID", "Name", "Source", "Target", "Profile", "Safety")
        self.mapping_tree = ttk.Treeview(mapping_frame, columns=mapping_columns, 
                                        show="headings", height=6)
        for col in mapping_columns:
            self.mapping_tree.heading(col, text=col)
            self.mapping_tree.column(col, width=80)
        self.mapping_tree.column("Name", width=120)
        self.mapping_tree.pack(fill="both", expand=True, side="left")
        
        scrollbar = ttk.Scrollbar(mapping_frame, orient="vertical", command=self.mapping_tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.mapping_tree.configure(yscrollcommand=scrollbar.set)
        
        # Mapping Buttons
        btn_frame = ttk.Frame(self)
        btn_frame.pack(fill="x", pady=5)
        
        ttk.Button(btn_frame, text="Add Example Mappings", 
                  command=self._add_example_mappings).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Clear Mappings", 
                  command=self._clear_mappings).pack(side="left", padx=5)
        
        # Global Settings
        settings_frame = ttk.LabelFrame(self, text="Global Settings", padding=5)
        settings_frame.pack(fill="x", pady=5)
        
        ttk.Label(settings_frame, text="Window Size:").grid(row=0, column=0, sticky="w", padx=5)
        self.window_size_var = tk.StringVar(value="3")
        ttk.Entry(settings_frame, textvariable=self.window_size_var, width=5).grid(row=0, column=1, padx=5)
        
        ttk.Label(settings_frame, text="Timeout (ms):").grid(row=0, column=2, sticky="w", padx=5)
        self.timeout_var = tk.StringVar(value="100")
        ttk.Entry(settings_frame, textvariable=self.timeout_var, width=8).grid(row=0, column=3, padx=5)
    
    def _load_config(self):
        """Load configuration into GUI."""
        # Load profile selections
        for profile, cfg in self.config.profiles.items():
            if profile in self.profile_vars:
                self.profile_vars[profile].set(True)
        
        # Load mappings
        for mapping in self.config.data_id_mappings:
            profile_name = mapping.profile.name if mapping.profile else "None"
            self.mapping_tree.insert("", "end", values=(
                f"0x{mapping.data_id:04X}", mapping.name, 
                mapping.source_component, mapping.target_component,
                profile_name, mapping.safety_level
            ))
    
    def _add_example_mappings(self):
        """Add example Data ID mappings."""
        examples = [
            (0x100, "VehicleSpeed", "Sensor_ECU", "Powertrain_ECU", E2EProfile.PROFILE_5, "ASIL_B"),
            (0x101, "EngineTorque", "Engine_ECU", "Trans_ECU", E2EProfile.PROFILE_5, "ASIL_D"),
            (0x200, "BrakePressure", "Brake_ECU", "Chassis_ECU", E2EProfile.PROFILE_7, "ASIL_D"),
            (0x300, "SteeringAngle", "Steer_ECU", "Chassis_ECU", E2EProfile.PROFILE_7, "ASIL_D"),
        ]
        
        for data_id, name, src, tgt, profile, safety in examples:
            mapping = DataIDMapping(
                data_id=data_id, name=name, source_component=src,
                target_component=tgt, data_type="uint32",
                signal_name=name, profile=profile, safety_level=safety
            )
            self.config.add_data_id_mapping(mapping)
            self.mapping_tree.insert("", "end", values=(
                f"0x{data_id:04X}", name, src, tgt, profile.name, safety
            ))
            
            # Add default profile config if not exists
            if profile not in self.config.profiles:
                default_profiles = self.config.get_default_profiles()
                if profile in default_profiles:
                    self.config.add_profile(default_profiles[profile])
    
    def _clear_mappings(self):
        """Clear all mappings."""
        self.config.data_id_mappings = []
        for item in self.mapping_tree.get_children():
            self.mapping_tree.delete(item)
    
    def save_config(self):
        """Save GUI values to configuration object."""
        # Add selected profiles
        default_profiles = self.config.get_default_profiles()
        for profile, var in self.profile_vars.items():
            if var.get() and profile not in self.config.profiles:
                if profile in default_profiles:
                    self.config.add_profile(default_profiles[profile])
        
        # Save global settings
        self.config.global_window_size = int(self.window_size_var.get())
        self.config.global_timeout_ms = int(self.timeout_var.get())


class OTAConfigFrame(ttk.LabelFrame):
    """Frame for OTA configuration."""
    
    def __init__(self, parent, config: OTAConfig):
        super().__init__(parent, text="OTA Update Configuration", padding=10)
        self.config = config
        self._create_widgets()
        self._load_config()
    
    def _create_widgets(self):
        """Create GUI widgets."""
        # Notebook for sub-tabs
        notebook = ttk.Notebook(self)
        notebook.pack(fill="both", expand=True, pady=5)
        
        # Partition Layout Tab
        partition_tab = ttk.Frame(notebook, padding=10)
        notebook.add(partition_tab, text="Partition Layout")
        self._create_partition_tab(partition_tab)
        
        # Download Parameters Tab
        download_tab = ttk.Frame(notebook, padding=10)
        notebook.add(download_tab, text="Download")
        self._create_download_tab(download_tab)
        
        # Security Tab
        security_tab = ttk.Frame(notebook, padding=10)
        notebook.add(security_tab, text="Security")
        self._create_security_tab(security_tab)
        
        # Campaigns Tab
        campaigns_tab = ttk.Frame(notebook, padding=10)
        notebook.add(campaigns_tab, text="Campaigns")
        self._create_campaigns_tab(campaigns_tab)
    
    def _create_partition_tab(self, parent):
        """Create partition layout tab."""
        # Partition Tree
        columns = ("Name", "Type", "Start Address", "Size", "Active", "Bootable")
        self.partition_tree = ttk.Treeview(parent, columns=columns, show="headings", height=8)
        for col in columns:
            self.partition_tree.heading(col, text=col)
            self.partition_tree.column(col, width=80)
        self.partition_tree.column("Name", width=100)
        self.partition_tree.pack(fill="both", expand=True, side="left")
        
        scrollbar = ttk.Scrollbar(parent, orient="vertical", command=self.partition_tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.partition_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill="x", pady=5)
        
        ttk.Button(btn_frame, text="Add Default Layout", 
                  command=self._add_default_layout).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Clear Layouts", 
                  command=self._clear_layouts).pack(side="left", padx=5)
        
        # Active Bank
        bank_frame = ttk.Frame(parent)
        bank_frame.pack(fill="x", pady=5)
        
        ttk.Label(bank_frame, text="Active Bank:").pack(side="left", padx=5)
        self.active_bank_var = tk.StringVar(value="A")
        ttk.Combobox(bank_frame, textvariable=self.active_bank_var, 
                    values=["A", "B"], width=5, state="readonly").pack(side="left", padx=5)
    
    def _create_download_tab(self, parent):
        """Create download parameters tab."""
        # Chunk Size
        ttk.Label(parent, text="Chunk Size (bytes):").grid(row=0, column=0, sticky="w", padx=5, pady=5)
        self.chunk_size_var = tk.StringVar(value="4096")
        ttk.Entry(parent, textvariable=self.chunk_size_var, width=10).grid(row=0, column=1, padx=5, pady=5)
        
        # Max Retries
        ttk.Label(parent, text="Max Retries:").grid(row=0, column=2, sticky="w", padx=5, pady=5)
        self.max_retries_var = tk.StringVar(value="3")
        ttk.Entry(parent, textvariable=self.max_retries_var, width=5).grid(row=0, column=3, padx=5, pady=5)
        
        # Timeout
        ttk.Label(parent, text="Timeout (ms):").grid(row=1, column=0, sticky="w", padx=5, pady=5)
        self.download_timeout_var = tk.StringVar(value="30000")
        ttk.Entry(parent, textvariable=self.download_timeout_var, width=10).grid(row=1, column=1, padx=5, pady=5)
        
        # Compression
        ttk.Label(parent, text="Compression:").grid(row=1, column=2, sticky="w", padx=5, pady=5)
        self.compression_var = tk.StringVar(value="none")
        ttk.Combobox(parent, textvariable=self.compression_var, 
                    values=["none", "gzip", "zstd", "lz4"], width=10).grid(row=1, column=3, padx=5, pady=5)
        
        # Encryption
        ttk.Label(parent, text="Encryption:").grid(row=2, column=0, sticky="w", padx=5, pady=5)
        self.encryption_var = tk.StringVar(value="none")
        ttk.Combobox(parent, textvariable=self.encryption_var, 
                    values=["none", "aes-256-gcm"], width=15).grid(row=2, column=1, padx=5, pady=5)
        
        # Resume
        self.resume_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(parent, text="Support Resume", variable=self.resume_var).grid(
            row=2, column=2, columnspan=2, sticky="w", padx=5, pady=5)
    
    def _create_security_tab(self, parent):
        """Create security verification tab."""
        # Signature
        self.sig_required_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(parent, text="Signature Required", variable=self.sig_required_var).grid(
            row=0, column=0, columnspan=2, sticky="w", padx=5, pady=5)
        
        # Signature Algorithm
        ttk.Label(parent, text="Signature Algorithm:").grid(row=1, column=0, sticky="w", padx=5, pady=5)
        self.sig_algo_var = tk.StringVar(value="ecdsa-p256")
        ttk.Combobox(parent, textvariable=self.sig_algo_var, 
                    values=["ecdsa-p256", "rsa-pss-sha256"], width=15).grid(row=1, column=1, padx=5, pady=5)
        
        # Hash Algorithm
        ttk.Label(parent, text="Hash Algorithm:").grid(row=1, column=2, sticky="w", padx=5, pady=5)
        self.hash_algo_var = tk.StringVar(value="sha256")
        ttk.Combobox(parent, textvariable=self.hash_algo_var, 
                    values=["sha256", "sha384", "sha512"], width=10).grid(row=1, column=3, padx=5, pady=5)
        
        # Verification Options
        self.verify_cert_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(parent, text="Verify Certificate Chain", variable=self.verify_cert_var).grid(
            row=2, column=0, columnspan=2, sticky="w", padx=5, pady=5)
        
        self.verify_rollback_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(parent, text="Verify Version Rollback", variable=self.verify_rollback_var).grid(
            row=2, column=2, columnspan=2, sticky="w", padx=5, pady=5)
        
        self.secure_boot_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(parent, text="Secure Boot Required", variable=self.secure_boot_var).grid(
            row=3, column=0, columnspan=2, sticky="w", padx=5, pady=5)
    
    def _create_campaigns_tab(self, parent):
        """Create campaigns tab."""
        # Campaign Tree
        columns = ("Campaign ID", "Name", "Priority", "ECU Count", "Strategy")
        self.campaign_tree = ttk.Treeview(parent, columns=columns, show="headings", height=6)
        for col in columns:
            self.campaign_tree.heading(col, text=col)
            self.campaign_tree.column(col, width=80)
        self.campaign_tree.column("Name", width=150)
        self.campaign_tree.pack(fill="both", expand=True, side="left")
        
        scrollbar = ttk.Scrollbar(parent, orient="vertical", command=self.campaign_tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.campaign_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.pack(fill="x", pady=5)
        
        ttk.Button(btn_frame, text="Add Example Campaign", 
                  command=self._add_example_campaign).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Clear Campaigns", 
                  command=self._clear_campaigns).pack(side="left", padx=5)
    
    def _load_config(self):
        """Load configuration into GUI."""
        # Load partition layouts
        for layout in self.config.partition_layouts:
            all_parts = layout.bank_a_partitions + layout.bank_b_partitions + layout.shared_partitions
            for part in all_parts:
                self.partition_tree.insert("", "end", values=(
                    part.name, part.partition_type.value,
                    f"0x{part.start_address:08X}", part.size_bytes,
                    "Yes" if part.is_active else "No",
                    "Yes" if part.is_bootable else "No"
                ))
            self.active_bank_var.set(layout.active_bank)
        
        # Load campaigns
        for campaign in self.config.campaigns:
            self.campaign_tree.insert("", "end", values=(
                campaign.campaign_id, campaign.name, campaign.priority,
                len(campaign.ecu_updates), campaign.rollout_strategy
            ))
    
    def _add_default_layout(self):
        """Add default partition layout."""
        layout = self.config.get_default_partition_layout(8)  # 8MB flash
        self.config.add_partition_layout(layout)
        
        # Refresh tree
        for item in self.partition_tree.get_children():
            self.partition_tree.delete(item)
        
        all_parts = layout.bank_a_partitions + layout.bank_b_partitions + layout.shared_partitions
        for part in all_parts:
            self.partition_tree.insert("", "end", values=(
                part.name, part.partition_type.value,
                f"0x{part.start_address:08X}", part.size_bytes,
                "Yes" if part.is_active else "No",
                "Yes" if part.is_bootable else "No"
            ))
    
    def _clear_layouts(self):
        """Clear all partition layouts."""
        self.config.partition_layouts = []
        for item in self.partition_tree.get_children():
            self.partition_tree.delete(item)
    
    def _add_example_campaign(self):
        """Add example campaign."""
        campaign = OTACampaign(
            campaign_id="CAMP-2026-001",
            name="Q2 Firmware Update",
            description="Infotainment and Powertrain update",
            priority="high",
            ecu_updates=[
                ECUUpdate(
                    ecu_id=0x0101,
                    name="Infotainment_HU",
                    hardware_version="2.1.0",
                    firmware_version_from="3.2.1",
                    firmware_version_to="3.3.0",
                    package_file="ecu_0101.epkg",
                    package_size=52428800,
                    package_hash="sha256:abc123..."
                )
            ],
            rollout_strategy="wave",
            batch_size_percent=10,
            success_threshold_percent=95
        )
        self.config.add_campaign(campaign)
        
        self.campaign_tree.insert("", "end", values=(
            campaign.campaign_id, campaign.name, campaign.priority,
            len(campaign.ecu_updates), campaign.rollout_strategy
        ))
    
    def _clear_campaigns(self):
        """Clear all campaigns."""
        self.config.campaigns = []
        for item in self.campaign_tree.get_children():
            self.campaign_tree.delete(item)
    
    def save_config(self):
        """Save GUI values to configuration object."""
        # Save download params
        self.config.download_params = DownloadParams(
            chunk_size_bytes=int(self.chunk_size_var.get()),
            max_retries=int(self.max_retries_var.get()),
            timeout_ms=int(self.download_timeout_var.get()),
            compression=self.compression_var.get(),
            encryption=self.encryption_var.get(),
            resume_supported=self.resume_var.get()
        )
        
        # Save security verification
        self.config.security_verification = SecurityVerification(
            signature_required=self.sig_required_var.get(),
            signature_algorithm=self.sig_algo_var.get(),
            hash_algorithm=self.hash_algo_var.get(),
            verify_certificate_chain=self.verify_cert_var.get(),
            verify_version_rollback=self.verify_rollback_var.get(),
            secure_boot_required=self.secure_boot_var.get()
        )
        
        # Update active bank for first layout
        if self.config.partition_layouts:
            self.config.partition_layouts[0].active_bank = self.active_bank_var.get()


class MainWindow:
    """
    Main application window for DDS Configuration Tool.
    
    Integrates diagnostic, E2E, and OTA configuration with DDS base configuration.
    """
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("DDS Configuration Tool - Automotive Edition")
        self.root.geometry("1200x800")
        
        # Configuration objects
        self.diag_config = DiagnosticConfig()
        self.e2e_config = E2EConfig()
        self.ota_config = OTAConfig()
        
        # Validator and Generator
        self.validator = ConfigValidator()
        self.generator = CCodeGenerator()
        
        self._create_menu()
        self._create_widgets()
    
    def _create_menu(self):
        """Create application menu."""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New Configuration", command=self._new_config)
        file_menu.add_command(label="Load Configuration...", command=self._load_config)
        file_menu.add_command(label="Save Configuration...", command=self._save_config)
        file_menu.add_separator()
        file_menu.add_command(label="Generate C Code...", command=self._generate_code)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Tools menu
        tools_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Tools", menu=tools_menu)
        tools_menu.add_command(label="Validate Configuration", command=self._validate_config)
        tools_menu.add_separator()
        tools_menu.add_command(label="Load Defaults", command=self._load_defaults)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self._show_about)
    
    def _create_widgets(self):
        """Create main GUI widgets."""
        # Main notebook (tabs)
        self.notebook = ttk.Notebook(self.root, padding=10)
        self.notebook.pack(fill="both", expand=True)
        
        # Diagnostic Configuration Tab
        self.diag_frame = DiagnosticConfigFrame(self.notebook, self.diag_config)
        self.notebook.add(self.diag_frame, text="Diagnostic (UDS/DCM)")
        
        # E2E Configuration Tab
        self.e2e_frame = E2EConfigFrame(self.notebook, self.e2e_config)
        self.notebook.add(self.e2e_frame, text="E2E Protection")
        
        # OTA Configuration Tab
        self.ota_frame = OTAConfigFrame(self.notebook, self.ota_config)
        self.notebook.add(self.ota_frame, text="OTA Update")
        
        # Status Bar
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief="sunken", anchor="w")
        status_bar.pack(fill="x", side="bottom", pady=2)
        
        # Button Frame
        btn_frame = ttk.Frame(self.root, padding=10)
        btn_frame.pack(fill="x", side="bottom")
        
        ttk.Button(btn_frame, text="Save All", command=self._save_all_configs).pack(side="right", padx=5)
        ttk.Button(btn_frame, text="Validate All", command=self._validate_config).pack(side="right", padx=5)
        ttk.Button(btn_frame, text="Generate C Headers", command=self._generate_code).pack(side="right", padx=5)
    
    def _save_all_configs(self):
        """Save all configurations from GUI to objects."""
        self.diag_frame.save_config()
        self.e2e_frame.save_config()
        self.ota_frame.save_config()
        self.status_var.set("Configuration saved to memory")
    
    def _new_config(self):
        """Create new configuration."""
        if messagebox.askyesno("New Configuration", "Clear all current configurations?"):
            self.diag_config = DiagnosticConfig()
            self.e2e_config = E2EConfig()
            self.ota_config = OTAConfig()
            
            # Refresh frames
            self.diag_frame.destroy()
            self.e2e_frame.destroy()
            self.ota_frame.destroy()
            
            self.diag_frame = DiagnosticConfigFrame(self.notebook, self.diag_config)
            self.notebook.insert(0, self.diag_frame, text="Diagnostic (UDS/DCM)")
            
            self.e2e_frame = E2EConfigFrame(self.notebook, self.e2e_config)
            self.notebook.insert(1, self.e2e_frame, text="E2E Protection")
            
            self.ota_frame = OTAConfigFrame(self.notebook, self.ota_config)
            self.notebook.insert(2, self.ota_frame, text="OTA Update")
            
            self.status_var.set("New configuration created")
    
    def _load_config(self):
        """Load configuration from JSON file."""
        filepath = filedialog.askopenfilename(
            title="Load Configuration",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not filepath:
            return
        
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            
            if "diagnostic" in data:
                self.diag_config.from_dict(data["diagnostic"])
            if "e2e" in data:
                self.e2e_config.from_dict(data["e2e"])
            if "ota" in data:
                self.ota_config.from_dict(data["ota"])
            
            # Refresh frames
            self.diag_frame.destroy()
            self.e2e_frame.destroy()
            self.ota_frame.destroy()
            
            self.diag_frame = DiagnosticConfigFrame(self.notebook, self.diag_config)
            self.notebook.add(self.diag_frame, text="Diagnostic (UDS/DCM)")
            
            self.e2e_frame = E2EConfigFrame(self.notebook, self.e2e_config)
            self.notebook.add(self.e2e_frame, text="E2E Protection")
            
            self.ota_frame = OTAConfigFrame(self.notebook, self.ota_config)
            self.notebook.add(self.ota_frame, text="OTA Update")
            
            self.status_var.set(f"Configuration loaded from {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load configuration:\n{str(e)}")
    
    def _save_config(self):
        """Save configuration to JSON file."""
        self._save_all_configs()
        
        filepath = filedialog.asksaveasfilename(
            title="Save Configuration",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not filepath:
            return
        
        try:
            data = {
                "diagnostic": self.diag_config.to_dict(),
                "e2e": self.e2e_config.to_dict(),
                "ota": self.ota_config.to_dict()
            }
            
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            
            self.status_var.set(f"Configuration saved to {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save configuration:\n{str(e)}")
    
    def _validate_config(self):
        """Validate all configurations."""
        self._save_all_configs()
        
        results = self.validator.validate_all(
            diag_config=self.diag_config,
            e2e_config=self.e2e_config,
            ota_config=self.ota_config
        )
        
        all_errors = []
        for config_type, errors in results.items():
            if errors:
                all_errors.append(f"=== {config_type.upper()} Errors ===")
                all_errors.extend(errors)
        
        if all_errors:
            messagebox.showwarning("Validation Errors", "\n".join(all_errors))
            self.status_var.set(f"Validation failed with {len(all_errors)} errors")
        else:
            messagebox.showinfo("Validation", "All configurations are valid!")
            self.status_var.set("Validation passed")
    
    def _generate_code(self):
        """Generate C header files."""
        self._save_all_configs()
        
        output_dir = filedialog.askdirectory(title="Select Output Directory")
        if not output_dir:
            return
        
        try:
            generated = self.generator.save_all(
                diag_config=self.diag_config,
                e2e_config=self.e2e_config,
                ota_config=self.ota_config,
                output_dir=output_dir
            )
            
            messagebox.showinfo("Code Generation", 
                              f"Generated {len(generated)} files:\n" + "\n".join(generated))
            self.status_var.set(f"Generated {len(generated)} C header files")
        except Exception as e:
            messagebox.showerror("Error", f"Code generation failed:\n{str(e)}")
    
    def _load_defaults(self):
        """Load default configurations."""
        # Load default diagnostic services
        for svc in self.diag_config.get_default_services():
            self.diag_config.add_service(svc)
        
        # Load default partition layout
        layout = self.ota_config.get_default_partition_layout(8)
        self.ota_config.add_partition_layout(layout)
        
        # Refresh GUI
        self.diag_frame.destroy()
        self.ota_frame.destroy()
        
        self.diag_frame = DiagnosticConfigFrame(self.notebook, self.diag_config)
        self.notebook.add(self.diag_frame, text="Diagnostic (UDS/DCM)")
        
        self.ota_frame = OTAConfigFrame(self.notebook, self.ota_config)
        self.notebook.add(self.ota_frame, text="OTA Update")
        
        self.status_var.set("Default configurations loaded")
    
    def _show_about(self):
        """Show about dialog."""
        messagebox.showinfo(
            "About DDS Configuration Tool",
            "DDS Configuration Tool - Automotive Edition\n\n"
            "Version: 1.0.0\n"
            "Features:\n"
            "- Diagnostic (UDS/DCM) Configuration\n"
            "- E2E Protection Configuration\n"
            "- OTA Update Configuration\n"
            "- C Header Code Generation\n\n"
            "Author: Hermes Agent\n"
            "Compliant with: ISO 14229, AUTOSAR, UNECE R156"
        )


def main():
    """Main entry point."""
    root = tk.Tk()
    app = MainWindow(root)
    root.mainloop()


if __name__ == "__main__":
    main()