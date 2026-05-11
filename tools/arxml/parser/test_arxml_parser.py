#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Parser 测试模块

测试内容:
1. 基本解析功能测试
2. 数据类型解析测试
3. 端口接口解析测试
4. 软件组件解析测试
5. ECU配置解析测试
6. 验证功能测试
7. 错误处理测试
"""

import unittest
import sys
import os
from pathlib import Path

# 添加父目录到路径
sys.path.insert(0, str(Path(__file__).parent))

from arxml_parser import (
    ARXMLParser,
    parse_arxml,
    parse_arxml_string,
    ARXMLParseError,
    ARXMLNotFoundError,
    DataType,
    PortInterface,
    SoftwareComponent,
    ECUConfiguration,
)


# ============================================================================
# 测试数据 - 示例ARXML内容
# ============================================================================
SAMPLE_ARXML = '''<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
    <AR-PACKAGES>
        <AR-PACKAGE UUID="package-001">
            <SHORT-NAME>DataTypes</SHORT-NAME>
            <ELEMENTS>
                <APPLICATION-PRIMITIVE-DATA-TYPE UUID="type-001">
                    <SHORT-NAME>VehicleSpeed</SHORT-NAME>
                    <CATEGORY>VALUE</CATEGORY>
                    <SW-DATA-DEF-PROPS>
                        <SW-DATA-DEF-PROPS-VARIANTS>
                            <SW-DATA-DEF-PROPS-CONDITIONAL>
                                <COMPU-METHOD-REF>/DataTypes/CompuMethods/VehicleSpeed</COMPU-METHOD-REF>
                                <DATA-CONSTR-REF>/DataTypes/DataConstrs/VehicleSpeed</DATA-CONSTR-REF>
                            </SW-DATA-DEF-PROPS-CONDITIONAL>
                        </SW-DATA-DEF-PROPS-VARIANTS>
                    </SW-DATA-DEF-PROPS>
                </APPLICATION-PRIMITIVE-DATA-TYPE>
                <APPLICATION-PRIMITIVE-DATA-TYPE UUID="type-002">
                    <SHORT-NAME>EngineTemp</SHORT-NAME>
                    <CATEGORY>VALUE</CATEGORY>
                </APPLICATION-PRIMITIVE-DATA-TYPE>
            </ELEMENTS>
        </AR-PACKAGE>
        
        <AR-PACKAGE UUID="package-002">
            <SHORT-NAME>Interfaces</SHORT-NAME>
            <ELEMENTS>
                <SENDER-RECEIVER-INTERFACE UUID="intf-001">
                    <SHORT-NAME>VehicleData</SHORT-NAME>
                    <DATA-ELEMENTS>
                        <DATA-ELEMENT-PROTOTYPE UUID="de-001">
                            <SHORT-NAME>Speed</SHORT-NAME>
                            <TYPE-TREF>/DataTypes/VehicleSpeed</TYPE-TREF>
                        </DATA-ELEMENT-PROTOTYPE>
                        <DATA-ELEMENT-PROTOTYPE UUID="de-002">
                            <SHORT-NAME>Temperature</SHORT-NAME>
                            <TYPE-TREF>/DataTypes/EngineTemp</TYPE-TREF>
                        </DATA-ELEMENT-PROTOTYPE>
                    </DATA-ELEMENTS>
                </SENDER-RECEIVER-INTERFACE>
            </ELEMENTS>
        </AR-PACKAGE>
        
        <AR-PACKAGE UUID="package-003">
            <SHORT-NAME>Components</SHORT-NAME>
            <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="comp-001">
                    <SHORT-NAME>EngineController</SHORT-NAME>
                    <PORTS>
                        <P-PORT-PROTOTYPE UUID="port-001">
                            <SHORT-NAME>VehicleDataProvider</SHORT-NAME>
                            <PROVIDED-INTERFACE-TREF>/Interfaces/VehicleData</PROVIDED-INTERFACE-TREF>
                        </P-PORT-PROTOTYPE>
                        <R-PORT-PROTOTYPE UUID="port-002">
                            <SHORT-NAME>SensorDataReceiver</SHORT-NAME>
                            <REQUIRED-INTERFACE-TREF>/Interfaces/VehicleData</REQUIRED-INTERFACE-TREF>
                        </R-PORT-PROTOTYPE>
                    </PORTS>
                    <INTERNAL-BEHAVIORS>
                        <SWC-INTERNAL-BEHAVIOR UUID="beh-001">
                            <SHORT-NAME>EngineControllerBehavior</SHORT-NAME>
                            <RUNNABLES>
                                <RUNNABLE-ENTITY UUID="run-001">
                                    <SHORT-NAME>ProcessEngineData</SHORT-NAME>
                                    <SYMBOL>ProcessEngineData_Runnable</SYMBOL>
                                    <CAN-BE-INVOKED-CONCURRENTLY>false</CAN-BE-INVOKED-CONCURRENTLY>
                                    <MINIMUM-START-INTERVAL>0.01</MINIMUM-START-INTERVAL>
                                </RUNNABLE-ENTITY>
                            </RUNNABLES>
                            <EVENTS>
                                <TIMING-EVENT UUID="evt-001">
                                    <SHORT-NAME>ProcessEngineDataEvent</SHORT-NAME>
                                    <START-ON-EVENT-REF>/Components/EngineController/EngineControllerBehavior/ProcessEngineData</START-ON-EVENT-REF>
                                    <PERIOD>0.01</PERIOD>
                                </TIMING-EVENT>
                            </EVENTS>
                        </SWC-INTERNAL-BEHAVIOR>
                    </INTERNAL-BEHAVIORS>
                </APPLICATION-SW-COMPONENT-TYPE>
            </ELEMENTS>
        </AR-PACKAGE>
        
        <AR-PACKAGE UUID="package-004">
            <SHORT-NAME>ECUConfig</SHORT-NAME>
            <ELEMENTS>
                <ECU-CONFIGURATION UUID="ecu-001">
                    <SHORT-NAME>MyECU</SHORT-NAME>
                    <ECU-ID>ECU001</ECU-ID>
                    <ECU-EXTRACT-REF>/ECUExtracts/MyECUExtract</ECU-EXTRACT-REF>
                </ECU-CONFIGURATION>
            </ELEMENTS>
        </AR-PACKAGE>
    </AR-PACKAGES>
</AUTOSAR>'''


# ============================================================================
# 测试用例
# ============================================================================
class TestARXMLParserBasic(unittest.TestCase):
    """基本解析功能测试"""
    
    def test_parse_string(self):
        """测试从字符串解析"""
        parser = ARXMLParser()
        result = parser.parse_string(SAMPLE_ARXML)
        self.assertIsNotNone(parser._root)
        self.assertEqual(parser, result)
    
    def test_get_packages(self):
        """测试获取包列表"""
        parser = ARXMLParser()
        parser.parse_string(SAMPLE_ARXML)
        packages = parser.get_all_packages()
        self.assertEqual(len(packages), 4)
        self.assertIn('DataTypes', packages)
        self.assertIn('Interfaces', packages)
        self.assertIn('Components', packages)
        self.assertIn('ECUConfig', packages)
    
    def test_find_package(self):
        """测试查找特定包"""
        parser = ARXMLParser()
        parser.parse_string(SAMPLE_ARXML)
        
        data_types_pkg = parser.find_package('DataTypes')
        self.assertIsNotNone(data_types_pkg)
        
        non_existent = parser.find_package('NonExistent')
        self.assertIsNone(non_existent)


class TestDataTypeParsing(unittest.TestCase):
    """数据类型解析测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_parse_all_data_types(self):
        """测试解析所有数据类型"""
        data_types = self.parser.parse_data_types()
        self.assertEqual(len(data_types), 2)
    
    def test_data_type_properties(self):
        """测试数据类型属性"""
        data_types = self.parser.parse_data_types()
        
        # 查找VehicleSpeed
        vehicle_speed = next((dt for dt in data_types if dt.name == 'VehicleSpeed'), None)
        self.assertIsNotNone(vehicle_speed)
        self.assertEqual(vehicle_speed.category, 'VALUE')
        self.assertEqual(vehicle_speed.uuid, 'type-001')
        
        # 检查SW数据定义属性
        self.assertIn('compu_method', vehicle_speed.sw_data_def_props)
        self.assertIn('data_constraint', vehicle_speed.sw_data_def_props)
    
    def test_get_data_type(self):
        """测试通过名称获取数据类型"""
        self.parser.parse_data_types()
        
        data_type = self.parser.get_data_type('VehicleSpeed')
        self.assertIsNotNone(data_type)
        self.assertEqual(data_type.name, 'VehicleSpeed')
        
        non_existent = self.parser.get_data_type('NonExistent')
        self.assertIsNone(non_existent)
    
    def test_parse_data_types_by_package(self):
        """测试按包名解析数据类型"""
        data_types = self.parser.parse_data_types('DataTypes')
        self.assertEqual(len(data_types), 2)
        
        # 解析不存在的包
        empty_types = self.parser.parse_data_types('NonExistent')
        self.assertEqual(len(empty_types), 0)


