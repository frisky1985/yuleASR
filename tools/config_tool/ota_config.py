"""
OTA (Over-The-Air) Configuration Module for DDS Configuration Tool.

This module provides configuration management for OTA update campaigns,
partition layouts, download parameters, and security verification settings.

Compliant with ISO 24089, UNECE R156, and AUTOSAR FOTA standards.

Author: Hermes Agent
Version: 1.0.0
"""

import json
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import List, Dict, Optional, Any, Union
from pathlib import Path


class OTAState(Enum):
    """OTA state machine states."""
    IDLE = "idle"
    CAMPAIGN_RECEIVED = "campaign_received"
    DOWNLOADING = "downloading"
    VERIFYING = "verifying"
    READY_TO_INSTALL = "ready_to_install"
    INSTALLING = "installing"
    ACTIVATING = "activating"
    VERIFYING_BOOT = "verifying_boot"
    SUCCESS = "success"
    ROLLING_BACK = "rolling_back"
    FAILED = "failed"


class OTAErrorCode(Enum):
    """OTA error codes."""
    NONE = 0
    MANIFEST_INVALID = 1
    DOWNLOAD_FAILED = 2
    SIGNATURE_INVALID = 3
    HASH_MISMATCH = 4
    INSUFFICIENT_SPACE = 5
    BATTERY_TOO_LOW = 6
    VEHICLE_IN_MOTION = 7
    INSTALLATION_FAILED = 8
    ACTIVATION_FAILED = 9
    ROLLBACK_FAILED = 10
    TIMEOUT = 11
    NETWORK_ERROR = 12


class PartitionType(Enum):
    """Flash partition types."""
    BOOTLOADER = "bootloader"
    APPLICATION = "application"
    CALIBRATION = "calibration"
    DATA = "data"
    BOOT_BANK_A = "boot_bank_a"
    BOOT_BANK_B = "boot_bank_b"
    APP_BANK_A = "app_bank_a"
    APP_BANK_B = "app_bank_b"


@dataclass
class PartitionConfig:
    """Flash partition configuration."""
    name: str
    partition_type: PartitionType
    start_address: int
    size_bytes: int
    erase_size: int = 4096  # Default 4KB sectors
    write_size: int = 4  # Default 4 bytes
    is_active: bool = True
    is_bootable: bool = False
    rollback_supported: bool = True
    version_address: Optional[int] = None
    magic_marker: Optional[int] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "partition_type": self.partition_type.value,
            "start_address": f"0x{self.start_address:08X}",
            "size_bytes": self.size_bytes,
            "erase_size": self.erase_size,
            "write_size": self.write_size,
            "is_active": self.is_active,
            "is_bootable": self.is_bootable,
            "rollback_supported": self.rollback_supported,
            "version_address": f"0x{self.version_address:08X}" if self.version_address else None,
            "magic_marker": f"0x{self.magic_marker:08X}" if self.magic_marker else None
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PartitionConfig':
        """Create from dictionary."""
        return cls(
            name=data["name"],
            partition_type=PartitionType(data["partition_type"]),
            start_address=int(data["start_address"], 16),
            size_bytes=data["size_bytes"],
            erase_size=data.get("erase_size", 4096),
            write_size=data.get("write_size", 4),
            is_active=data.get("is_active", True),
            is_bootable=data.get("is_bootable", False),
            rollback_supported=data.get("rollback_supported", True),
            version_address=int(data["version_address"], 16) if data.get("version_address") else None,
            magic_marker=int(data["magic_marker"], 16) if data.get("magic_marker") else None
        )


