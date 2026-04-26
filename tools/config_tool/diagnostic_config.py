"""
Diagnostic Configuration Module for DDS Configuration Tool.

This module provides configuration management for UDS diagnostic services,
DCM (Diagnostic Communication Manager) parameters, DID database, and security policies.

Author: Hermes Agent
Version: 1.0.0
"""

import json
from dataclasses import dataclass, field, asdict
from enum import Enum, auto
from typing import List, Dict, Optional, Any, Union
from pathlib import Path


class DiagSessionType(Enum):
    """UDS Diagnostic Session Types per ISO 14229-1."""
    DEFAULT = 0x01
    PROGRAMMING = 0x02
    EXTENDED = 0x03
    SAFETY_SYSTEM = 0x04


class DiagServiceID(Enum):
    """UDS Service IDs per ISO 14229-1."""
    DIAGNOSTIC_SESSION_CONTROL = 0x10
    ECU_RESET = 0x11
    READ_DATA_BY_ID = 0x22
    WRITE_DATA_BY_ID = 0x2E
    ROUTINE_CONTROL = 0x31
    REQUEST_DOWNLOAD = 0x34
    REQUEST_UPLOAD = 0x35
    TRANSFER_DATA = 0x36
    REQUEST_TRANSFER_EXIT = 0x37
    TESTER_PRESENT = 0x3E
    CONTROL_DTC_SETTING = 0x85
    RESPONSE_ON_EVENT = 0x86


class DiagPriority(Enum):
    """Diagnostic Service Priority Levels."""
    P0 = "P0"  # Highest priority - session control
    P1 = "P1"  # High priority - read/write data
    P2 = "P2"  # Normal priority - configuration
    P3 = "P3"  # Low priority - diagnostic


@dataclass
class DiagServiceConfig:
    """Configuration for a single diagnostic service."""
    service_id: int
    name: str
    priority: DiagPriority
    enabled: bool = True
    supported_sessions: List[DiagSessionType] = field(default_factory=list)
    timeout_ms: int = 5000
    security_level_required: int = 0

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "service_id": f"0x{self.service_id:02X}",
            "name": self.name,
            "priority": self.priority.value,
            "enabled": self.enabled,
            "supported_sessions": [s.name for s in self.supported_sessions],
            "timeout_ms": self.timeout_ms,
            "security_level_required": self.security_level_required
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DiagServiceConfig':
        """Create from dictionary."""
        sessions = [DiagSessionType[s] for s in data.get("supported_sessions", [])]
        return cls(
            service_id=int(data["service_id"], 16),
            name=data["name"],
            priority=DiagPriority(data["priority"]),
            enabled=data.get("enabled", True),
            supported_sessions=sessions,
            timeout_ms=data.get("timeout_ms", 5000),
            security_level_required=data.get("security_level_required", 0)
        )


@dataclass
class DIDDefinition:
    """Data Identifier (DID) definition."""
    did: int
    name: str
    description: str
    data_length: int
    data_type: str
    read_security_level: int = 0
    write_security_level: int = 1
    read_callback: Optional[str] = None
    write_callback: Optional[str] = None
    default_value: Optional[Any] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "did": f"0x{self.did:04X}",
            "name": self.name,
            "description": self.description,
            "data_length": self.data_length,
            "data_type": self.data_type,
            "read_security_level": self.read_security_level,
            "write_security_level": self.write_security_level,
            "read_callback": self.read_callback,
            "write_callback": self.write_callback,
            "default_value": self.default_value
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DIDDefinition':
        """Create from dictionary."""
        return cls(
            did=int(data["did"], 16),
            name=data["name"],
            description=data.get("description", ""),
            data_length=data["data_length"],
            data_type=data["data_type"],
            read_security_level=data.get("read_security_level", 0),
            write_security_level=data.get("write_security_level", 1),
            read_callback=data.get("read_callback"),
            write_callback=data.get("write_callback"),
            default_value=data.get("default_value")
        )


@dataclass
class DTCConfig:
    """Diagnostic Trouble Code (DTC) configuration."""
    dtc: int
    severity: int
    fault_type: str
    debounce_counter_threshold: int = 3
    aging_counter_threshold: int = 40
    enable_freeze_frame: bool = True
    freeze_frame_records: int = 3
    enable_extended_data: bool = True

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "dtc": f"0x{self.dtc:06X}",
            "severity": self.severity,
            "fault_type": self.fault_type,
            "debounce_counter_threshold": self.debounce_counter_threshold,
            "aging_counter_threshold": self.aging_counter_threshold,
            "enable_freeze_frame": self.enable_freeze_frame,
            "freeze_frame_records": self.freeze_frame_records,
            "enable_extended_data": self.enable_extended_data
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DTCConfig':
        """Create from dictionary."""
        return cls(
            dtc=int(data["dtc"], 16),
            severity=data["severity"],
            fault_type=data["fault_type"],
            debounce_counter_threshold=data.get("debounce_counter_threshold", 3),
            aging_counter_threshold=data.get("aging_counter_threshold", 40),
            enable_freeze_frame=data.get("enable_freeze_frame", True),
            freeze_frame_records=data.get("freeze_frame_records", 3),
            enable_extended_data=data.get("enable_extended_data", True)
        )


