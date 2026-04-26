"""
ARXML Converter for DDS Configuration

This module provides bidirectional conversion between YAML DDS configuration
and AUTOSAR ARXML format, supporting:
- DdsDomainParticipantConfig
- DdsTopicConfig
- DdsQosProfile
- AUTOSAR 4.x compatibility
"""

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
from xml.etree import ElementTree as ET
from xml.dom import minidom

from .config_model import (
    DDSConfig, Domain, Participant, Topic, QoS, Transport,
    TSN, TSNStream, Security, SystemInfo,
    Publisher, Subscriber,
    ReliabilityKind, DurabilityKind, HistoryKind, LivelinessKind
)


class ARXMLConversionError(Exception):
    """ARXML conversion error"""
    
    def __init__(self, message: str, element: Optional[str] = None,
                 line: Optional[int] = None):
        self.message = message
        self.element = element
        self.line = line
        super().__init__(self._format_message())
    
    def _format_message(self) -> str:
        parts = [self.message]
        if self.element:
            parts.append(f"Element: {self.element}")
        if self.line:
            parts.append(f"Line: {self.line}")
        return " | ".join(parts)
    
    def __str__(self) -> str:
        return self._format_message()


@dataclass
class ARXMLNamespace:
    """ARXML namespace configuration"""
    autosar: str = "http://autosar.org/schema/r4.0"
    xsi: str = "http://www.w3.org/2001/XMLSchema-instance"
    
    def nsmap(self) -> Dict[str, str]:
        """Get namespace map for ElementTree"""
        return {
            "autosar": self.autosar,
            "xsi": self.xsi
        }


