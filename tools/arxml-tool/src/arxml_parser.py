#!/usr/bin/env python3
"""
ARXML Parser Core Module

A comprehensive parser for AUTOSAR R4.0 ARXML files.
Supports parsing of ECU configurations, software components, interfaces,
data types, and system mappings with XPath query capabilities.

Author: YuleTech
Version: 1.0.0
"""

import xml.etree.ElementTree as ET
from xml.etree.ElementTree import Element
from pathlib import Path
from typing import Dict, List, Optional, Union, Any, Callable
from dataclasses import dataclass, field
from enum import Enum
import logging
from lxml import etree

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class AutosarVersion(Enum):
    """AUTOSAR schema versions"""
    R4_0_1 = "4.0.1"
    R4_0_2 = "4.0.2"
    R4_0_3 = "4.0.3"
    R4_1_1 = "4.1.1"
    R4_2_1 = "4.2.1"
    R4_3_0 = "4.3.0"
    R4_4_0 = "4.4.0"


class PortInterfaceType(Enum):
    """Port interface types"""
    SENDER_RECEIVER = "SENDER-RECEIVER-INTERFACE"
    CLIENT_SERVER = "CLIENT-SERVER-INTERFACE"
    MODE_SWITCH = "MODE-SWITCH-INTERFACE"
    NV_DATA = "NV-DATA-INTERFACE"
    PARAMETER = "PARAMETER-INTERFACE"
    TRIGGER = "TRIGGER-INTERFACE"


@dataclass
class DataElement:
    """Data element definition"""
    name: str
    type_ref: str
    category: Optional[str] = None
    length: Optional[int] = None
    unit: Optional[str] = None
    description: Optional[str] = None


@dataclass
class PortInterface:
    """Port interface definition"""
    name: str
    interface_type: PortInterfaceType
    data_elements: List[DataElement] = field(default_factory=list)
    operations: List[Dict[str, Any]] = field(default_factory=list)
    description: Optional[str] = None


@dataclass
class Port:
    """Port definition"""
    name: str
    port_type: str  # 'P-PORT' or 'R-PORT'
    interface_ref: str
    interface_name: Optional[str] = None
    description: Optional[str] = None


@dataclass
class SoftwareComponent:
    """Software component definition"""
    name: str
    component_type: str
    ports: List[Port] = field(default_factory=list)
    internal_behaviors: List[Dict[str, Any]] = field(default_factory=list)
    description: Optional[str] = None


@dataclass
class EcuConfiguration:
    """ECU configuration definition"""
    name: str
    ecu_id: Optional[str] = None
    modules: List[Dict[str, Any]] = field(default_factory=list)
    partition_mappings: List[Dict[str, Any]] = field(default_factory=list)
    description: Optional[str] = None


@dataclass
class DataType:
    """Data type definition"""
    name: str
    type_category: str  # 'APPLICATION', 'IMPLEMENTATION', 'BASE'
    base_type: Optional[str] = None
    compu_method: Optional[str] = None
    data_constraint: Optional[str] = None
    value_range: Optional[Dict[str, Any]] = None
    description: Optional[str] = None


@dataclass
class SystemMapping:
    """System mapping definition"""
    name: str
    mappings: List[Dict[str, Any]] = field(default_factory=list)
    connections: List[Dict[str, Any]] = field(default_factory=list)
    description: Optional[str] = None