@dataclass
class SecurityPolicy:
    """Diagnostic security access policy."""
    level: int
    name: str
    seed_length: int = 4
    key_length: int = 4
    max_attempts: int = 3
    delay_timer_ms: int = 10000
    algorithm: str = "default"
    key_derivation_seed: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "level": self.level,
            "name": self.name,
            "seed_length": self.seed_length,
            "key_length": self.key_length,
            "max_attempts": self.max_attempts,
            "delay_timer_ms": self.delay_timer_ms,
            "algorithm": self.algorithm,
            "key_derivation_seed": self.key_derivation_seed
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'SecurityPolicy':
        """Create from dictionary."""
        return cls(
            level=data["level"],
            name=data["name"],
            seed_length=data.get("seed_length", 4),
            key_length=data.get("key_length", 4),
            max_attempts=data.get("max_attempts", 3),
            delay_timer_ms=data.get("delay_timer_ms", 10000),
            algorithm=data.get("algorithm", "default"),
            key_derivation_seed=data.get("key_derivation_seed")
        )


@dataclass
class DCMParams:
    """DCM (Diagnostic Communication Manager) parameters."""
    ecu_id: int
    logical_address: int
    functional_address: int
    max_response_length: int = 4095
    p2_timeout_ms: int = 50
    p2_star_timeout_ms: int = 5000
    s3_timeout_ms: int = 2000
    default_session_timeout_ms: int = 5000
    tester_present_required: bool = True
    tester_present_period_ms: int = 2000

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "ecu_id": f"0x{self.ecu_id:04X}",
            "logical_address": f"0x{self.logical_address:04X}",
            "functional_address": f"0x{self.functional_address:04X}",
            "max_response_length": self.max_response_length,
            "p2_timeout_ms": self.p2_timeout_ms,
            "p2_star_timeout_ms": self.p2_star_timeout_ms,
            "s3_timeout_ms": self.s3_timeout_ms,
            "default_session_timeout_ms": self.default_session_timeout_ms,
            "tester_present_required": self.tester_present_required,
            "tester_present_period_ms": self.tester_present_period_ms
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DCMParams':
        """Create from dictionary."""
        return cls(
            ecu_id=int(data["ecu_id"], 16),
            logical_address=int(data["logical_address"], 16),
            functional_address=int(data["functional_address"], 16),
            max_response_length=data.get("max_response_length", 4095),
            p2_timeout_ms=data.get("p2_timeout_ms", 50),
            p2_star_timeout_ms=data.get("p2_star_timeout_ms", 5000),
            s3_timeout_ms=data.get("s3_timeout_ms", 2000),
            default_session_timeout_ms=data.get("default_session_timeout_ms", 5000),
            tester_present_required=data.get("tester_present_required", True),
            tester_present_period_ms=data.get("tester_present_period_ms", 2000)
        )


@dataclass
class DoIPConfig:
    """DoIP (Diagnostic over IP) configuration."""
    enabled: bool = True
    local_port: int = 13400
    discovery_port: int = 13400
    max_connections: int = 8
    routing_activation_timeout_ms: int = 5000
    alive_check_interval_ms: int = 500
    vehicle_announcement_interval_ms: int = 500
    vehicle_announcement_count: int = 3
    vin: str = ""
    eid: str = ""
    gid: str = ""

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "enabled": self.enabled,
            "local_port": self.local_port,
            "discovery_port": self.discovery_port,
            "max_connections": self.max_connections,
            "routing_activation_timeout_ms": self.routing_activation_timeout_ms,
            "alive_check_interval_ms": self.alive_check_interval_ms,
            "vehicle_announcement_interval_ms": self.vehicle_announcement_interval_ms,
            "vehicle_announcement_count": self.vehicle_announcement_count,
            "vin": self.vin,
            "eid": self.eid,
            "gid": self.gid
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DoIPConfig':
        """Create from dictionary."""
        return cls(
            enabled=data.get("enabled", True),
            local_port=data.get("local_port", 13400),
            discovery_port=data.get("discovery_port", 13400),
            max_connections=data.get("max_connections", 8),
            routing_activation_timeout_ms=data.get("routing_activation_timeout_ms", 5000),
            alive_check_interval_ms=data.get("alive_check_interval_ms", 500),
            vehicle_announcement_interval_ms=data.get("vehicle_announcement_interval_ms", 500),
            vehicle_announcement_count=data.get("vehicle_announcement_count", 3),
            vin=data.get("vin", ""),
            eid=data.get("eid", ""),
            gid=data.get("gid", "")
        )