@dataclass
class ABPartitionLayout:
    """A/B partition layout configuration."""
    name: str
    bank_a_partitions: List[PartitionConfig] = field(default_factory=list)
    bank_b_partitions: List[PartitionConfig] = field(default_factory=list)
    shared_partitions: List[PartitionConfig] = field(default_factory=list)
    active_bank: str = "A"  # "A" or "B"
    switch_delay_ms: int = 100  # Delay before bank switch
    rollback_timeout_ms: int = 30000  # Timeout for automatic rollback

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "bank_a_partitions": [p.to_dict() for p in self.bank_a_partitions],
            "bank_b_partitions": [p.to_dict() for p in self.bank_b_partitions],
            "shared_partitions": [p.to_dict() for p in self.shared_partitions],
            "active_bank": self.active_bank,
            "switch_delay_ms": self.switch_delay_ms,
            "rollback_timeout_ms": self.rollback_timeout_ms
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ABPartitionLayout':
        """Create from dictionary."""
        return cls(
            name=data["name"],
            bank_a_partitions=[PartitionConfig.from_dict(p) for p in data.get("bank_a_partitions", [])],
            bank_b_partitions=[PartitionConfig.from_dict(p) for p in data.get("bank_b_partitions", [])],
            shared_partitions=[PartitionConfig.from_dict(p) for p in data.get("shared_partitions", [])],
            active_bank=data.get("active_bank", "A"),
            switch_delay_ms=data.get("switch_delay_ms", 100),
            rollback_timeout_ms=data.get("rollback_timeout_ms", 30000)
        )

    def get_active_partitions(self) -> List[PartitionConfig]:
        """Get currently active partition list."""
        if self.active_bank == "A":
            return self.bank_a_partitions + self.shared_partitions
        else:
            return self.bank_b_partitions + self.shared_partitions

    def get_inactive_partitions(self) -> List[PartitionConfig]:
        """Get currently inactive (update target) partition list."""
        if self.active_bank == "A":
            return self.bank_b_partitions
        else:
            return self.bank_a_partitions


@dataclass
class DownloadParams:
    """OTA download parameters."""
    chunk_size_bytes: int = 4096
    max_retries: int = 3
    retry_delay_ms: int = 1000
    timeout_ms: int = 30000
    concurrent_downloads: int = 1
    resume_supported: bool = True
    compression: str = "none"  # none, gzip, zstd, lz4
    encryption: str = "none"  # none, aes-256-gcm
    delta_update: bool = False
    delta_algorithm: str = "bsdiff"  # bsdiff, xdelta3, hdiffpatch

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "chunk_size_bytes": self.chunk_size_bytes,
            "max_retries": self.max_retries,
            "retry_delay_ms": self.retry_delay_ms,
            "timeout_ms": self.timeout_ms,
            "concurrent_downloads": self.concurrent_downloads,
            "resume_supported": self.resume_supported,
            "compression": self.compression,
            "encryption": self.encryption,
            "delta_update": self.delta_update,
            "delta_algorithm": self.delta_algorithm
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DownloadParams':
        """Create from dictionary."""
        return cls(
            chunk_size_bytes=data.get("chunk_size_bytes", 4096),
            max_retries=data.get("max_retries", 3),
            retry_delay_ms=data.get("retry_delay_ms", 1000),
            timeout_ms=data.get("timeout_ms", 30000),
            concurrent_downloads=data.get("concurrent_downloads", 1),
            resume_supported=data.get("resume_supported", True),
            compression=data.get("compression", "none"),
            encryption=data.get("encryption", "none"),
            delta_update=data.get("delta_update", False),
            delta_algorithm=data.get("delta_algorithm", "bsdiff")
        )


@dataclass
class SecurityVerification:
    """OTA security verification settings."""
    signature_required: bool = True
    signature_algorithm: str = "ecdsa-p256"  # ecdsa-p256, rsa-pss-sha256
    hash_algorithm: str = "sha256"  # sha256, sha384, sha512
    verify_certificate_chain: bool = True
    verify_version_rollback: bool = True
    verify_compatibility: bool = True
    secure_boot_required: bool = True
    root_cert_hash: Optional[str] = None
    oem_cert_hash: Optional[str] = None
    key_rotation_interval_days: int = 90

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "signature_required": self.signature_required,
            "signature_algorithm": self.signature_algorithm,
            "hash_algorithm": self.hash_algorithm,
            "verify_certificate_chain": self.verify_certificate_chain,
            "verify_version_rollback": self.verify_version_rollback,
            "verify_compatibility": self.verify_compatibility,
            "secure_boot_required": self.secure_boot_required,
            "root_cert_hash": self.root_cert_hash,
            "oem_cert_hash": self.oem_cert_hash,
            "key_rotation_interval_days": self.key_rotation_interval_days
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'SecurityVerification':
        """Create from dictionary."""
        return cls(
            signature_required=data.get("signature_required", True),
            signature_algorithm=data.get("signature_algorithm", "ecdsa-p256"),
            hash_algorithm=data.get("hash_algorithm", "sha256"),
            verify_certificate_chain=data.get("verify_certificate_chain", True),
            verify_version_rollback=data.get("verify_version_rollback", True),
            verify_compatibility=data.get("verify_compatibility", True),
            secure_boot_required=data.get("secure_boot_required", True),
            root_cert_hash=data.get("root_cert_hash"),
            oem_cert_hash=data.get("oem_cert_hash"),
            key_rotation_interval_days=data.get("key_rotation_interval_days", 90)
        )