class ArxmlParser:
    """
    ARXML Parser for AUTOSAR R4.0 files
    
    Provides comprehensive parsing capabilities for AUTOSAR ARXML files
    including ECU configurations, software components, interfaces, data types,
    and system mappings.
    
    Attributes:
        namespaces: Dictionary of XML namespaces
        autosar_version: Detected AUTOSAR version
        schema_path: Path to XSD schema for validation
    
    Example:
        >>> parser = ArxmlParser()
        >>> parser.parse("system.arxml")
        >>> swcs = parser.get_software_components()
        >>> ecu_configs = parser.get_ecu_configurations()
    """
    
    # AUTOSAR R4.0 namespaces
    NAMESPACES = {
        'autosar': 'http://autosar.org/schema/r4.0',
        'xsi': 'http://www.w3.org/2001/XMLSchema-instance'
    }
    
    def __init__(self, schema_path: Optional[str] = None):
        """
        Initialize ARXML parser
        
        Args:
            schema_path: Optional path to XSD schema file for validation
        """
        self.root: Optional[Element] = None
        self.tree: Optional[ET.ElementTree] = None
        self.schema_path = schema_path
        self.schema: Optional[etree.XMLSchema] = None
        self.autosar_version: Optional[AutosarVersion] = None
        
        # Parsed elements storage
        self._swcs: List[SoftwareComponent] = []
        self._ecu_configs: List[EcuConfiguration] = []
        self._interfaces: List[PortInterface] = []
        self._data_types: List[DataType] = []
        self._system_mappings: List[SystemMapping] = []
        
        if schema_path:
            self._load_schema(schema_path)
    
    def _load_schema(self, schema_path: str) -> None:
        """
        Load XSD schema for validation
        
        Args:
            schema_path: Path to XSD schema file
        """
        try:
            schema_doc = etree.parse(schema_path)
            self.schema = etree.XMLSchema(schema_doc)
            logger.info(f"Loaded schema from: {schema_path}")
        except Exception as e:
            logger.error(f"Failed to load schema: {e}")
            raise
    
    def parse(self, arxml_path: Union[str, Path]) -> 'ArxmlParser':
        """
        Parse an ARXML file
        
        Args:
            arxml_path: Path to ARXML file
            
        Returns:
            Self for method chaining
            
        Raises:
            FileNotFoundError: If file doesn't exist
            ET.ParseError: If XML parsing fails
        """
        arxml_path = Path(arxml_path)
        
        if not arxml_path.exists():
            raise FileNotFoundError(f"ARXML file not found: {arxml_path}")
        
        logger.info(f"Parsing ARXML file: {arxml_path}")
        
        # Parse XML
        self.tree = ET.parse(arxml_path)
        self.root = self.tree.getroot()
        
        # Detect AUTOSAR version
        self._detect_version()
        
        # Parse all elements
        self._parse_all()
        
        logger.info("ARXML parsing completed")
        return self
    
    def parse_string(self, xml_content: str) -> 'ArxmlParser':
        """
        Parse ARXML content from string
        
        Args:
            xml_content: XML content as string
            
        Returns:
            Self for method chaining
        """
        logger.info("Parsing ARXML from string")
        
        self.root = ET.fromstring(xml_content)
        self.tree = ET.ElementTree(self.root)
        
        self._detect_version()
        self._parse_all()
        
        return self
    
    def _detect_version(self) -> None:
        """Detect AUTOSAR version from namespace"""
        if self.root is None:
            return
        
        # Try to get version from root tag attributes
        xsi_ns = '{http://www.w3.org/2001/XMLSchema-instance}'
        version_attr = f'{xsi_ns}schemaVersion'
        
        version = self.root.get(version_attr)
        if version:
            for v in AutosarVersion:
                if v.value == version:
                    self.autosar_version = v
                    break
        
        logger.info(f"Detected AUTOSAR version: {self.autosar_version}")
    
    def validate(self, arxml_path: Optional[Union[str, Path]] = None) -> bool:
        """
        Validate ARXML against XSD schema
        
        Args:
            arxml_path: Optional path to ARXML file (uses parsed tree if None)
            
        Returns:
            True if validation passes, False otherwise
        """
        if self.schema is None:
            logger.warning("No schema loaded, skipping validation")
            return True
        
        try:
            if arxml_path:
                doc = etree.parse(str(arxml_path))
            elif self.tree:
                # Convert ElementTree to lxml
                xml_string = ET.tostring(self.root, encoding='unicode')
                doc = etree.fromstring(xml_string.encode())
            else:
                logger.error("No XML content to validate")
                return False
            
            self.schema.assertValid(doc)
            logger.info("Schema validation passed")
            return True
            
        except etree.DocumentInvalid as e:
            logger.error(f"Schema validation failed: {e}")
            return False
        except Exception as e:
            logger.error(f"Validation error: {e}")
            return False
    
    def _parse_all(self) -> None:
        """Parse all AUTOSAR elements"""
        self._parse_software_components()
        self._parse_ecu_configurations()
        self._parse_interfaces()
        self._parse_data_types()
        self._parse_system_mappings()
    
    def _parse_software_components(self) -> None:
        """Parse software components from ARXML"""
        self._swcs = []
        
        swc_types = [
            'APPLICATION-SW-COMPONENT-TYPE',
            'SERVICE-SW-COMPONENT-TYPE',
            'COMPOSITION-SW-COMPONENT-TYPE',
            'SENSOR-ACTUATOR-SW-COMPONENT-TYPE',
            'PARAMETER-SW-COMPONENT-TYPE'
        ]
        
        for swc_type in swc_types:
            xpath = f".//autosar:{swc_type}"
            elements = self._xpath_query(xpath)
            
            for elem in elements:
                swc = self._parse_single_swc(elem, swc_type)
                if swc:
                    self._swcs.append(swc)
        
        logger.info(f"Parsed {len(self._swcs)} software components")
    
    def _parse_single_swc(self, elem: Element, swc_type: str) -> Optional[SoftwareComponent]:
        """Parse a single software component"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        ports = self._parse_ports(elem)
        
        return SoftwareComponent(
            name=name,
            component_type=swc_type.replace('-SW-COMPONENT-TYPE', ''),
            ports=ports,
            description=description
        )
    
    def _parse_ports(self, swc_elem: Element) -> List[Port]:
        """Parse ports from software component element"""
        ports = []
        
        # P-PORTs
        for pport in swc_elem.findall('.//autosar:P-PORT-PROTOTYPE', self.NAMESPACES):
            port = self._parse_single_port(pport, 'P-PORT')
            if port:
                ports.append(port)
        
        # R-PORTs
        for rport in swc_elem.findall('.//autosar:R-PORT-PROTOTYPE', self.NAMESPACES):
            port = self._parse_single_port(rport, 'R-PORT')
            if port:
                ports.append(port)
        
        return ports
    
    def _parse_single_port(self, elem: Element, port_type: str) -> Optional[Port]:
        """Parse a single port"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        
        # Get interface reference
        interface_ref = None
        interface_name = None
        
        if port_type == 'P-PORT':
            ref_elem = elem.find('autosar:PROVIDED-INTERFACE-TREF', self.NAMESPACES)
        else:
            ref_elem = elem.find('autosar:REQUIRED-INTERFACE-TREF', self.NAMESPACES)
        
        if ref_elem is not None and ref_elem.text:
            interface_ref = ref_elem.text
            # Extract interface name from reference path
            interface_name = interface_ref.split('/')[-1]
        
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        return Port(
            name=name,
            port_type=port_type,
            interface_ref=interface_ref or '',
            interface_name=interface_name,
            description=description
        )
    
    def _parse_ecu_configurations(self) -> None:
        """Parse ECU configurations from ARXML"""
        self._ecu_configs = []
        
        ecu_configs = self._xpath_query(".//autosar:ECU-CONFIGURATION")
        
        for elem in ecu_configs:
            ecu_config = self._parse_single_ecu_config(elem)
            if ecu_config:
                self._ecu_configs.append(ecu_config)
        
        logger.info(f"Parsed {len(self._ecu_configs)} ECU configurations")
    
    def _parse_single_ecu_config(self, elem: Element) -> Optional[EcuConfiguration]:
        """Parse a single ECU configuration"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        
        # Parse ECU ID
        ecu_elem = elem.find('autosar:ECU', self.NAMESPACES)
        ecu_id = ecu_elem.text if ecu_elem is not None else None
        
        # Parse module configurations
        modules = []
        for module in elem.findall('.//autosar:MODULE-CONFIGURATION', self.NAMESPACES):
            module_name = module.find('autosar:SHORT-NAME', self.NAMESPACES)
            if module_name is not None:
                modules.append({'name': module_name.text})
        
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        return EcuConfiguration(
            name=name,
            ecu_id=ecu_id,
            modules=modules,
            description=description
        )
    
    def _parse_interfaces(self) -> None:
        """Parse port interfaces from ARXML"""
        self._interfaces = []
        
        interface_types = [
            ('SENDER-RECEIVER-INTERFACE', PortInterfaceType.SENDER_RECEIVER),
            ('CLIENT-SERVER-INTERFACE', PortInterfaceType.CLIENT_SERVER),
            ('MODE-SWITCH-INTERFACE', PortInterfaceType.MODE_SWITCH),
            ('NV-DATA-INTERFACE', PortInterfaceType.NV_DATA),
            ('PARAMETER-INTERFACE', PortInterfaceType.PARAMETER)
        ]
        
        for tag, if_type in interface_types:
            xpath = f".//autosar:{tag}"
            elements = self._xpath_query(xpath)
            
            for elem in elements:
                interface = self._parse_single_interface(elem, if_type)
                if interface:
                    self._interfaces.append(interface)
        
        logger.info(f"Parsed {len(self._interfaces)} interfaces")
    
    def _parse_single_interface(self, elem: Element, 
                                if_type: PortInterfaceType) -> Optional[PortInterface]:
        """Parse a single interface"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        
        # Parse data elements for sender-receiver interfaces
        data_elements = []
        if if_type == PortInterfaceType.SENDER_RECEIVER:
            for de in elem.findall('.//autosar:DATA-ELEMENT-PROTOTYPE', self.NAMESPACES):
                de_name = de.find('autosar:SHORT-NAME', self.NAMESPACES)
                type_ref = de.find('.//autosar:TYPE-TREF', self.NAMESPACES)
                
                if de_name is not None:
                    data_elements.append(DataElement(
                        name=de_name.text,
                        type_ref=type_ref.text if type_ref is not None else ''
                    ))
        
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        return PortInterface(
            name=name,
            interface_type=if_type,
            data_elements=data_elements,
            description=description
        )
    
    def _parse_data_types(self) -> None:
        """Parse data types from ARXML"""
        self._data_types = []
        
        # Application data types
        app_types = self._xpath_query(".//autosar:APPLICATION-PRIMITIVE-DATA-TYPE")
        for elem in app_types:
            dtype = self._parse_single_data_type(elem, 'APPLICATION')
            if dtype:
                self._data_types.append(dtype)
        
        # Implementation data types
        impl_types = self._xpath_query(".//autosar:IMPLEMENTATION-DATA-TYPE")
        for elem in impl_types:
            dtype = self._parse_single_data_type(elem, 'IMPLEMENTATION')
            if dtype:
                self._data_types.append(dtype)
        
        logger.info(f"Parsed {len(self._data_types)} data types")
    
    def _parse_single_data_type(self, elem: Element, 
                                category: str) -> Optional[DataType]:
        """Parse a single data type"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        
        # Get category if available
        cat_elem = elem.find('autosar:CATEGORY', self.NAMESPACES)
        type_category = cat_elem.text if cat_elem is not None else category
        
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        return DataType(
            name=name,
            type_category=type_category,
            description=description
        )
    
    def _parse_system_mappings(self) -> None:
        """Parse system mappings from ARXML"""
        self._system_mappings = []
        
        mappings = self._xpath_query(".//autosar:SYSTEM-MAPPING")
        
        for elem in mappings:
            mapping = self._parse_single_system_mapping(elem)
            if mapping:
                self._system_mappings.append(mapping)
        
        logger.info(f"Parsed {len(self._system_mappings)} system mappings")
    
    def _parse_single_system_mapping(self, elem: Element) -> Optional[SystemMapping]:
        """Parse a single system mapping"""
        name_elem = elem.find('autosar:SHORT-NAME', self.NAMESPACES)
        if name_elem is None or not name_elem.text:
            return None
        
        name = name_elem.text
        
        # Parse ECU mappings
        mappings = []
        for ecu_mapping in elem.findall('.//autosar:ECU-MAPPING', self.NAMESPACES):
            comp_ref = ecu_mapping.find('autosar:COMPONENT-REF', self.NAMESPACES)
            ecu_ref = ecu_mapping.find('autosar:ECU-REF', self.NAMESPACES)
            
            if comp_ref is not None and ecu_ref is not None:
                mappings.append({
                    'component': comp_ref.text,
                    'ecu': ecu_ref.text
                })
        
        desc_elem = elem.find('autosar:DESC/autosar:L-2', self.NAMESPACES)
        description = desc_elem.text if desc_elem is not None else None
        
        return SystemMapping(
            name=name,
            mappings=mappings,
            description=description
        )
    
    def _xpath_query(self, xpath: str) -> List[Element]:
        """
        Execute XPath query on parsed XML
        
        Args:
            xpath: XPath expression with autosar namespace prefix
            
        Returns:
            List of matching Element objects
        """
        if self.root is None:
            return []
        
        return self.root.findall(xpath, self.NAMESPACES)
    
    def xpath(self, xpath: str, 
              namespaces: Optional[Dict[str, str]] = None) -> List[Element]:
        """
        Execute custom XPath query
        
        Args:
            xpath: XPath expression
            namespaces: Optional namespace dictionary (uses default if None)
            
        Returns:
            List of matching Element objects
        """
        if self.root is None:
            raise RuntimeError("No ARXML file parsed. Call parse() first.")
        
        ns = namespaces or self.NAMESPACES
        return self.root.findall(xpath, ns)
    
    def get_software_components(self) -> List[SoftwareComponent]:
        """
        Get parsed software components
        
        Returns:
            List of SoftwareComponent objects
        """
        return self._swcs
    
    def get_ecu_configurations(self) -> List[EcuConfiguration]:
        """
        Get parsed ECU configurations
        
        Returns:
            List of EcuConfiguration objects
        """
        return self._ecu_configs
    
    def get_interfaces(self) -> List[PortInterface]:
        """
        Get parsed port interfaces
        
        Returns:
            List of PortInterface objects
        """
        return self._interfaces
    
    def get_data_types(self) -> List[DataType]:
        """
        Get parsed data types
        
        Returns:
            List of DataType objects
        """
        return self._data_types
    
    def get_system_mappings(self) -> List[SystemMapping]:
        """
        Get parsed system mappings
        
        Returns:
            List of SystemMapping objects
        """
        return self._system_mappings
    
    def find_swc_by_name(self, name: str) -> Optional[SoftwareComponent]:
        """
        Find software component by name
        
        Args:
            name: Component name to search for
            
        Returns:
            SoftwareComponent if found, None otherwise
        """
        for swc in self._swcs:
            if swc.name == name:
                return swc
        return None
    
    def find_interface_by_name(self, name: str) -> Optional[PortInterface]:
        """
        Find interface by name
        
        Args:
            name: Interface name to search for
            
        Returns:
            PortInterface if found, None otherwise
        """
        for interface in self._interfaces:
            if interface.name == name:
                return interface
        return None
    
    def find_data_type_by_name(self, name: str) -> Optional[DataType]:
        """
        Find data type by name
        
        Args:
            name: Data type name to search for
            
        Returns:
            DataType if found, None otherwise
        """
        for dtype in self._data_types:
            if dtype.name == name:
                return dtype
        return None
    
    def get_summary(self) -> Dict[str, int]:
        """
        Get summary of parsed elements
        
        Returns:
            Dictionary with counts of each element type
        """
        return {
            'software_components': len(self._swcs),
            'ecu_configurations': len(self._ecu_configs),
            'interfaces': len(self._interfaces),
            'data_types': len(self._data_types),
            'system_mappings': len(self._system_mappings),
            'autosar_version': self.autosar_version.value if self.autosar_version else 'Unknown'
        }


class ArxmlQueryEngine:
    """
    Advanced query engine for ARXML content
    
    Provides complex filtering and search capabilities on parsed ARXML content.
    """
    
    def __init__(self, parser: ArxmlParser):
        """
        Initialize query engine
        
        Args:
            parser: Parsed ArxmlParser instance
        """
        self.parser = parser
    
    def find_ports_with_interface(self, interface_name: str) -> List[Dict[str, Any]]:
        """
        Find all ports that use a specific interface
        
        Args:
            interface_name: Name of the interface to search for
            
        Returns:
            List of dictionaries containing port and component info
        """
        results = []
        
        for swc in self.parser.get_software_components():
            for port in swc.ports:
                if port.interface_name == interface_name:
                    results.append({
                        'port_name': port.name,
                        'port_type': port.port_type,
                        'component_name': swc.name,
                        'interface_name': port.interface_name
                    })
        
        return results
    
    def find_connected_components(self, swc_name: str) -> List[Dict[str, Any]]:
        """
        Find components connected to a specific component
        
        Args:
            swc_name: Name of the component to analyze
            
        Returns:
            List of connected component information
        """
        results = []
        target_swc = self.parser.find_swc_by_name(swc_name)
        
        if not target_swc:
            return results
        
        # Get interfaces used by this component
        interface_names = [p.interface_name for p in target_swc.ports if p.interface_name]
        
        # Find other components using the same interfaces
        for swc in self.parser.get_software_components():
            if swc.name == swc_name:
                continue
            
            for port in swc.ports:
                if port.interface_name in interface_names:
                    results.append({
                        'component_name': swc.name,
                        'port_name': port.name,
                        'port_type': port.port_type,
                        'shared_interface': port.interface_name
                    })
        
        return results
    
    def filter_swcs_by_type(self, swc_type: str) -> List[SoftwareComponent]:
        """
        Filter software components by type
        
        Args:
            swc_type: Type of component (e.g., 'APPLICATION', 'SERVICE')
            
        Returns:
            List of matching SoftwareComponent objects
        """
        return [swc for swc in self.parser.get_software_components() 
                if swc.component_type == swc_type.upper()]
    
    def filter_interfaces_by_type(self, 
                                  interface_type: PortInterfaceType) -> List[PortInterface]:
        """
        Filter interfaces by type
        
        Args:
            interface_type: Type of interface to filter by
            
        Returns:
            List of matching PortInterface objects
        """
        return [iface for iface in self.parser.get_interfaces() 
                if iface.interface_type == interface_type]


# Convenience functions
def parse_arxml(arxml_path: Union[str, Path], 
                schema_path: Optional[str] = None) -> ArxmlParser:
    """
    Parse ARXML file and return parser instance
    
    Args:
        arxml_path: Path to ARXML file
        schema_path: Optional path to XSD schema for validation
        
    Returns:
        Configured ArxmlParser instance
    """
    parser = ArxmlParser(schema_path=schema_path)
    return parser.parse(arxml_path)


def validate_arxml(arxml_path: Union[str, Path], 
                   schema_path: str) -> bool:
    """
    Validate ARXML file against schema
    
    Args:
        arxml_path: Path to ARXML file
        schema_path: Path to XSD schema file
        
    Returns:
        True if valid, False otherwise
    """
    parser = ArxmlParser(schema_path=schema_path)
    return parser.validate(arxml_path)


if __name__ == '__main__':
    # Example usage
    import json
    
    print("ARXML Parser v1.0.0")
    print("==================")
    
    # Example: Parse ARXML file
    # parser = parse_arxml("example.arxml")
    # print(json.dumps(parser.get_summary(), indent=2))
