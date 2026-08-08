"""
Schema Validator for DDS Configuration

This module provides comprehensive validation of DDS configuration files:
- JSON Schema validation
- Integrity checking
- Reference consistency validation
- QoS combination validity checking
"""

import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Any, Set, Tuple
from enum import Enum

from .config_model import (
    DDSConfig, Domain, Participant, Topic, QoS, 
    Transport, TSN, Security, SystemInfo,
    ReliabilityKind, DurabilityKind, HistoryKind
)


class Severity(Enum):
    """Validation severity levels"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class ValidationError:
    """Validation error with context"""
    message: str
    path: str
    severity: Severity = Severity.ERROR
    line: Optional[int] = None
    column: Optional[int] = None
    suggestion: Optional[str] = None
    
    def __str__(self) -> str:
        result = f"[{self.severity.value.upper()}] {self.path}: {self.message}"
        if self.suggestion:
            result += f"\n  Suggestion: {self.suggestion}"
        if self.line:
            result += f"\n  Location: line {self.line}"
            if self.column:
                result += f", column {self.column}"
        return result


@dataclass
class ValidationWarning:
    """Validation warning with context"""
    message: str
    path: str
    suggestion: Optional[str] = None
    
    def __str__(self) -> str:
        result = f"[WARNING] {self.path}: {self.message}"
        if self.suggestion:
            result += f"\n  Suggestion: {self.suggestion}"
        return result


@dataclass
class ValidationReport:
    """Complete validation report"""
    is_valid: bool = True
    errors: List[ValidationError] = field(default_factory=list)
    warnings: List[ValidationWarning] = field(default_factory=list)
    info: List[str] = field(default_factory=list)
    stats: Dict[str, int] = field(default_factory=dict)
    
    def add_error(self, message: str, path: str, **kwargs):
        """Add error to report"""
        self.errors.append(ValidationError(message, path, **kwargs))
        self.is_valid = False
    
    def add_warning(self, message: str, path: str, **kwargs):
        """Add warning to report"""
        self.warnings.append(ValidationWarning(message, path, **kwargs))
    
    def add_info(self, message: str):
        """Add info message"""
        self.info.append(message)
    
    def merge(self, other: "ValidationReport"):
        """Merge another report into this one"""
        self.errors.extend(other.errors)
        self.warnings.extend(other.warnings)
        self.info.extend(other.info)
        if not other.is_valid:
            self.is_valid = False
    
    def get_summary(self) -> str:
        """Get human-readable summary"""
        lines = [
            "=" * 60,
            "DDS Configuration Validation Report",
            "=" * 60,
        ]
        
        lines.append(f"\nStatus: {'VALID' if self.is_valid else 'INVALID'}")
        lines.append(f"Errors: {len(self.errors)}")
        lines.append(f"Warnings: {len(self.warnings)}")
        lines.append(f"Info: {len(self.info)}")
        
        if self.errors:
            lines.append("\n--- Errors ---")
            for error in self.errors:
                lines.append(str(error))
        
        if self.warnings:
            lines.append("\n--- Warnings ---")
            for warning in self.warnings:
                lines.append(str(warning))
        
        if self.info:
            lines.append("\n--- Info ---")
            for info in self.info:
                lines.append(f"  {info}")
        
        lines.append("\n" + "=" * 60)
        return '\n'.join(lines)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert report to dictionary"""
        return {
            "is_valid": self.is_valid,
            "error_count": len(self.errors),
            "warning_count": len(self.warnings),
            "errors": [
                {
                    "message": e.message,
                    "path": e.path,
                    "severity": e.severity.value,
                    "line": e.line,
                    "column": e.column,
                    "suggestion": e.suggestion
                }
                for e in self.errors
            ],
            "warnings": [
                {
                    "message": w.message,
                    "path": w.path,
                    "suggestion": w.suggestion
                }
                for w in self.warnings
            ],
            "info": self.info,
            "stats": self.stats
        }