@dataclass
class PreCondition:
    """OTA installation pre-conditions."""
    battery_voltage_min_v: float = 12.0
    battery_voltage_max_v: float = 16.0
    vehicle_speed_max_kmh: float = 0.0
    engine_state: str = "off"  # off, idle, any
    parking_brake: str = "engaged"  # engaged, any
    gear: str = "park"  # park, neutral, any
    ignition_state: str = "on"  # on, accessory, any
    safe_to_update: bool = True

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "battery_voltage_min_v": self.battery_voltage_min_v,
            "battery_voltage_max_v": self.battery_voltage_max_v,
            "vehicle_speed_max_kmh": self.vehicle_speed_max_kmh,
            "engine_state": self.engine_state,
            "parking_brake": self.parking_brake,
            "gear": self.gear,
            "ignition_state": self.ignition_state,
            "safe_to_update": self.safe_to_update
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PreCondition':
        """Create from dictionary."""
        return cls(
            battery_voltage_min_v=data.get("battery_voltage_min_v", 12.0),
            battery_voltage_max_v=data.get("battery_voltage_max_v", 16.0),
            vehicle_speed_max_kmh=data.get("vehicle_speed_max_kmh", 0.0),
            engine_state=data.get("engine_state", "off"),
            parking_brake=data.get("parking_brake", "engaged"),
            gear=data.get("gear", "park"),
            ignition_state=data.get("ignition_state", "on"),
            safe_to_update=data.get("safe_to_update", True)
        )


@dataclass
class InstallationParams:
    """OTA installation parameters."""
    estimated_time_seconds: int = 300
    reboot_required: bool = True
    user_confirmation: bool = False
    confirmation_prompt: Optional[str] = None
    can_interrupted: bool = False
    progress_report_interval_ms: int = 1000
    watchdog_timeout_ms: int = 60000

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "estimated_time_seconds": self.estimated_time_seconds,
            "reboot_required": self.reboot_required,
            "user_confirmation": self.user_confirmation,
            "confirmation_prompt": self.confirmation_prompt,
            "can_interrupted": self.can_interrupted,
            "progress_report_interval_ms": self.progress_report_interval_ms,
            "watchdog_timeout_ms": self.watchdog_timeout_ms
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'InstallationParams':
        """Create from dictionary."""
        return cls(
            estimated_time_seconds=data.get("estimated_time_seconds", 300),
            reboot_required=data.get("reboot_required", True),
            user_confirmation=data.get("user_confirmation", False),
            confirmation_prompt=data.get("confirmation_prompt"),
            can_interrupted=data.get("can_interrupted", False),
            progress_report_interval_ms=data.get("progress_report_interval_ms", 1000),
            watchdog_timeout_ms=data.get("watchdog_timeout_ms", 60000)
        )


@dataclass
class ECUUpdate:
    """ECU update configuration."""
    ecu_id: int
    name: str
    hardware_version: str
    firmware_version_from: str
    firmware_version_to: str
    package_file: str
    package_size: int
    package_hash: str
    signature_required: bool = True
    dependencies: List[Dict[str, str]] = field(default_factory=list)
    pre_conditions: PreCondition = field(default_factory=PreCondition)
    installation: InstallationParams = field(default_factory=InstallationParams)

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "ecu_id": f"0x{self.ecu_id:04X}",
            "name": self.name,
            "hardware_version": self.hardware_version,
            "firmware_version_from": self.firmware_version_from,
            "firmware_version_to": self.firmware_version_to,
            "package_file": self.package_file,
            "package_size": self.package_size,
            "package_hash": self.package_hash,
            "signature_required": self.signature_required,
            "dependencies": self.dependencies,
            "pre_conditions": self.pre_conditions.to_dict(),
            "installation": self.installation.to_dict()
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ECUUpdate':
        """Create from dictionary."""
        return cls(
            ecu_id=int(data["ecu_id"], 16),
            name=data["name"],
            hardware_version=data["hardware_version"],
            firmware_version_from=data["firmware_version_from"],
            firmware_version_to=data["firmware_version_to"],
            package_file=data["package_file"],
            package_size=data["package_size"],
            package_hash=data["package_hash"],
            signature_required=data.get("signature_required", True),
            dependencies=data.get("dependencies", []),
            pre_conditions=PreCondition.from_dict(data.get("pre_conditions", {})),
            installation=InstallationParams.from_dict(data.get("installation", {}))
        )