class TestPortInterfaceParsing(unittest.TestCase):
    """端口接口解析测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_parse_port_interfaces(self):
        """测试解析端口接口"""
        interfaces = self.parser.parse_port_interfaces()
        self.assertEqual(len(interfaces), 1)
        
        vehicle_data = interfaces[0]
        self.assertEqual(vehicle_data.name, 'VehicleData')
        self.assertEqual(vehicle_data.interface_type, 'SENDER_RECEIVER')
        self.assertEqual(vehicle_data.uuid, 'intf-001')
    
    def test_port_interface_data_elements(self):
        """测试端口接口的数据元素"""
        interfaces = self.parser.parse_port_interfaces()
        
        vehicle_data = interfaces[0]
        self.assertEqual(len(vehicle_data.data_elements), 2)
        
        speed_elem = next((de for de in vehicle_data.data_elements if de.name == 'Speed'), None)
        self.assertIsNotNone(speed_elem)
        self.assertEqual(speed_elem.type_ref, '/DataTypes/VehicleSpeed')
    
    def test_get_port_interface(self):
        """测试通过名称获取端口接口"""
        self.parser.parse_port_interfaces()
        
        interface = self.parser.get_port_interface('VehicleData')
        self.assertIsNotNone(interface)
        self.assertEqual(interface.name, 'VehicleData')


class TestSoftwareComponentParsing(unittest.TestCase):
    """软件组件解析测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_parse_software_components(self):
        """测试解析软件组件"""
        components = self.parser.parse_software_components()
        self.assertEqual(len(components), 1)
        
        engine_controller = components[0]
        self.assertEqual(engine_controller.name, 'EngineController')
        self.assertEqual(engine_controller.component_type, 'APPLICATION')
        self.assertEqual(engine_controller.uuid, 'comp-001')
    
    def test_component_ports(self):
        """测试组件端口"""
        components = self.parser.parse_software_components()
        
        engine_controller = components[0]
        self.assertEqual(len(engine_controller.ports), 2)
        
        # 检查P-Port
        p_port = next((p for p in engine_controller.ports if p.port_type == 'P_PORT'), None)
        self.assertIsNotNone(p_port)
        self.assertEqual(p_port.name, 'VehicleDataProvider')
        self.assertEqual(p_port.interface_ref, '/Interfaces/VehicleData')
        
        # 检查R-Port
        r_port = next((p for p in engine_controller.ports if p.port_type == 'R_PORT'), None)
        self.assertIsNotNone(r_port)
        self.assertEqual(r_port.name, 'SensorDataReceiver')
    
    def test_component_internal_behavior(self):
        """测试组件内部行为"""
        components = self.parser.parse_software_components()
        
        engine_controller = components[0]
        self.assertEqual(len(engine_controller.internal_behaviors), 1)
        
        behavior = engine_controller.internal_behaviors[0]
        self.assertEqual(behavior.name, 'EngineControllerBehavior')
        
        # 检查runnables
        self.assertEqual(len(behavior.runnables), 1)
        runnable = behavior.runnables[0]
        self.assertEqual(runnable.name, 'ProcessEngineData')
        self.assertEqual(runnable.symbol, 'ProcessEngineData_Runnable')
        self.assertFalse(runnable.can_be_invoked_concurrently)
        
        # 检查events
        self.assertEqual(len(behavior.events), 1)
        event = behavior.events[0]
        self.assertEqual(event.name, 'ProcessEngineDataEvent')
        self.assertEqual(event.event_type, 'TIMING')
        self.assertIsNotNone(event.period_ms)
    
    def test_get_software_component(self):
        """测试通过名称获取软件组件"""
        self.parser.parse_software_components()
        
        component = self.parser.get_software_component('EngineController')
        self.assertIsNotNone(component)
        self.assertEqual(component.name, 'EngineController')


