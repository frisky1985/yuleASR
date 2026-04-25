#!/usr/bin/env python3
"""
AUTOSAR ARXML Parser
Parses AUTOSAR ARXML files and extracts software component information
for DDS/RTE integration
"""

import xml.etree.ElementTree as ET
import re
import os
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Union, Any, Set
from pathlib import Path
from enum import Enum


class ARXMLTypeCategory(Enum):
    """ARXML data type categories"""
    APPLICATION_PRIMITIVE = "application_primitive"
    APPLICATION_ARRAY = "application_array"
    APPLICATION_RECORD = "application_record"
    IMPLEMENTATION = "implementation"
    BASE_TYPE = "base_type"


class PortInterfaceType(Enum):
    """Port interface types"""
    SENDER_RECEIVER = "sender_receiver"
    CLIENT_SERVER = "client_server"
    MODE_SWITCH = "mode_switch"
    NONVOLATILE_DATA = "nonvolatile_data"
    PARAMETER = "parameter"
    TRIGGER = "trigger"


@dataclass
class ARXMLAnnotation:
    """ARXML annotation/attribute"""
    name: str
    value: Any = None


@dataclass
class ARXMLDataType:
    """ARXML data type definition"""
    name: str
    uuid: Optional[str] = None
    category: ARXMLTypeCategory = ARXMLTypeCategory.APPLICATION_PRIMITIVE
    parent_package: str = ""
    sw_data_def_props: Dict[str, Any] = field(default_factory=dict)
    compu_method: Optional[str] = None
    unit: Optional[str] = None
    
    # For primitive types
    base_type_ref: Optional[str] = None
    
    # For array types
    element_type: Optional[str] = None
    array_size: int = 0
    
    # For record types
    elements: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class ARXMLDataElement:
    """Data element definition (for sender-receiver interfaces)"""
    name: str
    type_ref: str
    uuid: Optional[str] = None
    is_queued: bool = False
    queue_length: int = 1
    init_value: Any = None
    
    # QoS-related attributes for DDS mapping
    reliability: str = "BEST_EFFORT"  # BEST_EFFORT or RELIABLE
    durability: str = "VOLATILE"  # VOLATILE, TRANSIENT_LOCAL, etc.
    deadline_ms: Optional[int] = None


@dataclass
class ARXMLOperation:
    """Client-server operation"""
    name: str
    uuid: Optional[str] = None
    arguments: List[Dict[str, Any]] = field(default_factory=list)
    return_type: Optional[str] = None
    is_reentrant: bool = False


@dataclass
class ARXMLPortInterface:
    """Port interface definition"""
    name: str
    uuid: Optional[str] = None
    interface_type: PortInterfaceType = PortInterfaceType.SENDER_RECEIVER
    parent_package: str = ""
    
    # For sender-receiver
    data_elements: List[ARXMLDataElement] = field(default_factory=list)
    
    # For client-server
    operations: List[ARXMLOperation] = field(default_factory=list)
    
    # Mode management
    mode_group: Optional[str] = None


@dataclass
class ARXMLPort:
    """Software component port"""
    name: str
    uuid: Optional[str] = None
    interface_ref: str = ""
    is_provided: bool = True  # P-Port (provided) or R-Port (required)
    
    # DDS-specific mapping
    dds_topic_name: Optional[str] = None
    dds_qos_profile: Optional[str] = None


@dataclass
class ARXMLSoftwareComponent:
    """Software component (SWC)"""
    name: str
    uuid: Optional[str] = None
    component_type: str = "application"  # application, service, sensor_actuator, etc.
    parent_package: str = ""
    
    ports: List[ARXMLPort] = field(default_factory=list)
    internal_behaviors: List[Dict[str, Any]] = field(default_factory=list)
    
    # Implementation details
    implementation_uuid: Optional[str] = None
    programming_language: str = "C"
    
    # DDS mapping
    dds_domain_id: int = 0


@dataclass
class ARXMLComposition:
    """Composition SWC containing component instances and connections"""
    name: str
    uuid: Optional[str] = None
    parent_package: str = ""
    
    components: List[Dict[str, str]] = field(default_factory=list)  # instance -> type
    connectors: List[Dict[str, Any]] = field(default_factory=list)
    
    # Data flow mapping
    data_mappings: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class ARXMLECUConfiguration:
    """ECU configuration"""
    name: str
    uuid: Optional[str] = None
    ecu_id: str = ""
    
    # RTE configuration
    rte_generation_vendor: str = ""
    rte_version: str = ""
    
    # OS configuration refs
    os_ref: Optional[str] = None
    
    # Component instances on this ECU
    component_instances: List[str] = field(default_factory=list)


