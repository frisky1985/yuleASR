"""
E2E (End-to-End) Configuration Module for DDS Configuration Tool.

This module provides configuration management for E2E protection profiles,
data ID mappings, CRC polynomials, and communication protection settings.

Supports AUTOSAR E2E profiles 1-7 with configurable parameters.

Author: Hermes Agent
Version: 1.0.0
"""

import json
from dataclasses import dataclass, field, asdict
from enum import Enum, auto
from typing import List, Dict, Optional, Any, Union, Tuple
from pathlib import Path


class E2EProfile(Enum):
    """AUTOSAR E2E Profile Types."""
    PROFILE_1 = 1  # CRC8 with Sequence Counter
    PROFILE_2 = 2  # CRC8 with Sequence Counter and Data ID
    PROFILE_4 = 4  # CRC16 with Sequence Counter
    PROFILE_5 = 5  # CRC16 with Sequence Counter and Data ID
    PROFILE_6 = 6  # CRC16 with Sequence Counter and Data ID (nibble)
    PROFILE_7 = 7  # CRC32 with Sequence Counter and Data ID


class E2EDataIDMode(Enum):
    """Data ID inclusion modes for E2E."""
    ALL_16_BIT = "all16bit"  # 16-bit Data ID in CRC calculation
    ALTERNATING_8_BIT = "alternating8bit"  # Alternating 8-bit Data ID
    NIBBLE = "nibble"  # 4-bit Data ID in nibble
    MASK = "mask"  # Masked Data ID


class CRCType(Enum):
    """CRC algorithm types."""
    CRC_8 = "CRC8"
    CRC_16 = "CRC16"
    CRC_32 = "CRC32"
    CRC_8_SAE_J1850 = "CRC8_SAE_J1850"
    CRC_16_CCITT = "CRC16_CCITT"
    CRC_32_ETHERNET = "CRC32_ETHERNET"


@dataclass
class CRCPolynomial:
    """CRC polynomial definition."""
    name: str
    crc_type: CRCType
    polynomial: int
    initial_value: int
    final_xor: int
    input_reflected: bool
    result_reflected: bool

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "crc_type": self.crc_type.value,
            "polynomial": f"0x{self.polynomial:X}",
            "initial_value": f"0x{self.initial_value:X}",
            "final_xor": f"0x{self.final_xor:X}",
            "input_reflected": self.input_reflected,
            "result_reflected": self.result_reflected
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'CRCPolynomial':
        """Create from dictionary."""
        return cls(
            name=data["name"],
            crc_type=CRCType(data["crc_type"]),
            polynomial=int(data["polynomial"], 16),
            initial_value=int(data["initial_value"], 16),
            final_xor=int(data["final_xor"], 16),
            input_reflected=data["input_reflected"],
            result_reflected=data["result_reflected"]
        )


# Standard CRC polynomials
STANDARD_CRC_POLYNOMIALS = {
    CRCType.CRC_8: CRCPolynomial(
        name="CRC-8",
        crc_type=CRCType.CRC_8,
        polynomial=0x1D,
        initial_value=0xFF,
        final_xor=0xFF,
        input_reflected=False,
        result_reflected=False
    ),
    CRCType.CRC_8_SAE_J1850: CRCPolynomial(
        name="CRC-8-SAE-J1850",
        crc_type=CRCType.CRC_8_SAE_J1850,
        polynomial=0x1D,
        initial_value=0xFF,
        final_xor=0xFF,
        input_reflected=False,
        result_reflected=False
    ),
    CRCType.CRC_16: CRCPolynomial(
        name="CRC-16",
        crc_type=CRCType.CRC_16,
        polynomial=0x1021,
        initial_value=0xFFFF,
        final_xor=0x0000,
        input_reflected=False,
        result_reflected=False
    ),
    CRCType.CRC_16_CCITT: CRCPolynomial(
        name="CRC-16-CCITT",
        crc_type=CRCType.CRC_16_CCITT,
        polynomial=0x1021,
        initial_value=0xFFFF,
        final_xor=0x0000,
        input_reflected=False,
        result_reflected=False
    ),
    CRCType.CRC_32: CRCPolynomial(
        name="CRC-32",
        crc_type=CRCType.CRC_32,
        polynomial=0x04C11DB7,
        initial_value=0xFFFFFFFF,
        final_xor=0xFFFFFFFF,
        input_reflected=True,
        result_reflected=True
    ),
    CRCType.CRC_32_ETHERNET: CRCPolynomial(
        name="CRC-32-Ethernet",
        crc_type=CRCType.CRC_32_ETHERNET,
        polynomial=0x04C11DB7,
        initial_value=0xFFFFFFFF,
        final_xor=0xFFFFFFFF,
        input_reflected=True,
        result_reflected=True
    )
}