class SchemaValidator:
    """
    DDS Configuration Schema Validator
    
    Validates DDS configuration against:
    - JSON Schema definition
    - Semantic constraints
    - Reference consistency
    - QoS policy combinations
    """
    
    # Valid QoS policy values
    QOS_POLICIES = {
        "reliability": ["BEST_EFFORT", "RELIABLE"],
        "durability": ["VOLATILE", "TRANSIENT_LOCAL", "TRANSIENT", "PERSISTENT"],
        "history": ["KEEP_LAST", "KEEP_ALL"],
        "liveliness": ["AUTOMATIC", "MANUAL_BY_TOPIC", "MANUAL_BY_PARTICIPANT"],
        "destination_order": ["BY_RECEPTION_TIMESTAMP", "BY_SOURCE_TIMESTAMP"],
        "ownership": ["SHARED", "EXCLUSIVE"]
    }
    
    # Valid transport kinds
    TRANSPORT_KINDS = ["UDP", "TCP", "SHM", "TSN"]
    
    # IP address patterns
    IP_PATTERN = re.compile(
        r'^(\d{1,3}\.){3}\d{1,3}$|'
        r'^([0-9a-fA-F]{0,4}:){2,7}[0-9a-fA-F]{0,4}$'
    )
    
    MULTICAST_PATTERN = re.compile(
        r'^22[4-9]\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.'
        r'(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.'
        r'(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$|'
        r'^23[0-9]\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.'
        r'(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.'
        r'(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$'
    )
    
    def __init__(self, schema_path: Optional[str] = None):
        """
        Initialize validator
        
        Args:
            schema_path: Optional path to JSON Schema file
        """
        self.schema_path = schema_path
        self.schema = None
        self._load_schema()
    
    def _load_schema(self):
        """Load JSON Schema if available"""
        if self.schema_path and Path(self.schema_path).exists():
            try:
                with open(self.schema_path, 'r') as f:
                    self.schema = json.load(f)
            except (json.JSONDecodeError, IOError) as e:
                print(f"Warning: Could not load schema: {e}")
    
    def validate(self, config: DDSConfig, strict: bool = False) -> ValidationReport:
        """
        Validate DDS configuration
        
        Args:
            config: DDSConfig object to validate
            strict: If True, treat warnings as errors
            
        Returns:
            ValidationReport with results
        """
        report = ValidationReport()
        
        # Collect statistics
        self._collect_stats(config, report)
        
        # Validate system
        self._validate_system(config.system, report)
        
        # Validate domains
        self._validate_domains(config.domains, report)
        
        # Validate transport
        self._validate_transport(config.transport, report)
        
        # Validate TSN
        self._validate_tsn(config.tsn, report)
        
        # Validate security
        self._validate_security(config.security, report)
        
        # Validate QoS profiles
        self._validate_qos_profiles(config.qos_profiles, report)
        
        # Validate topics
        self._validate_topics(config.topics, report)
        
        # Cross-reference validation
        self._validate_cross_references(config, report)
        
        # QoS compatibility checks
        self._validate_qos_compatibility(config, report)
        
        if strict and report.warnings:
            report.is_valid = False
        
        return report
    
    def _collect_stats(self, config: DDSConfig, report: ValidationReport):
        """Collect configuration statistics"""
        report.stats = {
            "domains": len(config.domains),
            "participants": config.get_participant_count(),
            "topics": len(config.get_all_topics()),
            "qos_profiles": len(config.qos_profiles),
            "tsn_streams": len(config.tsn.streams) if config.tsn.enabled else 0
        }
        
        # Count publishers and subscribers
        publishers = 0
        subscribers = 0
        for domain in config.domains:
            for participant in domain.participants:
                publishers += len(participant.publishers)
                subscribers += len(participant.subscribers)
        
        report.stats["publishers"] = publishers
        report.stats["subscribers"] = subscribers
    
    def _validate_system(self, system: SystemInfo, report: ValidationReport):
        """Validate system section"""
        if not system.name:
            report.add_error(
                "System name is required",
                "system.name",
                suggestion="Add a system name, e.g., 'name: MySystem'"
            )
        
        if not system.name.replace('_', '').replace('-', '').isalnum():
            report.add_warning(
                "System name should be alphanumeric with underscores or hyphens",
                "system.name",
                suggestion="Use names like 'ADAS_System' or 'adas-system'"
            )
        
        if not system.version:
            report.add_warning(
                "System version is not specified",
                "system.version",
                suggestion="Add a version, e.g., 'version: 1.0.0'"
            )
    
    def _validate_domains(self, domains: List[Domain], report: ValidationReport):
        """Validate domains configuration"""
        if not domains:
            report.add_error(
                "At least one domain is required",
                "domains",
                suggestion="Add a domain configuration"
            )
            return
        
        domain_ids: Set[int] = set()
        domain_names: Set[str] = set()
        
        for idx, domain in enumerate(domains):
            path = f"domains[{idx}]"
            
            # Check domain ID
            if domain.id < 0 or domain.id > 232:
                report.add_error(
                    f"Domain ID {domain.id} out of valid range (0-232)",
                    f"{path}.id",
                    suggestion="Use a domain ID between 0 and 232"
                )
            
            # Check uniqueness
            if domain.id in domain_ids:
                report.add_error(
                    f"Duplicate domain ID: {domain.id}",
                    f"{path}.id"
                )
            domain_ids.add(domain.id)
            
            if domain.name in domain_names:
                report.add_error(
                    f"Duplicate domain name: {domain.name}",
                    f"{path}.name"
                )
            domain_names.add(domain.name)
            
            # Validate participants
            self._validate_participants(domain.participants, path, report)
    
    def _validate_participants(self, participants: List[Participant], 
                               parent_path: str, report: ValidationReport):
        """Validate participants in a domain"""
        participant_names: Set[str] = set()
        
        for idx, participant in enumerate(participants):
            path = f"{parent_path}.participants[{idx}]"
            
            if not participant.name:
                report.add_error(
                    "Participant name is required",
                    f"{path}.name"
                )
                continue
            
            if participant.name in participant_names:
                report.add_error(
                    f"Duplicate participant name: {participant.name}",
                    f"{path}.name"
                )
            participant_names.add(participant.name)
            
            # Validate publishers
            self._validate_publishers(participant.publishers, path, report)
            
            # Validate subscribers
            self._validate_subscribers(participant.subscribers, path, report)
    
    def _validate_publishers(self, publishers: List[Any], 
                             parent_path: str, report: ValidationReport):
        """Validate publishers"""
        for idx, pub in enumerate(publishers):
            path = f"{parent_path}.publishers[{idx}]"
            
            if not pub.topic:
                report.add_error(
                    "Publisher topic is required",
                    f"{path}.topic"
                )
            
            if pub.qos:
                self._validate_qos(pub.qos, f"{path}.qos", report)
    
    def _validate_subscribers(self, subscribers: List[Any], 
                              parent_path: str, report: ValidationReport):
        """Validate subscribers"""
        for idx, sub in enumerate(subscribers):
            path = f"{parent_path}.subscribers[{idx}]"
            
            if not sub.topic:
                report.add_error(
                    "Subscriber topic is required",
                    f"{path}.topic"
                )
            
            if sub.qos:
                self._validate_qos(sub.qos, f"{path}.qos", report)
    
    def _validate_qos(self, qos: QoS, path: str, report: ValidationReport):
        """Validate QoS configuration"""
        # Validate history depth
        if qos.history.depth < 1:
            report.add_warning(
                f"History depth ({qos.history.depth}) should be at least 1",
                f"{path}.history.depth",
                suggestion="Set depth to 1 or higher"
            )
        
        if qos.history.depth > 1000:
            report.add_warning(
                f"History depth ({qos.history.depth}) is very large",
                f"{path}.history.depth",
                suggestion="Consider reducing memory usage"
            )
    
    def _validate_transport(self, transport: Transport, report: ValidationReport):
        """Validate transport configuration"""
        # Get transport kind as string
        kind_value = transport.kind.value if hasattr(transport.kind, 'value') else str(transport.kind)
        
        if kind_value not in self.TRANSPORT_KINDS:
            report.add_error(
                f"Invalid transport kind: {kind_value}",
                "transport.kind",
                suggestion=f"Use one of: {', '.join(self.TRANSPORT_KINDS)}"
            )
        
        # Validate port range
        if transport.port_base < 1024:
            report.add_warning(
                f"Port base ({transport.port_base}) is in privileged range",
                "transport.port_base",
                suggestion="Use ports >= 1024 to avoid needing root privileges"
            )
        
        if transport.port_base + transport.port_range > 65535:
            report.add_error(
                "Port range exceeds maximum port number (65535)",
                "transport.port_range"
            )
        
        # Validate multicast address if specified
        if transport.multicast_address:
            if not self.IP_PATTERN.match(transport.multicast_address):
                report.add_error(
                    f"Invalid multicast address: {transport.multicast_address}",
                    "transport.multicast_address",
                    suggestion="Use valid IPv4 (239.255.x.x) or IPv6 address"
                )
            elif not self.MULTICAST_PATTERN.match(transport.multicast_address):
                report.add_warning(
                    "Multicast address may not be in valid multicast range",
                    "transport.multicast_address",
                    suggestion="Use addresses in range 224.0.0.0-239.255.255.255"
                )
    
    def _validate_tsn(self, tsn: TSN, report: ValidationReport):
        """Validate TSN configuration"""
        if not tsn.enabled:
            return
        
        for idx, stream in enumerate(tsn.streams):
            path = f"tsn.streams[{idx}]"
            
            if not stream.name:
                report.add_error(
                    "TSN stream name is required",
                    f"{path}.name"
                )
            
            if stream.priority < 0 or stream.priority > 7:
                report.add_error(
                    f"TSN priority {stream.priority} out of range (0-7)",
                    f"{path}.priority"
                )
            
            if stream.bandwidth < 0:
                report.add_error(
                    "TSN bandwidth must be non-negative",
                    f"{path}.bandwidth"
                )
            
            if stream.max_latency < 0:
                report.add_error(
                    "TSN max_latency must be non-negative",
                    f"{path}.max_latency"
                )
    
    def _validate_security(self, security: Security, report: ValidationReport):
        """Validate security configuration"""
        if not security.enabled:
            return
        
        # If security is enabled, check for certificate files
        if security.authentication:
            if not security.identity_certificate:
                report.add_warning(
                    "Authentication enabled but no identity certificate specified",
                    "security.identity_certificate",
                    suggestion="Add path to identity certificate"
                )
            
            if not security.private_key:
                report.add_warning(
                    "Authentication enabled but no private key specified",
                    "security.private_key",
                    suggestion="Add path to private key"
                )
    
    def _validate_qos_profiles(self, profiles: Dict[str, QoS], report: ValidationReport):
        """Validate QoS profiles"""
        for name, qos in profiles.items():
            path = f"qos_profiles.{name}"
            self._validate_qos(qos, path, report)
    
    def _validate_topics(self, topics: List[Topic], report: ValidationReport):
        """Validate topic definitions"""
        topic_names: Set[str] = set()
        
        for idx, topic in enumerate(topics):
            path = f"topics[{idx}]"
            
            if not topic.name:
                report.add_error("Topic name is required", f"{path}.name")
                continue
            
            if topic.name in topic_names:
                report.add_error(
                    f"Duplicate topic name: {topic.name}",
                    f"{path}.name"
                )
            topic_names.add(topic.name)
            
            if not topic.type:
                report.add_warning(
                    f"Topic '{topic.name}' has no type specified",
                    f"{path}.type",
                    suggestion="Add a data type for the topic"
                )
    
    def _validate_cross_references(self, config: DDSConfig, report: ValidationReport):
        """Validate cross-references between entities"""
        # Collect all defined topics
        defined_topics: Set[str] = set()
        for topic in config.get_all_topics():
            defined_topics.add(topic.name)
        
        # Check publishers/subscribers reference valid topics
        for domain in config.domains:
            for participant in domain.participants:
                for pub in participant.publishers:
                    if pub.topic and pub.topic not in defined_topics:
                        # Topic used but not defined - this is just a warning
                        report.add_warning(
                            f"Topic '{pub.topic}' used but not defined in topics list",
                            f"domains.{domain.name}.participants.{participant.name}.publishers",
                            suggestion="Add topic definition to 'topics' section"
                        )
                
                for sub in participant.subscribers:
                    if sub.topic and sub.topic not in defined_topics:
                        report.add_warning(
                            f"Topic '{sub.topic}' used but not defined in topics list",
                            f"domains.{domain.name}.participants.{participant.name}.subscribers",
                            suggestion="Add topic definition to 'topics' section"
                        )
        
        # Check QoS profile references
        for domain in config.domains:
            for participant in domain.participants:
                if participant.qos_profile:
                    if participant.qos_profile not in config.qos_profiles:
                        report.add_error(
                            f"Referenced QoS profile not found: {participant.qos_profile}",
                            f"domains.{domain.name}.participants.{participant.name}.qos_profile"
                        )
    
    def _validate_qos_compatibility(self, config: DDSConfig, report: ValidationReport):
        """Validate QoS compatibility between publishers and subscribers"""
        # Build topic -> (publishers, subscribers) mapping
        topic_endpoints: Dict[str, Tuple[List[QoS], List[QoS]]] = {}
        
        for domain in config.domains:
            for participant in domain.participants:
                for pub in participant.publishers:
                    if pub.topic not in topic_endpoints:
                        topic_endpoints[pub.topic] = ([], [])
                    topic_endpoints[pub.topic][0].append(pub.qos)
                
                for sub in participant.subscribers:
                    if sub.topic not in topic_endpoints:
                        topic_endpoints[sub.topic] = ([], [])
                    topic_endpoints[sub.topic][1].append(sub.qos)
        
        # Check compatibility for each topic
        for topic_name, (pub_qos_list, sub_qos_list) in topic_endpoints.items():
            for pub_qos in pub_qos_list:
                for sub_qos in sub_qos_list:
                    self._check_qos_pair_compatibility(
                        topic_name, pub_qos, sub_qos, report
                    )
    
    def _check_qos_pair_compatibility(self, topic: str, pub_qos: QoS, 
                                      sub_qos: QoS, report: ValidationReport):
        """Check QoS compatibility between a publisher and subscriber"""
        # Reliability compatibility:
        # RELIABLE publisher can work with BEST_EFFORT subscriber
        # BEST_EFFORT publisher with RELIABLE subscriber is a problem
        if pub_qos.reliability == ReliabilityKind.BEST_EFFORT and \
           sub_qos.reliability == ReliabilityKind.RELIABLE:
            report.add_warning(
                f"QoS mismatch on topic '{topic}': "
                f"BEST_EFFORT publisher with RELIABLE subscriber",
                f"topic.{topic}.qos.reliability",
                suggestion="Change subscriber reliability to BEST_EFFORT "
                          "or publisher reliability to RELIABLE"
            )
        
        # Durability compatibility
        if pub_qos.durability.value not in ["VOLATILE", "TRANSIENT_LOCAL"]:
            if sub_qos.durability == DurabilityKind.VOLATILE:
                report.add_warning(
                    f"Durability mismatch on topic '{topic}': "
                    f"Non-volatile publisher with volatile subscriber",
                    f"topic.{topic}.qos.durability"
                )


