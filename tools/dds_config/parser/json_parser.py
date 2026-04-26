"""
DDS Configuration JSON Parser

Parses DDS configuration from JSON format.
Provides conversion to/from XML and validation.
"""

import json
from pathlib import Path
from typing import Dict, Any, List, Optional, Union
import re


class JsonParser:
    """Parser for DDS JSON configuration files."""
    
    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []
    
    def parse(self, file_path: Union[str, Path]) -> Dict[str, Any]:
        """
        Parse DDS configuration from JSON file.
        
        Args:
            file_path: Path to JSON configuration file
            
        Returns:
            Dictionary containing parsed configuration
            
        Raises:
            FileNotFoundError: If file doesn't exist
            json.JSONDecodeError: If JSON is malformed
        """
        file_path = Path(file_path)
        
        if not file_path.exists():
            raise FileNotFoundError(f"Configuration file not found: {file_path}")
        
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        config = {
            'file_path': str(file_path),
            'format': 'json',
            'domain': {},
            'topics': [],
            'qos_profiles': [],
            'transports': [],
            'security': {},
            'autosar': {},
            'metadata': {}
        }
        
        # Parse domain configuration
        if 'domain' in data:
            config['domain'] = self._parse_domain(data['domain'])
        
        # Parse topics
        if 'topics' in data:
            config['topics'] = self._parse_topics(data['topics'])
        elif 'topic' in data:
            config['topics'] = [self._parse_topic(data['topic'])]
        
        # Parse QoS profiles
        if 'qos_profiles' in data:
            config['qos_profiles'] = self._parse_qos_profiles(data['qos_profiles'])
        elif 'qos' in data:
            config['qos_profiles'] = [self._parse_qos_profile(data['qos'])]
        
        # Parse transports
        if 'transports' in data:
            config['transports'] = self._parse_transports(data['transports'])
        elif 'transport' in data:
            config['transports'] = [self._parse_transport(data['transport'])]
        
        # Parse security
        if 'security' in data:
            config['security'] = self._parse_security(data['security'])
        
        # Parse AUTOSAR
        if 'autosar' in data:
            config['autosar'] = self._parse_autosar(data['autosar'])
        
        # Parse metadata
        if 'metadata' in data:
            config['metadata'] = data['metadata']
        
        return config
    
    def write(self, config: Dict[str, Any], file_path: Union[str, Path]) -> None:
        """
        Write configuration to JSON file.
        
        Args:
            config: Configuration dictionary
            file_path: Output file path
        """
        file_path = Path(file_path)
        
        output = {
            'version': '1.0',
            'format': 'dds_config',
            'domain': config.get('domain', {}),
            'topics': config.get('topics', []),
            'qos_profiles': config.get('qos_profiles', []),
            'transports': config.get('transports', [])
        }
        
        if 'security' in config and config['security']:
            output['security'] = config['security']
        
        if 'autosar' in config and config['autosar']:
            output['autosar'] = config['autosar']
        
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(output, f, indent=2, ensure_ascii=False)
    
    def parse_string(self, json_string: str) -> Dict[str, Any]:
        """
        Parse DDS configuration from JSON string.
        
        Args:
            json_string: JSON configuration as string
            
        Returns:
            Dictionary containing parsed configuration
        """
        data = json.loads(json_string)
        
        config = {
            'format': 'json',
            'domain': {},
            'topics': [],
            'qos_profiles': [],
            'transports': [],
            'security': {},
            'autosar': {}
        }
        
        if 'domain' in data:
            config['domain'] = self._parse_domain(data['domain'])
        
        if 'topics' in data:
            config['topics'] = self._parse_topics(data['topics'])
        
        if 'qos_profiles' in data:
            config['qos_profiles'] = self._parse_qos_profiles(data['qos_profiles'])
        
        if 'transports' in data:
            config['transports'] = self._parse_transports(data['transports'])
        
        return config
    
    def to_string(self, config: Dict[str, Any], pretty: bool = True) -> str:
        """
        Convert configuration to JSON string.
        
        Args:
            config: Configuration dictionary
            pretty: Whether to format with indentation
            
        Returns:
            JSON string
        """
        indent = 2 if pretty else None
        return json.dumps(config, indent=indent, ensure_ascii=False)
    
    def _parse_domain(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse domain configuration."""
        return {
            'domain_id': data.get('domain_id', 0),
            'name': data.get('name', ''),
            'description': data.get('description', ''),
            'discovery': self._parse_discovery(data.get('discovery', {})),
            'transport': self._parse_transport_config(data.get('transport', {})),
            'participants': data.get('participants', [])
        }
    
    def _parse_discovery(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse discovery configuration."""
        return {
            'protocol': data.get('protocol', 'SIMPLE'),
            'lease_duration': data.get('lease_duration', 10),
            'announce_period': data.get('announce_period', 5),
            'static_peers': data.get('static_peers', []),
            'multicast': {
                'enabled': data.get('multicast', {}).get('enabled', True),
                'address': data.get('multicast', {}).get('address', '239.255.0.1'),
                'port': data.get('multicast', {}).get('port', 7400)
            }
        }
    
    def _parse_transport_config(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse transport configuration."""
        return {
            'default': data.get('default', 'UDPv4'),
            'shm_enabled': data.get('shm_enabled', True),
            'intra_process': data.get('intra_process', True)
        }
    
    def _parse_topics(self, data: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Parse topics configuration."""
        topics = []
        for topic_data in data:
            topics.append(self._parse_topic(topic_data))
        return topics
    
    def _parse_topic(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse topic configuration."""
        return {
            'name': data.get('name', ''),
            'kind': data.get('kind', 'STANDARD'),
            'data_type': data.get('data_type', ''),
            'description': data.get('description', ''),
            'key_fields': data.get('key_fields', ''),
            'max_instances': data.get('max_instances', 1),
            'qos': self._parse_topic_qos(data.get('qos', {})),
            'security': self._parse_topic_security(data.get('security', {})),
            'autosar': self._parse_topic_autosar(data.get('autosar', {}))
        }
    
    def _parse_topic_qos(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse topic QoS configuration."""
        return {
            'reliability': data.get('reliability', 'RELIABLE'),
            'durability': data.get('durability', 'VOLATILE'),
            'history_kind': data.get('history_kind', 'KEEP_LAST'),
            'history_depth': data.get('history_depth', 1),
            'deadline_enabled': data.get('deadline_enabled', False),
            'deadline_period': data.get('deadline_period', 1000),
            'latency_budget': data.get('latency_budget', 0),
            'liveliness': data.get('liveliness', 'AUTOMATIC'),
            'lease_duration': data.get('lease_duration', 10)
        }
    
    def _parse_topic_security(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse topic security configuration."""
        return {
            'e2e_enabled': data.get('e2e_enabled', False),
            'e2e_profile': data.get('e2e_profile', 'P01'),
            'e2e_data_id': data.get('e2e_data_id', 0),
            'encryption_enabled': data.get('encryption_enabled', False),
            'encryption_algo': data.get('encryption_algo', 'AES-128-GCM'),
            'auth_enabled': data.get('auth_enabled', False)
        }
    
    def _parse_topic_autosar(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse topic AUTOSAR configuration."""
        return {
            'enabled': data.get('enabled', False),
            'ecu_id': data.get('ecu_id', ''),
            'soad_socket': data.get('soad_socket', ''),
            'pdur_src_pdu': data.get('pdur_src_pdu', '')
        }
    
    def _parse_qos_profiles(self, data: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Parse QoS profiles."""
        profiles = []
        for profile_data in data:
            profiles.append(self._parse_qos_profile(profile_data))
        return profiles
    
    def _parse_qos_profile(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse QoS profile configuration."""
        return {
            'name': data.get('name', ''),
            'description': data.get('description', ''),
            'reliability': data.get('reliability', {}),
            'durability': data.get('durability', {}),
            'history': data.get('history', {}),
            'deadline': data.get('deadline', {}),
            'lifespan': data.get('lifespan', {}),
            'liveliness': data.get('liveliness', {}),
            'resource_limits': data.get('resource_limits', {}),
            'transport_priority': data.get('transport_priority', 0),
            'partition': data.get('partition', {})
        }
    
    def _parse_transports(self, data: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Parse transports configuration."""
        transports = []
        for transport_data in data:
            transports.append(self._parse_transport(transport_data))
        return transports
    
    def _parse_transport(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse transport configuration."""
        return {
            'name': data.get('name', ''),
            'type': data.get('type', 'UDPv4'),
            'enabled': data.get('enabled', True),
            'priority': data.get('priority', 0),
            'whitelist': data.get('whitelist', []),
            'blacklist': data.get('blacklist', []),
            'udp': data.get('udp', {}),
            'tcp': data.get('tcp', {}),
            'shm': data.get('shm', {}),
            'advanced': data.get('advanced', {})
        }
    
    def _parse_security(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse security configuration."""
        return {
            'authentication': data.get('authentication', ''),
            'access_control': data.get('access_control', False),
            'encryption': data.get('encryption', ''),
            'key_store': data.get('key_store', '')
        }
    
    def _parse_autosar(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Parse AUTOSAR configuration."""
        return {
            'ecu_id': data.get('ecu_id', ''),
            'swc_name': data.get('swc_name', ''),
            'bswm_rules': data.get('bswm_rules', [])
        }


class JsonValidationError(Exception):
    """Exception for JSON validation errors."""
    pass