class ARXMLConverter:
    """
    DDS Configuration ARXML Converter
    
    Supports:
    - YAML to ARXML conversion
    - ARXML to YAML conversion
    - AUTOSAR 4.x format
    - DdsDomainParticipantConfig elements
    - DdsTopicConfig elements
    - DdsQosProfile elements
    """
    
    # AUTOSAR ARXML namespaces
    NS = "{http://autosar.org/schema/r4.0}"
    NSMAP = {
        "": "http://autosar.org/schema/r4.0"
    }
    
    def __init__(self, namespace: Optional[ARXMLNamespace] = None):
        self.ns = namespace or ARXMLNamespace()
        self.warnings: List[str] = []
    
    def yaml_to_arxml(self, config: DDSConfig, 
                      package_name: str = "DDSConfiguration") -> str:
        """
        Convert DDSConfig to ARXML string
        
        Args:
            config: DDSConfig object
            package_name: Name for the AR package
            
        Returns:
            ARXML string
        """
        root = ET.Element("AUTOSAR")
        root.set("xmlns", self.ns.autosar)
        
        # Create AR-PACKAGES
        ar_packages = ET.SubElement(root, "AR-PACKAGES")
        ar_package = ET.SubElement(ar_packages, "AR-PACKAGE")
        
        # Add SHORT-NAME
        short_name = ET.SubElement(ar_package, "SHORT-NAME")
        short_name.text = package_name
        
        # Create ELEMENTS
        elements = ET.SubElement(ar_package, "ELEMENTS")
        
        # Add DDS Domain configurations
        for domain in config.domains:
            domain_elem = self._create_domain_element(domain)
            elements.append(domain_elem)
        
        # Add DDS QoS Profiles
        for profile_name, qos in config.qos_profiles.items():
            qos_elem = self._create_qos_profile_element(profile_name, qos)
            elements.append(qos_elem)
        
        # Add DDS Topics
        for topic in config.get_all_topics():
            topic_elem = self._create_topic_element(topic)
            elements.append(topic_elem)
        
        # Add Transport configuration
        transport_elem = self._create_transport_element(config.transport)
        elements.append(transport_elem)
        
        # Add TSN configuration
        tsn_elem = self._create_tsn_element(config.tsn)
        elements.append(tsn_elem)
        
        # Add Security configuration
        security_elem = self._create_security_element(config.security)
        elements.append(security_elem)
        
        # Pretty print XML
        return self._prettify_xml(root)
    
    def arxml_to_yaml(self, arxml_string: str) -> DDSConfig:
        """
        Convert ARXML string to DDSConfig
        
        Args:
            arxml_string: ARXML content
            
        Returns:
            DDSConfig object
        """
        try:
            root = ET.fromstring(arxml_string)
        except ET.ParseError as e:
            raise ARXMLConversionError(f"Invalid ARXML: {e}")
        
        # Find AR-PACKAGE
        ar_packages = root.find(f".//{self.NS}AR-PACKAGES")
        if ar_packages is None:
            raise ARXMLConversionError("Missing AR-PACKAGES element")
        
        ar_package = ar_packages.find(f"{self.NS}AR-PACKAGE")
        if ar_package is None:
            raise ARXMLConversionError("Missing AR-PACKAGE element")
        
        elements = ar_package.find(f"{self.NS}ELEMENTS")
        if elements is None:
            raise ARXMLConversionError("Missing ELEMENTS section")
        
        # Parse configuration elements
        system = SystemInfo(name="ImportedConfig", version="1.0.0")
        domains: List[Domain] = []
        qos_profiles: Dict[str, QoS] = {}
        topics: List[Topic] = []
        transport = Transport()
        tsn = TSN()
        security = Security()
        
        for elem in elements:
            tag = self._strip_namespace(elem.tag)
            
            if tag == "DDS-DOMAIN-PARTICIPANT-CONFIG":
                domain = self._parse_domain_element(elem)
                domains.append(domain)
            elif tag == "DDS-QOS-PROFILE":
                name, qos = self._parse_qos_profile_element(elem)
                qos_profiles[name] = qos
            elif tag == "DDS-TOPIC-CONFIG":
                topic = self._parse_topic_element(elem)
                topics.append(topic)
            elif tag == "DDS-TRANSPORT-CONFIG":
                transport = self._parse_transport_element(elem)
            elif tag == "DDS-TSN-CONFIG":
                tsn = self._parse_tsn_element(elem)
            elif tag == "DDS-SECURITY-CONFIG":
                security = self._parse_security_element(elem)
        
        return DDSConfig(
            system=system,
            domains=domains,
            transport=transport,
            tsn=tsn,
            security=security,
            qos_profiles=qos_profiles,
            topics=topics
        )
    
    def _create_domain_element(self, domain: Domain) -> ET.Element:
        """Create DDS-DOMAIN-PARTICIPANT-CONFIG element"""
        elem = ET.Element("DDS-DOMAIN-PARTICIPANT-CONFIG")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = f"Domain{domain.id}_{domain.name}"
        
        # DOMAIN-ID
        domain_id = ET.SubElement(elem, "DOMAIN-ID")
        domain_id.text = str(domain.id)
        
        # Add participants
        for participant in domain.participants:
            participant_elem = self._create_participant_element(participant)
            elem.append(participant_elem)
        
        return elem
    
    def _create_participant_element(self, participant: Participant) -> ET.Element:
        """Create DDS-DOMAIN-PARTICIPANT element"""
        elem = ET.Element("DDS-DOMAIN-PARTICIPANT")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = participant.name
        
        # QoS Profile reference
        if participant.qos_profile:
            qos_ref = ET.SubElement(elem, "QOS-PROFILE-REF")
            qos_ref.text = participant.qos_profile
            qos_ref.set("DEST", "DDS-QOS-PROFILE")
        
        # Publishers
        for pub in participant.publishers:
            pub_elem = self._create_publisher_element(pub)
            elem.append(pub_elem)
        
        # Subscribers
        for sub in participant.subscribers:
            sub_elem = self._create_subscriber_element(sub)
            elem.append(sub_elem)
        
        return elem
    
    def _create_publisher_element(self, publisher: Publisher) -> ET.Element:
        """Create DDS-PUBLISHER element"""
        elem = ET.Element("DDS-PUBLISHER")
        
        # TOPIC-REF
        topic_ref = ET.SubElement(elem, "TOPIC-REF")
        topic_ref.text = publisher.topic
        topic_ref.set("DEST", "DDS-TOPIC-CONFIG")
        
        # Data type
        if publisher.type:
            data_type = ET.SubElement(elem, "DATA-TYPE")
            data_type.text = publisher.type
        
        # QoS
        qos_elem = self._create_qos_element(publisher.qos)
        elem.append(qos_elem)
        
        return elem
    
    def _create_subscriber_element(self, subscriber: Subscriber) -> ET.Element:
        """Create DDS-SUBSCRIBER element"""
        elem = ET.Element("DDS-SUBSCRIBER")
        
        # TOPIC-REF
        topic_ref = ET.SubElement(elem, "TOPIC-REF")
        topic_ref.text = subscriber.topic
        topic_ref.set("DEST", "DDS-TOPIC-CONFIG")
        
        # Data type
        if subscriber.type:
            data_type = ET.SubElement(elem, "DATA-TYPE")
            data_type.text = subscriber.type
        
        # QoS
        qos_elem = self._create_qos_element(subscriber.qos)
        elem.append(qos_elem)
        
        return elem
    
    def _create_qos_element(self, qos: QoS) -> ET.Element:
        """Create DDS-QOS element"""
        elem = ET.Element("DDS-QOS")
        
        # Reliability
        reliability = ET.SubElement(elem, "RELIABILITY")
        reliability.text = qos.reliability.value
        
        # Durability
        durability = ET.SubElement(elem, "DURABILITY")
        durability.text = qos.durability.value
        
        # History
        history = ET.SubElement(elem, "HISTORY")
        history.text = qos.history.kind.value
        
        # History depth
        history_depth = ET.SubElement(elem, "HISTORY-DEPTH")
        history_depth.text = str(qos.history.depth)
        
        # Deadline
        if qos.deadline:
            deadline = ET.SubElement(elem, "DEADLINE")
            sec = ET.SubElement(deadline, "SEC")
            sec.text = str(qos.deadline.sec)
            nanosec = ET.SubElement(deadline, "NANOSEC")
            nanosec.text = str(qos.deadline.nanosec)
        
        # Liveliness
        liveliness = ET.SubElement(elem, "LIVELINESS")
        liveliness.text = qos.liveliness.value
        
        return elem
    
    def _create_qos_profile_element(self, name: str, qos: QoS) -> ET.Element:
        """Create DDS-QOS-PROFILE element"""
        elem = ET.Element("DDS-QOS-PROFILE")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = name
        
        # QoS settings
        qos_elem = self._create_qos_element(qos)
        elem.append(qos_elem)
        
        return elem
    
    def _create_topic_element(self, topic: Topic) -> ET.Element:
        """Create DDS-TOPIC-CONFIG element"""
        elem = ET.Element("DDS-TOPIC-CONFIG")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = topic.name
        
        # DATA-TYPE
        data_type = ET.SubElement(elem, "DATA-TYPE")
        data_type.text = topic.type
        
        # Description
        if topic.description:
            desc = ET.SubElement(elem, "DESC")
            desc.text = topic.description
        
        # QoS
        qos_elem = self._create_qos_element(topic.qos)
        elem.append(qos_elem)
        
        return elem
    
    def _create_transport_element(self, transport: Transport) -> ET.Element:
        """Create DDS-TRANSPORT-CONFIG element"""
        elem = ET.Element("DDS-TRANSPORT-CONFIG")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = "TransportConfig"
        
        # KIND
        kind = ET.SubElement(elem, "KIND")
        kind.text = transport.kind.value
        
        # Interface
        if transport.interface:
            interface = ET.SubElement(elem, "INTERFACE")
            interface.text = transport.interface
        
        # Multicast address
        if transport.multicast_address:
            mc_addr = ET.SubElement(elem, "MULTICAST-ADDRESS")
            mc_addr.text = transport.multicast_address
        
        # Port base
        port_base = ET.SubElement(elem, "PORT-BASE")
        port_base.text = str(transport.port_base)
        
        return elem
    
    def _create_tsn_element(self, tsn: TSN) -> ET.Element:
        """Create DDS-TSN-CONFIG element"""
        elem = ET.Element("DDS-TSN-CONFIG")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = "TSNConfig"
        
        # ENABLED
        enabled = ET.SubElement(elem, "ENABLED")
        enabled.text = str(tsn.enabled).lower()
        
        # Streams
        for stream in tsn.streams:
            stream_elem = self._create_tsn_stream_element(stream)
            elem.append(stream_elem)
        
        return elem
    
    def _create_tsn_stream_element(self, stream: TSNStream) -> ET.Element:
        """Create TSN-STREAM element"""
        elem = ET.Element("TSN-STREAM")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = stream.name
        
        # PRIORITY
        priority = ET.SubElement(elem, "PRIORITY")
        priority.text = str(stream.priority)
        
        # BANDWIDTH
        bandwidth = ET.SubElement(elem, "BANDWIDTH")
        bandwidth.text = str(stream.bandwidth)
        
        # MAX-LATENCY
        max_latency = ET.SubElement(elem, "MAX-LATENCY")
        max_latency.text = str(stream.max_latency)
        
        return elem
    
    def _create_security_element(self, security: Security) -> ET.Element:
        """Create DDS-SECURITY-CONFIG element"""
        elem = ET.Element("DDS-SECURITY-CONFIG")
        
        # SHORT-NAME
        short_name = ET.SubElement(elem, "SHORT-NAME")
        short_name.text = "SecurityConfig"
        
        # ENABLED
        enabled = ET.SubElement(elem, "ENABLED")
        enabled.text = str(security.enabled).lower()
        
        # Authentication
        if security.authentication:
            auth = ET.SubElement(elem, "AUTHENTICATION")
            auth.text = "true"
            
            if security.identity_certificate:
                cert = ET.SubElement(elem, "IDENTITY-CERTIFICATE")
                cert.text = security.identity_certificate
        
        # Encryption
        if security.encryption:
            enc = ET.SubElement(elem, "ENCRYPTION")
            enc.text = "true"
        
        return elem
    
    def _parse_domain_element(self, elem: ET.Element) -> Domain:
        """Parse DDS-DOMAIN-PARTICIPANT-CONFIG element"""
        domain_id = 0
        name = "Domain"
        participants: List[Participant] = []
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "DOMAIN-ID":
                domain_id = int(child.text or 0)
            elif tag == "SHORT-NAME":
                # Extract name from SHORT-NAME
                short_name = child.text or "Domain"
                if "_" in short_name:
                    name = short_name.split("_", 1)[1]
            elif tag == "DDS-DOMAIN-PARTICIPANT":
                participant = self._parse_participant_element(child)
                participants.append(participant)
        
        return Domain(id=domain_id, name=name, participants=participants)
    
    def _parse_participant_element(self, elem: ET.Element) -> Participant:
        """Parse DDS-DOMAIN-PARTICIPANT element"""
        name = ""
        qos_profile = None
        publishers: List[Publisher] = []
        subscribers: List[Subscriber] = []
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "SHORT-NAME":
                name = child.text or ""
            elif tag == "QOS-PROFILE-REF":
                qos_profile = child.text
            elif tag == "DDS-PUBLISHER":
                publisher = self._parse_publisher_element(child)
                publishers.append(publisher)
            elif tag == "DDS-SUBSCRIBER":
                subscriber = self._parse_subscriber_element(child)
                subscribers.append(subscriber)
        
        return Participant(
            name=name,
            qos_profile=qos_profile,
            publishers=publishers,
            subscribers=subscribers
        )
    
    def _parse_publisher_element(self, elem: ET.Element) -> Publisher:
        """Parse DDS-PUBLISHER element"""
        topic = ""
        data_type = None
        qos = QoS()
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "TOPIC-REF":
                topic = child.text or ""
            elif tag == "DATA-TYPE":
                data_type = child.text
            elif tag == "DDS-QOS":
                qos = self._parse_qos_element(child)
        
        return Publisher(topic=topic, type=data_type, qos=qos)
    
    def _parse_subscriber_element(self, elem: ET.Element) -> Subscriber:
        """Parse DDS-SUBSCRIBER element"""
        topic = ""
        data_type = None
        qos = QoS()
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "TOPIC-REF":
                topic = child.text or ""
            elif tag == "DATA-TYPE":
                data_type = child.text
            elif tag == "DDS-QOS":
                qos = self._parse_qos_element(child)
        
        return Subscriber(topic=topic, type=data_type, qos=qos)
    
    def _parse_qos_element(self, elem: ET.Element) -> QoS:
        """Parse DDS-QOS element"""
        qos_data: Dict[str, Any] = {}
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "RELIABILITY":
                qos_data["reliability"] = child.text
            elif tag == "DURABILITY":
                qos_data["durability"] = child.text
            elif tag == "HISTORY":
                qos_data["history"] = child.text
            elif tag == "HISTORY-DEPTH":
                if "history" not in qos_data:
                    qos_data["history"] = {}
                if isinstance(qos_data["history"], str):
                    qos_data["history"] = {"kind": qos_data["history"]}
                qos_data["history"]["depth"] = int(child.text or 1)
            elif tag == "DEADLINE":
                deadline: Dict[str, int] = {}
                for deadline_child in child:
                    deadline_tag = self._strip_namespace(deadline_child.tag)
                    if deadline_tag == "SEC":
                        deadline["sec"] = int(deadline_child.text or 0)
                    elif deadline_tag == "NANOSEC":
                        deadline["nanosec"] = int(deadline_child.text or 0)
                qos_data["deadline"] = deadline
            elif tag == "LIVELINESS":
                qos_data["liveliness"] = child.text
        
        return QoS.from_dict(qos_data)
    
    def _parse_qos_profile_element(self, elem: ET.Element) -> Tuple[str, QoS]:
        """Parse DDS-QOS-PROFILE element"""
        name = ""
        qos = QoS()
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "SHORT-NAME":
                name = child.text or ""
            elif tag == "DDS-QOS":
                qos = self._parse_qos_element(child)
        
        return name, qos
    
    def _parse_topic_element(self, elem: ET.Element) -> Topic:
        """Parse DDS-TOPIC-CONFIG element"""
        name = ""
        data_type = ""
        qos = QoS()
        description = None
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "SHORT-NAME":
                name = child.text or ""
            elif tag == "DATA-TYPE":
                data_type = child.text or ""
            elif tag == "DESC":
                description = child.text
            elif tag == "DDS-QOS":
                qos = self._parse_qos_element(child)
        
        return Topic(name=name, type=data_type, qos=qos, description=description)
    
    def _parse_transport_element(self, elem: ET.Element) -> Transport:
        """Parse DDS-TRANSPORT-CONFIG element"""
        data: Dict[str, Any] = {}
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "KIND":
                data["kind"] = child.text
            elif tag == "INTERFACE":
                data["interface"] = child.text
            elif tag == "MULTICAST-ADDRESS":
                data["multicast_address"] = child.text
            elif tag == "PORT-BASE":
                data["port_base"] = int(child.text or 7400)
        
        return Transport.from_dict(data)
    
    def _parse_tsn_element(self, elem: ET.Element) -> TSN:
        """Parse DDS-TSN-CONFIG element"""
        enabled = False
        streams: List[TSNStream] = []
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "ENABLED":
                enabled = child.text.lower() == "true" if child.text else False
            elif tag == "TSN-STREAM":
                stream = self._parse_tsn_stream_element(child)
                streams.append(stream)
        
        return TSN(enabled=enabled, streams=streams)
    
    def _parse_tsn_stream_element(self, elem: ET.Element) -> TSNStream:
        """Parse TSN-STREAM element"""
        name = ""
        priority = 0
        bandwidth = 0
        max_latency = 0
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "SHORT-NAME":
                name = child.text or ""
            elif tag == "PRIORITY":
                priority = int(child.text or 0)
            elif tag == "BANDWIDTH":
                bandwidth = int(child.text or 0)
            elif tag == "MAX-LATENCY":
                max_latency = int(child.text or 0)
        
        return TSNStream(
            name=name,
            priority=priority,
            bandwidth=bandwidth,
            max_latency=max_latency
        )
    
    def _parse_security_element(self, elem: ET.Element) -> Security:
        """Parse DDS-SECURITY-CONFIG element"""
        data: Dict[str, Any] = {"enabled": False}
        
        for child in elem:
            tag = self._strip_namespace(child.tag)
            
            if tag == "ENABLED":
                data["enabled"] = child.text.lower() == "true" if child.text else False
            elif tag == "AUTHENTICATION":
                data["authentication"] = child.text.lower() == "true" if child.text else False
            elif tag == "ENCRYPTION":
                data["encryption"] = child.text.lower() == "true" if child.text else False
            elif tag == "IDENTITY-CERTIFICATE":
                data["identity_certificate"] = child.text
        
        return Security.from_dict(data)
    
    def _strip_namespace(self, tag: str) -> str:
        """Remove namespace from XML tag"""
        if "}" in tag:
            return tag.split("}", 1)[1]
        return tag
    
    def _prettify_xml(self, elem: ET.Element) -> str:
        """Pretty print XML element"""
        rough_string = ET.tostring(elem, encoding='unicode')
        reparsed = minidom.parseString(rough_string)
        
        # Remove empty text nodes and format
        pretty = reparsed.toprettyxml(indent="  ")
        
        # Remove extra blank lines
        lines = [line for line in pretty.split('\n') if line.strip()]
        
        # Add XML declaration
        result = '<?xml version="1.0" encoding="UTF-8"?>\n' + '\n'.join(lines[1:])
        return result
    
    def get_warnings(self) -> List[str]:
        """Get list of conversion warnings"""
        return self.warnings