def validate_config(config: DDSConfig, schema_path: Optional[str] = None,
                   strict: bool = False) -> ValidationReport:
    """
    Convenience function to validate DDS configuration
    
    Args:
        config: DDSConfig object to validate
        schema_path: Optional path to JSON Schema file
        strict: If True, treat warnings as errors
        
    Returns:
        ValidationReport with results
    """
    validator = SchemaValidator(schema_path=schema_path)
    return validator.validate(config, strict=strict)


def validate_yaml_file(file_path: str, schema_path: Optional[str] = None) -> ValidationReport:
    """
    Validate a YAML configuration file
    
    Args:
        file_path: Path to YAML file
        schema_path: Optional path to JSON Schema file
        
    Returns:
        ValidationReport with results
    """
    from .yaml_parser import parse_yaml_file, YAMLParseError
    
    report = ValidationReport()
    
    try:
        config, warnings = parse_yaml_file(file_path)
        for warning in warnings:
            report.add_warning(warning, "parser")
    except YAMLParseError as e:
        report.add_error(
            f"Parse error: {e.message}",
            e.context or "unknown",
            line=e.line,
            column=e.column
        )
        return report
    except FileNotFoundError as e:
        report.add_error(str(e), "file")
        return report
    
    validator = SchemaValidator(schema_path=schema_path)
    validation_report = validator.validate(config)
    report.merge(validation_report)
    
    return report