@dataclass
class OTACampaign:
    """OTA campaign configuration."""
    campaign_id: str
    name: str
    description: str
    priority: str = "normal"  # low, normal, high, critical
    scheduled_start: Optional[str] = None
    scheduled_end: Optional[str] = None
    ecu_updates: List[ECUUpdate] = field(default_factory=list)
    rollout_strategy: str = "wave"  # wave, canary, all_at_once
    batch_size_percent: int = 10
    success_threshold_percent: int = 95

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "campaign_id": self.campaign_id,
            "name": self.name,
            "description": self.description,
            "priority": self.priority,
            "scheduled_start": self.scheduled_start,
            "scheduled_end": self.scheduled_end,
            "ecu_updates": [u.to_dict() for u in self.ecu_updates],
            "rollout_strategy": self.rollout_strategy,
            "batch_size_percent": self.batch_size_percent,
            "success_threshold_percent": self.success_threshold_percent
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'OTACampaign':
        """Create from dictionary."""
        return cls(
            campaign_id=data["campaign_id"],
            name=data["name"],
            description=data["description"],
            priority=data.get("priority", "normal"),
            scheduled_start=data.get("scheduled_start"),
            scheduled_end=data.get("scheduled_end"),
            ecu_updates=[ECUUpdate.from_dict(u) for u in data.get("ecu_updates", [])],
            rollout_strategy=data.get("rollout_strategy", "wave"),
            batch_size_percent=data.get("batch_size_percent", 10),
            success_threshold_percent=data.get("success_threshold_percent", 95)
        )


