#!/usr/bin/env python3
"""
Design Rule Checker for DDS Configurations
Validates design patterns and best practices for automotive DDS systems
"""

import re
from typing import List, Dict, Optional, Any, Set
from dataclasses import dataclass, field
from enum import Enum
import yaml


class RuleSeverity(Enum):
    """Rule severity levels"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"
    HINT = "hint"


@dataclass
class DesignRule:
    """Design rule definition"""
    id: str
    name: str
    description: str
    severity: RuleSeverity
    category: str
    check_fn: Any  # Function to check the rule


@dataclass
class RuleViolation:
    """Rule violation report"""
    rule_id: str
    severity: RuleSeverity
    message: str
    path: str
    suggestion: Optional[str] = None
    references: List[str] = field(default_factory=list)


class DesignRuleChecker:
    """
    Checks DDS configurations against automotive design rules
    
    Validates:
    - Naming conventions
    - Safety requirements
    - Performance constraints
    - Security policies
    - AUTOSAR compliance
    """
    
    # AUTOSAR naming conventions
    TOPIC_NAME_PATTERN = re.compile(r'^[A-Z][a-zA-Z0-9]*$')
    TYPE_NAME_PATTERN = re.compile(r'^[A-Z][a-zA-Z0-9]*$')
    QOS_PROFILE_PATTERN = re.compile(r'^[a-z][a-zA-Z0-9_]*$')
    
    # Safety-critical topic patterns
    SAFETY_CRITICAL_PATTERNS = [
        'brake', 'steering', 'throttle', 'airbag', 'abs', 'esp'
    ]
    
    def __init__(self):
        self.rules = self._initialize_rules()
        self.violations: List[RuleViolation] = []
        
    def _initialize_rules(self) -> List[DesignRule]:
        """Initialize design rules"""
        return [
            # Naming Convention Rules
            DesignRule(
                id="NAMING-001",
                name="Topic PascalCase Naming",
                description="Topic names should use PascalCase",
                severity=RuleSeverity.WARNING,
                category="Naming",
                check_fn=self._check_topic_naming
            ),
            DesignRule(
                id="NAMING-002",
                name="Type PascalCase Naming",
                description="Type names should use PascalCase",
                severity=RuleSeverity.WARNING,
                category="Naming",
                check_fn=self._check_type_naming
            ),
            DesignRule(
                id="NAMING-003",
                name="QOS Profile camelCase Naming",
                description="QoS profile names should use camelCase or snake_case",
                severity=RuleSeverity.HINT,
                category="Naming",
                check_fn=self._check_qos_naming
            ),
            
            # Safety Rules
            DesignRule(
                id="SAFETY-001",
                name="Safety-Critical QoS",
                description="Safety-critical topics must use RELIABLE reliability",
                severity=RuleSeverity.ERROR,
                category="Safety",
                check_fn=self._check_safety_qos
            ),
            DesignRule(
                id="SAFETY-002",
                name="Deadline for Control Topics",
                description="Control topics should have deadline configured",
                severity=RuleSeverity.WARNING,
                category="Safety",
                check_fn=self._check_deadline_config
            ),
            DesignRule(
                id="SAFETY-003",
                name="E2E Protection for Critical Data",
                description="Safety-critical data should have E2E protection",
                severity=RuleSeverity.WARNING,
                category="Safety",
                check_fn=self._check_e2e_protection
            ),
            
            # Performance Rules
            DesignRule(
                id="PERF-001",
                name="History Depth Limit",
                description="History depth should not exceed reasonable limits",
                severity=RuleSeverity.WARNING,
                category="Performance",
                check_fn=self._check_history_depth
            ),
            DesignRule(
                id="PERF-002",
                name="Resource Limits Configured",
                description="Resource limits should be configured for bounded memory",
                severity=RuleSeverity.INFO,
                category="Performance",
                check_fn=self._check_resource_limits
            ),
            
            # Security Rules
            DesignRule(
                id="SEC-001",
                name="Security for Production",
                description="Security should be enabled for production systems",
                severity=RuleSeverity.WARNING,
                category="Security",
                check_fn=self._check_security_enabled
            ),
            DesignRule(
                id="SEC-002",
                name="Authentication Configured",
                description="Authentication should be configured when security is enabled",
                severity=RuleSeverity.ERROR,
                category="Security",
                check_fn=self._check_authentication_config
            ),
            
            # AUTOSAR Compliance
            DesignRule(
                id="AUTOSAR-001",
                name="Domain ID Range",
                description="Domain IDs should be in valid range (0-230)",
                severity=RuleSeverity.ERROR,
                category="AUTOSAR",
                check_fn=self._check_domain_id_range
            ),
            DesignRule(
                id="AUTOSAR-002",
                name="ASIL Compliance",
                description="ASIL levels should be properly assigned",
                severity=RuleSeverity.WARNING,
                category="AUTOSAR",
                check_fn=self._check_asil_compliance
            ),
        ]
    
    def check(self, config: Dict[str, Any]) -> List[RuleViolation]:
        """Run all design rules against configuration"""
        self.violations = []
        
        for rule in self.rules:
            try:
                rule.check_fn(config, rule)
            except Exception as e:
                # Rule check failed
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=RuleSeverity.ERROR,
                    message=f"Rule check failed: {e}",
                    path="",
                    suggestion="Check configuration structure"
                ))
        
        return self.violations
    
    def check_file(self, filepath: str) -> List[RuleViolation]:
        """Check configuration from file"""
        with open(filepath, 'r') as f:
            if filepath.endswith('.json'):
                import json
                config = json.load(f)
            else:
                config = yaml.safe_load(f)
        return self.check(config)
    
    # Rule check implementations
    def _check_topic_naming(self, config: Dict, rule: DesignRule):
        """Check topic naming convention"""
        for i, topic in enumerate(config.get('topics', [])):
            name = topic.get('name', '')
            if name and not self.TOPIC_NAME_PATTERN.match(name):
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"Topic name '{name}' does not follow PascalCase convention",
                    path=f"topics[{i}].name",
                    suggestion="Rename to PascalCase (e.g., 'VehicleSpeed')"
                ))
    
    def _check_type_naming(self, config: Dict, rule: DesignRule):
        """Check type naming convention"""
        for i, type_def in enumerate(config.get('types', [])):
            name = type_def.get('name', '')
            if name and not self.TYPE_NAME_PATTERN.match(name):
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"Type name '{name}' does not follow PascalCase convention",
                    path=f"types[{i}].name",
                    suggestion="Rename to PascalCase (e.g., 'SpeedData')"
                ))
    
    def _check_qos_naming(self, config: Dict, rule: DesignRule):
        """Check QoS profile naming convention"""
        for i, profile in enumerate(config.get('qos_profiles', [])):
            name = profile.get('name', '')
            if name and not self.QOS_PROFILE_PATTERN.match(name):
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"QoS profile name '{name}' should use camelCase or snake_case",
                    path=f"qos_profiles[{i}].name",
                    suggestion="Use 'reliableLowLatency' or 'reliable_low_latency'"
                ))
    
    def _check_safety_qos(self, config: Dict, rule: DesignRule):
        """Check safety-critical topics have proper QoS"""
        for i, topic in enumerate(config.get('topics', [])):
            name = topic.get('name', '').lower()
            
            # Check if topic is safety-critical
            is_critical = any(pattern in name for pattern in self.SAFETY_CRITICAL_PATTERNS)
            
            if is_critical:
                qos_profile = topic.get('qos_profile', '')
                profiles = {p['name']: p for p in config.get('qos_profiles', [])}
                
                profile = profiles.get(qos_profile, {})
                reliability = profile.get('reliability', {}).get('kind', 'BEST_EFFORT')
                
                if reliability != 'RELIABLE':
                    self.violations.append(RuleViolation(
                        rule_id=rule.id,
                        severity=rule.severity,
                        message=f"Safety-critical topic '{topic['name']}' must use RELIABLE reliability",
                        path=f"topics[{i}].qos_profile",
                        suggestion="Use a QoS profile with RELIABLE reliability"
                    ))
    
    def _check_deadline_config(self, config: Dict, rule: DesignRule):
        """Check control topics have deadline configured"""
        control_patterns = ['control', 'command', 'setpoint', 'request']
        
        for i, topic in enumerate(config.get('topics', [])):
            name = topic.get('name', '').lower()
            
            if any(pattern in name for pattern in control_patterns):
                qos_profile = topic.get('qos_profile', '')
                profiles = {p['name']: p for p in config.get('qos_profiles', [])}
                
                profile = profiles.get(qos_profile, {})
                
                if 'deadline' not in profile:
                    self.violations.append(RuleViolation(
                        rule_id=rule.id,
                        severity=rule.severity,
                        message=f"Control topic '{topic['name']}' should have deadline configured",
                        path=f"topics[{i}].qos_profile",
                        suggestion="Add deadline QoS policy to the profile"
                    ))
    
    def _check_e2e_protection(self, config: Dict, rule: DesignRule):
        """Check E2E protection for critical data"""
        # Check if global E2E protection is enabled
        e2e_enabled = config.get('e2e_protection', {}).get('enabled', False)
        
        if not e2e_enabled:
            # Check for safety-critical topics without protection
            for i, topic in enumerate(config.get('topics', [])):
                name = topic.get('name', '').lower()
                if any(pattern in name for pattern in self.SAFETY_CRITICAL_PATTERNS):
                    self.violations.append(RuleViolation(
                        rule_id=rule.id,
                        severity=rule.severity,
                        message=f"Safety-critical topic '{topic['name']}' should have E2E protection",
                        path=f"topics[{i}]",
                        suggestion="Enable E2E protection in configuration"
                    ))
    
    def _check_history_depth(self, config: Dict, rule: DesignRule):
        """Check history depth is reasonable"""
        for i, profile in enumerate(config.get('qos_profiles', [])):
            history = profile.get('history', {})
            depth = history.get('depth', 1)
            
            if depth > 100:
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"QoS profile '{profile['name']}' has large history depth ({depth})",
                    path=f"qos_profiles[{i}].history.depth",
                    suggestion="Consider reducing history depth to conserve memory"
                ))
    
    def _check_resource_limits(self, config: Dict, rule: DesignRule):
        """Check resource limits are configured"""
        for i, profile in enumerate(config.get('qos_profiles', [])):
            if 'resource_limits' not in profile:
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"QoS profile '{profile['name']}' should have resource_limits configured",
                    path=f"qos_profiles[{i}]",
                    suggestion="Add resource_limits for bounded memory usage"
                ))
    
    def _check_security_enabled(self, config: Dict, rule: DesignRule):
        """Check security is enabled for production"""
        security_enabled = False
        
        for domain in config.get('domains', []):
            if domain.get('security', {}).get('enabled', False):
                security_enabled = True
                break
        
        if not security_enabled:
            self.violations.append(RuleViolation(
                rule_id=rule.id,
                severity=rule.severity,
                message="Security is not enabled for any domain",
                path="domains",
                suggestion="Enable security for production deployments"
            ))
    
    def _check_authentication_config(self, config: Dict, rule: DesignRule):
        """Check authentication is properly configured"""
        for i, domain in enumerate(config.get('domains', [])):
            security = domain.get('security', {})
            
            if security.get('enabled', False):
                auth = security.get('authentication', {})
                required_fields = ['identity_ca', 'identity_certificate', 'private_key']
                
                for field in required_fields:
                    if field not in auth:
                        self.violations.append(RuleViolation(
                            rule_id=rule.id,
                            severity=rule.severity,
                            message=f"Security enabled but {field} not configured",
                            path=f"domains[{i}].security.authentication",
                            suggestion=f"Add {field} to security configuration"
                        ))
    
    def _check_domain_id_range(self, config: Dict, rule: DesignRule):
        """Check domain IDs are in valid range"""
        for i, domain in enumerate(config.get('domains', [])):
            domain_id = domain.get('id')
            if domain_id is not None and (domain_id < 0 or domain_id > 230):
                self.violations.append(RuleViolation(
                    rule_id=rule.id,
                    severity=rule.severity,
                    message=f"Domain ID {domain_id} is out of valid range (0-230)",
                    path=f"domains[{i}].id",
                    suggestion="Use a domain ID between 0 and 230"
                ))
    
    def _check_asil_compliance(self, config: Dict, rule: DesignRule):
        """Check ASIL levels are properly assigned"""
        valid_asil = ['QM', 'ASIL_A', 'ASIL_B', 'ASIL_C', 'ASIL_D']
        
        # Check system ASIL
        system_asil = config.get('system', {}).get('asil_level')
        if system_asil and system_asil not in valid_asil:
            self.violations.append(RuleViolation(
                rule_id=rule.id,
                severity=rule.severity,
                message=f"Invalid ASIL level: {system_asil}",
                path="system.asil_level",
                suggestion=f"Use one of: {', '.join(valid_asil)}"
            ))
    
    def get_summary(self) -> Dict[str, Any]:
        """Get summary of violations"""
        errors = [v for v in self.violations if v.severity == RuleSeverity.ERROR]
        warnings = [v for v in self.violations if v.severity == RuleSeverity.WARNING]
        infos = [v for v in self.violations if v.severity == RuleSeverity.INFO]
        hints = [v for v in self.violations if v.severity == RuleSeverity.HINT]
        
        return {
            'total': len(self.violations),
            'errors': len(errors),
            'warnings': len(warnings),
            'infos': len(infos),
            'hints': len(hints),
            'passed': len(errors) == 0
        }
    
    def print_report(self):
        """Print violation report"""
        summary = self.get_summary()
        
        print(f"\n{'='*70}")
        print(f"Design Rule Check Report")
        print(f"{'='*70}")
        print(f"Total: {summary['total']} | Errors: {summary['errors']} | "
              f"Warnings: {summary['warnings']} | Info: {summary['infos']}")
        print(f"Result: {'PASSED' if summary['passed'] else 'FAILED'}")
        print(f"{'='*70}\n")
        
        # Group by severity
        for severity in [RuleSeverity.ERROR, RuleSeverity.WARNING, RuleSeverity.INFO, RuleSeverity.HINT]:
            violations = [v for v in self.violations if v.severity == severity]
            if violations:
                print(f"\n{severity.value.upper()} ({len(violations)}):")
                for v in violations:
                    print(f"  [{v.rule_id}] {v.message}")
                    if v.path:
                        print(f"    Path: {v.path}")
                    if v.suggestion:
                        print(f"    Suggestion: {v.suggestion}")


# Example usage
if __name__ == '__main__':
    checker = DesignRuleChecker()
    
    test_config = {
        'version': '1.0.0',
        'system': {
            'name': 'TestSystem',
            'asil_level': 'ASIL_D'
        },
        'domains': [
            {'id': 0, 'name': 'Domain0', 'security': {'enabled': False}}
        ],
        'topics': [
            {'name': 'brakeCommand', 'type': 'BrakeData'},  # Wrong naming
            {'name': 'VehicleSpeed', 'type': 'SpeedData', 'qos_profile': 'default'}
        ],
        'qos_profiles': [
            {'name': 'default', 'reliability': {'kind': 'BEST_EFFORT'}}
        ]
    }
    
    checker.check(test_config)
    checker.print_report()