class TestECUConfigurationParsing(unittest.TestCase):
    """ECU配置解析测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_parse_ecu_configuration(self):
        """测试解析ECU配置"""
        ecu_config = self.parser.parse_ecu_configuration()
        
        self.assertIsNotNone(ecu_config)
        self.assertEqual(ecu_config.name, 'MyECU')
        self.assertEqual(ecu_config.ecu_id, 'ECU001')
        self.assertEqual(ecu_config.uuid, 'ecu-001')
        self.assertEqual(ecu_config.ecu_extract_ref, '/ECUExtracts/MyECUExtract')


class TestParseAll(unittest.TestCase):
    """完整解析测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_parse_all(self):
        """测试解析所有元素"""
        result = self.parser.parse_all()
        
        self.assertIn('data_types', result)
        self.assertIn('port_interfaces', result)
        self.assertIn('software_components', result)
        self.assertIn('ecu_configuration', result)
        
        self.assertEqual(len(result['data_types']), 2)
        self.assertEqual(len(result['port_interfaces']), 1)
        self.assertEqual(len(result['software_components']), 1)
        self.assertIsNotNone(result['ecu_configuration'])


class TestValidation(unittest.TestCase):
    """验证功能测试"""
    
    def setUp(self):
        self.parser = ARXMLParser()
        self.parser.parse_string(SAMPLE_ARXML)
    
    def test_validate_success(self):
        """测试验证通过"""
        # 先解析所有元素
        self.parser.parse_data_types()
        self.parser.parse_port_interfaces()
        self.parser.parse_software_components()
        
        errors = self.parser.validate()
        # 示例数据应该验证通过
        self.assertEqual(len(errors), 0)