def yaml_to_arxml(config: DDSConfig, package_name: str = "DDSConfiguration") -> str:
    """
    Convenience function to convert DDSConfig to ARXML
    
    Args:
        config: DDSConfig object
        package_name: Name for the AR package
        
    Returns:
        ARXML string
    """
    converter = ARXMLConverter()
    return converter.yaml_to_arxml(config, package_name)


def arxml_to_yaml(arxml_string: str) -> DDSConfig:
    """
    Convenience function to convert ARXML to DDSConfig
    
    Args:
        arxml_string: ARXML content
        
    Returns:
        DDSConfig object
        
    Raises:
        ARXMLConversionError: If conversion fails
    """
    converter = ARXMLConverter()
    return converter.arxml_to_yaml(arxml_string)


def convert_file_yaml_to_arxml(yaml_path: str, arxml_path: str,
                               package_name: str = "DDSConfiguration") -> None:
    """
    Convert YAML file to ARXML file
    
    Args:
        yaml_path: Path to input YAML file
        arxml_path: Path to output ARXML file
        package_name: Name for the AR package
    """
    from .yaml_parser import parse_yaml_file
    
    config, warnings = parse_yaml_file(yaml_path)
    
    converter = ARXMLConverter()
    for warning in warnings:
        converter.warnings.append(warning)
    
    arxml_content = converter.yaml_to_arxml(config, package_name)
    
    with open(arxml_path, 'w', encoding='utf-8') as f:
        f.write(arxml_content)


def convert_file_arxml_to_yaml(arxml_path: str, yaml_path: str) -> None:
    """
    Convert ARXML file to YAML file
    
    Args:
        arxml_path: Path to input ARXML file
        yaml_path: Path to output YAML file
    """
    from .yaml_parser import write_yaml_file
    
    with open(arxml_path, 'r', encoding='utf-8') as f:
        arxml_content = f.read()
    
    converter = ARXMLConverter()
    config = converter.arxml_to_yaml(arxml_content)
    
    write_yaml_file(config, yaml_path, include_comments=True)