@dataclass
class E2EProfileConfig:
    """E2E Profile configuration parameters."""
    profile: E2EProfile
    crc_offset: int = 0  # CRC byte offset in data
    counter_offset: int = 1  # Counter byte offset
    data_id_offset: Optional[int] = None  # Data ID offset (if applicable)
    data_id_mode: E2EDataIDMode = E2EDataIDMode.ALL_16_BIT
    data_id: int = 0x0000  # 16-bit Data ID
    data_length: int = 8  # Data length in bytes
    max_delta_counter_init: int = 1  # Max allowed counter jump
    max_no_new_or_repeated_data: int = 3  # Max allowed repeated data
    sync_counter_init: int = 0  # Sync counter initial value

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "profile": self.profile.value,
            "crc_offset": self.crc_offset,
            "counter_offset": self.counter_offset,
            "data_id_offset": self.data_id_offset,
            "data_id_mode": self.data_id_mode.value,
            "data_id": f"0x{self.data_id:04X}",
            "data_length": self.data_length,
            "max_delta_counter_init": self.max_delta_counter_init,
            "max_no_new_or_repeated_data": self.max_no_new_or_repeated_data,
            "sync_counter_init": self.sync_counter_init
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'E2EProfileConfig':
        """Create from dictionary."""
        return cls(
            profile=E2EProfile(data["profile"]),
            crc_offset=data.get("crc_offset", 0),
            counter_offset=data.get("counter_offset", 1),
            data_id_offset=data.get("data_id_offset"),
            data_id_mode=E2EDataIDMode(data.get("data_id_mode", "all16bit")),
            data_id=int(data.get("data_id", "0x0000"), 16),
            data_length=data.get("data_length", 8),
            max_delta_counter_init=data.get("max_delta_counter_init", 1),
            max_no_new_or_repeated_data=data.get("max_no_new_or_repeated_data", 3),
            sync_counter_init=data.get("sync_counter_init", 0)
        )


@dataclass
class DataIDMapping:
    """Mapping between data element and E2E Data ID."""
    data_id: int
    name: str
    source_component: str
    target_component: str
    data_type: str
    signal_name: str
    profile: Optional[E2EProfile] = None
    crc_type: Optional[CRCType] = None
    safety_level: str = "ASIL_B"  # QM, ASIL_A, ASIL_B, ASIL_C, ASIL_D
    enabled: bool = True

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "data_id": f"0x{self.data_id:04X}",
            "name": self.name,
            "source_component": self.source_component,
            "target_component": self.target_component,
            "data_type": self.data_type,
            "signal_name": self.signal_name,
            "profile": self.profile.value if self.profile else None,
            "crc_type": self.crc_type.value if self.crc_type else None,
            "safety_level": self.safety_level,
            "enabled": self.enabled
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DataIDMapping':
        """Create from dictionary."""
        profile = None
        if data.get("profile"):
            profile = E2EProfile(data["profile"])
        crc_type = None
        if data.get("crc_type"):
            crc_type = CRCType(data["crc_type"])
        
        return cls(
            data_id=int(data["data_id"], 16),
            name=data["name"],
            source_component=data["source_component"],
            target_component=data["target_component"],
            data_type=data["data_type"],
            signal_name=data["signal_name"],
            profile=profile,
            crc_type=crc_type,
            safety_level=data.get("safety_level", "ASIL_B"),
            enabled=data.get("enabled", True)
        )


