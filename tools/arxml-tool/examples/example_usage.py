#!/usr/bin/env python3
"""
ARXML Parser Usage Examples

Demonstrates how to use the ARXML parser module for various use cases.
"""

import sys
sys.path.insert(0, '/home/admin/yuleASR/tools/arxml-tool/src')

from arxml_parser import (
    ArxmlParser,
    ArxmlQueryEngine,
    parse_arxml,
    validate_arxml,
    PortInterfaceType
)


def example_basic_parsing():
    """Example: Basic ARXML file parsing"""
    print("=== Example 1: Basic Parsing ===")
    
    # Create parser instance
    parser = ArxmlParser()
    
    # Parse ARXML file (replace with actual file path)
    # parser.parse("/path/to/your/system.arxml")
    
    # Get summary of parsed elements
    # summary = parser.get_summary()
    # print(f"Found {summary['software_components']} software components")
    # print(f"Found {summary['interfaces']} interfaces")
    
    print("Basic parsing example complete")
    print()


def example_query_elements():
    """Example: Query parsed elements"""
    print("=== Example 2: Query Elements ===")
    
    parser = ArxmlParser()
    # parser.parse("/path/to/your/system.arxml")
    
    # Get all software components
    # swcs = parser.get_software_components()
    # for swc in swcs:
    #     print(f"Component: {swc.name} (Type: {swc.component_type})")
    #     for port in swc.ports:
    #         print(f"  Port: {port.name} ({port.port_type})")
    
    # Get all interfaces
    # interfaces = parser.get_interfaces()
    # for interface in interfaces:
    #     print(f"Interface: {interface.name} ({interface.interface_type.value})")
    
    # Get all ECU configurations
    # ecu_configs = parser.get_ecu_configurations()
    # for ecu in ecu_configs:
    #     print(f"ECU: {ecu.name}")
    
    print("Element query example complete")
    print()


def example_xpath_queries():
    """Example: XPath queries"""
    print("=== Example 3: XPath Queries ===")
    
    parser = ArxmlParser()
    # parser.parse("/path/to/your/system.arxml")
    
    # Custom XPath queries
    # results = parser.xpath(".//autosar:APPLICATION-SW-COMPONENT-TYPE")
    # for elem in results:
    #     print(f"Found: {elem.tag}")
    
    print("XPath query example complete")
    print()


def example_advanced_queries():
    """Example: Advanced queries using query engine"""
    print("=== Example 4: Advanced Queries ===")
    
    parser = ArxmlParser()
    # parser.parse("/path/to/your/system.arxml")
    
    # Create query engine
    # query_engine = ArxmlQueryEngine(parser)
    
    # Find ports using specific interface
    # ports = query_engine.find_ports_with_interface("MyInterface")
    
    # Find connected components
    # connected = query_engine.find_connected_components("EngineControl")
    
    # Filter by type
    # app_swcs = query_engine.filter_swcs_by_type("APPLICATION")
    # sr_interfaces = query_engine.filter_interfaces_by_type(
    #     PortInterfaceType.SENDER_RECEIVER
    # )
    
    print("Advanced query example complete")
    print()


def example_validation():
    """Example: Schema validation"""
    print("=== Example 5: Schema Validation ===")
    
    # Validate ARXML against XSD schema
    # is_valid = validate_arxml(
    #     "/path/to/your/system.arxml",
    #     "/path/to/autosar_schema.xsd"
    # )
    # print(f"Validation result: {'PASS' if is_valid else 'FAIL'}")
    
    # Or with parser instance
    # parser = ArxmlParser(schema_path="/path/to/autosar_schema.xsd")
    # parser.parse("/path/to/your/system.arxml")
    # is_valid = parser.validate()
    
    print("Validation example complete")
    print()


def example_find_by_name():
    """Example: Find elements by name"""
    print("=== Example 6: Find By Name ===")
    
    parser = ArxmlParser()
    # parser.parse("/path/to/your/system.arxml")
    
    # Find specific component
    # swc = parser.find_swc_by_name("EngineControl")
    # if swc:
    #     print(f"Found component: {swc.name}")
    #     print(f"Type: {swc.component_type}")
    #     print(f"Ports: {len(swc.ports)}")
    
    # Find specific interface
    # interface = parser.find_interface_by_name("EngineDataInterface")
    # if interface:
    #     print(f"Found interface: {interface.name}")
    
    # Find specific data type
    # dtype = parser.find_data_type_by_name("uint16")
    # if dtype:
    #     print(f"Found data type: {dtype.name}")
    
    print("Find by name example complete")
    print()


def main():
    """Run all examples"""
    print("ARXML Parser Usage Examples")
    print("=" * 40)
    print()
    
    example_basic_parsing()
    example_query_elements()
    example_xpath_queries()
    example_advanced_queries()
    example_validation()
    example_find_by_name()
    
    print("All examples completed!")


if __name__ == '__main__':
    main()
