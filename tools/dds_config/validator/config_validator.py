"""
DDS Configuration Validator

Validates DDS configuration for consistency and correctness.
Checks domain settings, topics, QoS profiles, and AUTOSAR mappings.
"""

from typing import Dict, Any, List, Optional, Set, Tuple
from dataclasses import dataclass, field
import re


@dataclass
class ValidationResult:
    """Result of configuration validation."""
    is_valid: bool = True
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    
    def add_error(self, message: str) -> None:
        """Add error message."""
        self.errors.append(message)
        self.is_valid = False
    
    def add_warning(self, message: str) -> None:
        """Add warning message."""
        self.warnings.append(message)
    
    def merge(self, other: 'ValidationResult') -> None:
        """Merge another validation result."""
        self.errors.extend(other.errors)
        self.warnings.extend(other.warnings)
        if not other.is_valid:
            self.is_valid = False


class ConfigValidator:
    """Validator for DDS configuration."""
    
    # Valid DDS domain IDs (0-232)
    MAX_DOMAIN_ID = 232
    
    # Valid transport types
    VALID_TRANSPORTS = {'UDPv4', 'UDPv6', 'SHM', 'TCPv4', 'TCPv6', 'INTRA', 'SHM+UDP'}
    
    # Valid discovery protocols
    VALID_DISCOVERY_PROTOCOLS = {'SIMPLE', 'STATIC', 'DYNAMIC', 'SERVER', 'RTPS_AUTO'}
    
    # Valid QoS kinds
    VALID_RELIABILITY_KINDS = {'BEST_EFFORT', 'RELIABLE'}
    VALID_DURABILITY_KINDS = {'VOLATILE', 'TRANSIENT_LOCAL', 'TRANSIENT', 'PERSISTENT'}
    VALID_HISTORY_KINDS = {'KEEP_LAST', 'KEEP_ALL'}
    VALID_LIVELINESS_KINDS = {'AUTOMATIC', 'MANUAL_BY_PARTICIPANT', 'MANUAL_BY_TOPIC'}
    VALID_TOPIC_KINDS = {'STANDARD', 'SIGNAL', 'SERVICE', 'EVENT', 'FIELD'}
    
    # Valid E2E profiles
    VALID_E2E_PROFILES = {'P01', 'P02', 'P04', 'P05', 'P06', 'P07', 'P11', 'P22'}
    
    def __init__(self):
        self.result = ValidationResult()
    
    def validate(self, config: Dict[str, Any]) -> ValidationResult:
        """
        Validate complete DDS configuration.
        
        Args:
            config: Configuration dictionary
            
        Returns:
            ValidationResult with errors and warnings
        """
        self.result = ValidationResult()
        
        # Validate domain configuration
        if 'domain' in config:
            self._validate_domain(config['domain'])
        
        # Validate topics
        if 'topics' in config:
            self._validate_topics(config['topics'])
        
        # Validate QoS profiles
        if 'qos_profiles' in config:
            self._validate_qos_profiles(config['qos_profiles'])
        
        # Validate transports
        if 'transports' in config:
            self._validate_transports(config['transports'])
        
        # Validate security
        if 'security' in config:
            self._validate_security(config['security'])
        
        # Validate AUTOSAR
        if 'autosar' in config:
            self._validate_autosar(config['autosar'])
        
        # Cross-reference validation
        self._validate_cross_references(config)
        
        return self.result
    
    def _validate_domain(self, domain: Dict[str, Any]) -> None:
        """Validate domain configuration."""
        # Domain ID
        domain_id = domain.get('domain_id', 0)
        if not isinstance(domain_id, int):
            self.result.add_error(f"Domain ID must be an integer, got {type(domain_id)}")
        elif domain_id < 0 or domain_id > self.MAX_DOMAIN_ID:
            self.result.add_error(f"Domain ID {domain_id} out of range (0-{self.MAX_DOMAIN_ID})")
        
        # Discovery
        if 'discovery' in domain:
            self._validate_discovery(domain['discovery'])
        
        # Transport
        if 'transport' in domain:
            self._validate_transport_config(domain['transport'])
        
        # Resources
        if 'resources' in domain:
            self._validate_resources(domain['resources'])
    
    def _validate_discovery(self, discovery: Dict[str, Any]) -> None:
        """Validate discovery configuration."""
        # Protocol
        protocol = discovery.get('protocol', 'SIMPLE')
        if protocol not in self.VALID_DISCOVERY_PROTOCOLS:
            self.result.add_error(f"Invalid discovery protocol: {protocol}")
        
        # Lease duration
        lease_duration = discovery.get('lease_duration', 10)
        if not isinstance(lease_duration, (int, float)) or lease_duration <= 0:
            self.result.add_error(f"Invalid lease duration: {lease_duration}")
        
        # Announce period
        announce_period = discovery.get('announce_period', 5)
        if not isinstance(announce_period, (int, float)) or announce_period <= 0:
            self.result.add_error(f"Invalid announce period: {announce_period}")
        
        # Multicast
        if 'multicast' in discovery:
            self._validate_multicast(discovery['multicast'])
    
    def _validate_multicast(self, multicast: Dict[str, Any]) -> None:
        """Validate multicast configuration."""
        # Address
        address = multicast.get('address', '')
        if address and not self._is_valid_multicast_address(address):
            self.result.add_error(f"Invalid multicast address: {address}")
        
        # Port
        port = multicast.get('port', 7400)
        if not isinstance(port, int) or port < 1024 or port > 65535:
            self.result.add_error(f"Invalid multicast port: {port}")
    
    def _validate_transport_config(self, transport: Dict[str, Any]) -> None:
        """Validate transport configuration."""
        default = transport.get('default', 'UDPv4')
        if default not in self.VALID_TRANSPORTS:
            self.result.add_error(f"Invalid default transport: {default}")
    
    def _validate_resources(self, resources: Dict[str, Any]) -> None:
        """Validate resource limits."""
        # Max participants
        max_participants = resources.get('max_participants', 10)
        if not isinstance(max_participants, int) or max_participants < 1:
            self.result.add_error(f"Invalid max_participants: {max_participants}")
        
        # Max domains
        max_domains = resources.get('max_domains', 1)
        if not isinstance(max_domains, int) or max_domains < 1:
            self.result.add_error(f"Invalid max_domains: {max_domains}")
    
    def _validate_topics(self, topics: List[Dict[str, Any]]) -> None:
        """Validate topics configuration."""
        topic_names: Set[str] = set()
        
        for i, topic in enumerate(topics):
            prefix = f"Topic[{i}]"
            
            # Name
            name = topic.get('name', '')
            if not name:
                self.result.add_error(f"{prefix}: Topic name is required")
            elif not self._is_valid_identifier(name):
                self.result.add_error(f"{prefix}: Invalid topic name: {name}")
            elif name in topic_names:
                self.result.add_error(f"{prefix}: Duplicate topic name: {name}")
            else:
                topic_names.add(name)
            
            # Kind
            kind = topic.get('kind', 'STANDARD')
            if kind not in self.VALID_TOPIC_KINDS:
                self.result.add_error(f"{prefix}: Invalid topic kind: {kind}")
            
            # Data type
            data_type = topic.get('data_type', '')
            if not data_type:
                self.result.add_warning(f"{prefix}: Data type not specified for topic {name}")
            
            # QoS
            if 'qos' in topic:
                self._validate_topic_qos(f"{prefix}.qos", topic['qos'])
            
            # Security
            if 'security' in topic:
                self._validate_topic_security(f"{prefix}.security", topic['security'])
    
    def _validate_topic_qos(self, prefix: str, qos: Dict[str, Any]) -> None:
        """Validate topic QoS configuration."""
        # Reliability
        reliability = qos.get('reliability', 'RELIABLE')
        if reliability not in self.VALID_RELIABILITY_KINDS:
            self.result.add_error(f"{prefix}: Invalid reliability: {reliability}")
        
        # Durability
        durability = qos.get('durability', 'VOLATILE')
        if durability not in self.VALID_DURABILITY_KINDS:
            self.result.add_error(f"{prefix}: Invalid durability: {durability}")
        
        # History
        history_kind = qos.get('history_kind', 'KEEP_LAST')
        if history_kind not in self.VALID_HISTORY_KINDS:
            self.result.add_error(f"{prefix}: Invalid history kind: {history_kind}")
        
        # History depth
        history_depth = qos.get('history_depth', 1)
        if not isinstance(history_depth, int) or history_depth < 1:
            self.result.add_error(f"{prefix}: Invalid history depth: {history_depth}")
    
    def _validate_topic_security(self, prefix: str, security: Dict[str, Any]) -> None:
        """Validate topic security configuration."""
        # E2E profile
        if security.get('e2e_enabled', False):
            profile = security.get('e2e_profile', 'P01')
            if profile not in self.VALID_E2E_PROFILES:
                self.result.add_error(f"{prefix}: Invalid E2E profile: {profile}")
            
            # Data ID
            data_id = security.get('e2e_data_id', 0)
            if not isinstance(data_id, int) or data_id < 0 or data_id > 65535:
                self.result.add_error(f"{prefix}: Invalid E2E data ID: {data_id}")
    
    def _validate_qos_profiles(self, profiles: List[Dict[str, Any]]) -> None:
        """Validate QoS profiles."""
        profile_names: Set[str] = set()
        
        for i, profile in enumerate(profiles):
            prefix = f"QoSProfile[{i}]"
            
            # Name
            name = profile.get('name', '')
            if not name:
                self.result.add_error(f"{prefix}: Profile name is required")
            elif name in profile_names:
                self.result.add_error(f"{prefix}: Duplicate profile name: {name}")
            else:
                profile_names.add(name)
            
            # Reliability
            if 'reliability' in profile:
                rel = profile['reliability'].get('kind', 'RELIABLE')
                if rel not in self.VALID_RELIABILITY_KINDS:
                    self.result.add_error(f"{prefix}: Invalid reliability: {rel}")
            
            # Durability
            if 'durability' in profile:
                dur = profile['durability'].get('kind', 'VOLATILE')
                if dur not in self.VALID_DURABILITY_KINDS:
                    self.result.add_error(f"{prefix}: Invalid durability: {dur}")
            
            # History
            if 'history' in profile:
                hist_kind = profile['history'].get('kind', 'KEEP_LAST')
                if hist_kind not in self.VALID_HISTORY_KINDS:
                    self.result.add_error(f"{prefix}: Invalid history kind: {hist_kind}")
            
            # Liveliness
            if 'liveliness' in profile:
                live = profile['liveliness'].get('kind', 'AUTOMATIC')
                if live not in self.VALID_LIVELINESS_KINDS:
                    self.result.add_error(f"{prefix}: Invalid liveliness: {live}")
    
    def _validate_transports(self, transports: List[Dict[str, Any]]) -> None:
        """Validate transport configurations."""
        transport_names: Set[str] = set()
        
        for i, transport in enumerate(transports):
            prefix = f"Transport[{i}]"
            
            # Name
            name = transport.get('name', '')
            if not name:
                self.result.add_error(f"{prefix}: Transport name is required")
            elif name in transport_names:
                self.result.add_error(f"{prefix}: Duplicate transport name: {name}")
            else:
                transport_names.add(name)
            
            # Type
            ttype = transport.get('type', 'UDPv4')
            if ttype not in self.VALID_TRANSPORTS:
                self.result.add_error(f"{prefix}: Invalid transport type: {ttype}")
    
    def _validate_security(self, security: Dict[str, Any]) -> None:
        """Validate security configuration."""
        # Authentication plugin
        auth = security.get('authentication', '')
        if auth and not auth.startswith('DDS:Auth:'):
            self.result.add_warning(f"Security: Unusual authentication plugin format: {auth}")
        
        # Encryption algorithm
        encryption = security.get('encryption', '')
        valid_encryptions = {'AES-128-GCM', 'AES-256-GCM'}
        if encryption and encryption not in valid_encryptions:
            self.result.add_warning(f"Security: Unusual encryption algorithm: {encryption}")
    
    def _validate_autosar(self, autosar: Dict[str, Any]) -> None:
        """Validate AUTOSAR configuration."""
        # ECU ID
        ecu_id = autosar.get('ecu_id', '')
        if ecu_id and not self._is_valid_identifier(ecu_id):
            self.result.add_warning(f"AUTOSAR: Unusual ECU ID format: {ecu_id}")
        
        # BswM rules
        if 'bswm_rules' in autosar:
            for i, rule in enumerate(autosar['bswm_rules']):
                if 'name' not in rule:
                    self.result.add_error(f"AUTOSAR BswM rule[{i}]: Name is required")
    
    def _validate_cross_references(self, config: Dict[str, Any]) -> None:
        """Validate cross-references between configuration sections."""
        # Check that QoS profiles referenced by topics exist
        profile_names = {p.get('name', '') for p in config.get('qos_profiles', [])}
        
        for topic in config.get('topics', []):
            qos_profile = topic.get('qos_profile', '')
            if qos_profile and qos_profile not in profile_names:
                self.result.add_warning(
                    f"Topic '{topic.get('name')}': Referenced QoS profile '{qos_profile}' not found"
                )
        
        # Check for topics without QoS specification
        for topic in config.get('topics', []):
            if 'qos' not in topic and 'qos_profile' not in topic:
                self.result.add_warning(
                    f"Topic '{topic.get('name')}': No QoS configuration specified"
                )
    
    def _is_valid_identifier(self, name: str) -> bool:
        """Check if name is a valid C identifier."""
        if not name:
            return False
        # Must start with letter or underscore
        if not (name[0].isalpha() or name[0] == '_'):
            return False
        # Can contain letters, digits, and underscores
        return all(c.isalnum() or c == '_' for c in name)
    
    def _is_valid_multicast_address(self, address: str) -> bool:
        """Check if address is a valid multicast IP address."""
        # IPv4 multicast: 224.0.0.0 to 239.255.255.255
        pattern = r'^(22[4-9]|23[0-9])\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$'
        if not re.match(pattern, address):
            return False
        
        # Check each octet is 0-255
        octets = address.split('.')
        return all(0 <= int(o) <= 255 for o in octets)


class ConfigValidationError(Exception):
    """Exception for configuration validation errors."""
    pass
