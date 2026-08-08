"""
DDS Configuration XML Parser

Parses DDS configuration from XML format.
Supports DDS standard XML format and custom extensions for AUTOSAR integration.
"""

import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, Any, List, Optional, Union
import re


class XmlParser:
    """Parser for DDS XML configuration files."""
    
    # Namespace mappings
    NAMESPACES = {
        'dds': 'http://www.omg.org/dds',
        'xsi': 'http://www.w3.org/2001/XMLSchema-instance',
        'autosar': 'http://autosar.org/schema/r4.0'
    }
    
    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []
    
    def parse(self, file_path: Union[str, Path]) -> Dict[str, Any]:
        """
        Parse DDS configuration from XML file.
        
        Args:
            file_path: Path to XML configuration file
            
        Returns:
            Dictionary containing parsed configuration
            
        Raises:
            FileNotFoundError: If file doesn't exist
            ET.ParseError: If XML is malformed
        """
        file_path = Path(file_path)
        
        if not file_path.exists():
            raise FileNotFoundError(f"Configuration file not found: {file_path}")
        
        # Parse XML
        tree = ET.parse(file_path)
        root = tree.getroot()
        
        config = {
            'file_path': str(file_path),
            'format': 'xml',
            'domain': {},
            'topics': [],
            'qos_profiles': [],
            'transports': [],
            'security': {},
            'autosar': {},
            'metadata': {}
        }
        
        # Parse based on root element type
        tag = self._get_tag_without_ns(root.tag)
        
        if tag == 'dds':
            config.update(self._parse_dds_root(root))
        elif tag == 'profiles':
            config['qos_profiles'] = self._parse_profiles(root)
        elif tag == 'domain':
            config['domain'] = self._parse_domain_element(root)
        elif tag == 'AUTOSAR':
            config['autosar'] = self._parse_autosar_root(root)
        else:
            # Generic parsing
            config.update(self._parse_generic(root))
        
        return config
    
    def write(self, config: Dict[str, Any], file_path: Union[str, Path]) -> None:
        """
        Write configuration to XML file.
        
        Args:
            config: Configuration dictionary
            file_path: Output file path
        """
        file_path = Path(file_path)
        
        # Create root element
        root = ET.Element('dds')
        root.set('xmlns', self.NAMESPACES['dds'])
        root.set('xmlns:xsi', self.NAMESPACES['xsi'])
        
        # Add domain configuration
        if 'domain' in config and config['domain']:
            domain_elem = self._create_domain_element(config['domain'])
            root.append(domain_elem)
        
        # Add QoS profiles
        if 'qos_profiles' in config and config['qos_profiles']:
            profiles_elem = ET.SubElement(root, 'profiles')
            for profile in config['qos_profiles']:
                profile_elem = self._create_qos_profile_element(profile)
                profiles_elem.append(profile_elem)
        
        # Add topics
        if 'topics' in config and config['topics']:
            topics_elem = ET.SubElement(root, 'topics')
            for topic in config['topics']:
                topic_elem = self._create_topic_element(topic)
                topics_elem.append(topic_elem)
        
        # Add transports
        if 'transports' in config and config['transports']:
            transports_elem = ET.SubElement(root, 'transports')
            for transport in config['transports']:
                transport_elem = self._create_transport_element(transport)
                transports_elem.append(transport_elem)
        
        # Write to file
        tree = ET.ElementTree(root)
        ET.indent(tree, space='  ')
        tree.write(file_path, encoding='utf-8', xml_declaration=True)
    
    def _get_tag_without_ns(self, tag: str) -> str:
        """Get tag name without namespace."""
        if '}' in tag:
            return tag.split('}', 1)[1]
        return tag
    
    def _get_text(self, element: Optional[ET.Element], default: str = '') -> str:
        """Get text content from element."""
        if element is None or element.text is None:
            return default
        return element.text.strip()
    
    def _get_int(self, element: Optional[ET.Element], default: int = 0) -> int:
        """Get integer value from element."""
        try:
            return int(self._get_text(element, str(default)))
        except ValueError:
            return default
    
    def _get_bool(self, element: Optional[ET.Element], default: bool = False) -> bool:
        """Get boolean value from element."""
        text = self._get_text(element, str(default)).lower()
        return text in ('true', '1', 'yes', 'on')
    
    def _parse_dds_root(self, root: ET.Element) -> Dict[str, Any]:
        """Parse DDS root element."""
        result = {
            'domain': {},
            'qos_profiles': [],
            'topics': [],
            'transports': [],
            'security': {},
            'autosar': {}
        }
        
        for child in root:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'domain':
                result['domain'] = self._parse_domain_element(child)
            elif tag == 'profiles':
                result['qos_profiles'] = self._parse_profiles(child)
            elif tag == 'topics':
                result['topics'] = self._parse_topics(child)
            elif tag == 'transports':
                result['transports'] = self._parse_transports(child)
            elif tag == 'security':
                result['security'] = self._parse_security(child)
            elif tag == 'autosar':
                result['autosar'] = self._parse_autosar(child)
        
        return result
    
    def _parse_domain_element(self, element: ET.Element) -> Dict[str, Any]:
        """Parse domain configuration element."""
        domain = {
            'domain_id': 0,
            'name': '',
            'description': '',
            'discovery': {},
            'transport': {},
            'participants': []
        }
        
        # Get attributes
        if 'domain_id' in element.attrib:
            domain['domain_id'] = int(element.attrib['domain_id'])
        if 'name' in element.attrib:
            domain['name'] = element.attrib['name']
        
        # Parse children
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'domain_id':
                domain['domain_id'] = self._get_int(child, 0)
            elif tag == 'name':
                domain['name'] = self._get_text(child)
            elif tag == 'description':
                domain['description'] = self._get_text(child)
            elif tag == 'discovery':
                domain['discovery'] = self._parse_discovery(child)
            elif tag == 'transport':
                domain['transport'] = self._parse_transport_config(child)
            elif tag == 'participants':
                domain['participants'] = self._parse_participants(child)
        
        return domain
    
    def _parse_discovery(self, element: ET.Element) -> Dict[str, Any]:
        """Parse discovery configuration."""
        discovery = {
            'protocol': 'SIMPLE',
            'lease_duration': 10,
            'announce_period': 5,
            'static_peers': [],
            'multicast': {
                'enabled': True,
                'address': '239.255.0.1',
                'port': 7400
            }
        }
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'protocol':
                discovery['protocol'] = self._get_text(child, 'SIMPLE')
            elif tag == 'lease_duration':
                discovery['lease_duration'] = self._parse_duration(child)
            elif tag == 'announce_period':
                discovery['announce_period'] = self._parse_duration(child)
            elif tag == 'static_peers':
                discovery['static_peers'] = self._parse_static_peers(child)
            elif tag == 'multicast':
                discovery['multicast'] = self._parse_multicast(child)
        
        return discovery
    
    def _parse_duration(self, element: ET.Element) -> Union[int, float]:
        """Parse duration value (can be in seconds or with unit)."""
        text = self._get_text(element, '0')
        
        # Check for unit suffix
        if text.endswith('ms'):
            return float(text[:-2]) / 1000
        elif text.endswith('us'):
            return float(text[:-2]) / 1000000
        elif text.endswith('s'):
            return float(text[:-1])
        elif text.endswith('min'):
            return float(text[:-3]) * 60
        else:
            try:
                return float(text)
            except ValueError:
                return 0
    
    def _parse_static_peers(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse static peers configuration."""
        peers = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'peer':
                peer = {
                    'participant_id': child.get('participant_id', ''),
                    'locator': child.get('locator', ''),
                    'kind': child.get('kind', 'UDPv4')
                }
                peers.append(peer)
        
        return peers
    
    def _parse_multicast(self, element: ET.Element) -> Dict[str, Any]:
        """Parse multicast configuration."""
        return {
            'enabled': self._get_bool(element.find('.//enabled'), True),
            'address': self._get_text(element.find('.//address'), '239.255.0.1'),
            'port': self._get_int(element.find('.//port'), 7400)
        }
    
    def _parse_transport_config(self, element: ET.Element) -> Dict[str, Any]:
        """Parse transport configuration."""
        transport = {
            'default': 'UDPv4',
            'shm_enabled': True,
            'intra_process': True
        }
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'default':
                transport['default'] = self._get_text(child, 'UDPv4')
            elif tag == 'shm_enabled':
                transport['shm_enabled'] = self._get_bool(child, True)
            elif tag == 'intra_process':
                transport['intra_process'] = self._get_bool(child, True)
        
        return transport
    
    def _parse_participants(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse participants configuration."""
        participants = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'participant':
                participant = {
                    'name': child.get('name', ''),
                    'id': child.get('id', ''),
                    'app_id': child.get('app_id', ''),
                    'priority': int(child.get('priority', 0))
                }
                participants.append(participant)
        
        return participants
    
    def _parse_profiles(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse QoS profiles."""
        profiles = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'qos_profile':
                profile = self._parse_qos_profile(child)
                profiles.append(profile)
        
        return profiles
    
    def _parse_qos_profile(self, element: ET.Element) -> Dict[str, Any]:
        """Parse QoS profile element."""
        profile = {
            'name': element.get('name', ''),
            'description': '',
            'reliability': {},
            'durability': {},
            'history': {},
            'deadline': {},
            'lifespan': {},
            'liveliness': {},
            'resource_limits': {}
        }
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'description':
                profile['description'] = self._get_text(child)
            elif tag == 'reliability':
                profile['reliability'] = self._parse_reliability_qos(child)
            elif tag == 'durability':
                profile['durability'] = self._parse_durability_qos(child)
            elif tag == 'history':
                profile['history'] = self._parse_history_qos(child)
            elif tag == 'deadline':
                profile['deadline'] = self._parse_deadline_qos(child)
            elif tag == 'lifespan':
                profile['lifespan'] = self._parse_lifespan_qos(child)
            elif tag == 'liveliness':
                profile['liveliness'] = self._parse_liveliness_qos(child)
            elif tag == 'resource_limits':
                profile['resource_limits'] = self._parse_resource_limits_qos(child)
        
        return profile
    
    def _parse_reliability_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse reliability QoS."""
        return {
            'kind': self._get_text(element.find('.//kind'), 'RELIABLE'),
            'max_blocking_time': self._parse_duration(element.find('.//max_blocking_time')),
            'synchronous': self._get_bool(element.find('.//synchronous'), False)
        }
    
    def _parse_durability_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse durability QoS."""
        return {
            'kind': self._get_text(element.find('.//kind'), 'VOLATILE'),
            'service': self._get_text(element.find('.//service'), '')
        }
    
    def _parse_history_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse history QoS."""
        return {
            'kind': self._get_text(element.find('.//kind'), 'KEEP_LAST'),
            'depth': self._get_int(element.find('.//depth'), 1)
        }
    
    def _parse_deadline_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse deadline QoS."""
        period_elem = element.find('.//period')
        period = self._parse_duration(period_elem) if period_elem is not None else 0
        
        return {
            'enabled': period > 0,
            'period': period
        }
    
    def _parse_lifespan_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse lifespan QoS."""
        duration_elem = element.find('.//duration')
        duration = self._parse_duration(duration_elem) if duration_elem is not None else 0
        
        return {
            'enabled': duration > 0,
            'duration': duration
        }
    
    def _parse_liveliness_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse liveliness QoS."""
        return {
            'kind': self._get_text(element.find('.//kind'), 'AUTOMATIC'),
            'lease_duration': self._parse_duration(element.find('.//lease_duration')),
            'assert_period': self._parse_duration(element.find('.//assert_period'))
        }
    
    def _parse_resource_limits_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse resource limits QoS."""
        return {
            'max_samples': self._get_int(element.find('.//max_samples'), -1),
            'max_instances': self._get_int(element.find('.//max_instances'), -1),
            'max_samples_per_instance': self._get_int(element.find('.//max_samples_per_instance'), -1)
        }
    
    def _parse_topics(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse topics configuration."""
        topics = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'topic':
                topic = self._parse_topic_element(child)
                topics.append(topic)
        
        return topics
    
    def _parse_topic_element(self, element: ET.Element) -> Dict[str, Any]:
        """Parse topic element."""
        topic = {
            'name': element.get('name', ''),
            'kind': element.get('kind', 'STANDARD'),
            'data_type': element.get('data_type', ''),
            'description': '',
            'qos': {},
            'security': {},
            'autosar': {}
        }
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'description':
                topic['description'] = self._get_text(child)
            elif tag == 'qos':
                topic['qos'] = self._parse_topic_qos(child)
            elif tag == 'security':
                topic['security'] = self._parse_topic_security(child)
            elif tag == 'autosar':
                topic['autosar'] = self._parse_topic_autosar(child)
        
        return topic
    
    def _parse_topic_qos(self, element: ET.Element) -> Dict[str, Any]:
        """Parse topic QoS configuration."""
        qos = {}
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'reliability':
                qos['reliability'] = self._get_text(child.find('.//kind'), 'RELIABLE')
            elif tag == 'durability':
                qos['durability'] = self._get_text(child.find('.//kind'), 'VOLATILE')
            elif tag == 'history':
                qos['history_kind'] = self._get_text(child.find('.//kind'), 'KEEP_LAST')
                qos['history_depth'] = self._get_int(child.find('.//depth'), 1)
        
        return qos
    
    def _parse_topic_security(self, element: ET.Element) -> Dict[str, Any]:
        """Parse topic security configuration."""
        return {
            'e2e_enabled': self._get_bool(element.find('.//e2e_enabled'), False),
            'e2e_profile': self._get_text(element.find('.//e2e_profile'), 'P01'),
            'e2e_data_id': self._get_int(element.find('.//e2e_data_id'), 0),
            'encryption_enabled': self._get_bool(element.find('.//encryption_enabled'), False),
            'auth_enabled': self._get_bool(element.find('.//auth_enabled'), False)
        }
    
    def _parse_topic_autosar(self, element: ET.Element) -> Dict[str, Any]:
        """Parse topic AUTOSAR configuration."""
        return {
            'enabled': self._get_bool(element.find('.//enabled'), False),
            'ecu_id': self._get_text(element.find('.//ecu_id'), ''),
            'soad_socket': self._get_text(element.find('.//soad_socket'), ''),
            'pdur_src_pdu': self._get_text(element.find('.//pdur_src_pdu'), '')
        }
    
    def _parse_transports(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse transports configuration."""
        transports = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'transport':
                transport = self._parse_transport_element(child)
                transports.append(transport)
        
        return transports
    
    def _parse_transport_element(self, element: ET.Element) -> Dict[str, Any]:
        """Parse transport element."""
        transport = {
            'name': element.get('name', ''),
            'type': element.get('type', 'UDPv4'),
            'enabled': self._get_bool(element.find('.//enabled'), True),
            'priority': self._get_int(element.find('.//priority'), 0),
            'udp': {},
            'tcp': {},
            'shm': {}
        }
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'udp':
                transport['udp'] = self._parse_udp_config(child)
            elif tag == 'tcp':
                transport['tcp'] = self._parse_tcp_config(child)
            elif tag == 'shm':
                transport['shm'] = self._parse_shm_config(child)
        
        return transport
    
    def _parse_udp_config(self, element: ET.Element) -> Dict[str, Any]:
        """Parse UDP configuration."""
        return {
            'port': self._get_int(element.find('.//port'), 7400),
            'ttl': self._get_int(element.find('.//ttl'), 1),
            'recv_buffer': self._get_int(element.find('.//recv_buffer'), 2097152),
            'send_buffer': self._get_int(element.find('.//send_buffer'), 65536)
        }
    
    def _parse_tcp_config(self, element: ET.Element) -> Dict[str, Any]:
        """Parse TCP configuration."""
        return {
            'server': self._get_bool(element.find('.//server'), False),
            'listening_port': self._get_int(element.find('.//listening_port'), 7400),
            'max_connections': self._get_int(element.find('.//max_connections'), 100),
            'keepalive': self._get_int(element.find('.//keepalive'), 60)
        }
    
    def _parse_shm_config(self, element: ET.Element) -> Dict[str, Any]:
        """Parse SHM configuration."""
        return {
            'mode': self._get_text(element.find('.//mode'), 'POSIX'),
            'segment_size': self._get_int(element.find('.//segment_size'), 65536),
            'max_segments': self._get_int(element.find('.//max_segments'), 10),
            'zero_copy': self._get_bool(element.find('.//zero_copy'), True)
        }
    
    def _parse_security(self, element: ET.Element) -> Dict[str, Any]:
        """Parse security configuration."""
        return {
            'authentication': self._get_text(element.find('.//authentication'), ''),
            'access_control': self._get_bool(element.find('.//access_control'), False),
            'encryption': self._get_text(element.find('.//encryption'), ''),
            'key_store': self._get_text(element.find('.//key_store'), '')
        }
    
    def _parse_autosar(self, element: ET.Element) -> Dict[str, Any]:
        """Parse AUTOSAR configuration."""
        return {
            'ecu_id': self._get_text(element.find('.//ecu_id'), ''),
            'swc_name': self._get_text(element.find('.//swc_name'), ''),
            'bswm_rules': self._parse_bswm_rules(element.find('.//bswm_rules'))
        }
    
    def _parse_bswm_rules(self, element: Optional[ET.Element]) -> List[Dict[str, Any]]:
        """Parse BswM rules."""
        if element is None:
            return []
        
        rules = []
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'rule':
                rule = {
                    'name': child.get('name', ''),
                    'condition': self._get_text(child.find('.//condition')),
                    'action': self._get_text(child.find('.//action'))
                }
                rules.append(rule)
        
        return rules
    
    def _parse_autosar_root(self, root: ET.Element) -> Dict[str, Any]:
        """Parse AUTOSAR root element."""
        # Handle ARXML format
        return {
            'format': 'autosar',
            'packages': self._parse_ar_packages(root)
        }
    
    def _parse_ar_packages(self, element: ET.Element) -> List[Dict[str, Any]]:
        """Parse AR:PACKAGES."""
        packages = []
        
        for child in element:
            tag = self._get_tag_without_ns(child.tag)
            
            if tag == 'AR-PACKAGE':
                package = self._parse_ar_package(child)
                packages.append(package)
        
        return packages
    
    def _parse_ar_package(self, element: ET.Element) -> Dict[str, Any]:
        """Parse AR:PACKAGE element."""
        package = {
            'short_name': self._get_text(element.find('.//SHORT-NAME')),
            'elements': []
        }
        
        elements_elem = element.find('.//ELEMENTS')
        if elements_elem is not None:
            for child in elements_elem:
                tag = self._get_tag_without_ns(child.tag)
                
                if tag == 'ECUC-MODULE-CONFIGURATION-VALUES':
                    package['elements'].append(self._parse_ecuc_module(child))
        
        return package
    
    def _parse_ecuc_module(self, element: ET.Element) -> Dict[str, Any]:
        """Parse ECUC module configuration."""
        return {
            'type': 'ecuc_module',
            'short_name': self._get_text(element.find('.//SHORT-NAME')),
            'definition': element.get('DEFINITION-REF', '')
        }
    
    def _parse_generic(self, root: ET.Element) -> Dict[str, Any]:
        """Parse generic XML structure."""
        result = {}
        
        for child in root:
            tag = self._get_tag_without_ns(child.tag)
            result[tag] = self._element_to_dict(child)
        
        return result
    
    def _element_to_dict(self, element: ET.Element) -> Any:
        """Convert XML element to dictionary."""
        result: Dict[str, Any] = {}
        
        # Add attributes
        for key, value in element.attrib.items():
            result[f'@{key}'] = value
        
        # Add children
        children = list(element)
        if children:
            for child in children:
                tag = self._get_tag_without_ns(child.tag)
                child_data = self._element_to_dict(child)
                
                if tag in result:
                    if not isinstance(result[tag], list):
                        result[tag] = [result[tag]]
                    result[tag].append(child_data)
                else:
                    result[tag] = child_data
        elif element.text and element.text.strip():
            return element.text.strip()
        
        return result
    
    def _create_domain_element(self, domain: Dict[str, Any]) -> ET.Element:
        """Create domain XML element."""
        elem = ET.Element('domain')
        
        if 'domain_id' in domain:
            elem.set('domain_id', str(domain['domain_id']))
        if 'name' in domain:
            elem.set('name', domain['name'])
        
        return elem
    
    def _create_qos_profile_element(self, profile: Dict[str, Any]) -> ET.Element:
        """Create QoS profile XML element."""
        elem = ET.Element('qos_profile')
        
        if 'name' in profile:
            elem.set('name', profile['name'])
        
        return elem
    
    def _create_topic_element(self, topic: Dict[str, Any]) -> ET.Element:
        """Create topic XML element."""
        elem = ET.Element('topic')
        
        if 'name' in topic:
            elem.set('name', topic['name'])
        if 'data_type' in topic:
            elem.set('data_type', topic['data_type'])
        
        return elem
    
    def _create_transport_element(self, transport: Dict[str, Any]) -> ET.Element:
        """Create transport XML element."""
        elem = ET.Element('transport')
        
        if 'name' in transport:
            elem.set('name', transport['name'])
        if 'type' in transport:
            elem.set('type', transport['type'])
        
        return elem


class XMLParseError(Exception):
    """Exception for XML parsing errors."""
    pass