@dataclass
class E2EProtection:
    """E2E protection configuration for a specific communication."""
    name: str
    source: str
    destination: str
    data_id: int
    profile_config: E2EProfileConfig
    crc_polynomial: Optional[CRCPolynomial] = None
    header_length: int = 2  # E2E header length in bytes
    window_size: int = 3  # Monitoring window size
    timeout_ms: int = 100  # Timeout for E2E monitoring

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "source": self.source,
            "destination": self.destination,
            "data_id": f"0x{self.data_id:04X}",
            "profile_config": self.profile_config.to_dict(),
            "crc_polynomial": self.crc_polynomial.to_dict() if self.crc_polynomial else None,
            "header_length": self.header_length,
            "window_size": self.window_size,
            "timeout_ms": self.timeout_ms
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'E2EProtection':
        """Create from dictionary."""
        crc_poly = None
        if data.get("crc_polynomial"):
            crc_poly = CRCPolynomial.from_dict(data["crc_polynomial"])
        
        return cls(
            name=data["name"],
            source=data["source"],
            destination=data["destination"],
            data_id=int(data["data_id"], 16),
            profile_config=E2EProfileConfig.from_dict(data["profile_config"]),
            crc_polynomial=crc_poly,
            header_length=data.get("header_length", 2),
            window_size=data.get("window_size", 3),
            timeout_ms=data.get("timeout_ms", 100)
        )


@dataclass
class E2EMonitoring:
    """E2E monitoring configuration for receive side."""
    name: str
    source: str
    data_id: int
    max_delta_counter: int = 1
    max_error_state_init: int = 3
    max_error_state_invalid: int = 3
    max_error_state_valid: int = 3
    window_size: int = 3
    debounce_cycles: int = 0
    error_callback: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "source": self.source,
            "data_id": f"0x{self.data_id:04X}",
            "max_delta_counter": self.max_delta_counter,
            "max_error_state_init": self.max_error_state_init,
            "max_error_state_invalid": self.max_error_state_invalid,
            "max_error_state_valid": self.max_error_state_valid,
            "window_size": self.window_size,
            "debounce_cycles": self.debounce_cycles,
            "error_callback": self.error_callback
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'E2EMonitoring':
        """Create from dictionary."""
        return cls(
            name=data["name"],
            source=data["source"],
            data_id=int(data["data_id"], 16),
            max_delta_counter=data.get("max_delta_counter", 1),
            max_error_state_init=data.get("max_error_state_init", 3),
            max_error_state_invalid=data.get("max_error_state_invalid", 3),
            max_error_state_valid=data.get("max_error_state_valid", 3),
            window_size=data.get("window_size", 3),
            debounce_cycles=data.get("debounce_cycles", 0),
            error_callback=data.get("error_callback")
        )


