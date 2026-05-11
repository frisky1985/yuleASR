#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML生成器测试套件

测试范围:
- ECUC配置模型验证
- MCAL配置生成
- BSW配置生成
- ARXML格式验证
"""

import sys
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

# 添加src到路径
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from ecuc_config_model import (
    EcucModuleConfigurationValues,
    EcucContainerValue,
    EcucBooleanParamValue,
    EcucIntegerParamValue,
    EcucFloatParamValue,
    EcucStringParamValue,
    EcucEnumParamValue,
    EcucDefinitionRef,
    create_module_config,
    create_container,
    create_boolean_param,
    create_integer_param
)
from arxml_ecuc_generator import ArxmlEcucGenerator
from mcal_config_generator import McuConfigGenerator, create_mcu_config
from bsw_config_generator import ComConfigGenerator, create_com_config


class TestEcucConfigModel(unittest.TestCase):
    """测试ECUC配置模型"""
    
    def test_create_module_config(self):
        """测试创建模块配置"""
        config = create_module_config(
            short_name="Test",
            module_def_path="/AUTOSAR/EcucDefs/Test"
        )
        self.assertEqual(config.short_name, "Test")
        self.assertEqual(config.definition_ref.value, "/AUTOSAR/EcucDefs/Test")
    
    def test_create_container(self):
        """测试创建容器"""
        container = create_container(
            short_name="TestContainer",
            def_path="/AUTOSAR/EcucDefs/Test/TestContainer"
        )
        self.assertEqual(container.short_name, "TestContainer")
        self.assertEqual(len(container.parameter_values), 0)
    
    def test_add_parameter(self):
        """测试添加参数"""
        container = create_container("Test", "/Test")
        param = create_boolean_param("EnableFeature", "/Test/EnableFeature", True)
        container.add_parameter(param)
        self.assertEqual(len(container.parameter_values), 1)


class TestArxmlEcucGenerator(unittest.TestCase):
    """测试ARXML生成器"""
    
    def setUp(self):
        self.generator = ArxmlEcucGenerator()
    
    def test_register_module(self):
        """测试注册模块"""
        config = create_module_config("Os", "/AUTOSAR/EcucDefs/Os")
        self.generator.register_module(config)
        self.assertEqual(len(self.generator.generated_modules), 1)
    
    def test_generate_xml_structure(self):
        """测试生成XML结构"""
        config = create_module_config("Os", "/AUTOSAR/EcucDefs/Os")
        self.generator.register_module(config)
        root = self.generator.generate()
        self.assertIsNotNone(root)
        # 验证是否包含AR-PACKAGES
        ar_packages = root.find("{http://autosar.org/schema/r4.0}AR-PACKAGES")
        self.assertIsNotNone(ar_packages)
    
    def test_to_string_output(self):
        """测试字符串输出"""
        config = create_module_config("Os", "/AUTOSAR/EcucDefs/Os")
        self.generator.register_module(config)
        xml_str = self.generator.to_string(pretty=True)
        self.assertIn("AUTOSAR", xml_str)
        self.assertIn("ECUC-MODULE-CONFIGURATION-VALUES", xml_str)


class TestMcalConfigGenerator(unittest.TestCase):
    """测试MCAL配置生成器"""
    
    def test_create_mcu_config(self):
        """测试创建MCU配置"""
        gen = create_mcu_config("ECU0")
        self.assertEqual(gen.module_name, "Mcu")
        self.assertEqual(gen.ecu_name, "ECU0")
    
    def test_add_general_config(self):
        """测试添加通用配置"""
        gen = create_mcu_config("ECU0")
        gen.add_general_config(dev_error_detect=True, version_info_api=False)
        containers = gen.module_config.containers
        self.assertTrue(len(containers) > 0)
        # 查找McuGeneral容器
        general = next((c for c in containers if "General" in c.short_name), None)
        self.assertIsNotNone(general)
    
    def test_add_clock_config(self):
        """测试添加时钟配置"""
        gen = create_mcu_config("ECU0")
        gen.add_clock_config(cpu_clock=80000000, peripheral_clock=40000000)
        containers = gen.module_config.containers
        clock_container = next((c for c in containers if "Clock" in c.short_name), None)
        self.assertIsNotNone(clock_container)
    
    def test_to_arxml_output(self):
        """测试生成ARXML"""
        gen = create_mcu_config("ECU0")
        gen.add_general_config()
        arxml = gen.to_arxml()
        self.assertIn("ECUC-MODULE-CONFIGURATION-VALUES", arxml)
        self.assertIn("Mcu", arxml)


class TestBswConfigGenerator(unittest.TestCase):
    """测试BSW配置生成器"""
    
    def test_create_com_config(self):
        """测试创建Com配置"""
        gen = create_com_config("ECU0")
        self.assertEqual(gen.module_name, "Com")
        self.assertEqual(gen.ecu_name, "ECU0")
    
    def test_add_signal_config(self):
        """测试添加信号配置"""
        gen = create_com_config("ECU0")
        gen.add_signal_config(
            signal_name="EngineSpeed",
            ipdu_ref="EnginePDU",
            start_bit=0,
            bit_length=16
        )
        # 检查是否有容器被添加
        containers = gen.module_config.containers
        self.assertTrue(len(containers) > 0)


class TestXmlValidation(unittest.TestCase):
    """测试XML验证"""
    
    def test_valid_xml(self):
        """测试生成的XML是有效的"""
        gen = create_mcu_config("ECU0")
        gen.add_general_config()
        arxml = gen.to_arxml()
        # 尝试解析XML
        try:
            root = ET.fromstring(arxml)
            self.assertIsNotNone(root)
        except ET.ParseError as e:
            self.fail(f"XML解析失败: {e}")
    
    def test_autosar_namespace(self):
        """测试AUTOSAR命名空间"""
        gen = create_mcu_config("ECU0")
        gen.add_general_config()
        arxml = gen.to_arxml()
        self.assertIn("autosar.org/schema/r4.0", arxml)


def run_tests():
    """运行测试套件"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # 添加测试类
    suite.addTests(loader.loadTestsFromTestCase(TestEcucConfigModel))
    suite.addTests(loader.loadTestsFromTestCase(TestArxmlEcucGenerator))
    suite.addTests(loader.loadTestsFromTestCase(TestMcalConfigGenerator))
    suite.addTests(loader.loadTestsFromTestCase(TestBswConfigGenerator))
    suite.addTests(loader.loadTestsFromTestCase(TestXmlValidation))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