class DiagnosticConfig:
    """
    Complete diagnostic configuration manager.
    
    Manages DCM parameters, diagnostic services, DID database,
    DTC configurations, security policies, and DoIP settings.
    """

    def __init__(self):
        self.dcm_params: Optional[DCMParams] = None
        self.services: List[DiagServiceConfig] = []
        self.dids: List[DIDDefinition] = []
        self.dtcs: List[DTCConfig] = []
        self.security_policies: List[SecurityPolicy] = []
        self.doip_config: DoIPConfig = DoIPConfig()
        self.supported_sessions: List[DiagSessionType] = [
            DiagSessionType.DEFAULT,
            DiagSessionType.EXTENDED
        ]

    def set_dcm_params(self, params: DCMParams) -> None:
        """Set DCM parameters."""
        self.dcm_params = params

    def add_service(self, service: DiagServiceConfig) -> None:
        """Add a diagnostic service."""
        self.services.append(service)

    def remove_service(self, service_id: int) -> bool:
        """Remove a diagnostic service by ID."""
        for i, svc in enumerate(self.services):
            if svc.service_id == service_id:
                self.services.pop(i)
                return True
        return False

    def add_did(self, did: DIDDefinition) -> None:
        """Add a DID definition."""
        self.dids.append(did)

    def remove_did(self, did_id: int) -> bool:
        """Remove a DID by ID."""
        for i, d in enumerate(self.dids):
            if d.did == did_id:
                self.dids.pop(i)
                return True
        return False

    def add_dtc(self, dtc: DTCConfig) -> None:
        """Add a DTC configuration."""
        self.dtcs.append(dtc)

    def add_security_policy(self, policy: SecurityPolicy) -> None:
        """Add a security policy."""
        self.security_policies.append(policy)

    def set_doip_config(self, config: DoIPConfig) -> None:
        """Set DoIP configuration."""
        self.doip_config = config

    def get_default_services(self) -> List[DiagServiceConfig]:
        """Get default diagnostic service configurations."""
        return [
            DiagServiceConfig(
                service_id=DiagServiceID.DIAGNOSTIC_SESSION_CONTROL.value,
                name="DiagnosticSessionControl",
                priority=DiagPriority.P0,
                supported_sessions=[DiagSessionType.DEFAULT],
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.ECU_RESET.value,
                name="ECUReset",
                priority=DiagPriority.P0,
                supported_sessions=[
                    DiagSessionType.DEFAULT,
                    DiagSessionType.EXTENDED,
                    DiagSessionType.PROGRAMMING
                ],
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.READ_DATA_BY_ID.value,
                name="ReadDataByIdentifier",
                priority=DiagPriority.P1,
                supported_sessions=[
                    DiagSessionType.DEFAULT,
                    DiagSessionType.EXTENDED
                ],
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.WRITE_DATA_BY_ID.value,
                name="WriteDataByIdentifier",
                priority=DiagPriority.P1,
                supported_sessions=[DiagSessionType.EXTENDED],
                security_level_required=1,
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.ROUTINE_CONTROL.value,
                name="RoutineControl",
                priority=DiagPriority.P1,
                supported_sessions=[DiagSessionType.EXTENDED],
                security_level_required=1,
                timeout_ms=10000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.REQUEST_DOWNLOAD.value,
                name="RequestDownload",
                priority=DiagPriority.P1,
                supported_sessions=[DiagSessionType.PROGRAMMING],
                security_level_required=2,
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.TRANSFER_DATA.value,
                name="TransferData",
                priority=DiagPriority.P1,
                supported_sessions=[DiagSessionType.PROGRAMMING],
                security_level_required=2,
                timeout_ms=10000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.REQUEST_TRANSFER_EXIT.value,
                name="RequestTransferExit",
                priority=DiagPriority.P1,
                supported_sessions=[DiagSessionType.PROGRAMMING],
                security_level_required=2,
                timeout_ms=5000
            ),
            DiagServiceConfig(
                service_id=DiagServiceID.TESTER_PRESENT.value,
                name="TesterPresent",
                priority=DiagPriority.P0,
                supported_sessions=[
                    DiagSessionType.DEFAULT,
                    DiagSessionType.EXTENDED,
                    DiagSessionType.PROGRAMMING
                ],
                timeout_ms=2000
            )
        ]

    def to_dict(self) -> Dict[str, Any]:
        """Convert entire configuration to dictionary."""
        return {
            "dcm_params": self.dcm_params.to_dict() if self.dcm_params else None,
            "services": [s.to_dict() for s in self.services],
            "dids": [d.to_dict() for d in self.dids],
            "dtcs": [d.to_dict() for d in self.dtcs],
            "security_policies": [s.to_dict() for s in self.security_policies],
            "doip_config": self.doip_config.to_dict(),
            "supported_sessions": [s.name for s in self.supported_sessions]
        }

    def from_dict(self, data: Dict[str, Any]) -> None:
        """Load configuration from dictionary."""
        if data.get("dcm_params"):
            self.dcm_params = DCMParams.from_dict(data["dcm_params"])
        
        self.services = [DiagServiceConfig.from_dict(s) for s in data.get("services", [])]
        self.dids = [DIDDefinition.from_dict(d) for d in data.get("dids", [])]
        self.dtcs = [DTCConfig.from_dict(d) for d in data.get("dtcs", [])]
        self.security_policies = [SecurityPolicy.from_dict(s) for s in data.get("security_policies", [])]
        
        if data.get("doip_config"):
            self.doip_config = DoIPConfig.from_dict(data["doip_config"])
        
        if data.get("supported_sessions"):
            self.supported_sessions = [DiagSessionType[s] for s in data["supported_sessions"]]

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
        Validate the diagnostic configuration.
        
        Returns:
            List of validation error messages (empty if valid)
        """
        errors = []
        
        if not self.dcm_params:
            errors.append("DCM parameters not configured")
        
        if not self.services:
            errors.append("No diagnostic services configured")
        
        # Check for duplicate service IDs
        service_ids = [s.service_id for s in self.services]
        if len(service_ids) != len(set(service_ids)):
            errors.append("Duplicate service IDs found")
        
        # Check for duplicate DID IDs
        did_ids = [d.did for d in self.dids]
        if len(did_ids) != len(set(did_ids)):
            errors.append("Duplicate DID IDs found")
        
        # Validate security policy levels
        policy_levels = [p.level for p in self.security_policies]
        if len(policy_levels) != len(set(policy_levels)):
            errors.append("Duplicate security policy levels found")
        
        # Validate service references
        for svc in self.services:
            if svc.security_level_required > 0:
                if svc.security_level_required not in policy_levels:
                    errors.append(f"Service {svc.name} references non-existent security level {svc.security_level_required}")
        
        return errors


# Common DID definitions
COMMON_DIDS = {
    0xF180: ("BootSoftwareIdentification", "Boot software version"),
    0xF181: ("ApplicationSoftwareIdentification", "Application software version"),
    0xF182: ("ApplicationDataIdentification", "Application data version"),
    0xF183: ("BootSoftwareFingerprint", "Boot software fingerprint"),
    0xF184: ("ApplicationSoftwareFingerprint", "Application software fingerprint"),
    0xF185: ("ApplicationDataFingerprint", "Application data fingerprint"),
    0xF186: ("ActiveDiagnosticSession", "Current diagnostic session"),
    0xF187: ("VehicleManufacturerSparePartNumber", "Spare part number"),
    0xF188: ("VehicleManufacturerECUSoftwareNumber", "ECU software number"),
    0xF189: ("VehicleManufacturerECUSoftwareVersionNumber", "ECU software version"),
    0xF18A: ("SystemNameOrEngineType", "System name"),
    0xF18B: ("RepairShopCodeOrTesterSerialNumber", "Repair shop code"),
    0xF18C: ("ProgrammingDate", "Programming date"),
    0xF190: ("VIN", "Vehicle Identification Number"),
    0xF191: ("VehicleManufacturerECUHardwareNumber", "ECU hardware number"),
    0xF192: ("VehicleManufacturerECUHardwareVersionNumber", "ECU hardware version"),
    0xF193: ("VehicleManufacturerECUSerialNumber", "ECU serial number"),
}


def create_common_did(did_id: int) -> Optional[DIDDefinition]:
    """Create a common DID definition by ID."""
    if did_id in COMMON_DIDS:
        name, desc = COMMON_DIDS[did_id]
        return DIDDefinition(
            did=did_id,
            name=name,
            description=desc,
            data_length=16,
            data_type="uint8_array"
        )
    return None