class TestErrorHandling(unittest.TestCase):
    """错误处理测试"""
    
    def test_parse_nonexistent_file(self):
        """测试解析不存在的文件"""
        parser = ARXMLParser()
        with self.assertRaises(ARXMLNotFoundError):
            parser.parse_file('/path/to/nonexistent/file.arxml')
    
    def test_parse_invalid_xml(self):
        """测试解析无效XML"""
        parser = ARXMLParser()
        invalid_xml = "<invalid>unclosed tag"
        with self.assertRaises(ARXMLParseError):
            parser.parse_string(invalid_xml)
    
    def test_parse_empty_string(self):
        """测试解析空字符串"""
        parser = ARXMLParser()
        with self.assertRaises(ARXMLParseError):
            parser.parse_string("")


class TestConvenienceFunctions(unittest.TestCase):
    """便捷函数测试"""
    
    def test_parse_arxml_string(self):
        """测试parse_arxml_string便捷函数"""
        parser = parse_arxml_string(SAMPLE_ARXML)
        self.assertIsInstance(parser, ARXMLParser)
        self.assertIsNotNone(parser._root)
        
        packages = parser.get_all_packages()
        self.assertEqual(len(packages), 4)


# ============================================================================
# 运行测试
# ============================================================================
if __name__ == '__main__':
    # 配置测试输出
    unittest.main(verbosity=2)