class E2EConfig:
    """
    Complete E2E configuration manager.
    
    Manages E2E profiles, Data ID mappings, CRC polynomials,
    and protection configurations for safety-critical communications.
    """

    def __init__(self):
        self.profiles: Dict[E2EProfile, E2EProfileConfig] = {}
        self.data_id_mappings: List[DataIDMapping] = []
        self.protections: List[E2EProtection] = []
        self.monitorings: List[E2EMonitoring] = []
        self.crc_polynomials: Dict[CRCType, CRCPolynomial] = dict(STANDARD_CRC_POLYNOMIALS)
        self.global_window_size: int = 3
        self.global_timeout_ms: int = 100

    def add_profile(self, profile_config: E2EProfileConfig) -> None:
        """Add or update an E2E profile configuration."""
        self.profiles[profile_config.profile] = profile_config

    def get_profile(self, profile: E2EProfile) -> Optional[E2EProfileConfig]:
        """Get profile configuration by type."""
        return self.profiles.get(profile)

    def add_data_id_mapping(self, mapping: DataIDMapping) -> None:
        """Add a Data ID mapping."""
        self.data_id_mappings.append(mapping)

    def remove_data_id_mapping(self, data_id: int) -> bool:
        """Remove a Data ID mapping."""
        for i, m in enumerate(self.data_id_mappings):
            if m.data_id == data_id:
                self.data_id_mappings.pop(i)
                return True
        return False

    def add_protection(self, protection: E2EProtection) -> None:
        """Add an E2E protection configuration."""
        self.protections.append(protection)

    def add_monitoring(self, monitoring: E2EMonitoring) -> None:
        """Add an E2E monitoring configuration."""
        self.monitorings.append(monitoring)

    def add_crc_polynomial(self, polynomial: CRCPolynomial) -> None:
        """Add a custom CRC polynomial."""
        self.crc_polynomials[polynomial.crc_type] = polynomial

    def get_crc_polynomial(self, crc_type: CRCType) -> Optional[CRCPolynomial]:
        """Get CRC polynomial by type."""
        return self.crc_polynomials.get(crc_type)

    def get_profile_for_data_id(self, data_id: int) -> Optional[E2EProfileConfig]:
        """Get the E2E profile configuration for a specific Data ID."""
        for mapping in self.data_id_mappings:
            if mapping.data_id == data_id and mapping.profile:
                return self.profiles.get(mapping.profile)
        return None

    def get_default_profiles(self) -> Dict[E2EProfile, E2EProfileConfig]:
        """Get default E2E profile configurations."""
        return {
            E2EProfile.PROFILE_1: E2EProfileConfig(
                profile=E2EProfile.PROFILE_1,
                crc_offset=0,
                counter_offset=1,
                data_id_mode=E2EDataIDMode.ALL_16_BIT,
                data_length=8
            ),
            E2EProfile.PROFILE_2: E2EProfileConfig(
                profile=E2EProfile.PROFILE_2,
                crc_offset=0,
                counter_offset=1,
                data_id_mode=E2EDataIDMode.ALTERNATING_8_BIT,
                data_length=8
            ),
            E2EProfile.PROFILE_4: E2EProfileConfig(
                profile=E2EProfile.PROFILE_4,
                crc_offset=0,
                counter_offset=2,
                data_id_mode=E2EDataIDMode.ALL_16_BIT,
                data_length=16
            ),
            E2EProfile.PROFILE_5: E2EProfileConfig(
                profile=E2EProfile.PROFILE_5,
                crc_offset=0,
                counter_offset=2,
                data_id_mode=E2EDataIDMode.ALL_16_BIT,
                data_length=16
            ),
            E2EProfile.PROFILE_6: E2EProfileConfig(
                profile=E2EProfile.PROFILE_6,
                crc_offset=0,
                counter_offset=2,
                data_id_mode=E2EDataIDMode.NIBBLE,
                data_length=16
            ),
            E2EProfile.PROFILE_7: E2EProfileConfig(
                profile=E2EProfile.PROFILE_7,
                crc_offset=0,
                counter_offset=4,
                data_id_mode=E2EDataIDMode.ALL_16_BIT,
                data_length=32
            )
        }

    def to_dict(self) -> Dict[str, Any]:
        """Convert entire configuration to dictionary."""
        return {
            "profiles": {p.value: cfg.to_dict() for p, cfg in self.profiles.items()},
            "data_id_mappings": [m.to_dict() for m in self.data_id_mappings],
            "protections": [p.to_dict() for p in self.protections],
            "monitorings": [m.to_dict() for m in self.monitorings],
            "crc_polynomials": {c.value: p.to_dict() for c, p in self.crc_polynomials.items()},
            "global_window_size": self.global_window_size,
            "global_timeout_ms": self.global_timeout_ms
        }

    def from_dict(self, data: Dict[str, Any]) -> None:
        """Load configuration from dictionary."""
        self.profiles = {}
        for profile_value, profile_data in data.get("profiles", {}).items():
            profile = E2EProfile(int(profile_value))
            self.profiles[profile] = E2EProfileConfig.from_dict(profile_data)
        
        self.data_id_mappings = [DataIDMapping.from_dict(m) for m in data.get("data_id_mappings", [])]
        self.protections = [E2EProtection.from_dict(p) for p in data.get("protections", [])]
        self.monitorings = [E2EMonitoring.from_dict(m) for m in data.get("monitorings", [])]
        
        self.crc_polynomials = {}
        for crc_value, poly_data in data.get("crc_polynomials", {}).items():
            crc_type = CRCType(crc_value)
            self.crc_polynomials[crc_type] = CRCPolynomial.from_dict(poly_data)
        
        self.global_window_size = data.get("global_window_size", 3)
        self.global_timeout_ms = data.get("global_timeout_ms", 100)

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
        Validate the E2E configuration.
        
        Returns:
            List of validation error messages (empty if valid)
        """
        errors = []
        
        # Check for duplicate Data IDs
        data_ids = [m.data_id for m in self.data_id_mappings]
        if len(data_ids) != len(set(data_ids)):
            errors.append("Duplicate Data IDs found in mappings")
        
        # Validate that referenced profiles exist
        for mapping in self.data_id_mappings:
            if mapping.profile and mapping.profile not in self.profiles:
                errors.append(f"Data ID 0x{mapping.data_id:04X} references undefined profile {mapping.profile.name}")
        
        # Validate protection configurations
        for protection in self.protections:
            if protection.profile_config.profile not in self.profiles:
                errors.append(f"Protection '{protection.name}' uses undefined profile")
            
            # Validate offsets are within data length
            if protection.profile_config.crc_offset >= protection.profile_config.data_length:
                errors.append(f"Protection '{protection.name}': CRC offset exceeds data length")
            if protection.profile_config.counter_offset >= protection.profile_config.data_length:
                errors.append(f"Protection '{protection.name}': Counter offset exceeds data length")
        
        # Validate monitoring configurations have corresponding mappings
        monitoring_data_ids = {m.data_id for m in self.monitorings}
        mapping_data_ids = {m.data_id for m in self.data_id_mappings}
        
        for mon_id in monitoring_data_ids:
            if mon_id not in mapping_data_ids:
                errors.append(f"Monitoring for Data ID 0x{mon_id:04X} has no corresponding mapping")
        
        return errors


def get_recommended_profile(safety_level: str, data_length: int) -> E2EProfile:
    """
    Get recommended E2E profile based on safety level and data length.
    
    Args:
        safety_level: Safety integrity level (QM, ASIL_A, ASIL_B, ASIL_C, ASIL_D)
        data_length: Data payload length in bytes
        
    Returns:
        Recommended E2E profile
    """
    if safety_level == "ASIL_D":
        if data_length <= 16:
            return E2EProfile.PROFILE_5
        else:
            return E2EProfile.PROFILE_7
    elif safety_level in ("ASIL_C", "ASIL_B"):
        if data_length <= 8:
            return E2EProfile.PROFILE_2
        elif data_length <= 16:
            return E2EProfile.PROFILE_5
        else:
            return E2EProfile.PROFILE_7
    else:  # QM, ASIL_A
        if data_length <= 8:
            return E2EProfile.PROFILE_1
        elif data_length <= 16:
            return E2EProfile.PROFILE_4
        else:
            return E2EProfile.PROFILE_7