class OTAConfig:
    """
    Complete OTA configuration manager.
    
    Manages OTA campaigns, partition layouts, download parameters,
    security verification, and ECU update configurations.
    """

    def __init__(self):
        self.partition_layouts: List[ABPartitionLayout] = []
        self.download_params: DownloadParams = DownloadParams()
        self.security_verification: SecurityVerification = SecurityVerification()
        self.campaigns: List[OTACampaign] = []
        self.current_state: OTAState = OTAState.IDLE
        self.active_campaign: Optional[str] = None
        self.update_storage_path: str = "/tmp/ota_updates"
        self.max_storage_size_mb: int = 500
        self.uds_transport: str = "can"  # can, doip
        self.uds_tx_id: int = 0x7E0
        self.uds_rx_id: int = 0x7E8
        self.uds_timeout_ms: int = 5000

    def add_partition_layout(self, layout: ABPartitionLayout) -> None:
        """Add a partition layout."""
        self.partition_layouts.append(layout)

    def get_partition_layout(self, name: str) -> Optional[ABPartitionLayout]:
        """Get partition layout by name."""
        for layout in self.partition_layouts:
            if layout.name == name:
                return layout
        return None

    def set_download_params(self, params: DownloadParams) -> None:
        """Set download parameters."""
        self.download_params = params

    def set_security_verification(self, security: SecurityVerification) -> None:
        """Set security verification settings."""
        self.security_verification = security

    def add_campaign(self, campaign: OTACampaign) -> None:
        """Add an OTA campaign."""
        self.campaigns.append(campaign)

    def remove_campaign(self, campaign_id: str) -> bool:
        """Remove a campaign by ID."""
        for i, c in enumerate(self.campaigns):
            if c.campaign_id == campaign_id:
                self.campaigns.pop(i)
                return True
        return False

    def get_campaign(self, campaign_id: str) -> Optional[OTACampaign]:
        """Get campaign by ID."""
        for c in self.campaigns:
            if c.campaign_id == campaign_id:
                return c
        return None

    def get_default_partition_layout(self, flash_size_mb: int = 8) -> ABPartitionLayout:
        """Get default A/B partition layout."""
        flash_size = flash_size_mb * 1024 * 1024
        bank_size = (flash_size - 0x20000) // 2  # Reserve 128KB for bootloader
        
        return ABPartitionLayout(
            name="default",
            bank_a_partitions=[
                PartitionConfig(
                    name="app_a",
                    partition_type=PartitionType.APP_BANK_A,
                    start_address=0x20000,
                    size_bytes=bank_size - 0x10000,
                    is_active=True,
                    is_bootable=True,
                    version_address=0x20000
                ),
                PartitionConfig(
                    name="cal_a",
                    partition_type=PartitionType.CALIBRATION,
                    start_address=0x20000 + bank_size - 0x10000,
                    size_bytes=0x10000,
                    is_active=True
                )
            ],
            bank_b_partitions=[
                PartitionConfig(
                    name="app_b",
                    partition_type=PartitionType.APP_BANK_B,
                    start_address=0x20000 + bank_size,
                    size_bytes=bank_size - 0x10000,
                    is_active=False,
                    is_bootable=True,
                    version_address=0x20000 + bank_size
                ),
                PartitionConfig(
                    name="cal_b",
                    partition_type=PartitionType.CALIBRATION,
                    start_address=0x20000 + 2 * bank_size - 0x10000,
                    size_bytes=0x10000,
                    is_active=False
                )
            ],
            shared_partitions=[
                PartitionConfig(
                    name="bootloader",
                    partition_type=PartitionType.BOOTLOADER,
                    start_address=0x00000,
                    size_bytes=0x20000,
                    is_active=True,
                    is_bootable=True,
                    rollback_supported=False
                )
            ],
            active_bank="A"
        )

    def to_dict(self) -> Dict[str, Any]:
        """Convert entire configuration to dictionary."""
        return {
            "partition_layouts": [l.to_dict() for l in self.partition_layouts],
            "download_params": self.download_params.to_dict(),
            "security_verification": self.security_verification.to_dict(),
            "campaigns": [c.to_dict() for c in self.campaigns],
            "current_state": self.current_state.value,
            "active_campaign": self.active_campaign,
            "update_storage_path": self.update_storage_path,
            "max_storage_size_mb": self.max_storage_size_mb,
            "uds_transport": self.uds_transport,
            "uds_tx_id": f"0x{self.uds_tx_id:03X}",
            "uds_rx_id": f"0x{self.uds_rx_id:03X}",
            "uds_timeout_ms": self.uds_timeout_ms
        }

    def from_dict(self, data: Dict[str, Any]) -> None:
        """Load configuration from dictionary."""
        self.partition_layouts = [ABPartitionLayout.from_dict(l) for l in data.get("partition_layouts", [])]
        self.download_params = DownloadParams.from_dict(data.get("download_params", {}))
        self.security_verification = SecurityVerification.from_dict(data.get("security_verification", {}))
        self.campaigns = [OTACampaign.from_dict(c) for c in data.get("campaigns", [])]
        self.current_state = OTAState(data.get("current_state", "idle"))
        self.active_campaign = data.get("active_campaign")
        self.update_storage_path = data.get("update_storage_path", "/tmp/ota_updates")
        self.max_storage_size_mb = data.get("max_storage_size_mb", 500)
        self.uds_transport = data.get("uds_transport", "can")
        self.uds_tx_id = int(data.get("uds_tx_id", "0x7E0"), 16)
        self.uds_rx_id = int(data.get("uds_rx_id", "0x7E8"), 16)
        self.uds_timeout_ms = data.get("uds_timeout_ms", 5000)

    def save_to_file(self, filepath: Union[str, Path]) -> None:
        """Save configuration to JSON file."""
        filepath = Path(filepath)
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(self.to_dict(), f, indent=2, ensure_ascii=False)

    def load_from_file(self, filepath: Union[str, Path]) -> None:
        """Load configuration from JSON file."""
        filepath = Path(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
        self.from_dict(data)

    def validate(self) -> List[str]:
        """
        Validate the OTA configuration.
        
        Returns:
            List of validation error messages (empty if valid)
        """
        errors = []
        
        if not self.partition_layouts:
            errors.append("No partition layouts configured")
        
        # Validate partition layouts
        for layout in self.partition_layouts:
            addresses = []
            for p in layout.bank_a_partitions + layout.bank_b_partitions + layout.shared_partitions:
                addresses.append((p.start_address, p.size_bytes, p.name))
            
            # Check for overlapping partitions
            for i, (start1, size1, name1) in enumerate(addresses):
                end1 = start1 + size1
                for start2, size2, name2 in addresses[i+1:]:
                    end2 = start2 + size2
                    if not (end1 <= start2 or end2 <= start1):
                        errors.append(f"Partitions {name1} and {name2} overlap in layout {layout.name}")
        
        # Validate campaign IDs are unique
        campaign_ids = [c.campaign_id for c in self.campaigns]
        if len(campaign_ids) != len(set(campaign_ids)):
            errors.append("Duplicate campaign IDs found")
        
        # Validate ECU IDs are unique within campaigns
        for campaign in self.campaigns:
            ecu_ids = [u.ecu_id for u in campaign.ecu_updates]
            if len(ecu_ids) != len(set(ecu_ids)):
                errors.append(f"Campaign {campaign.campaign_id} has duplicate ECU IDs")
        
        # Validate security settings
        if self.security_verification.signature_required:
            if not self.security_verification.root_cert_hash:
                errors.append("Signature verification enabled but root certificate hash not set")
        
        return errors
