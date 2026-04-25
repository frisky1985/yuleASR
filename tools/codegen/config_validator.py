#!/usr/bin/env python3
"""
DDS Configuration Validator
Validates DDS configuration files against schema and design rules
"""

import yaml
import json
import re
from typing import List, Dict, Optional, Any, Set, Tuple
from dataclasses import dataclass, field
from pathlib import Path
from enum import Enum


class ValidationLevel(Enum):
    """Validation severity levels"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class ValidationIssue:
    """Validation issue/warning"""
    level: ValidationLevel
    code: str
    message: str
    path: str  # JSON path to the issue
    suggestion: Optional[str] = None


@dataclass
class ValidationResult:
    """Validation result"""
    is_valid: bool
    issues: List[ValidationIssue] = field(default_factory=list)
    
    def add_issue(self, level: ValidationLevel, code: str, message: str, path: str, suggestion: Optional[str] = None):
        """Add a validation issue"""
        self.issues.append(ValidationIssue(level, code, message, path, suggestion))
        if level == ValidationLevel.ERROR:
            self.is_valid = False
    
    def get_errors(self) -> List[ValidationIssue]:
        """Get all error-level issues"""
        return [i for i in self.issues if i.level == ValidationLevel.ERROR]
    
    def get_warnings(self) -> List[ValidationIssue]:
        """Get all warning-level issues"""
        return [i for i in self.issues if i.level == ValidationLevel.WARNING]
    
    def to_dict(self) -> Dict:
        """Convert to dictionary"""
        return {
            'is_valid': self.is_valid,
            'error_count': len(self.get_errors()),
            'warning_count': len(self.get_warnings()),
            'issues': [
                {
                    'level': i.level.value,
                    'code': i.code,
                    'message': i.message,
                    'path': i.path,
                    'suggestion': i.suggestion
                }
                for i in self.issues
            ]
        }


class ConfigValidator:
    """
    DDS Configuration Validator
    
    Validates:
    1. Schema compliance (structure, types, required fields)
    2. Design rules (naming conventions, best practices)
    3. Cross-reference integrity (topic-type consistency)
    4. Performance constraints (buffer sizes, timing)
    5. Security policies
    """
    
    # Version pattern
    VERSION_PATTERN = re.compile(r'^(\d+)\.(\d+)\.(\d+)(?:-([a-zA-Z0-9]+))?$')
    
    # Valid QoS policy values
    VALID_RELIABILITY = {'BEST_EFFORT', 'RELIABLE'}
    VALID_DURABILITY = {'VOLATILE', 'TRANSIENT_LOCAL', 'TRANSIENT', 'PERSISTENT'}
    VALID_HISTORY = {'KEEP_LAST', 'KEEP_ALL'}
    VALID_OWNERSHIP = {'SHARED', 'EXCLUSIVE'}
    VALID_LIVELINESS = {'AUTOMATIC', 'MANUAL_BY_PARTICIPANT', 'MANUAL_BY_TOPIC'}
    VALID_DEST_ORDER = {'BY_RECEPTION_TIMESTAMP', 'BY_SOURCE_TIMESTAMP'}
    
    # Valid transport protocols
    VALID_PROTOCOLS = {'UDPv4', 'UDPv6', 'SHMEM', 'TCPv4', 'TCPv6'}
    
    # Valid log levels
    VALID_LOG_LEVELS = {'FATAL', 'ERROR', 'WARN', 'INFO', 'DEBUG', 'TRACE'}
    
    # Naming conventions
    TOPIC_NAME_PATTERN = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_/]*$')
    TYPE_NAME_PATTERN = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')
    IDENTIFIER_PATTERN = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')
    
    def __init__(self, schema_path: Optional[str] = None):
        self.schema_path = schema_path
        self.schema = None
        if schema_path:
            self._load_schema(schema_path)
    
    def _load_schema(self, schema_path: str):
        """Load JSON schema file"""
        try:
            with open(schema_path, 'r') as f:
                self.schema = yaml.safe_load(f)
        except Exception as e:
            print(f"Warning: Could not load schema from {schema_path}: {e}")
    
    def validate_file(self, config_path: str) -> ValidationResult:
        """Validate a configuration file"""
        try:
            with open(config_path, 'r') as f:
                if config_path.endswith('.json'):
                    config = json.load(f)
                else:
                    config = yaml.safe_load(f)
            return self.validate(config)
        except Exception as e:
            result = ValidationResult(is_valid=False)
            result.add_issue(
                ValidationLevel.ERROR,
                "FILE_ERROR",
                f"Could not parse configuration file: {e}",
                ""
            )
            return result
    
    def validate(self, config: Dict[str, Any]) -> ValidationResult:
        """Validate configuration dictionary"""
        result = ValidationResult(is_valid=True)
        
        # Basic structure validation
        self._validate_structure(config, result)
        
        # Validate system section
        if 'system' in config:
            self._validate_system(config['system'], result, "system")
        
        # Validate domains
        if 'domains' in config:
            self._validate_domains(config['domains'], result)
        
        # Validate topics
        if 'topics' in config:
            self._validate_topics(config['topics'], result)
        
        # Validate QoS profiles
        if 'qos_profiles' in config:
            self._validate_qos_profiles(config['qos_profiles'], result)
        
        # Validate participants
        if 'participants' in config:
            self._validate_participants(config['participants'], config.get('domains', []), result)
        
        # Validate types
        if 'types' in config:
            self._validate_types(config['types'], result)
        
        # Validate logging
        if 'logging' in config:
            self._validate_logging(config['logging'], result)
        
        # Validate monitoring
        if 'monitoring' in config:
            self._validate_monitoring(config['monitoring'], result)
        
        # Cross-reference validation
        self._validate_cross_references(config, result)
        
        # Design rule validation
        self._validate_design_rules(config, result)
        
        return result
    
    def _validate_structure(self, config: Dict, result: ValidationResult):
        """Validate basic structure"""
        # Check required fields
        required = ['version', 'system']
        for field in required:
            if field not in config:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_REQUIRED_FIELD",
                    f"Required field '{field}' is missing",
                    "",
                    f"Add '{field}' section to configuration"
                )
        
        # Check version format
        if 'version' in config:
            version = config['version']
            if not isinstance(version, str):
                result.add_issue(
                    ValidationLevel.ERROR,
                    "INVALID_VERSION_TYPE",
                    f"Version must be a string, got {type(version).__name__}",
                    "version"
                )
            elif not self.VERSION_PATTERN.match(version):
                result.add_issue(
                    ValidationLevel.ERROR,
                    "INVALID_VERSION_FORMAT",
                    f"Version '{version}' does not match semantic versioning (X.Y.Z)",
                    "version",
                    "Use format like '1.0.0' or '2.1.0-beta'"
                )
    
    def _validate_system(self, system: Dict, result: ValidationResult, path: str):
        """Validate system section"""
        if 'name' not in system:
            result.add_issue(
                ValidationLevel.ERROR,
                "MISSING_SYSTEM_NAME",
                "System name is required",
                path,
                "Add 'name' field to system section"
            )
        elif not isinstance(system['name'], str):
            result.add_issue(
                ValidationLevel.ERROR,
                "INVALID_SYSTEM_NAME_TYPE",
                "System name must be a string",
                f"{path}.name"
            )
        elif len(system['name']) > 256:
            result.add_issue(
                ValidationLevel.WARNING,
                "SYSTEM_NAME_TOO_LONG",
                "System name exceeds 256 characters",
                f"{path}.name"
            )
        
        # Validate version if present
        if 'version' in system:
            if not self.VERSION_PATTERN.match(system['version']):
                result.add_issue(
                    ValidationLevel.WARNING,
                    "INVALID_SYSTEM_VERSION",
                    "System version does not match semantic versioning",
                    f"{path}.version"
                )
    
    def _validate_domains(self, domains: List, result: ValidationResult):
        """Validate domain configurations"""
        if not isinstance(domains, list):
            result.add_issue(
                ValidationLevel.ERROR,
                "INVALID_DOMAINS_TYPE",
                "Domains must be a list",
                "domains"
            )
            return
        
        domain_ids = set()
        domain_names = set()
        
        for i, domain in enumerate(domains):
            path = f"domains[{i}]"
            
            # Check required fields
            if 'id' not in domain:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_DOMAIN_ID",
                    f"Domain at index {i} is missing 'id' field",
                    path
                )
            else:
                domain_id = domain['id']
                if not isinstance(domain_id, int):
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "INVALID_DOMAIN_ID_TYPE",
                        f"Domain ID must be an integer",
                        f"{path}.id"
                    )
                elif domain_id < 0 or domain_id > 230:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "DOMAIN_ID_OUT_OF_RANGE",
                        f"Domain ID {domain_id} is out of valid range (0-230)",
                        f"{path}.id",
                        "Use a domain ID between 0 and 230"
                    )
                elif domain_id in domain_ids:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "DUPLICATE_DOMAIN_ID",
                        f"Duplicate domain ID: {domain_id}",
                        f"{path}.id"
                    )
                else:
                    domain_ids.add(domain_id)
            
            # Check name uniqueness
            if 'name' in domain:
                name = domain['name']
                if name in domain_names:
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "DUPLICATE_DOMAIN_NAME",
                        f"Duplicate domain name: {name}",
                        f"{path}.name"
                    )
                else:
                    domain_names.add(name)
            
            # Validate discovery settings
            if 'discovery' in domain:
                self._validate_discovery(domain['discovery'], result, f"{path}.discovery")
            
            # Validate transport settings
            if 'transport' in domain:
                self._validate_transport(domain['transport'], result, f"{path}.transport")
            
            # Validate security settings
            if 'security' in domain:
                self._validate_security(domain['security'], result, f"{path}.security")
    
    def _validate_discovery(self, discovery: Dict, result: ValidationResult, path: str):
        """Validate discovery configuration"""
        if 'protocol' in discovery:
            if discovery['protocol'] not in self.VALID_PROTOCOLS:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "INVALID_DISCOVERY_PROTOCOL",
                    f"Invalid discovery protocol: {discovery['protocol']}",
                    f"{path}.protocol",
                    f"Valid protocols: {', '.join(self.VALID_PROTOCOLS)}"
                )
        
        # Validate time durations
        for time_field in ['lease_duration', 'liveliness_check_interval']:
            if time_field in discovery:
                if not self._is_valid_duration(discovery[time_field]):
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "INVALID_DURATION_FORMAT",
                        f"Invalid duration format for {time_field}",
                        f"{path}.{time_field}",
                        "Use format like '30s', '100ms', '5m'"
                    )
    
    def _validate_transport(self, transport: Dict, result: ValidationResult, path: str):
        """Validate transport configuration"""
        # Validate unicast addresses
        if 'unicast' in transport:
            if not isinstance(transport['unicast'], list):
                result.add_issue(
                    ValidationLevel.ERROR,
                    "INVALID_UNICAST_TYPE",
                    "Unicast must be a list of addresses",
                    f"{path}.unicast"
                )
            else:
                for i, addr in enumerate(transport['unicast']):
                    if not self._is_valid_ip_address(addr):
                        result.add_issue(
                            ValidationLevel.WARNING,
                            "INVALID_IP_ADDRESS",
                            f"Potentially invalid IP address: {addr}",
                            f"{path}.unicast[{i}]"
                        )
    
    def _validate_security(self, security: Dict, result: ValidationResult, path: str):
        """Validate security configuration"""
        if security.get('enabled', False):
            # Validate authentication settings
            if 'authentication' in security:
                auth = security['authentication']
                if 'plugin' in auth:
                    valid_plugins = {'DDS:Auth:PKI-DH', 'DDS:Auth:PSK'}
                    if auth['plugin'] not in valid_plugins:
                        result.add_issue(
                            ValidationLevel.WARNING,
                            "UNKNOWN_AUTH_PLUGIN",
                            f"Unknown authentication plugin: {auth['plugin']}",
                            f"{path}.authentication.plugin"
                        )
                
                # Check required certificate files
                required_files = ['identity_ca', 'identity_certificate', 'private_key']
                for file_field in required_files:
                    if file_field not in auth:
                        result.add_issue(
                            ValidationLevel.WARNING,
                            "MISSING_CERTIFICATE_FILE",
                            f"Security is enabled but {file_field} is not specified",
                            f"{path}.authentication",
                            f"Add {file_field} to authentication configuration"
                        )
    
    def _validate_topics(self, topics: List, result: ValidationResult):
        """Validate topic configurations"""
        if not isinstance(topics, list):
            result.add_issue(
                ValidationLevel.ERROR,
                "INVALID_TOPICS_TYPE",
                "Topics must be a list",
                "topics"
            )
            return
        
        topic_names = set()
        
        for i, topic in enumerate(topics):
            path = f"topics[{i}]"
            
            # Check required fields
            if 'name' not in topic:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_TOPIC_NAME",
                    f"Topic at index {i} is missing 'name' field",
                    path
                )
            else:
                name = topic['name']
                
                # Validate naming convention
                if not self.TOPIC_NAME_PATTERN.match(name):
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "INVALID_TOPIC_NAME",
                        f"Topic name '{name}' does not follow naming convention",
                        f"{path}.name",
                        "Use alphanumeric characters, underscores, and forward slashes only"
                    )
                
                # Check uniqueness
                if name in topic_names:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "DUPLICATE_TOPIC_NAME",
                        f"Duplicate topic name: {name}",
                        f"{path}.name"
                    )
                else:
                    topic_names.add(name)
            
            # Check type
            if 'type' not in topic:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_TOPIC_TYPE",
                    f"Topic '{topic.get('name', i)}' is missing 'type' field",
                    path
                )
            
            # Validate domain_id
            if 'domain_id' in topic:
                domain_id = topic['domain_id']
                if not isinstance(domain_id, int) or domain_id < 0 or domain_id > 230:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "INVALID_TOPIC_DOMAIN_ID",
                        f"Topic domain ID {domain_id} is invalid",
                        f"{path}.domain_id"
                    )
    
    def _validate_qos_profiles(self, profiles: List, result: ValidationResult):
        """Validate QoS profiles"""
        if not isinstance(profiles, list):
            result.add_issue(
                ValidationLevel.ERROR,
                "INVALID_QOS_PROFILES_TYPE",
                "QoS profiles must be a list",
                "qos_profiles"
            )
            return
        
        profile_names = set()
        
        for i, profile in enumerate(profiles):
            path = f"qos_profiles[{i}]"
            
            if 'name' not in profile:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_QOS_PROFILE_NAME",
                    f"QoS profile at index {i} is missing 'name' field",
                    path
                )
            else:
                name = profile['name']
                if name in profile_names:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "DUPLICATE_QOS_PROFILE",
                        f"Duplicate QoS profile name: {name}",
                        f"{path}.name"
                    )
                else:
                    profile_names.add(name)
            
            # Validate reliability
            if 'reliability' in profile:
                rel = profile['reliability']
                if 'kind' in rel and rel['kind'] not in self.VALID_RELIABILITY:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "INVALID_RELIABILITY_KIND",
                        f"Invalid reliability kind: {rel['kind']}",
                        f"{path}.reliability.kind"
                    )
            
            # Validate durability
            if 'durability' in profile:
                dur = profile['durability']
                if 'kind' in dur and dur['kind'] not in self.VALID_DURABILITY:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "INVALID_DURABILITY_KIND",
                        f"Invalid durability kind: {dur['kind']}",
                        f"{path}.durability.kind"
                    )
    
    def _validate_participants(self, participants: List, domains: List, result: ValidationResult):
        """Validate participant configurations"""
        if not isinstance(participants, list):
            return
        
        valid_domain_ids = {d['id'] for d in domains if 'id' in d}
        participant_names = set()
        
        for i, participant in enumerate(participants):
            path = f"participants[{i}]"
            
            if 'name' not in participant:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_PARTICIPANT_NAME",
                    f"Participant at index {i} is missing 'name' field",
                    path
                )
            else:
                name = participant['name']
                if name in participant_names:
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "DUPLICATE_PARTICIPANT_NAME",
                        f"Duplicate participant name: {name}",
                        f"{path}.name"
                    )
                else:
                    participant_names.add(name)
            
            if 'domain_id' in participant:
                domain_id = participant['domain_id']
                if valid_domain_ids and domain_id not in valid_domain_ids:
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "UNDECLARED_DOMAIN_ID",
                        f"Participant references domain {domain_id} which is not defined",
                        f"{path}.domain_id",
                        f"Define domain {domain_id} in domains section"
                    )
    
    def _validate_types(self, types: List, result: ValidationResult):
        """Validate type definitions"""
        if not isinstance(types, list):
            return
        
        type_names = set()
        
        for i, type_def in enumerate(types):
            path = f"types[{i}]"
            
            if 'name' not in type_def:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_TYPE_NAME",
                    f"Type at index {i} is missing 'name' field",
                    path
                )
            else:
                name = type_def['name']
                if not self.TYPE_NAME_PATTERN.match(name):
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "INVALID_TYPE_NAME",
                        f"Type name '{name}' does not follow naming convention",
                        f"{path}.name"
                    )
                
                if name in type_names:
                    result.add_issue(
                        ValidationLevel.ERROR,
                        "DUPLICATE_TYPE_NAME",
                        f"Duplicate type name: {name}",
                        f"{path}.name"
                    )
                else:
                    type_names.add(name)
            
            if 'kind' not in type_def:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "MISSING_TYPE_KIND",
                    f"Type '{type_def.get('name', i)}' is missing 'kind' field",
                    path
                )
    
    def _validate_logging(self, logging: Dict, result: ValidationResult):
        """Validate logging configuration"""
        if 'level' in logging:
            if logging['level'] not in self.VALID_LOG_LEVELS:
                result.add_issue(
                    ValidationLevel.ERROR,
                    "INVALID_LOG_LEVEL",
                    f"Invalid log level: {logging['level']}",
                    "logging.level"
                )
    
    def _validate_monitoring(self, monitoring: Dict, result: ValidationResult):
        """Validate monitoring configuration"""
        if 'enabled' in monitoring and monitoring['enabled']:
            if 'statistics_interval' in monitoring:
                if not self._is_valid_duration(monitoring['statistics_interval']):
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "INVALID_STATISTICS_INTERVAL",
                        "Invalid statistics interval format",
                        "monitoring.statistics_interval"
                    )
    
    def _validate_cross_references(self, config: Dict, result: ValidationResult):
        """Validate cross-references between configuration sections"""
        # Build lookup tables
        types = {t['name']: t for t in config.get('types', []) if 'name' in t}
        qos_profiles = {p['name']: p for p in config.get('qos_profiles', []) if 'name' in p}
        
        # Validate topic types exist
        for i, topic in enumerate(config.get('topics', [])):
            if 'type' in topic:
                type_name = topic['type']
                if type_name not in types:
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "UNDEFINED_TOPIC_TYPE",
                        f"Topic '{topic.get('name', i)}' references undefined type: {type_name}",
                        f"topics[{i}].type",
                        f"Define type '{type_name}' in types section"
                    )
        
        # Validate QoS profile references
        for i, topic in enumerate(config.get('topics', [])):
            if 'qos_profile' in topic:
                profile_name = topic['qos_profile']
                if profile_name not in qos_profiles:
                    result.add_issue(
                        ValidationLevel.WARNING,
                        "UNDEFINED_QOS_PROFILE",
                        f"Topic '{topic.get('name', i)}' references undefined QoS profile: {profile_name}",
                        f"topics[{i}].qos_profile",
                        f"Define QoS profile '{profile_name}' in qos_profiles section"
                    )
    
    def _validate_design_rules(self, config: Dict, result: ValidationResult):
        """Validate design rules and best practices"""
        # Check for security in production
        domains = config.get('domains', [])
        security_enabled = any(
            d.get('security', {}).get('enabled', False)
            for d in domains
        )
        
        if not security_enabled and len(domains) > 0:
            result.add_issue(
                ValidationLevel.INFO,
                "SECURITY_NOT_ENABLED",
                "Security is not enabled for any domain",
                "domains",
                "Consider enabling security for production deployments"
            )
        
        # Check for monitoring in production
        monitoring = config.get('monitoring', {})
        if not monitoring.get('enabled', False):
            result.add_issue(
                ValidationLevel.INFO,
                "MONITORING_NOT_ENABLED",
                "Monitoring is not enabled",
                "monitoring",
                "Enable monitoring for production deployments"
            )
        
        # Check QoS consistency
        self._validate_qos_consistency(config, result)
    
    def _validate_qos_consistency(self, config: Dict, result: ValidationResult):
        """Validate QoS policy consistency"""
        profiles = config.get('qos_profiles', [])
        
        for i, profile in enumerate(profiles):
            path = f"qos_profiles[{i}]"
            
            # Check reliability-durability consistency
            reliability = profile.get('reliability', {}).get('kind', 'BEST_EFFORT')
            durability = profile.get('durability', {}).get('kind', 'VOLATILE')
            
            if reliability == 'BEST_EFFORT' and durability != 'VOLATILE':
                result.add_issue(
                    ValidationLevel.WARNING,
                    "INCONSISTENT_QOS",
                    f"Profile '{profile.get('name')}' uses BEST_EFFORT reliability with non-VOLATILE durability",
                    path,
                    "BEST_EFFORT typically requires VOLATILE durability"
                )
    
    def _is_valid_duration(self, value: str) -> bool:
        """Check if value is a valid duration string"""
        if value == "INFINITE":
            return True
        pattern = re.compile(r'^\d+(ns|us|ms|s|m|h)$')
        return bool(pattern.match(value))
    
    def _is_valid_ip_address(self, addr: str) -> bool:
        """Check if string is a valid IP address"""
        # IPv4 pattern
        ipv4_pattern = re.compile(
            r'^(\d{1,3}\.){3}\d{1,3}$'
        )
        # IPv6 pattern (simplified)
        ipv6_pattern = re.compile(
            r'^([0-9a-fA-F:]+)$'
        )
        return bool(ipv4_pattern.match(addr)) or bool(ipv6_pattern.match(addr))
    
    def validate_yaml_string(self, yaml_content: str) -> ValidationResult:
        """Validate YAML string directly"""
        try:
            config = yaml.safe_load(yaml_content)
            return self.validate(config)
        except Exception as e:
            result = ValidationResult(is_valid=False)
            result.add_issue(
                ValidationLevel.ERROR,
                "YAML_PARSE_ERROR",
                f"Could not parse YAML: {e}",
                ""
            )
            return result


# Example usage
if __name__ == '__main__':
    validator = ConfigValidator()
    
    # Example configuration
    test_config = {
        'version': '1.0.0',
        'system': {
            'name': 'TestSystem',
            'version': '1.0.0'
        },
        'domains': [
            {'id': 0, 'name': 'Domain0'}
        ],
        'topics': [
            {'name': 'TestTopic', 'type': 'TestType'}
        ]
    }
    
    result = validator.validate(test_config)
    print(f"Valid: {result.is_valid}")
    print(f"Errors: {len(result.get_errors())}")
    print(f"Warnings: {len(result.get_warnings())}")
    
    for issue in result.issues:
        print(f"[{issue.level.value.upper()}] {issue.code}: {issue.message}")