class AutosarARXMLParser:
    """
    AUTOSAR ARXML Parser
    
    Parses AUTOSAR 4.x ARXML files and extracts:
    - Software components (SWCs)
    - Port interfaces
    - Data types
    - Compositions
    - ECU configurations
    
    Supports DDS integration by extracting topic mapping information
    """
    
    # ARXML namespace URIs
    NAMESPACES = {
        'AR': 'http://autosar.org/schema/r4.0',
        'xsi': 'http://www.w3.org/2001/XMLSchema-instance'
    }
    
    def __init__(self):
        self.data_types: Dict[str, ARXMLDataType] = {}
        self.port_interfaces: Dict[str, ARXMLPortInterface] = {}
        self.software_components: Dict[str, ARXMLSoftwareComponent] = {}
        self.compositions: Dict[str, ARXMLComposition] = {}
        self.ecu_configs: Dict[str, ARXMLECUConfiguration] = {}
        
        self.packages: Dict[str, List[str]] = {}  # package name -> element UUIDs
        self.uuid_map: Dict[str, Any] = {}  # UUID -> object
        
        self._ns_prefix = "{http://autosar.org/schema/r4.0}"
        
    def parse_file(self, filepath: str) -> bool:
        """Parse an ARXML file"""
        try:
            tree = ET.parse(filepath)
            root = tree.getroot()
            
            # Detect namespace
            self._detect_namespace(root)
            
            # Parse packages
            self._parse_packages(root)
            
            return True
        except Exception as e:
            print(f"Error parsing ARXML file {filepath}: {e}")
            return False
    
    def parse_string(self, xml_content: str) -> bool:
        """Parse ARXML content from string"""
        try:
            root = ET.fromstring(xml_content)
            self._detect_namespace(root)
            self._parse_packages(root)
            return True
        except Exception as e:
            print(f"Error parsing ARXML content: {e}")
            return False
    
    def _detect_namespace(self, root: ET.Element):
        """Detect and set the ARXML namespace prefix"""
        # Get namespace from root tag
        tag = root.tag
        if '}' in tag:
            ns = tag.split('}')[0].strip('{')
            self._ns_prefix = f"{{{ns}}}"
        else:
            self._ns_prefix = ""
    
    def _get_tag(self, local_name: str) -> str:
        """Get full tag name with namespace"""
        return f"{self._ns_prefix}{local_name}"
    
    def _parse_packages(self, root: ET.Element):
        """Parse all AR packages"""
        # Find all AR-PACKAGE elements
        packages = root.findall(f".//{self._get_tag('AR-PACKAGE')}")
        
        for package in packages:
            self._parse_package(package)
    
    def _parse_package(self, package: ET.Element):
        """Parse a single AR package"""
        short_name_elem = package.find(f"./{self._get_tag('SHORT-NAME')}")
        if short_name_elem is None:
            return
        
        package_name = short_name_elem.text
        self.packages[package_name] = []
        
        # Parse elements in the package
        elements_elem = package.find(f"./{self._get_tag('ELEMENTS')}")
        if elements_elem is None:
            return
        
        for child in elements_elem:
            tag = child.tag.replace(self._ns_prefix, "")
            
            if tag == "APPLICATION-SW-COMPONENT-TYPE":
                self._parse_application_sw_component(child, package_name)
            elif tag == "SERVICE-SW-COMPONENT-TYPE":
                self._parse_service_sw_component(child, package_name)
            elif tag == "COMPOSITION-SW-COMPONENT-TYPE":
                self._parse_composition(child, package_name)
            elif tag == "SENDER-RECEIVER-INTERFACE":
                self._parse_sender_receiver_interface(child, package_name)
            elif tag == "CLIENT-SERVER-INTERFACE":
                self._parse_client_server_interface(child, package_name)
            elif tag == "APPLICATION-PRIMITIVE-DATA-TYPE":
                self._parse_primitive_data_type(child, package_name)
            elif tag == "APPLICATION-ARRAY-DATA-TYPE":
                self._parse_array_data_type(child, package_name)
            elif tag == "APPLICATION-RECORD-DATA-TYPE":
                self._parse_record_data_type(child, package_name)
            elif tag == "ECU-CONFIGURATION":
                self._parse_ecu_configuration(child, package_name)
    
    def _get_uuid(self, elem: ET.Element) -> Optional[str]:
        """Get UUID from element"""
        return elem.get('UUID')
    
    def _get_short_name(self, elem: ET.Element) -> str:
        """Get short name from element"""
        sn_elem = elem.find(f"./{self._get_tag('SHORT-NAME')}")
        return sn_elem.text if sn_elem is not None else ""
    
    def _parse_application_sw_component(self, elem: ET.Element, package: str):
        """Parse application software component"""
        swc = ARXMLSoftwareComponent(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            component_type="application",
            parent_package=package
        )
        
        # Parse ports
        ports_elem = elem.find(f"./{self._get_tag('PORTS')}")
        if ports_elem is not None:
            for port_elem in ports_elem:
                swc.ports.append(self._parse_port(port_elem))
        
        # Parse internal behavior
        behaviors_elem = elem.find(f"./{self._get_tag('INTERNAL-BEHAVIORS')}")
        if behaviors_elem is not None:
            for behavior_elem in behaviors_elem:
                swc.internal_behaviors.append(self._parse_internal_behavior(behavior_elem))
        
        self.software_components[swc.name] = swc
        self.uuid_map[swc.uuid] = swc if swc.uuid else None
        self.packages[package].append(swc.name)
    
    def _parse_service_sw_component(self, elem: ET.Element, package: str):
        """Parse service software component"""
        swc = ARXMLSoftwareComponent(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            component_type="service",
            parent_package=package
        )
        
        ports_elem = elem.find(f"./{self._get_tag('PORTS')}")
        if ports_elem is not None:
            for port_elem in ports_elem:
                swc.ports.append(self._parse_port(port_elem))
        
        self.software_components[swc.name] = swc
        self.uuid_map[swc.uuid] = swc if swc.uuid else None
        self.packages[package].append(swc.name)
    
    def _parse_port(self, elem: ET.Element) -> ARXMLPort:
        """Parse a port definition"""
        tag = elem.tag.replace(self._ns_prefix, "")
        
        port = ARXMLPort(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem)
        )
        
        # Determine port type
        if "P-PORT" in tag or "PROVIDED" in tag:
            port.is_provided = True
        elif "R-PORT" in tag or "REQUIRED" in tag:
            port.is_provided = False
        
        # Get interface reference
        iface_ref_elem = elem.find(f".//{self._get_tag('PORT-INTERFACE-REF')}")
        if iface_ref_elem is not None:
            port.interface_ref = iface_ref_elem.text
        
        return port
    
    def _parse_internal_behavior(self, elem: ET.Element) -> Dict[str, Any]:
        """Parse internal behavior"""
        behavior = {
            'name': self._get_short_name(elem),
            'uuid': self._get_uuid(elem),
            'runnables': [],
            'events': []
        }
        
        # Parse runnables
        runnables_elem = elem.find(f"./{self._get_tag('RUNNABLES')}")
        if runnables_elem is not None:
            for runnable_elem in runnables_elem:
                behavior['runnables'].append(self._parse_runnable(runnable_elem))
        
        return behavior
    
    def _parse_runnable(self, elem: ET.Element) -> Dict[str, Any]:
        """Parse a runnable entity"""
        return {
            'name': self._get_short_name(elem),
            'uuid': self._get_uuid(elem),
            'can_be_invoked_concurrently': elem.find(f"./{self._get_tag('CAN-BE-INVOKED-CONCURRENTLY')}") is not None,
            'min_start_interval': self._get_child_text(elem, 'MINIMUM-START-INTERVAL'),
            'symbol': self._get_child_text(elem, 'SYMBOL')
        }
    
    def _parse_sender_receiver_interface(self, elem: ET.Element, package: str):
        """Parse sender-receiver interface"""
        iface = ARXMLPortInterface(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            interface_type=PortInterfaceType.SENDER_RECEIVER,
            parent_package=package
        )
        
        # Parse data elements
        data_elements_elem = elem.find(f"./{self._get_tag('DATA-ELEMENTS')}")
        if data_elements_elem is not None:
            for de_elem in data_elements_elem:
                iface.data_elements.append(self._parse_data_element(de_elem))
        
        self.port_interfaces[iface.name] = iface
        self.uuid_map[iface.uuid] = iface if iface.uuid else None
    
    def _parse_client_server_interface(self, elem: ET.Element, package: str):
        """Parse client-server interface"""
        iface = ARXMLPortInterface(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            interface_type=PortInterfaceType.CLIENT_SERVER,
            parent_package=package
        )
        
        # Parse operations
        operations_elem = elem.find(f"./{self._get_tag('OPERATIONS')}")
        if operations_elem is not None:
            for op_elem in operations_elem:
                iface.operations.append(self._parse_operation(op_elem))
        
        self.port_interfaces[iface.name] = iface
    
    def _parse_data_element(self, elem: ET.Element) -> ARXMLDataElement:
        """Parse data element"""
        de = ARXMLDataElement(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            type_ref=self._get_child_text(elem, 'TYPE-TREF') or ""
        )
        
        # Parse isQueued
        is_queued_elem = elem.find(f"./{self._get_tag('IS-QUEUED')}")
        if is_queued_elem is not None:
            de.is_queued = is_queued_elem.text.lower() == 'true'
        
        # Parse queue length
        queue_length_elem = elem.find(f"./{self._get_tag('QUEUE-LENGTH')}")
        if queue_length_elem is not None:
            de.queue_length = int(queue_length_elem.text)
        
        return de
    
    def _parse_operation(self, elem: ET.Element) -> ARXMLOperation:
        """Parse client-server operation"""
        op = ARXMLOperation(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem)
        )
        
        # Parse arguments
        args_elem = elem.find(f"./{self._get_tag('ARGUMENTS')}")
        if args_elem is not None:
            for arg_elem in args_elem:
                op.arguments.append({
                    'name': self._get_short_name(arg_elem),
                    'type_ref': self._get_child_text(arg_elem, 'TYPE-TREF'),
                    'direction': 'IN' if 'IN' in arg_elem.tag else 'OUT'
                })
        
        return op
    
    def _parse_primitive_data_type(self, elem: ET.Element, package: str):
        """Parse application primitive data type"""
        dt = ARXMLDataType(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            category=ARXMLTypeCategory.APPLICATION_PRIMITIVE,
            parent_package=package
        )
        
        # Parse SW-DATA-DEF-PROPS
        sw_props_elem = elem.find(f"./{self._get_tag('SW-DATA-DEF-PROPS')}")
        if sw_props_elem is not None:
            dt.sw_data_def_props = self._parse_sw_data_def_props(sw_props_elem)
        
        self.data_types[dt.name] = dt
    
    def _parse_array_data_type(self, elem: ET.Element, package: str):
        """Parse application array data type"""
        dt = ARXMLDataType(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            category=ARXMLTypeCategory.APPLICATION_ARRAY,
            parent_package=package
        )
        
        # Parse element
        element_elem = elem.find(f"./{self._get_tag('ELEMENT')}")
        if element_elem is not None:
            dt.element_type = self._get_child_text(element_elem, 'TYPE-TREF')
            array_size_elem = element_elem.find(f".//{self._get_tag('MAX-NUMBER-OF-ELEMENTS')}")
            if array_size_elem is not None:
                dt.array_size = int(array_size_elem.text)
        
        self.data_types[dt.name] = dt
    
    def _parse_record_data_type(self, elem: ET.Element, package: str):
        """Parse application record (struct) data type"""
        dt = ARXMLDataType(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            category=ARXMLTypeCategory.APPLICATION_RECORD,
            parent_package=package
        )
        
        # Parse elements (fields)
        elements_elem = elem.find(f"./{self._get_tag('ELEMENTS')}")
        if elements_elem is not None:
            for field_elem in elements_elem:
                dt.elements.append({
                    'name': self._get_short_name(field_elem),
                    'type_ref': self._get_child_text(field_elem, 'TYPE-TREF')
                })
        
        self.data_types[dt.name] = dt
    
    def _parse_composition(self, elem: ET.Element, package: str):
        """Parse composition SWC"""
        comp = ARXMLComposition(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem),
            parent_package=package
        )
        
        # Parse components
        components_elem = elem.find(f"./{self._get_tag('COMPONENTS')}")
        if components_elem is not None:
            for comp_elem in components_elem:
                comp.components.append({
                    'instance_name': self._get_short_name(comp_elem),
                    'component_ref': self._get_child_text(comp_elem, 'TYPE-TREF')
                })
        
        # Parse connectors
        connectors_elem = elem.find(f"./{self._get_tag('CONNECTORS')}")
        if connectors_elem is not None:
            for conn_elem in connectors_elem:
                comp.connectors.append(self._parse_connector(conn_elem))
        
        self.compositions[comp.name] = comp
    
    def _parse_connector(self, elem: ET.Element) -> Dict[str, Any]:
        """Parse assembly or delegation connector"""
        tag = elem.tag.replace(self._ns_prefix, "")
        
        connector = {
            'type': tag,
            'provider': {
                'component': self._get_child_text(elem, 'CONTEXT-COMPONENT-REF'),
                'port': self._get_child_text(elem, 'TARGET-P-PORT-REF')
            },
            'requester': {
                'component': self._get_child_text(elem, 'CONTEXT-COMPONENT-REF'),
                'port': self._get_child_text(elem, 'TARGET-R-PORT-REF')
            }
        }
        
        return connector
    
    def _parse_ecu_configuration(self, elem: ET.Element, package: str):
        """Parse ECU configuration"""
        ecu = ARXMLECUConfiguration(
            name=self._get_short_name(elem),
            uuid=self._get_uuid(elem)
        )
        
        # Extract ECU ID
        ecu_id_elem = elem.find(f".//{self._get_tag('ECU-ID')}")
        if ecu_id_elem is not None:
            ecu.ecu_id = ecu_id_elem.text
        
        self.ecu_configs[ecu.name] = ecu
    
    def _parse_sw_data_def_props(self, elem: ET.Element) -> Dict[str, Any]:
        """Parse SW data definition properties"""
        props = {}
        
        # Parse compu method reference
        compu_method_elem = elem.find(f".//{self._get_tag('COMPU-METHOD-REF')}")
        if compu_method_elem is not None:
            props['compu_method'] = compu_method_elem.text
        
        # Parse unit reference
        unit_elem = elem.find(f".//{self._get_tag('UNIT-REF')}")
        if unit_elem is not None:
            props['unit'] = unit_elem.text
        
        # Parse data constraints
        constraints_elem = elem.find(f".//{self._get_tag('DATA-CONSTR-REF')}")
        if constraints_elem is not None:
            props['data_constraints'] = constraints_elem.text
        
        return props
    
    def _get_child_text(self, parent: ET.Element, tag: str) -> Optional[str]:
        """Get text content of a child element"""
        child = parent.find(f".//{self._get_tag(tag)}")
        return child.text if child is not None else None
    
    def get_dds_topic_mappings(self) -> List[Dict[str, Any]]:
        """
        Generate DDS topic mappings from port interfaces
        Maps sender-receiver interfaces to DDS topics
        """
        mappings = []
        
        for iface in self.port_interfaces.values():
            if iface.interface_type == PortInterfaceType.SENDER_RECEIVER:
                for data_element in iface.data_elements:
                    mapping = {
                        'interface_name': iface.name,
                        'data_element': data_element.name,
                        'topic_name': f"/{iface.parent_package}/{iface.name}/{data_element.name}",
                        'data_type': data_element.type_ref,
                        'qos_profile': self._infer_qos_profile(data_element),
                        'is_queued': data_element.is_queued
                    }
                    mappings.append(mapping)
        
        return mappings
    
    def _infer_qos_profile(self, data_element: ARXMLDataElement) -> str:
        """Infer DDS QoS profile from data element properties"""
        if data_element.is_queued:
            return "reliable_persistent"
        elif data_element.reliability == "RELIABLE":
            return "reliable_transient"
        else:
            return "best_effort_volatile"
    
    def get_component_data_flows(self) -> List[Dict[str, Any]]:
        """Get data flows between components"""
        flows = []
        
        for comp in self.compositions.values():
            for connector in comp.connectors:
                flow = {
                    'composition': comp.name,
                    'provider': connector['provider'],
                    'requester': connector['requester'],
                    'dds_mapping': {
                        'pattern': 'pub_sub',  # or 'rpc' for client-server
                        'topic': None  # Would be resolved from interface
                    }
                }
                flows.append(flow)
        
        return flows
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert parsed ARXML to dictionary"""
        return {
            'data_types': {
                name: self._data_type_to_dict(dt)
                for name, dt in self.data_types.items()
            },
            'port_interfaces': {
                name: self._interface_to_dict(iface)
                for name, iface in self.port_interfaces.items()
            },
            'software_components': {
                name: self._component_to_dict(swc)
                for name, swc in self.software_components.items()
            },
            'compositions': {
                name: self._composition_to_dict(comp)
                for name, comp in self.compositions.items()
            },
            'dds_topic_mappings': self.get_dds_topic_mappings(),
            'data_flows': self.get_component_data_flows()
        }
    
    def _data_type_to_dict(self, dt: ARXMLDataType) -> Dict:
        """Convert data type to dictionary"""
        result = {
            'name': dt.name,
            'uuid': dt.uuid,
            'category': dt.category.value,
            'package': dt.parent_package
        }
        
        if dt.category == ARXMLTypeCategory.APPLICATION_ARRAY:
            result['element_type'] = dt.element_type
            result['array_size'] = dt.array_size
        elif dt.category == ARXMLTypeCategory.APPLICATION_RECORD:
            result['elements'] = dt.elements
        elif dt.category == ARXMLTypeCategory.APPLICATION_PRIMITIVE:
            result['sw_data_def_props'] = dt.sw_data_def_props
        
        return result
    
    def _interface_to_dict(self, iface: ARXMLPortInterface) -> Dict:
        """Convert interface to dictionary"""
        result = {
            'name': iface.name,
            'uuid': iface.uuid,
            'type': iface.interface_type.value,
            'package': iface.parent_package
        }
        
        if iface.data_elements:
            result['data_elements'] = [
                {
                    'name': de.name,
                    'type': de.type_ref,
                    'is_queued': de.is_queued
                }
                for de in iface.data_elements
            ]
        
        if iface.operations:
            result['operations'] = [
                {
                    'name': op.name,
                    'arguments': op.arguments
                }
                for op in iface.operations
            ]
        
        return result
    
    def _component_to_dict(self, swc: ARXMLSoftwareComponent) -> Dict:
        """Convert component to dictionary"""
        return {
            'name': swc.name,
            'uuid': swc.uuid,
            'type': swc.component_type,
            'package': swc.parent_package,
            'ports': [
                {
                    'name': p.name,
                    'is_provided': p.is_provided,
                    'interface': p.interface_ref
                }
                for p in swc.ports
            ]
        }
    
    def _composition_to_dict(self, comp: ARXMLComposition) -> Dict:
        """Convert composition to dictionary"""
        return {
            'name': comp.name,
            'uuid': comp.uuid,
            'package': comp.parent_package,
            'components': comp.components,
            'connectors': comp.connectors
        }


# Example usage
if __name__ == '__main__':
    # Example ARXML content (simplified)
    arxml_content = '''<?xml version="1.0" encoding="UTF-8"?>
    <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
        <AR-PACKAGES>
            <AR-PACKAGE UUID="...">
                <SHORT-NAME>VehicleData</SHORT-NAME>
                <ELEMENTS>
                    <APPLICATION-SW-COMPONENT-TYPE UUID="comp-1">
                        <SHORT-NAME>SpeedSensor</SHORT-NAME>
                        <PORTS>
                            <P-PORT-PROTOTYPE UUID="port-1">
                                <SHORT-NAME>VehicleSpeed</SHORT-NAME>
                                <PROVIDED-INTERFACE-TREF DEST="SENDER-RECEIVER-INTERFACE">/Interfaces/SpeedInterface</PROVIDED-INTERFACE-TREF>
                            </P-PORT-PROTOTYPE>
                        </PORTS>
                    </APPLICATION-SW-COMPONENT-TYPE>
                    <SENDER-RECEIVER-INTERFACE UUID="iface-1">
                        <SHORT-NAME>SpeedInterface</SHORT-NAME>
                        <DATA-ELEMENTS>
                            <VARIABLE-DATA-PROTOTYPE UUID="de-1">
                                <SHORT-NAME>Speed</SHORT-NAME>
                                <TYPE-TREF DEST="APPLICATION-PRIMITIVE-DATA-TYPE">/Types/SpeedType</TYPE-TREF>
                            </VARIABLE-DATA-PROTOTYPE>
                        </DATA-ELEMENTS>
                    </SENDER-RECEIVER-INTERFACE>
                </ELEMENTS>
            </AR-PACKAGE>
        </AR-PACKAGES>
    </AUTOSAR>
    '''
    
    parser = AutosarARXMLParser()
    parser.parse_string(arxml_content)
    
    print("Parsed Software Components:")
    for name, swc in parser.software_components.items():
        print(f"  {name}: {len(swc.ports)} ports")
    
    print("\nDDS Topic Mappings:")
    for mapping in parser.get_dds_topic_mappings():
        print(f"  {mapping['topic_name']} -> {mapping['data_type']}")