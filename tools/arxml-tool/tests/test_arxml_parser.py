#!/usr/bin/env python3
"""
Unit tests for ARXML Parser

Tests the core functionality of the ARXML parser module.
"""

import unittest
import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'src'))

from arxml_parser import (
    ArxmlParser,
    ArxmlQueryEngine,
    SoftwareComponent,
    Port,
    PortInterface,
    DataType,
    EcuConfiguration,
    SystemMapping,
    PortInterfaceType,
    AutosarVersion
)


class TestArxmlParser(unittest.TestCase):
    """Test cases for ArxmlParser"""
    
    def setUp(self):
        """Set up test fixtures"""
        self.parser = ArxmlParser()
    
    def test_parser_initialization(self):
        """Test parser initialization"""
        parser = ArxmlParser()
        self.assertIsNone(parser.root)
        self.assertIsNone(parser.tree)
        self.assertEqual(parser._swcs, [])
        self.assertEqual(parser._interfaces, [])
        self.assertEqual(parser._data_types, [])
    
    def test_parse_string(self):
        """Test parsing from string"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>TestComponent</SHORT-NAME>
                        </APPLICATION-SW-COMPONENT-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        self.assertIsNotNone(self.parser.root)
        
        # Check parsed components
        swcs = self.parser.get_software_components()
        self.assertEqual(len(swcs), 1)
        self.assertEqual(swcs[0].name, 'TestComponent')
        self.assertEqual(swcs[0].component_type, 'APPLICATION')
    
    def test_parse_swc_with_ports(self):
        """Test parsing software component with ports"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>EngineControl</SHORT-NAME>
                            <PORTS>
                                <P-PORT-PROTOTYPE>
                                    <SHORT-NAME>EngineDataOut</SHORT-NAME>
                                    <PROVIDED-INTERFACE-TREF>/Interfaces/EngineData</PROVIDED-INTERFACE-TREF>
                                </P-PORT-PROTOTYPE>
                                <R-PORT-PROTOTYPE>
                                    <SHORT-NAME>SensorDataIn</SHORT-NAME>
                                    <REQUIRED-INTERFACE-TREF>/Interfaces/SensorData</REQUIRED-INTERFACE-TREF>
                                </R-PORT-PROTOTYPE>
                            </PORTS>
                        </APPLICATION-SW-COMPONENT-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        swcs = self.parser.get_software_components()
        
        self.assertEqual(len(swcs), 1)
        self.assertEqual(swcs[0].name, 'EngineControl')
        self.assertEqual(len(swcs[0].ports), 2)
        
        # Check P-PORT
        p_port = swcs[0].ports[0]
        self.assertEqual(p_port.name, 'EngineDataOut')
        self.assertEqual(p_port.port_type, 'P-PORT')
        self.assertEqual(p_port.interface_name, 'EngineData')
        
        # Check R-PORT
        r_port = swcs[0].ports[1]
        self.assertEqual(r_port.name, 'SensorDataIn')
        self.assertEqual(r_port.port_type, 'R-PORT')
        self.assertEqual(r_port.interface_name, 'SensorData')
    
    def test_parse_interfaces(self):
        """Test parsing sender-receiver interfaces"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <SENDER-RECEIVER-INTERFACE>
                            <SHORT-NAME>EngineData</SHORT-NAME>
                            <DATA-ELEMENTS>
                                <DATA-ELEMENT-PROTOTYPE>
                                    <SHORT-NAME>Rpm</SHORT-NAME>
                                    <TYPE-TREF>/Types/uint16</TYPE-TREF>
                                </DATA-ELEMENT-PROTOTYPE>
                                <DATA-ELEMENT-PROTOTYPE>
                                    <SHORT-NAME>Temperature</SHORT-NAME>
                                    <TYPE-TREF>/Types/sint16</TYPE-TREF>
                                </DATA-ELEMENT-PROTOTYPE>
                            </DATA-ELEMENTS>
                        </SENDER-RECEIVER-INTERFACE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        interfaces = self.parser.get_interfaces()
        
        self.assertEqual(len(interfaces), 1)
        self.assertEqual(interfaces[0].name, 'EngineData')
        self.assertEqual(interfaces[0].interface_type, PortInterfaceType.SENDER_RECEIVER)
        self.assertEqual(len(interfaces[0].data_elements), 2)
    
    def test_parse_data_types(self):
        """Test parsing data types"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-PRIMITIVE-DATA-TYPE>
                            <SHORT-NAME>VehicleSpeed</SHORT-NAME>
                            <CATEGORY>VALUE</CATEGORY>
                        </APPLICATION-PRIMITIVE-DATA-TYPE>
                        <IMPLEMENTATION-DATA-TYPE>
                            <SHORT-NAME>uint16</SHORT-NAME>
                            <CATEGORY>VALUE</CATEGORY>
                        </IMPLEMENTATION-DATA-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        data_types = self.parser.get_data_types()
        
        self.assertEqual(len(data_types), 2)
    
    def test_find_swc_by_name(self):
        """Test finding software component by name"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>EngineControl</SHORT-NAME>
                        </APPLICATION-SW-COMPONENT-TYPE>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>SensorHandler</SHORT-NAME>
                        </APPLICATION-SW-COMPONENT-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        
        # Find existing component
        swc = self.parser.find_swc_by_name('EngineControl')
        self.assertIsNotNone(swc)
        self.assertEqual(swc.name, 'EngineControl')
        
        # Find non-existing component
        swc = self.parser.find_swc_by_name('NonExistent')
        self.assertIsNone(swc)
    
    def test_get_summary(self):
        """Test summary generation"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>TestComponent</SHORT-NAME>
                        </APPLICATION-SW-COMPONENT-TYPE>
                        <SENDER-RECEIVER-INTERFACE>
                            <SHORT-NAME>TestInterface</SHORT-NAME>
                        </SENDER-RECEIVER-INTERFACE>
                        <APPLICATION-PRIMITIVE-DATA-TYPE>
                            <SHORT-NAME>TestType</SHORT-NAME>
                        </APPLICATION-PRIMITIVE-DATA-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        summary = self.parser.get_summary()
        
        self.assertEqual(summary['software_components'], 1)
        self.assertEqual(summary['interfaces'], 1)
        self.assertEqual(summary['data_types'], 1)
    
    def test_xpath_query(self):
        """Test XPath query functionality"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>TestComponent</SHORT-NAME>
                        </APPLICATION-SW-COMPONENT-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser.parse_string(xml_content)
        
        # Test custom XPath
        results = self.parser.xpath(".//autosar:APPLICATION-SW-COMPONENT-TYPE")
        self.assertEqual(len(results), 1)
        
        # Test element not found
        results = self.parser.xpath(".//autosar:NONEXISTENT")
        self.assertEqual(len(results), 0)


class TestArxmlQueryEngine(unittest.TestCase):
    """Test cases for ArxmlQueryEngine"""
    
    def setUp(self):
        """Set up test fixtures"""
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
            <AR-PACKAGES>
                <AR-PACKAGE>
                    <SHORT-NAME>TestPackage</SHORT-NAME>
                    <ELEMENTS>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>ComponentA</SHORT-NAME>
                            <PORTS>
                                <P-PORT-PROTOTYPE>
                                    <SHORT-NAME>DataOut</SHORT-NAME>
                                    <PROVIDED-INTERFACE-TREF>/Interfaces/SharedInterface</PROVIDED-INTERFACE-TREF>
                                </P-PORT-PROTOTYPE>
                            </PORTS>
                        </APPLICATION-SW-COMPONENT-TYPE>
                        <APPLICATION-SW-COMPONENT-TYPE>
                            <SHORT-NAME>ComponentB</SHORT-NAME>
                            <PORTS>
                                <R-PORT-PROTOTYPE>
                                    <SHORT-NAME>DataIn</SHORT-NAME>
                                    <REQUIRED-INTERFACE-TREF>/Interfaces/SharedInterface</REQUIRED-INTERFACE-TREF>
                                </R-PORT-PROTOTYPE>
                            </PORTS>
                        </APPLICATION-SW-COMPONENT-TYPE>
                    </ELEMENTS>
                </AR-PACKAGE>
            </AR-PACKAGES>
        </AUTOSAR>'''
        
        self.parser = ArxmlParser()
        self.parser.parse_string(xml_content)
        self.query_engine = ArxmlQueryEngine(self.parser)
    
    def test_find_ports_with_interface(self):
        """Test finding ports with specific interface"""
        ports = self.query_engine.find_ports_with_interface('SharedInterface')
        
        self.assertEqual(len(ports), 2)
        self.assertIn('ComponentA', [p['component_name'] for p in ports])
        self.assertIn('ComponentB', [p['component_name'] for p in ports])
    
    def test_find_connected_components(self):
        """Test finding connected components"""
        connected = self.query_engine.find_connected_components('ComponentA')
        
        self.assertEqual(len(connected), 1)
        self.assertEqual(connected[0]['component_name'], 'ComponentB')
        self.assertEqual(connected[0]['shared_interface'], 'SharedInterface')
    
    def test_filter_swcs_by_type(self):
        """Test filtering components by type"""
        app_swcs = self.query_engine.filter_swcs_by_type('APPLICATION')
        
        self.assertEqual(len(app_swcs), 2)


class TestDataClasses(unittest.TestCase):
    """Test data class functionality"""
    
    def test_software_component_creation(self):
        """Test SoftwareComponent dataclass"""
        port = Port(name='TestPort', port_type='P-PORT', interface_ref='/Interfaces/Test')
        swc = SoftwareComponent(
            name='TestComponent',
            component_type='APPLICATION',
            ports=[port]
        )
        
        self.assertEqual(swc.name, 'TestComponent')
        self.assertEqual(swc.component_type, 'APPLICATION')
        self.assertEqual(len(swc.ports), 1)
    
    def test_port_interface_creation(self):
        """Test PortInterface dataclass"""
        interface = PortInterface(
            name='TestInterface',
            interface_type=PortInterfaceType.SENDER_RECEIVER
        )
        
        self.assertEqual(interface.name, 'TestInterface')
        self.assertEqual(interface.interface_type, PortInterfaceType.SENDER_RECEIVER)


def run_tests():
    """Run all tests"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    suite.addTests(loader.loadTestsFromTestCase(TestArxmlParser))
    suite.addTests(loader.loadTestsFromTestCase(TestArxmlQueryEngine))
    suite.addTests(loader.loadTestsFromTestCase(TestDataClasses))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    sys.exit(0 if success else 1)
